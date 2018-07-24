/*
 * ov76_test.c
 *
 *  Created on: Mar 2015
 *      Author: enash
 * 
 *    Modified: cbrandt
 *              Jan 2016
 */

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <sys/mman.h>
#include <sys/time.h>
#include <linux/videodev2.h>
#include <linux/v4l2-mediabus.h>
 #include <linux/i2c-dev.h>//RVC
/*#include "jpeglib.h"*/ //RVC

/* The CEU HW can capture a 4:2:2 stream and convert it to 4:2:0 when writing
   to the capture buffers. This reduces the memory requirements by 25% for each buffer */

//#define CAPTURE_YCBCR420	/* Convert YCbCr 4:2:2 to 4:2:0 in HW */

/* Number of buffers in the DMA pool for the camera store data to.
   Buffers can be statically pre-defined by the driver (in the board file),
   or can be allocated at run time. If running with limited memory, you should
   pre-allocated with the driver

   Buffer Sizes:
	YCbCr 4:2:0 = width x height x 1.5
	YCbCr 4:2:2 = width x height x 2
 */

#define NUM_V4L_BUFS 1

/* MIPI */
#define WIDTH 640
#define HEIGHT 480
#define MIPI_ADDR 0x20
static char i2c_dev_path[] = "/dev/i2c-2";//KYT
static int mipi_fd;
static int mipi_fd_0;
static int mipi_fd_1;
static int mipi_fd_3;
/* From vdc5.c 
void vdc5_show_layer(int hide_bg);
void vdc5_hide_layer(void);
void *vdc5_set_fb_addr(void * phys_addr);*///RVC

#define CAP_CNT 10	/* the default number of times to capture */
int continuous_stream = 0;
int display = 0;
int savejpg = 1;
int quiet = 0;
char * ycbcr_fb;	/* A separate frame buffer for LCD LVDS display */

#ifndef PAGE_SIZE
#define PAGE_SIZE 0x1000
#endif

// macros for common checks
#define IOCTL_CHECK(ioctl_err, tag) \
		if (!quiet) printf("checking %s\n",tag);	\
		if(ioctl_err)                   \
		{                               \
			perror(tag);                \
			return ioctl_err;           \
		}								\

typedef struct
{
	int			fd;
	struct v4l2_format	fmt;
	struct v4l2_buffer	v4lBuf;
	char			*frames[NUM_V4L_BUFS];
	/*struct jpeg_compress_struct	cinfo;
	struct jpeg_error_mgr		jerr;*///RVC
} CamInfo;

static CamInfo mCam;

int kbhit()
{
	struct timeval tv;
	fd_set fds;
	tv.tv_sec = 0;
	tv.tv_usec = 0;
	FD_ZERO(&fds);
	FD_SET(STDIN_FILENO, &fds);	//STDIN_FILENO is 0
	select(STDIN_FILENO+1, &fds, NULL, NULL, &tv);
	return FD_ISSET(STDIN_FILENO, &fds);
}	
//KYT add
static int mipi_read(unsigned char reg, unsigned char *value)
{
	int ret;

	if( write(mipi_fd, &reg, 1) != 1) {
		printf("mipi_read error (setting reg=0x%X)\n",reg);
		return -1;
	}
	if( read(mipi_fd, value, 1) != 1) {
		printf("mipi_read error (read reg=0x%X)\n",reg);
		return -1;
	}
	return 0;	
}

