/*
 * Driver for Renesas R-Car VIN
 *
 * Copyright (C) 2016-2017 Renesas Electronics Corp.
 * Copyright (C) 2011-2013 Renesas Solutions Corp.
 * Copyright (C) 2013 Cogent Embedded, Inc., <source@cogentembedded.com>
 * Copyright (C) 2008 Magnus Damm
 *
 * Based on the soc-camera rcar_vin driver
 *
 * This program is free software; you can redistribute  it and/or modify it
 * under  the terms of  the GNU General  Public License as published by the
 * Free Software Foundation;  either version 2 of the  License, or (at your
 * option) any later version.
 */

#include <linux/clk-provider.h>
#include <linux/delay.h>
#include <linux/pm_runtime.h>

#include <media/v4l2-event.h>
#include <media/v4l2-ioctl.h>
#include <media/v4l2-rect.h>

#include "rcar-vin.h"

#define RVIN_DEFAULT_FORMAT	V4L2_PIX_FMT_YUYV
#define RVIN_DEFAULT_WIDTH	800
#define RVIN_DEFAULT_HEIGHT	600
#define RVIN_DEFAULT_COLORSPACE	V4L2_COLORSPACE_SRGB

/* -----------------------------------------------------------------------------
 * Format Conversions
 */

static const struct rvin_video_format rvin_formats[] = {
	{
		.fourcc			= V4L2_PIX_FMT_NV12,
		.bpp			= 1,
	},
	{
		.fourcc			= V4L2_PIX_FMT_NV16,
		.bpp			= 1,
	},
	{
		.fourcc			= V4L2_PIX_FMT_YUYV,
		.bpp			= 2,
	},
	{
		.fourcc			= V4L2_PIX_FMT_UYVY,
		.bpp			= 2,
	},
	{
		.fourcc			= V4L2_PIX_FMT_RGB565,
		.bpp			= 2,
	},
	{
		.fourcc			= V4L2_PIX_FMT_ARGB555,
		.bpp			= 2,
	},
	{
		.fourcc			= V4L2_PIX_FMT_ABGR32,
		.bpp			= 4,
	},
	{
		.fourcc			= V4L2_PIX_FMT_XBGR32,
		.bpp			= 4,
	},
};

const struct rvin_video_format *rvin_format_from_pixel(u32 pixelformat)
{
	int i;
RVC_log();
	for (i = 0; i < ARRAY_SIZE(rvin_formats); i++)
		if (rvin_formats[i].fourcc == pixelformat)
			return rvin_formats + i;

	return NULL;
}

static u32 rvin_format_bytesperline(struct v4l2_pix_format *pix)
{
	const struct rvin_video_format *fmt;
RVC_log();
	fmt = rvin_format_from_pixel(pix->pixelformat);

	if (WARN_ON(!fmt))
		return -EINVAL;

	return pix->width * fmt->bpp;
}

static u32 rvin_format_sizeimage(struct v4l2_pix_format *pix)
{
RVC_log();
	if (pix->pixelformat == V4L2_PIX_FMT_NV16)
		return pix->bytesperline * pix->height * 2;

	if (pix->pixelformat == V4L2_PIX_FMT_NV12)
		return pix->bytesperline * pix->height * 3 / 2;

	return pix->bytesperline * pix->height;
}

static void __rvin_format_aling_update(struct rvin_dev *vin,
				       struct v4l2_pix_format *pix)
{
	u32 walign;
RVC_log();
	/* HW limit width to a multiple of 32 (2^5) for NV16/12 else 2 (2^1) */
	if (pix->pixelformat == V4L2_PIX_FMT_NV12 ||
	    pix->pixelformat == V4L2_PIX_FMT_NV16)
		walign = 5;
	else if (pix->pixelformat == V4L2_PIX_FMT_YUYV ||
		 pix->pixelformat == V4L2_PIX_FMT_UYVY)
		walign = 1;
	else
		walign = 0;

	/* Limit to VIN capabilities */
	v4l_bound_align_image(&pix->width, 5, vin->info->max_width, walign,
			      &pix->height, 2, vin->info->max_height, 0, 0);

	pix->bytesperline = rvin_format_bytesperline(pix);
	pix->sizeimage = rvin_format_sizeimage(pix);
}

