/*
 * Driver for IMX219 CMOS Image Sensor from Sony
 *
 * Copyright (C) 2014, Andrew Chew <achew@nvidia.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */
#include <linux/clk.h>//RVC add
#include <linux/delay.h>
#include <linux/i2c.h>
#include <linux/v4l2-mediabus.h>
#include <linux/slab.h>
#include <linux/videodev2.h>
#include <linux/module.h>
//#include <media/v4l2-mediabus.h>
#include <media/soc_camera.h>
#include <media/v4l2-async.h>
#include <media/v4l2-clk.h>
#include <media/v4l2-subdev.h>
/* IMX219 supported geometry */
#define IMX219_WIDTH			3264//3280
#define IMX219_HEIGHT			2460
#define IMX219_TABLE_END		0xffff
#define IMX219_ANALOGUE_GAIN_MULTIPLIER	256
#define IMX219_ANALOGUE_GAIN_MIN	(1 * IMX219_ANALOGUE_GAIN_MULTIPLIER)
#define IMX219_ANALOGUE_GAIN_MAX	(8 * IMX219_ANALOGUE_GAIN_MULTIPLIER)
/* In dB*256 */
#define IMX219_DIGITAL_GAIN_MIN		256
#define IMX219_DIGITAL_GAIN_MAX		4095
struct imx219_reg {
	u16 addr;
	u8 val;
};

#define V4L2_MBUS_FROM_MEDIA_BUS_FMT(name)	\
	V4L2_MBUS_FMT_ ## name = MEDIA_BUS_FMT_ ## name

enum v4l2_mbus_pixelcode {//RVC add to fix error pixelcode
	V4L2_MBUS_FROM_MEDIA_BUS_FMT(FIXED),

	V4L2_MBUS_FROM_MEDIA_BUS_FMT(RGB444_2X8_PADHI_BE),
	V4L2_MBUS_FROM_MEDIA_BUS_FMT(RGB444_2X8_PADHI_LE),
	V4L2_MBUS_FROM_MEDIA_BUS_FMT(RGB555_2X8_PADHI_BE),
	V4L2_MBUS_FROM_MEDIA_BUS_FMT(RGB555_2X8_PADHI_LE),
	V4L2_MBUS_FROM_MEDIA_BUS_FMT(BGR565_2X8_BE),
	V4L2_MBUS_FROM_MEDIA_BUS_FMT(BGR565_2X8_LE),
	V4L2_MBUS_FROM_MEDIA_BUS_FMT(RGB565_2X8_BE),
	V4L2_MBUS_FROM_MEDIA_BUS_FMT(RGB565_2X8_LE),
	V4L2_MBUS_FROM_MEDIA_BUS_FMT(RGB666_1X18),
	V4L2_MBUS_FROM_MEDIA_BUS_FMT(RGB888_1X24),
	V4L2_MBUS_FROM_MEDIA_BUS_FMT(RGB888_2X12_BE),
	V4L2_MBUS_FROM_MEDIA_BUS_FMT(RGB888_2X12_LE),
	V4L2_MBUS_FROM_MEDIA_BUS_FMT(ARGB8888_1X32),

	V4L2_MBUS_FROM_MEDIA_BUS_FMT(Y8_1X8),
	V4L2_MBUS_FROM_MEDIA_BUS_FMT(UV8_1X8),
	V4L2_MBUS_FROM_MEDIA_BUS_FMT(UYVY8_1_5X8),
	V4L2_MBUS_FROM_MEDIA_BUS_FMT(VYUY8_1_5X8),
	V4L2_MBUS_FROM_MEDIA_BUS_FMT(YUYV8_1_5X8),
	V4L2_MBUS_FROM_MEDIA_BUS_FMT(YVYU8_1_5X8),
	V4L2_MBUS_FROM_MEDIA_BUS_FMT(UYVY8_2X8),
	V4L2_MBUS_FROM_MEDIA_BUS_FMT(VYUY8_2X8),
	V4L2_MBUS_FROM_MEDIA_BUS_FMT(YUYV8_2X8),
	V4L2_MBUS_FROM_MEDIA_BUS_FMT(YVYU8_2X8),
	V4L2_MBUS_FROM_MEDIA_BUS_FMT(Y10_1X10),
	V4L2_MBUS_FROM_MEDIA_BUS_FMT(UYVY10_2X10),
	V4L2_MBUS_FROM_MEDIA_BUS_FMT(VYUY10_2X10),
	V4L2_MBUS_FROM_MEDIA_BUS_FMT(YUYV10_2X10),
	V4L2_MBUS_FROM_MEDIA_BUS_FMT(YVYU10_2X10),
	V4L2_MBUS_FROM_MEDIA_BUS_FMT(Y12_1X12),
	V4L2_MBUS_FROM_MEDIA_BUS_FMT(UYVY8_1X16),
	V4L2_MBUS_FROM_MEDIA_BUS_FMT(VYUY8_1X16),
	V4L2_MBUS_FROM_MEDIA_BUS_FMT(YUYV8_1X16),
	V4L2_MBUS_FROM_MEDIA_BUS_FMT(YVYU8_1X16),
	V4L2_MBUS_FROM_MEDIA_BUS_FMT(YDYUYDYV8_1X16),
	V4L2_MBUS_FROM_MEDIA_BUS_FMT(UYVY10_1X20),
	V4L2_MBUS_FROM_MEDIA_BUS_FMT(VYUY10_1X20),
	V4L2_MBUS_FROM_MEDIA_BUS_FMT(YUYV10_1X20),
	V4L2_MBUS_FROM_MEDIA_BUS_FMT(YVYU10_1X20),
	V4L2_MBUS_FROM_MEDIA_BUS_FMT(YUV10_1X30),
	V4L2_MBUS_FROM_MEDIA_BUS_FMT(AYUV8_1X32),
	V4L2_MBUS_FROM_MEDIA_BUS_FMT(UYVY12_2X12),
	V4L2_MBUS_FROM_MEDIA_BUS_FMT(VYUY12_2X12),
	V4L2_MBUS_FROM_MEDIA_BUS_FMT(YUYV12_2X12),
	V4L2_MBUS_FROM_MEDIA_BUS_FMT(YVYU12_2X12),
	V4L2_MBUS_FROM_MEDIA_BUS_FMT(UYVY12_1X24),
	V4L2_MBUS_FROM_MEDIA_BUS_FMT(VYUY12_1X24),
	V4L2_MBUS_FROM_MEDIA_BUS_FMT(YUYV12_1X24),
	V4L2_MBUS_FROM_MEDIA_BUS_FMT(YVYU12_1X24),