static int mipi_write(unsigned char reg, unsigned char value)
{
	uint8_t data[2];

	data[0] = reg;
	data[1] = value;

	if( write(mipi_fd, data, 2) != 2) {
		printf("ov7670_write (reg=0x%X)\n",reg);
		return -1;
	}

	return 0;
}
int mipi_open(char *i2c_dev_path)
{
	int ret;
	int i;
	uint8_t value;
	printf("mipi_open start \n");
	mipi_fd = open("/dev/i2c-2", O_RDWR);
	if (mipi_fd < 0) {
		printf("mipi_open failed \n");
		return -1;
	}
	printf("mipi_open /dev/i2c-2 ok \n");

	if (ioctl(mipi_fd, I2C_SLAVE, 0x10) < 0) {
		printf("%s: I2C_SLAVE failed\n", __func__);
		return -1;
	}
/*
	mipi_fd_3 = open("/dev/i2c-3", O_RDWR);
	if (mipi_fd_3 < 0) {
		printf("mipi_open failed \n");
		return -1;
	}
	printf("mipi_open /dev/i2c-3 ok \n");
	
	if (ioctl(mipi_fd_3, I2C_SLAVE,0x50) < 0) {
		printf("%s: I2C_SLAVE mipi_fd_3 failed\n", __func__);
		return -1;
	}

	mipi_fd_0 = open("/dev/i2c-0", O_RDWR);
	if (mipi_fd_0 < 0) {
		printf("mipi_open failed \n");
		return -1;
	}
	printf("mipi_open /dev/i2c-0 ok \n");

	if (ioctl(mipi_fd_0, I2C_SLAVE, 0x20) < 0) {
		printf("%s: I2C_SLAVE failed\n", __func__);
		return -1;
	}

	mipi_fd_1 = open("/dev/i2c-1", O_RDWR);
	if (mipi_fd_1 < 0) {
		printf("mipi_open failed \n");
		return -1;
	}
	printf("mipi_open /dev/i2c-1 ok \n");

	if (ioctl(mipi_fd_1, I2C_SLAVE, 0x20) < 0) {
		printf("%s: I2C_SLAVE failed\n", __func__);
		return -1;
	}
*/
	return 0;
}

int mipi_set_format(void)
{	
	printf("mipi_set_format\n");
	char snd1[3] = {0x01,0x03,0x01}; //software reset
	char snd2[3] = {0x03,0x05,0x04};        //PREPLL DIV
	char snd3[4] = {0x02,0x02,0x03,0x20};
	char snd4[3] = {0x02,0x05,0xAF};
	char snd5[3] = {0x34,0x18,0x06};        //30fps
	
	char buf1[3];
	char buf2[3];
	char buf3[4];	
	char buf4[3];
	char buf5[3];
	int mount;
	
	buf1[0] = 0x01;
	buf1[1] = 0x03;
	buf1[2] = 0x01;

	buf2[0] = 0x03;
	buf2[1] = 0x05;
	buf2[2] = 0x04;

	buf3[0] = 0x02;
	buf3[1] = 0x02;
	buf3[2] = 0x03;
	buf3[3] = 0x20;

	buf4[0] = 0x02;
	buf4[1] = 0x05;
	buf4[2] = 0xAF;

	buf5[0] = 0x34;
	buf5[1] = 0x18;
	buf5[2] = 0x06;	
/*
	mount = write(mipi_fd_3, buf1, 3);

	if (mount !=3)
		printf("write buf1[] failed mount =%d \n", mount);

	mount = write(mipi_fd_0, buf2, 3);

	if (mount !=3)
		printf("write buf2[] failed mount =%d \n", mount);
	
	mount = write(mipi_fd_1, buf3, 4);

	if (mount !=4)
		printf("write buf3[] failed mount =%d \n", mount);
	
	mount = write(mipi_fd_3, buf4, 3);

	if (mount !=3)
		printf("write buf4[] failed mount =%d \n", mount);
	
	mount = write(mipi_fd_3, buf5, 3);

	if (mount !=3)
		printf("write buf5[] failed mount =%d \n", mount);*/
/*write to i2c-2*/

	if( write(mipi_fd, snd1, 3) != 3) {
		printf("mipi_write snd1 failed\n");
		return -1;
	}
	
	if( write(mipi_fd, snd2, 3) != 3) {
		printf("mipi_write snd2 failed\n");
		return -1;
	}

	if( write(mipi_fd, snd3, 4) != 4) {
		printf("mipi_write snd3 failed\n");
		return -1;
	}
	
	if( write(mipi_fd, snd4, 3) != 3) {
		printf("mipi_write snd4 failed \n");
		return -1;
	}	
	
	if( write(mipi_fd, snd5, 3) != 3) {
		printf("mipi_write  snd5 failed\n");
		return -1;
	}
	
	return 0;
}