static int rvin_format_align(struct rvin_dev *vin, struct v4l2_pix_format *pix)
{
	int width;
RVC_log();
	/* If requested format is not supported fallback to the default */
	if (!rvin_format_from_pixel(pix->pixelformat)) {
		vin_dbg(vin, "Format 0x%x not found, using default 0x%x\n",
			pix->pixelformat, RVIN_DEFAULT_FORMAT);
		pix->pixelformat = RVIN_DEFAULT_FORMAT;
	}

	switch (pix->field) {
	case V4L2_FIELD_TOP:
	case V4L2_FIELD_BOTTOM:
	case V4L2_FIELD_NONE:
	case V4L2_FIELD_INTERLACED_TB:
	case V4L2_FIELD_INTERLACED_BT:
	case V4L2_FIELD_INTERLACED:
		break;
	case V4L2_FIELD_SEQ_TB:
	case V4L2_FIELD_SEQ_BT:
		/*
		 * Due to extra hardware alignment restrictions on
		 * buffer addresses for multi plane formats they
		 * are not (yet) supported. This would be much simpler
		 * once support for the UDS scaler is added.
		 *
		 * Support for multi plane formats could be supported
		 * by having a different partitioning strategy when
		 * capturing the second field (start capturing one
		 * quarter in to the buffer instead of one half).
		 */

		if (pix->pixelformat == V4L2_PIX_FMT_NV16)
			pix->pixelformat = RVIN_DEFAULT_FORMAT;

		/*
		 * For sequential formats it's needed to write to
		 * the same buffer two times to capture both the top
		 * and bottom field. The second time it is written
		 * an offset is needed as to not overwrite the
		 * previous captured field. Due to hardware limitations
		 * the offsets must be a multiple of 128. Try to
		 * increase the width of the image until a size is
		 * found which can satisfy this constraint.
		 */

		width = pix->width;
		while (width < vin->info->max_width) {
			pix->width = width++;

			__rvin_format_aling_update(vin, pix);

			if (((pix->sizeimage / 2) & HW_BUFFER_MASK) == 0)
				break;
		}
		break;
	default:
		pix->field = V4L2_FIELD_NONE;
		break;
	}

	/* Check that colorspace is reasonable, if not keep current */
	if (!pix->colorspace || pix->colorspace >= 0xff)
		pix->colorspace = vin->format.colorspace;

	__rvin_format_aling_update(vin, pix);

	if (vin->info->chip == RCAR_M1 &&
	    pix->pixelformat == V4L2_PIX_FMT_XBGR32) {
		vin_err(vin, "pixel format XBGR32 not supported on M1\n");
		return -EINVAL;
	}

	if (vin->info->chip != RCAR_GEN3 &&
	    pix->pixelformat == V4L2_PIX_FMT_NV12) {
		vin_err(vin, "pixel format NV12 is supported from Gen3\n");
		return -EINVAL;
	}

	if (vin->info->chip != RCAR_GEN3 &&
	    pix->pixelformat == V4L2_PIX_FMT_ABGR32) {
		vin_err(vin, "pixel format ARGB8888 is supported from Gen3\n");
		return -EINVAL;
	}

	vin_dbg(vin, "Format %ux%u bpl: %d size: %d\n",
		pix->width, pix->height, pix->bytesperline, pix->sizeimage);

	return 0;
}

/* -----------------------------------------------------------------------------
 * V4L2
 */

static int rvin_get_sd_format(struct rvin_dev *vin, struct v4l2_pix_format *pix)
{
	struct v4l2_subdev_format fmt = {
		.which = V4L2_SUBDEV_FORMAT_ACTIVE,
	};
	int ret;
RVC_log();
	/* Get cropping size */
	if (!vin->digital) {
		struct v4l2_subdev *sd;
		struct media_pad *pad;

		pad = media_entity_remote_pad(&vin->pad);
		if (!pad)
			return -EPIPE;

		sd = media_entity_to_v4l2_subdev(pad->entity);
		if (!sd)
			return -EPIPE;

		if (v4l2_subdev_call(sd, pad, get_fmt, NULL, &fmt))
			return -EPIPE;

		pix->width = fmt.format.width;
		pix->height = fmt.format.height;
		vin->crop.width = fmt.format.width;
		vin->crop.height = fmt.format.height;

		if (fmt.format.field == V4L2_FIELD_ALTERNATE)
			vin->format.field = V4L2_FIELD_INTERLACED;
		else
			vin->format.field = fmt.format.field;

		vin->format.bytesperline =
			rvin_format_bytesperline(&vin->format);

		return 0;
	}

	fmt.pad = vin->digital->source_pad;

	ret = v4l2_subdev_call(vin_to_source(vin), pad, get_fmt, NULL, &fmt);
	if (ret)
		return ret;

	switch (fmt.format.field) {
	case V4L2_FIELD_TOP:
	case V4L2_FIELD_BOTTOM:
	case V4L2_FIELD_NONE:
	case V4L2_FIELD_INTERLACED_TB:
	case V4L2_FIELD_INTERLACED_BT:
	case V4L2_FIELD_INTERLACED:
	case V4L2_FIELD_SEQ_TB:
	case V4L2_FIELD_SEQ_BT:
		break;
	case V4L2_FIELD_ALTERNATE:
		/* Use VIN hardware to combine the two fields */
		fmt.format.field = V4L2_FIELD_INTERLACED;
		fmt.format.height *= 2;
		break;
	default:
		vin->format.field = V4L2_FIELD_NONE;
		break;
	}

	v4l2_fill_pix_format(pix, &fmt.format);

	return 0;
}