	V4L2_MBUS_FROM_MEDIA_BUS_FMT(SBGGR8_1X8),
	V4L2_MBUS_FROM_MEDIA_BUS_FMT(SGBRG8_1X8),
	V4L2_MBUS_FROM_MEDIA_BUS_FMT(SGRBG8_1X8),
	V4L2_MBUS_FROM_MEDIA_BUS_FMT(SRGGB8_1X8),
	V4L2_MBUS_FROM_MEDIA_BUS_FMT(SBGGR10_ALAW8_1X8),
	V4L2_MBUS_FROM_MEDIA_BUS_FMT(SGBRG10_ALAW8_1X8),
	V4L2_MBUS_FROM_MEDIA_BUS_FMT(SGRBG10_ALAW8_1X8),
	V4L2_MBUS_FROM_MEDIA_BUS_FMT(SRGGB10_ALAW8_1X8),
	V4L2_MBUS_FROM_MEDIA_BUS_FMT(SBGGR10_DPCM8_1X8),
	V4L2_MBUS_FROM_MEDIA_BUS_FMT(SGBRG10_DPCM8_1X8),
	V4L2_MBUS_FROM_MEDIA_BUS_FMT(SGRBG10_DPCM8_1X8),
	V4L2_MBUS_FROM_MEDIA_BUS_FMT(SRGGB10_DPCM8_1X8),
	V4L2_MBUS_FROM_MEDIA_BUS_FMT(SBGGR10_2X8_PADHI_BE),
	V4L2_MBUS_FROM_MEDIA_BUS_FMT(SBGGR10_2X8_PADHI_LE),
	V4L2_MBUS_FROM_MEDIA_BUS_FMT(SBGGR10_2X8_PADLO_BE),
	V4L2_MBUS_FROM_MEDIA_BUS_FMT(SBGGR10_2X8_PADLO_LE),
	V4L2_MBUS_FROM_MEDIA_BUS_FMT(SBGGR10_1X10),
	V4L2_MBUS_FROM_MEDIA_BUS_FMT(SGBRG10_1X10),
	V4L2_MBUS_FROM_MEDIA_BUS_FMT(SGRBG10_1X10),
	V4L2_MBUS_FROM_MEDIA_BUS_FMT(SRGGB10_1X10),
	V4L2_MBUS_FROM_MEDIA_BUS_FMT(SBGGR12_1X12),
	V4L2_MBUS_FROM_MEDIA_BUS_FMT(SGBRG12_1X12),
	V4L2_MBUS_FROM_MEDIA_BUS_FMT(SGRBG12_1X12),
	V4L2_MBUS_FROM_MEDIA_BUS_FMT(SRGGB12_1X12),

	V4L2_MBUS_FROM_MEDIA_BUS_FMT(JPEG_1X8),

	V4L2_MBUS_FROM_MEDIA_BUS_FMT(S5C_UYVY_JPEG_1X8),

	V4L2_MBUS_FROM_MEDIA_BUS_FMT(AHSV8888_1X32),
};

static const struct imx219_reg miscellaneous[] = {
	{ 0x30EB, 0x05 }, /* Access Code for address over 0x3000 */
	{ 0x30EB, 0x0C }, /* Access Code for address over 0x3000 */
	{ 0x300A, 0xFF }, /* Access Code for address over 0x3000 */
	{ 0x300B, 0xFF }, /* Access Code for address over 0x3000 */
	{ 0x30EB, 0x05 }, /* Access Code for address over 0x3000 */
	{ 0x30EB, 0x09 }, /* Access Code for address over 0x3000 */
	{ 0x0114, 0x03 }, /* CSI_LANE_MODE[1:0} */
	{ 0x0128, 0x00 }, /* DPHY_CNTRL */
	{ 0x012A, 0x18 }, /* EXCK_FREQ[15:8] */
	{ 0x012B, 0x00 }, /* EXCK_FREQ[7:0] */
	{ 0x0160, 0x0A }, /* FRM_LENGTH_A[15:8] */
	{ 0x0161, 0x83 }, /* FRM_LENGTH_A[7:0] */
	{ 0x0162, 0x0D }, /* LINE_LENGTH_A[15:8] */
	{ 0x0163, 0x78 }, /* LINE_LENGTH_A[7:0] */
	{ 0x0170, 0x01 }, /* X_ODD_INC_A[2:0] */
	{ 0x0171, 0x01 }, /* Y_ODD_INC_A[2:0] */
	{ 0x0174, 0x00 }, /* BINNING_MODE_H_A */
	{ 0x0175, 0x00 }, /* BINNING_MODE_V_A */
	{ 0x018C, 0x0A }, /* CSI_DATA_FORMAT_A[15:8] */
	{ 0x018D, 0x0A }, /* CSI_DATA_FORMAT_A[7:0] */
	{ 0x0301, 0x05 }, /* VTPXCK_DIV */
	{ 0x0303, 0x01 }, /* VTSYCK_DIV */
	{ 0x0304, 0x03 }, /* PREPLLCK_VT_DIV[3:0] */
	{ 0x0305, 0x03 }, /* PREPLLCK_OP_DIV[3:0] */
	{ 0x0306, 0x00 }, /* PLL_VT_MPY[10:8] */
	{ 0x0307, 0x57 }, /* PLL_VT_MPY[7:0] */
	{ 0x0309, 0x0A }, /* OPPXCK_DIV[4:0] */
	{ 0x030B, 0x01 }, /* OPSYCK_DIV */
	{ 0x030C, 0x00 }, /* PLL_OP_MPY[10:8] */
	{ 0x030D, 0x5A }, /* PLL_OP_MPY[7:0] */
	{ 0x455E, 0x00 }, /* CIS Tuning */
	{ 0x471E, 0x4B }, /* CIS Tuning */
	{ 0x4767, 0x0F }, /* CIS Tuning */
	{ 0x4750, 0x14 }, /* CIS Tuning */
	{ 0x4540, 0x00 }, /* CIS Tuning */
	{ 0x47B4, 0x14 }, /* CIS Tuning */
	{ 0x4713, 0x30 }, /* CIS Tuning */
	{ 0x478B, 0x10 }, /* CIS Tuning */
	{ 0x478F, 0x10 }, /* CIS Tuning */
	{ 0x4793, 0x10 }, /* CIS Tuning */
	{ 0x4797, 0x0E }, /* CIS Tuning */
	{ 0x479B, 0x0E }, /* CIS Tuning */
	{ IMX219_TABLE_END, 0x00 }
};
static const struct imx219_reg start[] = {
	{ 0x0100, 0x01 }, /* mode select streaming on */
	{ IMX219_TABLE_END, 0x00 }
};
static const struct imx219_reg stop[] = {
	{ 0x0100, 0x00 }, /* mode select streaming off */
	{ IMX219_TABLE_END, 0x00 }
};
enum {
	TEST_PATTERN_DISABLED,
	TEST_PATTERN_SOLID_BLACK,
	TEST_PATTERN_SOLID_WHITE,
	TEST_PATTERN_SOLID_RED,
	TEST_PATTERN_SOLID_GREEN,
	TEST_PATTERN_SOLID_BLUE,
	TEST_PATTERN_COLOR_BAR,
	TEST_PATTERN_FADE_TO_GREY_COLOR_BAR,
	TEST_PATTERN_PN9,
	TEST_PATTERN_16_SPLIT_COLOR_BAR,
	TEST_PATTERN_16_SPLIT_INVERTED_COLOR_BAR,
	TEST_PATTERN_COLUMN_COUNTER,
	TEST_PATTERN_INVERTED_COLUMN_COUNTER,
	TEST_PATTERN_PN31,
	TEST_PATTERN_MAX
};
static const char * const tp_qmenu[] = {
	"Disabled",
	"Solid Black",
	"Solid White",
	"Solid Red",
	"Solid Green",
	"Solid Blue",
	"Color Bar",
	"Fade to Grey Color Bar",
	"PN9",
	"16 Split Color Bar",
	"16 Split Inverted Color Bar",
	"Column Counter",
	"Inverted Column Counter",
	"PN31",
};
/* IMX219 has only one fixed colorspace per pixelcode */
struct imx219_datafmt {
	enum v4l2_mbus_pixelcode	code;
	enum v4l2_colorspace		colorspace;
};
static struct mt9v111_mbus_fmt {
	u32	code;
} mt9v111_formats[] = {
	{
		.code	= MEDIA_BUS_FMT_UYVY8_2X8,
	},
	{
		.code	= MEDIA_BUS_FMT_YUYV8_2X8,
	},
	{
		.code	= MEDIA_BUS_FMT_VYUY8_2X8,
	},
	{
		.code	= MEDIA_BUS_FMT_YVYU8_2X8,
	},
};