int mipi_set_clk(void)
{
	printf("mipi_set_clk\n");
	char snd0[1] = {0x01, 0x00, 0x01};
	if( write(mipi_fd, snd0, 1) != 1) {
		printf("mipi_write snd0 failed to set clk\n");
		return -1;
	}

	return 0;
}

static int read_mipi(unsigned char reg, unsigned char *v)
{
	if (write(mipi_fd, &reg, 1) != 1){
		printf("read_mipi write failed\n");
		return -1;
	}

	if (read(mipi_fd, v, 1) != 1){
		printf("read_mipi read failed\n");
		return -1;
	}

	return 0;

}

static int mipi_check_opp(void)
{
	unsigned char v;
	int ret;

	ret = read_mipi(0x01, &v);
	if (ret < 0)	
	{
		printf("failed to read 0x01 reg \n");			
		return -1;	
	}
	printf("value at 0x01 = %x\n", v);

	return 0;

}
//KYT
// the V4L2 camera init
static int camera_init(CamInfo *cam, char *name)
{
	int err, input, i;

	printf("camera init start \n");

	/* Open Device Driver interface */
	cam->fd = open(name, O_RDWR);
	if(cam->fd < 0)
	{
		perror("Could not open camera driver");
		perror(name);
		return errno;
	}

	// set the input for CSI -> memory
	// VIDIOC_S_INPUT = SET INPUT
	input = 0;
	err = ioctl(cam->fd, VIDIOC_S_INPUT, &input);
	IOCTL_CHECK(err, "VIDIOC_S_INPUT");

	// V4l format size
	memset(&cam->fmt, 0, sizeof(cam->fmt));

	/* The RZ/A CEU driver is a soc_camera driver. 
	   Only single-plane capture is supported by soc_camera) */
	cam->fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;

	cam->fmt.fmt.pix.width = WIDTH;
	cam->fmt.fmt.pix.height = HEIGHT;

	cam->fmt.fmt.pix.colorspace = V4L2_COLORSPACE_JPEG;
	cam->fmt.fmt.pix.field = V4L2_FIELD_NONE;

	/* These are the formats the CEU driver supports:NV12, NV21, NV16, NV61 */
	/* two planes -- one Y, one Cr + Cb interleaved
	/* NOTE: The OV7670 only outputs 4:2:2, so when you specifiying
	   4:2:0, the CEU HW simply ignores the color data on odd numbered
	   lines */
#ifdef CAPTURE_YCBCR420
	cam->fmt.fmt.pix.pixelformat = V4L2_PIX_FMT_NV12;	/* Y/CbCr 4:2:0  */
	//cam->fmt.fmt.pix.pixelformat = V4L2_PIX_FMT_NV21;	/* Y/CrCb 4:2:0  */
#else
	cam->fmt.fmt.pix.pixelformat = V4L2_PIX_FMT_NV16;	/* Y/CbCr 4:2:2  */
	//cam->fmt.fmt.pix.pixelformat = V4L2_PIX_FMT_NV61;	/* Y/CrCb 4:2:2  */
#endif

	// VIDIOC_S_FMT = Set Format
	err = ioctl(cam->fd, VIDIOC_S_FMT, &cam->fmt);
	IOCTL_CHECK(err, "VIDIOC_S_FMT");

	// V4l format size
	memset(&cam->fmt, 0, sizeof(cam->fmt));
	cam->fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	err = ioctl(cam->fd, VIDIOC_G_FMT, &cam->fmt);
	IOCTL_CHECK(err, "VIDIOC_G_FMT");

	// V4l format size
	cam->fmt.fmt.pix.priv = 0;
	err = ioctl(cam->fd, VIDIOC_S_FMT, &cam->fmt);
	IOCTL_CHECK(err, "VIDIOC_S_FMT");

	// allocate DMA buffers
	struct v4l2_requestbuffers req;
	memset(&req, 0, sizeof(req));
	req.count = NUM_V4L_BUFS;
	req.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	req.memory = V4L2_MEMORY_MMAP;
	err = ioctl(cam->fd, VIDIOC_REQBUFS, &req);
	IOCTL_CHECK(err, "VIDIOC_REQBUFS");

	// initialize each buffer
	for (i= 0; i < (int)NUM_V4L_BUFS; i++)
	{
		struct v4l2_buffer buf;
		memset(&buf, 0, sizeof(buf));
		buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
		buf.memory = V4L2_MEMORY_MMAP;
		buf.index = i;
		err = ioctl(cam->fd, VIDIOC_QUERYBUF, &buf);
		IOCTL_CHECK(err, "VIDIOC_QUERYBUF");

		// mmap it to user space address
		cam->frames[i] = (char *) mmap(NULL, buf.length, PROT_READ | PROT_WRITE,
				MAP_SHARED, cam->fd, buf.m.offset);

		// queue it (to the V4l ready_q)
		err = ioctl(cam->fd, VIDIOC_QBUF, &buf);
		IOCTL_CHECK(err, "VIDIOC_QBUF");
	}

	return 0;
}