int rvin_reset_format(struct rvin_dev *vin)
{
	int ret;
RVC_log();
	ret = rvin_get_sd_format(vin, &vin->format);
	if (ret)
		return ret;

	vin->crop.top = vin->crop.left = 0;
	vin->crop.width = vin->format.width;
	vin->crop.height = vin->format.height;

	vin->compose.top = vin->compose.left = 0;
	vin->compose.width = vin->format.width;
	vin->compose.height = vin->format.height;

	vin->format.bytesperline = rvin_format_bytesperline(&vin->format);
	vin->format.sizeimage = rvin_format_sizeimage(&vin->format);

	return 0;
}

static int __rvin_try_format_source(struct rvin_dev *vin,
				    u32 which, struct v4l2_pix_format *pix)
{
	struct v4l2_subdev *sd;
	struct v4l2_subdev_pad_config *pad_cfg;
	struct v4l2_subdev_format format = {
		.which = which,
	};
	enum v4l2_field field;
	u32 width, height;
	int ret;
RVC_log();
	sd = vin_to_source(vin);

	v4l2_fill_mbus_format(&format.format, pix, vin->code);

	pad_cfg = v4l2_subdev_alloc_pad_config(sd);
	if (pad_cfg == NULL)
		return -ENOMEM;

	format.pad = vin->digital->source_pad;

	/* Allow the video device to override field and to scale */
	field = pix->field;
	width = pix->width;
	height = pix->height;

	ret = v4l2_subdev_call(sd, pad, set_fmt, pad_cfg, &format);
	if (ret < 0 && ret != -ENOIOCTLCMD)
		goto done;

	v4l2_fill_pix_format(pix, &format.format);

	pix->field = field;
	pix->width = width;
	pix->height = height;
done:
	v4l2_subdev_free_pad_config(pad_cfg);
	return ret;
}

static int __rvin_try_format(struct rvin_dev *vin,
			     u32 which, struct v4l2_pix_format *pix)
{
	int ret;
RVC_log();
	/* Keep current field if no specific one is asked for */
	if (pix->field == V4L2_FIELD_ANY)
		pix->field = vin->format.field;

	/* Limit to source capabilities */
	ret = __rvin_try_format_source(vin, which, pix);
	if (ret)
		return ret;

	return rvin_format_align(vin, pix);
}

static int rvin_querycap(struct file *file, void *priv,
			 struct v4l2_capability *cap)
{
	struct rvin_dev *vin = video_drvdata(file);
RVC_log();
	strlcpy(cap->driver, KBUILD_MODNAME, sizeof(cap->driver));
	strlcpy(cap->card, "R_Car_VIN", sizeof(cap->card));
	snprintf(cap->bus_info, sizeof(cap->bus_info), "platform:%s",
		 dev_name(vin->dev));
	return 0;
}

static int rvin_try_fmt_vid_cap(struct file *file, void *priv,
				struct v4l2_format *f)
{
	struct rvin_dev *vin = video_drvdata(file);
RVC_log();
	return __rvin_try_format(vin, V4L2_SUBDEV_FORMAT_TRY, &f->fmt.pix);
}

static int rvin_s_fmt_vid_cap(struct file *file, void *priv,
			      struct v4l2_format *f)
{
	struct rvin_dev *vin = video_drvdata(file);
	int ret;
RVC_log();
	if (vb2_is_busy(&vin->queue))
		return -EBUSY;

	ret = __rvin_try_format(vin, V4L2_SUBDEV_FORMAT_ACTIVE, &f->fmt.pix);
	if (ret)
		return ret;

	vin->format = f->fmt.pix;

	return 0;
}

static int rvin_g_fmt_vid_cap(struct file *file, void *priv,
			      struct v4l2_format *f)
{
	struct rvin_dev *vin = video_drvdata(file);
RVC_log();
	f->fmt.pix = vin->format;

	return 0;
}

static int rvin_enum_fmt_vid_cap(struct file *file, void *priv,
				 struct v4l2_fmtdesc *f)
{
RVC_log();
	if (f->index >= ARRAY_SIZE(rvin_formats))
		return -EINVAL;

	f->pixelformat = rvin_formats[f->index].fourcc;

	return 0;
}

static int rvin_g_selection(struct file *file, void *fh,
			    struct v4l2_selection *s)
{
	struct rvin_dev *vin = video_drvdata(file);
	struct v4l2_pix_format pix;
	int ret;
RVC_log();
	if (s->type != V4L2_BUF_TYPE_VIDEO_CAPTURE)
		return -EINVAL;