static u32 mt9v111_frame_intervals[] = {5, 10, 15, 20, 30};

static struct v4l2_rect mt9v111_frame_sizes[] = {
	{
		.width	= 640,
		.height	= 480,
	},
	{
		.width	= 352,
		.height	= 288
	},
	{
		.width	= 320,
		.height	= 240,
	},
	{
		.width	= 176,
		.height	= 144,
	},
	{
		.width	= 160,
		.height	= 120,
	},
};
#define SIZEOF_I2C_TRANSBUF 32
enum pads {
	SINK,
	SOURCE,
	NR_PADS,
};
struct imx219 {
	struct v4l2_subdev		subdev;
	struct v4l2_ctrl_handler	ctrl_handler;
	struct v4l2_mbus_framefmt fmt;
	//const struct imx219_datafmt	*fmt;
	struct v4l2_clk			*clk;
	struct media_pad		pads[NR_PADS];//RVC add for pad_entity
	struct clk			*xclk;//RVC add for enable clk
	struct mutex			lock;//RVC add for lock
	struct v4l2_rect		crop_rect;
	int				hflip;
	int				vflip;
	u8				analogue_gain;
	u16				digital_gain; /* bits 11:0 */
	u16				test_pattern;
	u16				test_pattern_solid_color_r;
	u16				test_pattern_solid_color_gr;
	u16				test_pattern_solid_color_b;
	u16				test_pattern_solid_color_gb;
};
static inline struct v4l2_subdev *adv748x_get_remote_sd(struct media_pad *pad)
{
	pad = media_entity_remote_pad(pad);
	if (!pad)
		return NULL;

	return media_entity_to_v4l2_subdev(pad->entity);
}
static const struct imx219_datafmt imx219_colour_fmts[] = {
	{V4L2_MBUS_FMT_SRGGB10_1X10, V4L2_COLORSPACE_SRGB},
};
static struct imx219 *to_imx219(const struct i2c_client *client)
{
	return container_of(i2c_get_clientdata(client), struct imx219, subdev);
}
/* Find a data format by a pixel code in an array */
static const struct imx219_datafmt *imx219_find_datafmt(
	enum v4l2_mbus_pixelcode code)
{
	int i;
	for (i = 0; i < ARRAY_SIZE(imx219_colour_fmts); i++)
		if (imx219_colour_fmts[i].code == code)
			return imx219_colour_fmts + i;
	return NULL;
}
static int reg_write(struct i2c_client *client, const u16 addr, const u8 data)
{
	struct i2c_adapter *adap = client->adapter;
	struct i2c_msg msg;
	printk("reg_write\n");
	u8 tx[3];
	int ret;
	msg.addr = client->addr;
	msg.buf = tx;
	msg.len = 3;
	msg.flags = 0;
	tx[0] = addr >> 8;
	tx[1] = addr & 0xff;
	tx[2] = data;
	ret = i2c_transfer(adap, &msg, 1);
	mdelay(2);
	return ret == 1 ? 0 : -EIO;
}
static int reg_read(struct i2c_client *client, const u16 addr)
{
	u8 buf[2] = {addr >> 8, addr & 0xff};
	int ret;
	struct i2c_msg msgs[] = {
		{
			.addr  = client->addr,
			.flags = 0,
			.len   = 2,
			.buf   = buf,
		}, {
			.addr  = client->addr,
			.flags = I2C_M_RD,
			.len   = 1,
			.buf   = buf,
		},
	};
	ret = i2c_transfer(client->adapter, msgs, ARRAY_SIZE(msgs));
	if (ret < 0) {
		dev_warn(&client->dev, "Reading register %x from %x failed\n",
			 addr, client->addr);
		return ret;
	}
	return buf[0];
}
static int reg_write_table(struct i2c_client *client,
			   const struct imx219_reg table[])
{
	const struct imx219_reg *reg;
	int ret;
	for (reg = table; reg->addr != IMX219_TABLE_END; reg++) {
		ret = reg_write(client, reg->addr, reg->val);
		if (ret < 0)
			return ret;
	}
	return 0;
}
/* V4L2 subdev video operations */
static int imx219_s_stream(struct v4l2_subdev *sd, int enable)
{
	struct i2c_client *client = v4l2_get_subdevdata(sd);
	struct imx219 *priv = to_imx219(client);
	u8 reg = 0x00;
	int ret;
	printk("imx219_s_stream\n");
	if (!enable)
		return reg_write_table(client, stop);
	ret = reg_write_table(client, miscellaneous);
	if (ret)
		return ret;
	/* Handle crop */
	ret = reg_write(client, 0x0164, priv->crop_rect.left >> 8);
	ret |= reg_write(client, 0x0165, priv->crop_rect.left & 0xff);
	ret |= reg_write(client, 0x0166, (priv->crop_rect.width - 1) >> 8);
	ret |= reg_write(client, 0x0167, (priv->crop_rect.width - 1) & 0xff);
	ret |= reg_write(client, 0x0168, priv->crop_rect.top >> 8);
	ret |= reg_write(client, 0x0169, priv->crop_rect.top & 0xff);
	ret |= reg_write(client, 0x016A, (priv->crop_rect.height - 1) >> 8);
	ret |= reg_write(client, 0x016B, (priv->crop_rect.height - 1) & 0xff);
	ret |= reg_write(client, 0x016C, priv->crop_rect.width >> 8);
	ret |= reg_write(client, 0x016D, priv->crop_rect.width & 0xff);
	ret |= reg_write(client, 0x016E, priv->crop_rect.height >> 8);
	ret |= reg_write(client, 0x016F, priv->crop_rect.height & 0xff);
	if (ret)
		return ret;
	/* Handle flip/mirror */
	if (priv->hflip)
		reg |= 0x1;
	if (priv->vflip)
		reg |= 0x2;
	ret = reg_write(client, 0x0172, reg);
	if (ret)
		return ret;
	/* Handle analogue gain */
	ret = reg_write(client, 0x0157, priv->analogue_gain);
	if (ret)
		return ret;
	/* Handle digital gain */
	ret = reg_write(client, 0x0158, priv->digital_gain >> 8);
	ret |= reg_write(client, 0x0159, priv->digital_gain & 0xff);
	if (ret)
		return ret;
	/* Handle test pattern */
	if (priv->test_pattern) {
		ret = reg_write(client, 0x0600, priv->test_pattern >> 8);
		ret |= reg_write(client, 0x0601, priv->test_pattern & 0xff);
		ret |= reg_write(client, 0x0602,
				 priv->test_pattern_solid_color_r >> 8);
		ret |= reg_write(client, 0x0603,
				 priv->test_pattern_solid_color_r & 0xff);
		ret |= reg_write(client, 0x0604,
				 priv->test_pattern_solid_color_gr >> 8);
		ret |= reg_write(client, 0x0605,
				 priv->test_pattern_solid_color_gr & 0xff);
		ret |= reg_write(client, 0x0606,
				 priv->test_pattern_solid_color_b >> 8);
		ret |= reg_write(client, 0x0607,
				 priv->test_pattern_solid_color_b & 0xff);
		ret |= reg_write(client, 0x0608,
				 priv->test_pattern_solid_color_gb >> 8);
		ret |= reg_write(client, 0x0609,
				 priv->test_pattern_solid_color_gb & 0xff);
		ret |= reg_write(client, 0x0620, priv->crop_rect.left >> 8);
		ret |= reg_write(client, 0x0621, priv->crop_rect.left & 0xff);
		ret |= reg_write(client, 0x0622, priv->crop_rect.top >> 8);
		ret |= reg_write(client, 0x0623, priv->crop_rect.top & 0xff);
		ret |= reg_write(client, 0x0624, priv->crop_rect.width >> 8);
		ret |= reg_write(client, 0x0625, priv->crop_rect.width & 0xff);
		ret |= reg_write(client, 0x0626, priv->crop_rect.height >> 8);
		ret |= reg_write(client, 0x0627, priv->crop_rect.height & 0xff);
	} else {
		ret = reg_write(client, 0x0600, 0x00);
		ret |= reg_write(client, 0x0601, 0x00);
	}
	if (ret)
		return ret;
	return reg_write_table(client, start);
}