/* When the CEU reads in the images it separates the YCbCr422 into separate Y Cb Cr buffers
   automatically (and there's no way to turn that off by the way).

  If you want to display the raw YCbCr using the VDC5 LCD controller (which
  can handle a raw YCbCr frame buffer), you have to combine the
  data back into a true YCbCr422 interleaved format.

  When 4:2:0 conversion is enabled:
  The CEU can convert a YCbCr422 stream to YCbCr420. When this is done,
  we have to duplicate the color data for each odd numbered line.

  OV7670 out (YCbCr422) = Cb0, Y0, Cr0, Y1, Cb2, Y3, Cr2, etc...

  CEU buffer = Y0,Y1,Y2...........YN
               Cb0,Cr0,Cb1,Cr1,..CbN

  When 4:2:0 convertion is enabled:
    CEU buffer = Y0,Y1,Y2...........YN
                 Cb0,Cr0,Cb1,Cr1,..CbN (only half as much data as Y)

  VDC5 LCD buffer (YCbCr422) = Cb/Y0/Cr/Y1 (ITU-R BT.656 format)

*/
#if 0 //RVC
static void ceu_cap_to_vdc5(char *pBuf)
{
	char *y = pBuf;
	char *fb = ycbcr_fb;
	char *cbcr = y + WIDTH * HEIGHT ;
	int col, row;


	//memcpy(ycbcr_fb,pBuf,HEIGHT*WIDTH*2); return;	/* capture throughput testing */

	for(row=0; row<HEIGHT; row++)
	{
#ifdef CAPTURE_YCBCR420
		/* YCbCr420 to YCbCr422 */
		/* Duplicate color data over 2 rows */
		if( row & 1)
			cbcr -= WIDTH;
#endif

		/* Y0/Cr/Y1/Cb */
		for(col=0; col<WIDTH; col++)
		{
			*fb++ = *cbcr++;
			*fb++ = *y++;
		}
	}
}
#endif //RVC
/* When the CEU read in the images it separates the YCbCr422 into separate Y Cb/Cr buffers
   automatically (and there's no way to turn that off by the way).

  The JPEG library wants pixel data to come in interleaved where each Y will have
  its own Cb and Cr. Therefore our buffer that we pass to libjpeg has to be bigger
  and we have to duplicate the color data.

  OV7670 output (YCbCr422) = Cb0,Y0,Cr0,Y1,Cb2,Y3,Cr2, etc...

  CEU buffer = Y0,Y1,Y2...........YN
               Cb0,Cr0,Cb1,Cr1,..CbN
  
When 4:2:0 conversion is enabled:
    CEU buffer = Y0,Y1,Y2...........YN
                 Cb0,Cr0,Cb1,Cr1,..CbN (only half as much data as Y

  libjpeg input = Y0,Cb0,Cr0,Y1,Cb0,Cr0,Y2,Cb2,Cr2,Y3,Cb2,Cr2,Y4,etc..

*/
static char *planar_to_interleave(char *pBuf)
{
	static char ibuf[WIDTH*HEIGHT*3];

	char *y = pBuf;
	char *fb = ibuf;
	char *cbcr = y + WIDTH * HEIGHT ;
	int col, row;

	for(row=0; row<HEIGHT; row++)
	{
#ifdef CAPTURE_YCBCR420
		/* YCbCr420 to YCbCr422 */
		/* Duplicate color data over 2 rows */
		if( row & 1)
			cbcr -= WIDTH;
#endif
		/* Y0/Cb/Cr/Y1/Cb/Cr */
		for(col=0; col<WIDTH; col++)
		{
			*fb++ = *y++;
			if( col & 1)
			{
				/* on old colums, duplicate previous color data */
				*fb++ = *(cbcr - 2);
				*fb++ = *(cbcr - 1);
			}
			else
			{
				*fb++ = *cbcr++;
				*fb++ = *cbcr++;
			}
		}
	}
	return ibuf;
}
#if 0//RVC
static void save_jpeg(CamInfo *cam, struct v4l2_buffer *dqBuf)
{
	static int inited = 0;
	static int fileno = 0;
	int row_stride;
	JSAMPROW row_pointer[1];
	char *image_buf;
	FILE * outfile;
	char filename[32];

	if(!inited)
	{
		memset(&cam->cinfo, 0, sizeof(cam->cinfo));
		memset(&cam->jerr, 0, sizeof(cam->jerr));

		cam->cinfo.err = jpeg_std_error(&cam->jerr);
		jpeg_create_compress(&cam->cinfo);

		cam->cinfo.image_width = WIDTH; //image width and height, in pixels
		cam->cinfo.image_height = HEIGHT;
		cam->cinfo.input_components = 3;	// must be 3
		cam->cinfo.in_color_space = JCS_YCbCr;
		jpeg_set_defaults(&cam->cinfo);

		inited = 1;
	}

	if( continuous_stream )
		fileno = 0;	/* same filename each time */

	sprintf(filename, "capture%04d.jpg", fileno++);

	if ((outfile = fopen(filename, "wb")) == NULL)
	{
		fprintf(stderr, "can't open %s\n", filename);
		exit(1);
	}
	jpeg_stdio_dest(&cam->cinfo, outfile);

	jpeg_start_compress(&cam->cinfo, TRUE);

	row_stride = WIDTH * 3;

	image_buf = planar_to_interleave(mCam.frames[dqBuf->index]);

	while (cam->cinfo.next_scanline < cam->cinfo.image_height)
	{
		row_pointer[0] = &image_buf[cam->cinfo.next_scanline * row_stride];
		(void) jpeg_write_scanlines(&cam->cinfo, row_pointer, 1);
	}

	jpeg_finish_compress(&cam->cinfo);
	fclose(outfile);
}
#endif
static int stream(CamInfo *cam, int nFrames)
{
	int err;
	static int count = 0;

	// turn streaming on
	enum v4l2_buf_type type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	err = ioctl(cam->fd, VIDIOC_STREAMON, &type);
	IOCTL_CHECK(err, "VIDIOC_STREAMON");

	/* Look till we hit our number of frames, or use hits the keyboard */
	while(nFrames-- || continuous_stream)
	{
		/* Wait for a buffer
		   VIDIOC_DQBUF is called to request a filled buffer from the 
		    outgoing queue */
		struct v4l2_buffer dqBuf;
		memset(&dqBuf, 0, sizeof(dqBuf));
		dqBuf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
		err = ioctl(cam->fd, VIDIOC_DQBUF, &dqBuf);
		IOCTL_CHECK(err, "VIDIOC_DQBUF");

		if (!quiet)
			printf( "  dqBuf: .index=%u, .bytesused=%d\n", dqBuf.index, dqBuf.bytesused);

		count++;
		/* Throw away the first 10 frames because the camera needs
		   a couple captures to adjsut exposure and such */

		if((count > 10) && savejpg)
			//save_jpeg(cam, &dqBuf);//RVC
			printf( "  count: %d\n", count);//RVC
		else
			nFrames++;
/*
		if (display)
			ceu_cap_to_vdc5(mCam.frames[dqBuf.index]);*///RVC

		/* Return the buffer
		   VIDIOC_QBUF is called to put a buffer that you are done with
		   back into the incoming queue */
		err = ioctl(cam->fd, VIDIOC_QBUF, &dqBuf);
		IOCTL_CHECK(err, "VIDIOC_QBUF");

		if (continuous_stream)
		{
			system("if [ -e capture0000.jpg ] ; then cp capture0000.jpg image.jpg ; fi");
			if ( kbhit() )
				break;
			//sleep(1);

		}
	}

	//jpeg_destroy_compress(&cam->cinfo);//RVC

	return 0;
}