	switch (s->target) {
	case V4L2_SEL_TGT_CROP_BOUNDS:
	case V4L2_SEL_TGT_CROP_DEFAULT:
		ret = rvin_get_sd_format(vin, &pix);
		if (ret)
			return ret;
		s->r.left = s->r.top = 0;
		s->r.width = pix.width;
		s->r.height = pix.height;
		break;
	case V4L2_SEL_TGT_CROP:
		s->r = vin->crop;
		break;
	case V4L2_SEL_TGT_COMPOSE_BOUNDS:
	case V4L2_SEL_TGT_COMPOSE_DEFAULT:
		s->r.left = s->r.top = 0;
		s->r.width = vin->format.width;
		s->r.height = vin->format.height;
		break;
	case V4L2_SEL_TGT_COMPOSE:
		s->r = vin->compose;
		break;
	default:
		return -EINVAL;
	}

	return 0;
}

static int rvin_s_selection(struct file *file, void *fh,
			    struct v4l2_selection *s)
{
	struct rvin_dev *vin = video_drvdata(file);
	const struct rvin_video_format *fmt;
	struct v4l2_pix_format pix;
	struct v4l2_rect r = s->r;
	struct v4l2_rect max_rect;
	struct v4l2_rect min_rect = {
		.width = 6,
		.height = 2,
	};
	int ret;
RVC_log();
	if (s->type != V4L2_BUF_TYPE_VIDEO_CAPTURE)
		return -EINVAL;

	v4l2_rect_set_min_size(&r, &min_rect);

	switch (s->target) {
	case V4L2_SEL_TGT_CROP:
		/* Can't crop outside of source input */
		ret = rvin_get_sd_format(vin, &pix);
		if (ret)
			return ret;
		max_rect.top = max_rect.left = 0;
		max_rect.width = pix.width;
		max_rect.height = pix.height;
		v4l2_rect_map_inside(&r, &max_rect);

		v4l_bound_align_image(&r.width, 6, pix.width, 0,
				      &r.height, 2, pix.height, 0, 0);

		r.top  = clamp_t(s32, r.top, 0, pix.height - r.height);
		r.left = clamp_t(s32, r.left, 0, pix.width - r.width);

		vin->crop = s->r = r;

		vin_dbg(vin, "Cropped %dx%d@%d:%d of %dx%d\n",
			r.width, r.height, r.left, r.top,
			pix.width, pix.height);
		break;
	case V4L2_SEL_TGT_COMPOSE:
		/* Make sure compose rect fits inside output format */
		max_rect.top = max_rect.left = 0;
		max_rect.width = vin->format.width;
		max_rect.height = vin->format.height;
		v4l2_rect_map_inside(&r, &max_rect);

		/*
		 * Composing is done by adding a offset to the buffer address,
		 * the HW wants this address to be aligned to HW_BUFFER_MASK.
		 * Make sure the top and left values meets this requirement.
		 */
		while ((r.top * vin->format.bytesperline) & HW_BUFFER_MASK)
			r.top--;

		fmt = rvin_format_from_pixel(vin->format.pixelformat);
		while ((r.left * fmt->bpp) & HW_BUFFER_MASK)
			r.left--;

		vin->compose = s->r = r;

		vin_dbg(vin, "Compose %dx%d@%d:%d in %dx%d\n",
			r.width, r.height, r.left, r.top,
			vin->format.width, vin->format.height);
		break;
	default:
		return -EINVAL;
	}

	return 0;
}

static int rvin_cropcap(struct file *file, void *priv,
			struct v4l2_cropcap *crop)
{
	struct rvin_dev *vin = video_drvdata(file);
	struct v4l2_subdev *sd;
RVC_log();
	if (crop->type != V4L2_BUF_TYPE_VIDEO_CAPTURE)
		return -EINVAL;

	if (vin->digital)
		sd = vin_to_source(vin);
	else
		return 0;

	return v4l2_subdev_call(sd, video, g_pixelaspect, &crop->pixelaspect);
}

static int rvin_enum_input(struct file *file, void *priv,
			   struct v4l2_input *i)
{
	struct rvin_dev *vin = video_drvdata(file);
	struct v4l2_subdev *sd = vin_to_source(vin);
	int ret;
RVC_log();
	if (i->index != 0)
		return -EINVAL;

	ret = v4l2_subdev_call(sd, video, g_input_status, &i->status);
	if (ret < 0 && ret != -ENOIOCTLCMD && ret != -ENODEV)
		return ret;

	i->type = V4L2_INPUT_TYPE_CAMERA;

	if (v4l2_subdev_has_op(sd, pad, dv_timings_cap)) {
		i->capabilities = V4L2_IN_CAP_DV_TIMINGS;
		i->std = 0;
	} else {
		i->capabilities = V4L2_IN_CAP_STD;
		i->std = vin->vdev.tvnorms;
	}