#if 0
static int imx219_cropcap(struct v4l2_subdev *sd, struct v4l2_cropcap *crop)
{
	if (crop->type != V4L2_BUF_TYPE_VIDEO_CAPTURE)
		return -EINVAL;
	crop->bounds.left		= 0;
	crop->bounds.top		= 0;
	crop->bounds.width		= IMX219_WIDTH;
	crop->bounds.height		= IMX219_HEIGHT;
	crop->defrect			= crop->bounds;
	crop->pixelaspect.numerator	= 1;
	crop->pixelaspect.denominator	= 1;
	return 0;
}
static int imx219_g_crop(struct v4l2_subdev *sd, struct v4l2_crop *crop)
{
	struct i2c_client *client = v4l2_get_subdevdata(sd);
	struct imx219 *priv = to_imx219(client);
	crop->type	= V4L2_BUF_TYPE_VIDEO_CAPTURE;
	crop->c		= priv->crop_rect;
	return 0;
}
static int imx219_s_crop(struct v4l2_subdev *sd, const struct v4l2_crop *crop)
{
	struct i2c_client *client = v4l2_get_subdevdata(sd);
	struct imx219 *priv = to_imx219(client);
	struct v4l2_rect rect = crop->c;
	u8 reg;
	rect.left	= ALIGN(rect.left, 2);
	rect.width	= ALIGN(rect.width, 2);
	rect.top	= ALIGN(rect.top, 2);
	rect.height	= ALIGN(rect.height, 2);
	soc_camera_limit_side(&rect.left, &rect.width, 0, 2, IMX219_WIDTH);
	soc_camera_limit_side(&rect.top, &rect.height, 0, 2, IMX219_HEIGHT);
	priv->crop_rect = rect;
	/* If enabled, apply settings immediately */
	reg = reg_read(client, 0x0100);
	if ((reg & 0x1f) == 0x01)
		imx219_s_stream(sd, 1);
	return 0;
}
static int imx219_enum_mbus_fmt(struct v4l2_subdev *sd, unsigned int index,
				enum v4l2_mbus_pixelcode *code)
{
	if (index >= ARRAY_SIZE(imx219_colour_fmts))
		return -EINVAL;
	*code = imx219_colour_fmts[index].code;
	return 0;
}