int main(int argc, char *argv[])
{
	int cap_cnt = CAP_CNT;
	int i;
	int found = 0;//KYT

	if( argc < 2)
	{
		printf(
		"ov76_test: Capture images from a OV7670 camera and save a JPEG.\n"\
		"Usage: ov76_test [-c] [-d] [-n] [-q] [count]  \n"\
		"  -c: Continouse mode. Output file will always be capture0000.jpg and image.jpg\n"\
		"  -d: Display raw YCbCr422 on the LCD using VDC5.\n"\
		"  -n: Don't save JPEG images.\n"\
		"  -q: Quiet mode.\n"\
		" count: the number of images to caputre and save.\n"\
		);
		return -1;
	}


	i = 1;
	for( i=1; i < argc; i++)
	{
		if( !strcmp(argv[i],"-c") )
		{
			continuous_stream = 1;
		}
		else if( !strcmp(argv[i],"-d") )
		{
			display = 1;
		}
		else if( !strcmp(argv[i],"-n") )
		{
			savejpg = 0;
		}
		else if( !strcmp(argv[i],"-q") )
		{
			quiet = 1;
		}
		else
		{
			// Must be a count
			sscanf(argv[i], "%u", &cap_cnt);
		}
	}
	/* KYT look for MIPI camera */
	if( mipi_open(i2c_dev_path) == 0 )
	{
		found = 1;
		printf("mipi_open ok via i2c \n ");
		if( mipi_set_format() )
			goto camerr;
		//break;
	}
	if (!found)
	{
		printf("no cammera found\n");
		goto camerr;
	}
	
	if(camera_init(&mCam, "/dev/video0"))
		goto camerr;
#if 0//RVC
	if ( display )
	{

		/* Map VDC5 registers and frame buffer */
		ycbcr_fb = (char *) vdc5_map_in_fb();
		if( ycbcr_fb == MAP_FAILED )
		{
			perror("Failed to map VDC5 registers");
			goto camerr;
		}
		vdc5_show_layer(0);
	}
#endif
	if( continuous_stream )
	{
		printf("***********************************************\n");
		printf("Continuous Stream Mode.\n");
		printf("Press ENTER at any time to exit.\n");
		printf("***********************************************\n");
		setbuf(stdout, NULL);	/* disable stdout buffering */
	}

	if( mipi_set_clk() )
		goto camerr;
	if (mipi_check_opp() )
		goto camerr;

	if(stream(&mCam, cap_cnt))
		goto streamerr;

	sleep(2);

	close(mCam.fd);
#if 0 //RVC
	if( ycbcr_fb )
		vdc5_hide_layer();
#endif //RVC
	printf("Done without errors\n");
	return 0;

	streamerr:
	close(mCam.fd);
	camerr:
	printf("Done with errors\n");
	return 1;
}