	strlcpy(i->name, "Camera", sizeof(i->name));

	return 0;
}

static int rvin_g_input(struct file *file, void *priv, unsigned int *i)
{
	*i = 0;
RVC_log();
	return 0;
}

static int rvin_s_input(struct file *file, void *priv, unsigned int i)
{
RVC_log();
	if (i > 0)
		return -EINVAL;
	return 0;
}

static int rvin_querystd(struct file *file, void *priv, v4l2_std_id *a)
{
	struct rvin_dev *vin = video_drvdata(file);
	struct v4l2_subdev *sd = vin_to_source(vin);
RVC_log();
	return v4l2_subdev_call(sd, video, querystd, a);
}

static int rvin_s_std(struct file *file, void *priv, v4l2_std_id a)
{
	struct rvin_dev *vin = video_drvdata(file);
	int ret;
RVC_log();
	ret = v4l2_subdev_call(vin_to_source(vin), video, s_std, a);
	if (ret < 0)
		return ret;

	/* Changing the standard will change the width/height */
	return rvin_reset_format(vin);
}

static int rvin_g_std(struct file *file, void *priv, v4l2_std_id *a)
{
	struct rvin_dev *vin = video_drvdata(file);
	struct v4l2_subdev *sd = vin_to_source(vin);
RVC_log();
	return v4l2_subdev_call(sd, video, g_std, a);
}

static int rvin_subscribe_event(struct v4l2_fh *fh,
				const struct v4l2_event_subscription *sub)
{
RVC_log();
	switch (sub->type) {
	case V4L2_EVENT_SOURCE_CHANGE:
		return v4l2_event_subscribe(fh, sub, 4, NULL);
	}
	return v4l2_ctrl_subscribe_event(fh, sub);
}

static int rvin_enum_dv_timings(struct file *file, void *priv_fh,
				struct v4l2_enum_dv_timings *timings)
{
	struct rvin_dev *vin = video_drvdata(file);
	struct v4l2_subdev *sd = vin_to_source(vin);
	int ret;
RVC_log();
	if (timings->pad)
		return -EINVAL;

	timings->pad = vin->digital->sink_pad;

	ret = v4l2_subdev_call(sd, pad, enum_dv_timings, timings);

	timings->pad = 0;

	return ret;
}

static int rvin_s_dv_timings(struct file *file, void *priv_fh,
			     struct v4l2_dv_timings *timings)
{
	struct rvin_dev *vin = video_drvdata(file);
	struct v4l2_subdev *sd = vin_to_source(vin);
	int ret;
RVC_log();
	ret = v4l2_subdev_call(sd, video, s_dv_timings, timings);
	if (ret)
		return ret;

	/* Changing the timings will change the width/height */
	return rvin_reset_format(vin);
}

static int rvin_g_dv_timings(struct file *file, void *priv_fh,
			     struct v4l2_dv_timings *timings)
{
	struct rvin_dev *vin = video_drvdata(file);
	struct v4l2_subdev *sd = vin_to_source(vin);
RVC_log();
	return v4l2_subdev_call(sd, video, g_dv_timings, timings);
}

static int rvin_query_dv_timings(struct file *file, void *priv_fh,
				 struct v4l2_dv_timings *timings)
{
	struct rvin_dev *vin = video_drvdata(file);
	struct v4l2_subdev *sd = vin_to_source(vin);
RVC_log();
	return v4l2_subdev_call(sd, video, query_dv_timings, timings);
}

static int rvin_dv_timings_cap(struct file *file, void *priv_fh,
			       struct v4l2_dv_timings_cap *cap)
{
	struct rvin_dev *vin = video_drvdata(file);
	struct v4l2_subdev *sd = vin_to_source(vin);
	int ret;
RVC_log();
	if (cap->pad)
		return -EINVAL;

	cap->pad = vin->digital->sink_pad;
RVC_log();
	ret = v4l2_subdev_call(sd, pad, dv_timings_cap, cap);

	cap->pad = 0;

	return ret;
}

static int rvin_g_edid(struct file *file, void *fh, struct v4l2_edid *edid)
{
	struct rvin_dev *vin = video_drvdata(file);
	struct v4l2_subdev *sd = vin_to_source(vin);
	int ret;
RVC_log();
	if (edid->pad)
		return -EINVAL;

	edid->pad = vin->digital->sink_pad;

	ret = v4l2_subdev_call(sd, pad, get_edid, edid);

	edid->pad = 0;

	return ret;
}