static int imx219_g_mbus_fmt(struct v4l2_subdev *sd,
			     struct v4l2_mbus_framefmt *mf)
{
	struct i2c_client *client = v4l2_get_subdevdata(sd);
	struct imx219 *priv = to_imx219(client);
	const struct imx219_datafmt *fmt = priv->fmt;
	mf->code	= fmt->code;
	mf->colorspace	= fmt->colorspace;
	mf->width	= IMX219_WIDTH;
	mf->height	= IMX219_HEIGHT;
	mf->field	= V4L2_FIELD_NONE;
	return 0;
}
static int imx219_try_mbus_fmt(struct v4l2_subdev *sd,
			       struct v4l2_mbus_framefmt *mf)
{
	const struct imx219_datafmt *fmt = imx219_find_datafmt(mf->code);
	dev_dbg(sd->v4l2_dev->dev, "%s(%u)\n", __func__, mf->code);
	if (!fmt) {
		mf->code	= imx219_colour_fmts[0].code;
		mf->colorspace	= imx219_colour_fmts[0].colorspace;
	}
	mf->width	= IMX219_WIDTH;
	mf->height	= IMX219_HEIGHT;
	mf->field	= V4L2_FIELD_NONE;
	return 0;
}
static int imx219_s_mbus_fmt(struct v4l2_subdev *sd,
			     struct v4l2_mbus_framefmt *mf)
{
	struct i2c_client *client = v4l2_get_subdevdata(sd);
	struct imx219 *priv = to_imx219(client);
	dev_dbg(sd->v4l2_dev->dev, "%s(%u)\n", __func__, mf->code);
	/* MIPI CSI could have changed the format, double-check */
	if (!imx219_find_datafmt(mf->code))
		return -EINVAL;
	imx219_try_mbus_fmt(sd, mf);
	priv->fmt = imx219_find_datafmt(mf->code);
	return 0;
}
static int imx219_g_mbus_config(struct v4l2_subdev *sd,
				struct v4l2_mbus_config *cfg)
{
	cfg->type = V4L2_MBUS_CSI2;
	cfg->flags = V4L2_MBUS_CSI2_2_LANE |
		V4L2_MBUS_CSI2_CHANNEL_0 |
		V4L2_MBUS_CSI2_CONTINUOUS_CLOCK;
	return 0;
}
#endif //RVC remove
/* V4L2 subdev core operations */
static int imx219_s_power(struct v4l2_subdev *sd, int on)
{
	struct i2c_client *client = v4l2_get_subdevdata(sd);
	//struct soc_camera_subdev_desc *ssdd = soc_camera_i2c_to_desc(client);
	struct imx219 *priv = to_imx219(client);
	//return soc_camera_set_power(&client->dev, ssdd, priv->clk, on);//RVC
	int ret = 0;
printk("imx219_s_power\n");
	mutex_lock(&priv->lock);
	
	ret = clk_prepare_enable(priv->xclk);
	if (ret < 0) {
		dev_err(&client->dev, "clk prepare enable failed\n");
		goto out;
	}

out:
	mutex_unlock(&priv->lock);
	
	return ret;
}
/* V4L2 ctrl operations */
static int imx219_s_ctrl_test_pattern(struct v4l2_ctrl *ctrl)
{
	struct imx219 *priv =
		container_of(ctrl->handler, struct imx219, ctrl_handler);
	switch (ctrl->val) {
	case TEST_PATTERN_DISABLED:
		priv->test_pattern = 0x0000;
		break;
	case TEST_PATTERN_SOLID_BLACK:
		priv->test_pattern = 0x0001;
		priv->test_pattern_solid_color_r = 0x0000;
		priv->test_pattern_solid_color_gr = 0x0000;
		priv->test_pattern_solid_color_b = 0x0000;
		priv->test_pattern_solid_color_gb = 0x0000;
		break;
	case TEST_PATTERN_SOLID_WHITE:
		priv->test_pattern = 0x0001;
		priv->test_pattern_solid_color_r = 0x0fff;
		priv->test_pattern_solid_color_gr = 0x0fff;
		priv->test_pattern_solid_color_b = 0x0fff;
		priv->test_pattern_solid_color_gb = 0x0fff;
		break;
	case TEST_PATTERN_SOLID_RED:
		priv->test_pattern = 0x0001;
		priv->test_pattern_solid_color_r = 0x0fff;
		priv->test_pattern_solid_color_gr = 0x0000;
		priv->test_pattern_solid_color_b = 0x0000;
		priv->test_pattern_solid_color_gb = 0x0000;
		break;
	case TEST_PATTERN_SOLID_GREEN:
		priv->test_pattern = 0x0001;
		priv->test_pattern_solid_color_r = 0x0000;
		priv->test_pattern_solid_color_gr = 0x0fff;
		priv->test_pattern_solid_color_b = 0x0000;
		priv->test_pattern_solid_color_gb = 0x0fff;
		break;
	case TEST_PATTERN_SOLID_BLUE:
		priv->test_pattern = 0x0001;
		priv->test_pattern_solid_color_r = 0x0000;
		priv->test_pattern_solid_color_gr = 0x0000;
		priv->test_pattern_solid_color_b = 0x0fff;
		priv->test_pattern_solid_color_gb = 0x0000;
		break;
	case TEST_PATTERN_COLOR_BAR:
		priv->test_pattern = 0x0002;
		break;
	case TEST_PATTERN_FADE_TO_GREY_COLOR_BAR:
		priv->test_pattern = 0x0003;
		break;
	case TEST_PATTERN_PN9:
		priv->test_pattern = 0x0004;
		break;
	case TEST_PATTERN_16_SPLIT_COLOR_BAR:
		priv->test_pattern = 0x0005;
		break;
	case TEST_PATTERN_16_SPLIT_INVERTED_COLOR_BAR:
		priv->test_pattern = 0x0006;
		break;
	case TEST_PATTERN_COLUMN_COUNTER:
		priv->test_pattern = 0x0007;
		break;
	case TEST_PATTERN_INVERTED_COLUMN_COUNTER:
		priv->test_pattern = 0x0008;
		break;
	case TEST_PATTERN_PN31:
		priv->test_pattern = 0x0009;
		break;
	default:
		return -EINVAL;
	}
	return 0;
}

//RVC add for subdev
static int mt9v111_s_frame_interval(struct v4l2_subdev *sd,
				    struct v4l2_subdev_frame_interval *ival)
{	
	printk("mt9v111_s_frame_interval\n");
	return 0;
}

static int mt9v111_g_frame_interval(struct v4l2_subdev *sd,
				    struct v4l2_subdev_frame_interval *ival)
{
	printk("mt9v111_g_frame_interval\n");

	return 0;
}
static int mt9v111_enum_mbus_code(struct v4l2_subdev *subdev,
				  struct v4l2_subdev_pad_config *cfg,
				  struct v4l2_subdev_mbus_code_enum *code)
{
	printk("mt9v111_enum_mbus_code \n");
	if (code->pad || code->index > ARRAY_SIZE(mt9v111_formats) - 1)
		return -EINVAL;

	code->code = mt9v111_formats[code->index].code;
	return 0;
}

static int mt9v111_enum_frame_interval(struct v4l2_subdev *sd,
				struct v4l2_subdev_pad_config *cfg,
				struct v4l2_subdev_frame_interval_enum *fie)
{
	printk("mt9v111_enum_frame_interval \n");

	return 0;
}
static int mt9v111_enum_frame_size(struct v4l2_subdev *subdev,
				   struct v4l2_subdev_pad_config *cfg,
				   struct v4l2_subdev_frame_size_enum *fse)
{
	if (fse->pad || fse->index > ARRAY_SIZE(mt9v111_frame_sizes))
		return -EINVAL;
	printk("mt9v111_enum_frame_size \n");
	fse->min_width = mt9v111_frame_sizes[fse->index].width;
	fse->max_width = mt9v111_frame_sizes[fse->index].width;
	fse->min_height = mt9v111_frame_sizes[fse->index].height;
	fse->max_height = mt9v111_frame_sizes[fse->index].height;

	
	return 0;
}

static int mt9v111_get_format(struct v4l2_subdev *subdev,
			      struct v4l2_subdev_pad_config *cfg,
			      struct v4l2_subdev_format *format)
{

printk("mt9v111_get_format \n");
	return 0;
}

static int mt9v111_set_format(struct v4l2_subdev *subdev,
			      struct v4l2_subdev_pad_config *cfg,
			      struct v4l2_subdev_format *format)
{

printk("mt9v111_set_format \n");

	return 0;
}


//RVC
static int imx219_s_ctrl(struct v4l2_ctrl *ctrl)
{
	struct imx219 *priv =
		container_of(ctrl->handler, struct imx219, ctrl_handler);
	struct i2c_client *client = v4l2_get_subdevdata(&priv->subdev);
	u8 reg;
printk("imx219_s_ctrl\n");
	switch (ctrl->id) {
	case V4L2_CID_HFLIP:
		priv->hflip = ctrl->val;
		break;
	case V4L2_CID_VFLIP:
		priv->vflip = ctrl->val;
		break;
	case V4L2_CID_ANALOGUE_GAIN:
		/*
		 * Register value goes from 0 to 224, and the analog gain
		 * setting is 256 / (256 - reg).  This results in a total gain
		 * of 1.0f to 8.0f.  We multiply the control setting by some
		 * big number IMX219_ANALOGUE_GAIN_MULTIPLIER to make use of
		 * the full resolution of this register.
		 */
		priv->analogue_gain =
			256 - ((256 * IMX219_ANALOGUE_GAIN_MULTIPLIER) /
			        ctrl->val);
		break;
	case V4L2_CID_GAIN:
		/*
		 * Register value goes from 256 to 4095, and the digital gain
		 * setting is reg / 256.  This results in a total gain of 0dB
		 * to 24dB.
		 */
		priv->digital_gain = ctrl->val;
		break;
	case V4L2_CID_TEST_PATTERN:
		return imx219_s_ctrl_test_pattern(ctrl);
	default:
		return -EINVAL;
	}
	/* If enabled, apply settings immediately */
	reg = reg_read(client, 0x0100);
	if ((reg & 0x1f) == 0x01)
		imx219_s_stream(&priv->subdev, 1);
	return 0;
}
/* Various V4L2 operations tables */
static struct v4l2_subdev_video_ops imx219_subdev_video_ops = {
	.s_stream	= imx219_s_stream,
	.s_frame_interval	= mt9v111_s_frame_interval,//RVC
	.g_frame_interval	= mt9v111_g_frame_interval,//RVC
	/*.cropcap	= imx219_cropcap,
	.g_crop		= imx219_g_crop,
	.s_crop		= imx219_s_crop,
	.enum_mbus_fmt	= imx219_enum_mbus_fmt,
	.g_mbus_fmt	= imx219_g_mbus_fmt,
	.try_mbus_fmt	= imx219_try_mbus_fmt,
	.s_mbus_fmt	= imx219_s_mbus_fmt,
	.g_mbus_config	= imx219_g_mbus_config,*///RVC removed
};
static struct v4l2_subdev_core_ops imx219_subdev_core_ops = {
	.s_power	= imx219_s_power,
};

static const struct v4l2_ctrl_ops imx219_ctrl_ops = {
	.s_ctrl		= imx219_s_ctrl,
};

static const struct v4l2_subdev_pad_ops mt9v111_pad_ops = {
	.enum_mbus_code		= mt9v111_enum_mbus_code,
	.enum_frame_size	= mt9v111_enum_frame_size,
	.enum_frame_interval	= mt9v111_enum_frame_interval,
	.get_fmt		= mt9v111_get_format,
	.set_fmt		= mt9v111_set_format,
};

static struct v4l2_subdev_ops imx219_subdev_ops = {
	.core		= &imx219_subdev_core_ops,
	.video		= &imx219_subdev_video_ops,
	.pad		= &mt9v111_pad_ops,//RVC add for subdev
};
//#ifdef CONFIG_MEDIA_CONTROLLER
static const struct media_entity_operations mt9v111_subdev_entity_ops = {
	.link_validate = v4l2_subdev_link_validate,
};

static int mt9v111_open(struct v4l2_subdev *sd, struct v4l2_subdev_fh *fh)
{
	struct v4l2_mbus_framefmt *__fmt =
				v4l2_subdev_get_try_format(sd, fh->pad, 0);
printk("mt9v111_open\n");

	return 0;
}