static int rvin_s_edid(struct file *file, void *fh, struct v4l2_edid *edid)
{
	struct rvin_dev *vin = video_drvdata(file);
	struct v4l2_subdev *sd = vin_to_source(vin);
	int ret;
RVC_log();
	if (edid->pad)
		return -EINVAL;

	edid->pad = vin->digital->sink_pad;

	ret = v4l2_subdev_call(sd, pad, set_edid, edid);

	edid->pad = 0;

	return ret;
}

static const struct v4l2_ioctl_ops rvin_ioctl_ops = {
	.vidioc_querycap		= rvin_querycap,
	.vidioc_try_fmt_vid_cap		= rvin_try_fmt_vid_cap,
	.vidioc_g_fmt_vid_cap		= rvin_g_fmt_vid_cap,
	.vidioc_s_fmt_vid_cap		= rvin_s_fmt_vid_cap,
	.vidioc_enum_fmt_vid_cap	= rvin_enum_fmt_vid_cap,

	.vidioc_g_selection		= rvin_g_selection,
	.vidioc_s_selection		= rvin_s_selection,

	.vidioc_cropcap			= rvin_cropcap,

	.vidioc_enum_input		= rvin_enum_input,
	.vidioc_g_input			= rvin_g_input,
	.vidioc_s_input			= rvin_s_input,

	.vidioc_dv_timings_cap		= rvin_dv_timings_cap,
	.vidioc_enum_dv_timings		= rvin_enum_dv_timings,
	.vidioc_g_dv_timings		= rvin_g_dv_timings,
	.vidioc_s_dv_timings		= rvin_s_dv_timings,
	.vidioc_query_dv_timings	= rvin_query_dv_timings,

	.vidioc_g_edid			= rvin_g_edid,
	.vidioc_s_edid			= rvin_s_edid,

	.vidioc_querystd		= rvin_querystd,
	.vidioc_g_std			= rvin_g_std,
	.vidioc_s_std			= rvin_s_std,

	.vidioc_reqbufs			= vb2_ioctl_reqbufs,
	.vidioc_create_bufs		= vb2_ioctl_create_bufs,
	.vidioc_querybuf		= vb2_ioctl_querybuf,
	.vidioc_qbuf			= vb2_ioctl_qbuf,
	.vidioc_dqbuf			= vb2_ioctl_dqbuf,
	.vidioc_expbuf			= vb2_ioctl_expbuf,
	.vidioc_prepare_buf		= vb2_ioctl_prepare_buf,
	.vidioc_streamon		= vb2_ioctl_streamon,
	.vidioc_streamoff		= vb2_ioctl_streamoff,

	.vidioc_log_status		= v4l2_ctrl_log_status,
	.vidioc_subscribe_event		= rvin_subscribe_event,
	.vidioc_unsubscribe_event	= v4l2_event_unsubscribe,
};

/* -----------------------------------------------------------------------------
 * V4L2 Media Controller
 */

static int __rvin_mc_try_format(struct rvin_dev *vin,
				struct v4l2_pix_format *pix)
{
	/* Keep current field if no specific one is asked for */
	if (pix->field == V4L2_FIELD_ANY)
		pix->field = vin->format.field;
RVC_log();
	return rvin_format_align(vin, pix);
}

static int rvin_mc_try_fmt_vid_cap(struct file *file, void *priv,
				   struct v4l2_format *f)
{
	struct rvin_dev *vin = video_drvdata(file);
RVC_log();
	return __rvin_mc_try_format(vin, &f->fmt.pix);
}

static int rvin_mc_s_fmt_vid_cap(struct file *file, void *priv,
				 struct v4l2_format *f)
{
	struct rvin_dev *vin = video_drvdata(file);
	int ret;
RVC_log();
	if (vb2_is_busy(&vin->queue))
		return -EBUSY;

	ret = __rvin_mc_try_format(vin, &f->fmt.pix);
	if (ret)
		return ret;

	vin->format = f->fmt.pix;

	return 0;
}

static int rvin_mc_enum_input(struct file *file, void *priv,
			      struct v4l2_input *i)
{
RVC_log();
	if (i->index != 0)
		return -EINVAL;

	i->type = V4L2_INPUT_TYPE_CAMERA;
	strlcpy(i->name, "Camera", sizeof(i->name));

	return 0;
}