static const struct v4l2_subdev_internal_ops mt9v111_internal_ops = {
	.open = mt9v111_open,
};
//#endif
//RVC
static int imx219_video_probe(struct i2c_client *client)
{
	struct v4l2_subdev *subdev = i2c_get_clientdata(client);
	u16 model_id;
	u32 lot_id;
	u16 chip_id;
	int ret;
	ret = imx219_s_power(subdev, 1);
	if (ret < 0)
		return ret;
printk("imx219_video_probe check chip ID \n");
	/* Check and show model, lot, and chip ID. */
	ret = reg_read(client, 0x0000);
	if (ret < 0) {
		dev_err(&client->dev, "Failure to read Model ID (high byte)\n");
		goto done;
	}
	model_id = ret << 8;
	ret = reg_read(client, 0x0001);
	if (ret < 0) {
		dev_err(&client->dev, "Failure to read Model ID (low byte)\n");
		goto done;
	}
	model_id |= ret;
	ret = reg_read(client, 0x0004);
	if (ret < 0) {
		dev_err(&client->dev, "Failure to read Lot ID (high byte)\n");
		goto done;
	}
	lot_id = ret << 16;
	ret = reg_read(client, 0x0005);
	if (ret < 0) {
		dev_err(&client->dev, "Failure to read Lot ID (mid byte)\n");
		goto done;
	}
	lot_id |= ret << 8;
	ret = reg_read(client, 0x0006);
	if (ret < 0) {
		dev_err(&client->dev, "Failure to read Lot ID (low byte)\n");
		goto done;
	}
	lot_id |= ret;
	ret = reg_read(client, 0x000D);
	if (ret < 0) {
		dev_err(&client->dev, "Failure to read Chip ID (high byte)\n");
		goto done;
	}
	chip_id = ret << 8;
	ret = reg_read(client, 0x000E);
	if (ret < 0) {
		dev_err(&client->dev, "Failure to read Chip ID (low byte)\n");
		goto done;
	}
	chip_id |= ret;
	if (model_id != 0x0219) {
		dev_err(&client->dev, "Model ID: %x not supported!\n",
			model_id);
		ret = -ENODEV;
		goto done;
	}
	dev_info(&client->dev,
		 "Model ID 0x%04x, Lot ID 0x%06x, Chip ID 0x%04x\n",
		 model_id, lot_id, chip_id);
done:
	imx219_s_power(subdev, 0);
	return ret;
}
static int imx219_ctrls_init(struct v4l2_subdev *sd)
{
	struct i2c_client *client = v4l2_get_subdevdata(sd);
	struct imx219 *priv = to_imx219(client);
	int ret;
printk("imx219_ctrls_init\n");
	v4l2_ctrl_handler_init(&priv->ctrl_handler, 10);
	v4l2_ctrl_new_std(&priv->ctrl_handler, &imx219_ctrl_ops,
			  V4L2_CID_HFLIP, 0, 1, 1, 0);
	v4l2_ctrl_new_std(&priv->ctrl_handler, &imx219_ctrl_ops,
			  V4L2_CID_VFLIP, 0, 1, 1, 0);
	v4l2_ctrl_new_std(&priv->ctrl_handler, &imx219_ctrl_ops,
			  V4L2_CID_ANALOGUE_GAIN,
			  IMX219_ANALOGUE_GAIN_MIN,
			  IMX219_ANALOGUE_GAIN_MAX,
			  1,
			  IMX219_ANALOGUE_GAIN_MIN);
	v4l2_ctrl_new_std(&priv->ctrl_handler, &imx219_ctrl_ops,
			  V4L2_CID_GAIN,
			  IMX219_DIGITAL_GAIN_MIN,
			  IMX219_DIGITAL_GAIN_MAX,
			  1,
			  IMX219_DIGITAL_GAIN_MIN);
	v4l2_ctrl_new_std_menu_items(&priv->ctrl_handler, &imx219_ctrl_ops,
				     V4L2_CID_TEST_PATTERN,
				     ARRAY_SIZE(tp_qmenu) - 1, 0, 0, tp_qmenu);
	priv->subdev.ctrl_handler = &priv->ctrl_handler;
	if (priv->ctrl_handler.error) {
		dev_err(&client->dev, "Error %d adding controls\n",
			priv->ctrl_handler.error);
		ret = priv->ctrl_handler.error;
		goto error;
	}
	ret = v4l2_ctrl_handler_setup(&priv->ctrl_handler);
	if (ret < 0) {
		dev_err(&client->dev, "Error %d setting default controls\n",
			ret);
		goto error;
	}
	return 0;
error:
	v4l2_ctrl_handler_free(&priv->ctrl_handler);
	return ret;
}
#if 0
static int imx219_probe(struct i2c_client *client,
			const struct i2c_device_id *did)
{
	struct imx219 *priv;
	struct i2c_adapter *adapter = to_i2c_adapter(client->dev.parent);
	//struct soc_camera_subdev_desc *ssdd = soc_camera_i2c_to_desc(client);
	int ret;
	struct v4l2_subdev *sd;
	printk("imx219_probe \n");
	/*
	if (!ssdd) {
		dev_err(&client->dev, "IMX219: missing platform data!\n");
		return -EINVAL;//RVC
	}*/
	if (!i2c_check_functionality(adapter, I2C_FUNC_SMBUS_BYTE_DATA)) {
		dev_warn(&adapter->dev,
			 "I2C-Adapter doesn't support I2C_FUNC_SMBUS_BYTE\n");
		return -EIO;
	}
	priv = devm_kzalloc(&client->dev, sizeof(struct imx219), GFP_KERNEL);
	if (!priv)
		return -ENOMEM;
	/*RVC replace v4l2_clk by devm_clk*/
	/*
	priv->clk = v4l2_clk_get(&client->dev, "xclk");//mclk
	if (IS_ERR(priv->clk)) {
		dev_info(&client->dev, "Error %ld getting clock\n",
			 PTR_ERR(priv->clk));
		return -EPROBE_DEFER;
	}*/
		
	/* get system clock (xclk) */
	priv->xclk = devm_clk_get(&client->dev, NULL);
	if (IS_ERR(priv->xclk)) {
		dev_err(&client->dev, "could not get xclk");
		return PTR_ERR(priv->xclk);
	}
	//RVC1 end
	/*
	priv->fmt = &imx219_colour_fmts[0];
	priv->crop_rect.left	= 0;
	priv->crop_rect.top	= 0;
	priv->crop_rect.width	= IMX219_WIDTH;
	priv->crop_rect.height	= IMX219_HEIGHT;
	*/
	//RVC add
	/* Start with default configuration: 640x480 UYVY. */
	priv->fmt.width	= 640;
	priv->fmt.height	= 480;
	priv->fmt.code	= MEDIA_BUS_FMT_UYVY8_2X8;

	/* These are fixed for all supported formats. */
	priv->fmt.field	= V4L2_FIELD_NONE;
	priv->fmt.colorspace	= V4L2_COLORSPACE_SRGB;
	priv->fmt.ycbcr_enc	= V4L2_YCBCR_ENC_601;
	priv->fmt.quantization = V4L2_QUANTIZATION_LIM_RANGE;
	priv->fmt.xfer_func	= V4L2_XFER_FUNC_SRGB;
	//RVC add
	v4l2_i2c_subdev_init(&priv->subdev, client, &imx219_subdev_ops);
	//mutex_init(&priv->lock);

	sd = &priv->subdev;
	//#ifdef CONFIG_MEDIA_CONTROLLER

	priv->subdev.internal_ops = &mt9v111_internal_ops;
	priv->subdev.flags	|= V4L2_SUBDEV_FL_HAS_DEVNODE;
	priv->subdev.entity.ops	= &mt9v111_subdev_entity_ops;
	priv->subdev.entity.function = MEDIA_ENT_F_CAM_SENSOR;

	priv->pad.flags	= MEDIA_PAD_FL_SOURCE;
	ret = media_entity_pads_init(&priv->subdev.entity, 1, &priv->pad);
	if (ret)
		goto eclk;
	//#endif
	
	printk("imx219_probe 1\n");	
	/*RVC replace soc_power by enable_clk*/
	/*ret = soc_camera_power_init(&client->dev, ssdd);
	if (ret < 0)
		goto eclk;*/ 
	
	ret = imx219_ctrls_init(&priv->subdev);
	printk("imx219_probe 2\n");
	if (ret < 0)
		goto eclk;
	printk("imx219_probe 3\n");
	ret = imx219_video_probe(client);
	if (ret < 0)
		goto eclk;
	
	ret = v4l2_async_register_subdev(&priv->subdev);
	if (ret < 0)
		goto eclk;
	printk("imx219_probe 4 camera driver probed ok\n");
	imx219_s_stream(sd, 1);//RVC
	return 0;
eclk:
	v4l2_clk_put(priv->clk);
	return ret;
mutex_remove:
	mutex_destroy(&priv->lock);
	return ret;
}
#endif