static const struct v4l2_ioctl_ops rvin_mc_ioctl_ops = {
	.vidioc_querycap		= rvin_querycap,
	.vidioc_try_fmt_vid_cap		= rvin_mc_try_fmt_vid_cap,
	.vidioc_g_fmt_vid_cap		= rvin_g_fmt_vid_cap,
	.vidioc_s_fmt_vid_cap		= rvin_mc_s_fmt_vid_cap,
	.vidioc_enum_fmt_vid_cap	= rvin_enum_fmt_vid_cap,

	.vidioc_g_selection		= rvin_g_selection,
	.vidioc_s_selection		= rvin_s_selection,

	.vidioc_cropcap			= rvin_cropcap,

	.vidioc_enum_input		= rvin_mc_enum_input,
	.vidioc_g_input			= rvin_g_input,
	.vidioc_s_input			= rvin_s_input,

	.vidioc_reqbufs			= vb2_ioctl_reqbufs,
	.vidioc_create_bufs		= vb2_ioctl_create_bufs,
	.vidioc_querybuf		= vb2_ioctl_querybuf,
	.vidioc_qbuf			= vb2_ioctl_qbuf,
	.vidioc_dqbuf			= vb2_ioctl_dqbuf,
	.vidioc_expbuf			= vb2_ioctl_expbuf,
	.vidioc_prepare_buf		= vb2_ioctl_prepare_buf,
	.vidioc_streamon		= vb2_ioctl_streamon,
	.vidioc_streamoff		= vb2_ioctl_streamoff,

	.vidioc_log_status		= v4l2_ctrl_log_status,
	.vidioc_subscribe_event		= rvin_subscribe_event,
	.vidioc_unsubscribe_event	= v4l2_event_unsubscribe,
};

/* -----------------------------------------------------------------------------
 * File Operations
 */

static int rvin_power_on(struct rvin_dev *vin)
{
	int ret;
	struct v4l2_subdev *sd = vin_to_source(vin);
RVC_log();
	pm_runtime_get_sync(vin->v4l2_dev.dev);

	ret = v4l2_subdev_call(sd, core, s_power, 1);
	if (ret < 0 && ret != -ENOIOCTLCMD && ret != -ENODEV)
		return ret;
	return 0;
}

static int rvin_power_off(struct rvin_dev *vin)
{
	int ret;
	struct v4l2_subdev *sd = vin_to_source(vin);
RVC_log();
	ret = v4l2_subdev_call(sd, core, s_power, 0);

	pm_runtime_put(vin->v4l2_dev.dev);

	if (ret < 0 && ret != -ENOIOCTLCMD && ret != -ENODEV)
		return ret;

	return 0;
}

static int rvin_initialize_device(struct file *file)
{
	struct rvin_dev *vin = video_drvdata(file);
	int ret;
RVC_log();
	struct v4l2_format f = {
		.type = V4L2_BUF_TYPE_VIDEO_CAPTURE,
		.fmt.pix = {
			.width		= vin->format.width,
			.height		= vin->format.height,
			.field		= vin->format.field,
			.colorspace	= vin->format.colorspace,
			.pixelformat	= vin->format.pixelformat,
		},
	};

	ret = rvin_power_on(vin);
	if (ret < 0)
		return ret;

	pm_runtime_enable(&vin->vdev.dev);
	ret = pm_runtime_resume(&vin->vdev.dev);
	if (ret < 0 && ret != -ENOSYS)
		goto eresume;

	/*
	 * Try to configure with default parameters. Notice: this is the
	 * very first open, so, we cannot race against other calls,
	 * apart from someone else calling open() simultaneously, but
	 * .host_lock is protecting us against it.
	 */
	ret = rvin_s_fmt_vid_cap(file, NULL, &f);
	if (ret < 0)
		goto esfmt;

	v4l2_ctrl_handler_setup(&vin->ctrl_handler);

	return 0;
esfmt:
	pm_runtime_disable(&vin->vdev.dev);
eresume:
	rvin_power_off(vin);

	return ret;
}

static int rvin_open(struct file *file)
{
	struct rvin_dev *vin = video_drvdata(file);
	int ret;
RVC_log();
	mutex_lock(&vin->lock);

	if (!vin->digital->subdev) {
		ret = -ENODEV;
		goto unlock;
	}

	file->private_data = vin;

	ret = v4l2_fh_open(file);
	if (ret)
		goto unlock;

	if (!v4l2_fh_is_singular_file(file))
		goto unlock;

	if (rvin_initialize_device(file)) {
		v4l2_fh_release(file);
		ret = -ENODEV;
	}

unlock:
	mutex_unlock(&vin->lock);
	return ret;
}

static int rvin_release(struct file *file)
{
	struct rvin_dev *vin = video_drvdata(file);
	bool fh_singular;
	int ret;
RVC_log();
	mutex_lock(&vin->lock);

	/* Save the singular status before we call the clean-up helper */
	fh_singular = v4l2_fh_is_singular_file(file);

	/* the release helper will cleanup any on-going streaming */
	ret = _vb2_fop_release(file, NULL);

	/*
	 * If this was the last open file.
	 * Then de-initialize hw module.
	 */
	if (fh_singular) {
		pm_runtime_suspend(&vin->vdev.dev);
		pm_runtime_disable(&vin->vdev.dev);
		rvin_power_off(vin);
	}

	mutex_unlock(&vin->lock);

	return ret;
}