void imx219_subdev_init(struct v4l2_subdev *sd, struct i2c_client *client,
			 const struct v4l2_subdev_ops *ops/*, u32 function,
			 const char *ident*/)
{
	v4l2_subdev_init(sd, ops);
	sd->flags |= V4L2_SUBDEV_FL_HAS_DEVNODE;

	/* the owner is the same as the i2c_client's driver owner */
	sd->owner = client->dev.driver->owner;
	sd->dev = &client->dev;

	v4l2_set_subdevdata(sd, client);

	/* initialize name */
	snprintf(sd->name, sizeof(sd->name), "%s %d-%04x",
		client->dev.driver->name,
		i2c_adapter_id(client->adapter),
		client->addr/*, ident*/);
	printk("imx219_subdev_init sd->name %s\n",sd->name);
	sd->entity.function = MEDIA_ENT_F_PROC_VIDEO_PIXEL_FORMATTER;;//RVC add
	sd->flags = V4L2_SUBDEV_FL_HAS_DEVNODE;//RVC add
	
	sd->entity.ops = &mt9v111_subdev_entity_ops;
}

static int imx219_probe(struct i2c_client *client,
			const struct i2c_device_id *did)
{
	struct imx219 *priv;
	struct i2c_adapter *adapter = to_i2c_adapter(client->dev.parent);
	//struct soc_camera_subdev_desc *ssdd = soc_camera_i2c_to_desc(client);
	struct v4l2_subdev *sd;
	int ret;
	u8 reg = 0x00;
	printk("imx219_probe \n");
	//if (!ssdd) {
	//	dev_err(&client->dev, "IMX219: missing platform data!\n");
		//return -EINVAL;//RVC
	//}
	if (!i2c_check_functionality(adapter, I2C_FUNC_SMBUS_BYTE_DATA)) {
		dev_warn(&adapter->dev,
			 "I2C-Adapter doesn't support I2C_FUNC_SMBUS_BYTE\n");
		return -EIO;
	}
	priv = devm_kzalloc(&client->dev, sizeof(struct imx219), GFP_KERNEL);
	if (!priv)
		return -ENOMEM;
	/*RVC replace v4l2_clk by devm_clk*/
	/*
	priv->clk = v4l2_clk_get(&client->dev, "xclk");//mclk
	if (IS_ERR(priv->clk)) {
		dev_info(&client->dev, "Error %ld getting clock\n",
			 PTR_ERR(priv->clk));
		return -EPROBE_DEFER;
	}*/
		
	/* get system clock (xclk) */
	priv->xclk = devm_clk_get(&client->dev, NULL);
	if (IS_ERR(priv->xclk)) {
		dev_err(&client->dev, "could not get xclk");
		return PTR_ERR(priv->xclk);
	}
	//RVC1 end
	//priv->fmt = &imx219_colour_fmts[0];
	priv->crop_rect.left	= 0;
	priv->crop_rect.top	= 0;
	priv->crop_rect.width	= 640;//IMX219_WIDTH;
	priv->crop_rect.height	= 480;//IMX219_HEIGHT;
	/* Start with default configuration: 640x480 UYVY. */
	priv->fmt.width	= 640;
	priv->fmt.height	= 480;
	priv->fmt.code	= MEDIA_BUS_FMT_UYVY8_2X8;

	/* These are fixed for all supported formats. */
	priv->fmt.field	= V4L2_FIELD_NONE;
	priv->fmt.colorspace	= V4L2_COLORSPACE_SRGB;
	priv->fmt.ycbcr_enc	= V4L2_YCBCR_ENC_601;
	priv->fmt.quantization = V4L2_QUANTIZATION_LIM_RANGE;
	priv->fmt.xfer_func	= V4L2_XFER_FUNC_SRGB;
	v4l2_i2c_subdev_init(&priv->subdev, client, &imx219_subdev_ops);
	printk("imx219_probe 1\n");	

	/*RVC replace soc_power by enable_clk*/
	/*ret = soc_camera_power_init(&client->dev, ssdd);
	if (ret < 0)
		goto eclk;*/ 
	
/*RVC add pad*/
	mutex_init(&priv->lock);

	sd = &priv->subdev;
		imx219_subdev_init(&priv->subdev, client, &imx219_subdev_ops);
	
	priv->subdev.internal_ops = &mt9v111_internal_ops;
	priv->subdev.flags	|= V4L2_SUBDEV_FL_HAS_DEVNODE;
	priv->subdev.entity.ops	= &mt9v111_subdev_entity_ops;
	//priv->subdev.entity.function = MEDIA_ENT_F_CAM_SENSOR;
	priv->pad[SOURCE].flags	= MEDIA_PAD_FL_SOURCE;
	ret = media_entity_pads_init(&priv->subdev.entity, 1, &priv->pad);
	//ret = media_entity_pads_init(&sd->entity, 1, &priv->pad);
	if (ret){
		printk("imx219 Failed to add pad\n");	
		goto mutex_remove;
	}
	
	ret = v4l2_async_register_subdev(&priv->subdev);
	if (ret < 0)
		goto error;
//RVC

	ret = imx219_ctrls_init(&priv->subdev);
	printk("imx219_probe 2\n");
	if (ret < 0)
		goto eclk;
	printk("imx219_probe 3\n");
	ret = imx219_video_probe(client);
	if (ret < 0)
		goto eclk;
	
	ret = v4l2_async_register_subdev(&priv->subdev);
	if (ret < 0)
		goto eclk;
	printk("imx219_probe 4 camera driver probed ok\n");
//RVC add stream
	/* If enabled, apply settings immediately */
	//reg = reg_read(client, 0x0100);
	//if ((reg & 0x1f) == 0x01)
	imx219_s_stream(sd, 1);
	return 0;
eclk:
	media_entity_cleanup(&priv->subdev.entity);
	v4l2_clk_put(priv->clk);
	return ret;
mutex_remove:
	mutex_destroy(&priv->lock);
	return ret;
}
static int imx219_remove(struct i2c_client *client)
{
	//struct soc_camera_subdev_desc *ssdd = soc_camera_i2c_to_desc(client);RVC remove
	struct imx219 *priv = to_imx219(client);
	v4l2_async_unregister_subdev(&priv->subdev);
	v4l2_ctrl_handler_free(&priv->ctrl_handler);
	v4l2_clk_put(priv->clk);
	//if (ssdd->free_bus)
	//	ssdd->free_bus(ssdd);//RVC remove
	return 0;
}
static const struct i2c_device_id imx219_id[] = {
	{ "imx219", 0 },
	{ }
};
MODULE_DEVICE_TABLE(i2c, imx219_id);

static const struct of_device_id imx219_of_match[] = {
	{ .compatible = "sony,imx219" },
	{ /* sentinel */ },
};
MODULE_DEVICE_TABLE(of, ov5647_of_match);

static struct i2c_driver imx219_i2c_driver = {
	.driver = {
		.of_match_table = of_match_ptr(imx219_of_match),
		.name = "imx219",
	},
	.probe		= imx219_probe,
	.remove		= imx219_remove,
	.id_table	= imx219_id,
};
module_i2c_driver(imx219_i2c_driver);
MODULE_DESCRIPTION("Sony IMX219 Camera driver");
MODULE_AUTHOR("Guennadi Liakhovetski <g.liakhovetski@gmx.de>");
MODULE_LICENSE("GPL v2");