static const struct v4l2_file_operations rvin_fops = {
	.owner		= THIS_MODULE,
	.unlocked_ioctl	= video_ioctl2,
	.open		= rvin_open,
	.release	= rvin_release,
	.poll		= vb2_fop_poll,
	.mmap		= vb2_fop_mmap,
	.read		= vb2_fop_read,
};

/* -----------------------------------------------------------------------------
 * Media controller file Operations
 */

static int rvin_mc_open(struct file *file)
{
	struct rvin_dev *vin = video_drvdata(file);
	int ret;
RVC_log();
	mutex_lock(&vin->lock);

	file->private_data = vin;

	ret = v4l2_fh_open(file);
	if (ret)
		goto unlock;

	ret = rvin_get_sd_format(vin, &vin->format);
	if (ret) {
		v4l2_fh_release(file);
		goto unlock;
	}

	reset_control_deassert(vin->rstc);
	pm_runtime_get_sync(vin->dev);
	v4l2_pipeline_pm_use(&vin->vdev.entity, 1);

unlock:
	mutex_unlock(&vin->lock);

	return ret;
}

static int rvin_mc_release(struct file *file)
{
	struct rvin_dev *vin = video_drvdata(file);
	int ret;
	u32 timeout = MSTP_WAIT_TIME;
RVC_log();
	mutex_lock(&vin->lock);

	/* the release helper will cleanup any on-going streaming */
	ret = _vb2_fop_release(file, NULL);

	v4l2_pipeline_pm_use(&vin->vdev.entity, 0);
	pm_runtime_put_sync(vin->dev);
	while (1) {
		bool enable;

		enable = __clk_is_enabled(vin->clk);
		if (enable)
			break;
		if (!timeout) {
			dev_warn(vin->dev, "MSTP status timeout\n");
			break;
		}
		usleep_range(10, 15);
		timeout--;
	}
	reset_control_assert(vin->rstc);

	mutex_unlock(&vin->lock);

	return ret;
}

static const struct v4l2_file_operations rvin_mc_fops = {
	.owner		= THIS_MODULE,
	.unlocked_ioctl	= video_ioctl2,
	.open		= rvin_mc_open,
	.release	= rvin_mc_release,
	.poll		= vb2_fop_poll,
	.mmap		= vb2_fop_mmap,
	.read		= vb2_fop_read,
};

void rvin_v4l2_unregister(struct rvin_dev *vin)
{
	v4l2_info(&vin->v4l2_dev, "Removing %s\n",
		  video_device_node_name(&vin->vdev));
RVC_log();
	/* Checks internaly if vdev have been init or not */
	video_unregister_device(&vin->vdev);
}

static void rvin_notify(struct v4l2_subdev *sd,
			unsigned int notification, void *arg)
{
	struct rvin_dev *vin =
		container_of(sd->v4l2_dev, struct rvin_dev, v4l2_dev);
RVC_log();
	switch (notification) {
	case V4L2_DEVICE_NOTIFY_EVENT:
		v4l2_event_queue(&vin->vdev, arg);
		break;
	default:
		break;
	}
}

int rvin_v4l2_register(struct rvin_dev *vin)
{
	struct video_device *vdev = &vin->vdev;
	int ret;
RVC_log();
	vin->v4l2_dev.notify = rvin_notify;

	/* video node */
	vdev->v4l2_dev = &vin->v4l2_dev;
	vdev->queue = &vin->queue;
	snprintf(vdev->name, sizeof(vdev->name), "%s %s", KBUILD_MODNAME,
		 dev_name(vin->dev));
	vdev->release = video_device_release_empty;
	vdev->lock = &vin->lock;
	vdev->device_caps = V4L2_CAP_VIDEO_CAPTURE | V4L2_CAP_STREAMING |
		V4L2_CAP_READWRITE;

	/* Set some form of default format */
	vin->format.pixelformat	= RVIN_DEFAULT_FORMAT;
	vin->format.width = RVIN_DEFAULT_WIDTH;
	vin->format.height = RVIN_DEFAULT_HEIGHT;
	vin->format.colorspace = RVIN_DEFAULT_COLORSPACE;

	ret = rvin_format_align(vin, &vin->format);
	if (ret)
		return ret;

	if (vin->info->use_mc) {
		vdev->fops = &rvin_mc_fops;
		vdev->ioctl_ops = &rvin_mc_ioctl_ops;
	} else {
		vdev->fops = &rvin_fops;
		vdev->ioctl_ops = &rvin_ioctl_ops;
	}

	ret = video_register_device(&vin->vdev, VFL_TYPE_GRABBER, -1);
	if (ret) {
		vin_err(vin, "Failed to register video device\n");
		return ret;
	}

	video_set_drvdata(&vin->vdev, vin);

	vin_err(vin, "Device registered as %s\n",
		  video_device_node_name(&vin->vdev));

	return ret;
}
