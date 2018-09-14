// I2C test program for a MIPI camera

#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <linux/i2c-dev.h>
#include <linux/i2c.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <string.h>


// define type of camera
#define CAM_IU233 0
#define CAM_IMX219_B 1
#define CAM_IMX219_Q 2

#define CAM_TABLE_END 0xFFFF
#define CLIENT 0x10
int g_i2cFile;

// open the Linux device
struct cam_reg {
	uint16_t addr;
	uint8_t val;
};
struct cam_reg iu233[] = {
	{0x0000, 0x02}, // IU233 Low
	{0x0001, 0x33}, // IU233 High
	
	{0x0100, 0x00}, // Stop streaming
	{0x0101, 0x00}, // Initial setting for sensor
	
	{0x303C, 0x54},
	{0x303E, 0x38}, 
	{0x30DD, 0x91},
	{0x30E5, 0x09},
	
	{0x0112, 0x08}, // RAW10
	{0x0113, 0x08}, 
	
	{0x0340, 0x03}, // Mode setting
	{0x0341, 0x84},
	{0x0342, 0x05},
	{0x0343, 0xE8},
	{0x0381, 0x01},
	{0x0383, 0x01},
	{0x0385, 0x01},
	{0x0387, 0x01},
	{0x30AA, 0x02}, // Binning mode setting
	{0x308C, 0x00},
	{0x308D, 0x00},
	{0x308E, 0x00},
	{0x3091, 0x00},
	{0x30E4, 0x00},
	{0x307D, 0x00},	// FD4 setting
	{0x307E, 0x01},
	{0x30E1, 0x00},
	{0x30F8, 0x00},	//PS mode setting
	{0x30F9, 0x00},
	{0x30FA, 0xC0},
	{0x30FB,0x01},
	{0x30FC,0x00},
	
	{0x0344,0x00},	// Horizontal cropping start address
	{0x0345,0x00},
	{0x0346,0x04},	// Vertical cropping start address
	{0x0347,0x00},
	{0x0348,0x04},	// Horizontal croping end address
	{0x0349,0xFF},
	{0x034A,0x06},	// Vertical cropping end address
	{0x034B,0xCF},
	{0x034C,0x05},	// Horizontal cropping size
	{0x034D,0x00},
	{0x034E,0x02},	// Vertical cropping size
	{0x034F,0xD0},
	{0x0305,0x02}, 	// Clock setting
	{0x0307,0x5A},
	{0x304C,0x02},
	{0x3418,0x61},
	{0x0202,0x03},	// Integration time setting
	{0x0203,0x7F},
	{0x0205,0x00},	// Gain setting
	{0x020E,0x01},
	{0x020F,0x00},
	{0x0210,0x01},
	{0x0211,0x00},
	{0x0212,0x01},
	{0x0213,0x00},
	{0x0214,0x01},
	{0x0215,0x00},
	//~{0x0100,0x01},	// Start streaming
	//~{0x0100,0x00},	// Stop streaming
	{CAM_TABLE_END, 0x00 },
};
struct cam_reg imx219_b[] = {
	/* Initialize */
	/* 2-Lane */
	{0x0114, 0x01},
	/* timing setting:manual mode */
	{0x0128, 0x01},
	/* INCK = 12MHz */
	{0x012A, 0x0C},
	{0x012B, 0x00},
	
	//~// 320 X 240
	//~/* frame length(height) = 240(=0F0H) */
	//~/* line  length(width)  = 3448(=D78H)(default) */
	//~{0x0160, 0x00},
	//~{0x0161, 0xF0},
	//~/* X range */
	//~{0x0164, 0x04},
	//~{0x0165, 0x88},
	//~{0x0166, 0x08},
	//~{0x0167, 0x47},
	//~/* Y range */
	//~{0x0168, 0x04},
	//~{0x0169, 0x58},
	//~{0x016A, 0x05},
	//~{0x016B, 0x47},
	//~/* signal range */
	//~{0x016C, 0x01},
	//~{0x016D, 0x40},
	//~{0x016E, 0x00},
	//~{0x016F, 0xF0},
        
    // 800 X 480
	/* frame length(height) = 480(=1E0H) */
	/* line  length(width)  = 3448(=D78H)(default) */
	{0x0160, 0x01},
	{0x0161, 0xE0},
	/* X range */
	{0x0164, 0x03},
	{0x0165, 0x98},
	{0x0166, 0x0B},
	{0x0167, 0x17},
	/* Y range */
	{0x0168, 0x03},
	{0x0169, 0xE0},
	{0x016A, 0x05},
	{0x016B, 0xBF},
	/* signal range */
	{0x016C, 0x03},
	{0x016D, 0x20},
	{0x016E, 0x01},
	{0x016F, 0xE0},
        
        
        
	/* output format RAW8 */
	{0x018C, 0x08},
	{0x018D, 0x08},
	
	{0x0172, 0x03}, // image orientation
	/* binning (digital) mode */
	{0x0174, 0x01},
	{0x0175, 0x01},
	{0x0176, 0x01},
	{0x0177, 0x01},
	/* analog gain(LONG) control */
	{0x0157, 0xC0},
	/* clock setting */
	{0x0301, 0x05},
	{0x0304, 0x02},
	{0x0305, 0x02},
	{0x0306, 0x00},
	{0x0307, 0x3A},
	{0x0309, 0x08},
	{0x030C, 0x00},
	{0x030D, 0x32},
	{CAM_TABLE_END, 0x00 }
};
struct cam_reg imx219_q[] = {
	
    {0x0114,0x01},  // 2-Lane
	{0x0128,0x01},  // timing setting:0-auto 1-manual
	
	{0x012A,0x0C},	// INCK
	{0x012B,0x00}, 	// INCK = 12MHz
	
	// 320 X 240
	//~/* frame length(height) = 240(=0F0H) */
	//~/* line  length(width)  = 3448(=D78H)(default) */
	//~{0x0160, 0x00},
	//~{0x0161, 0xF0},
	//~/* X range */
	//~{0x0164, 0x04},
	//~{0x0165, 0x88},
	//~{0x0166, 0x08},
	//~{0x0167, 0x47},
	//~/* Y range */
	//~{0x0168, 0x04},
	//~{0x0169, 0x58},
	//~{0x016A, 0x05},
	//~{0x016B, 0x47},
	//~/* signal range */
	//~{0x016C, 0x01},
	//~{0x016D, 0x40},
	//~{0x016E, 0x00},
	//~{0x016F, 0xF0},
	
	
	// 800 X 480
	/* frame length(height) = 480(=1E0H) */
	/* line  length(width)  = 3448(=D78H)(default) */
	{0x0160, 0x01},
	{0x0161, 0xE0},
	/* X range */
	{0x0164, 0x03},
	{0x0165, 0x98},
	{0x0166, 0x0B},
	{0x0167, 0x17},
	/* Y range */
	{0x0168, 0x03},
	{0x0169, 0xE0},
	{0x016A, 0x05},
	{0x016B, 0xBF},
	/* signal range */
	{0x016C, 0x03},
	{0x016D, 0x20},
	{0x016E, 0x01},
	{0x016F, 0xE0},
	

	
	/* output format RAW8 */
	{0x018C, 0x08},
	{0x018D, 0x08},
	
	{0x0170,0x01},	// X Increment for odd pixels 1, 3
	{0x0171,0x01},	// Y Increment for odd pixels 1, 3 
	{0x0172,0x03},	// Image oriantation

					// binning(digital) mode
	{0x0174,0x01}, 	// H_direction
	{0x0175,0x01},	// V_direction
	{0x0176,0x01},	// binning cal - H
	{0x0177,0x01},  // binning cal - V
	
	{0x0157,0xC0},  // gain(LONG)
//~{0x015A,0x02},	// coarse intergration time
//~{0x015B,0xF2},	
					// clock setting
	{0x0301,0x05}, 	// Video timing pixel clock divider value
	//~{0x0303,0x01}, 	// Video timing system clock divider value 
	 
	{0x0304,0x02},	// Pre PLL clock video timing system divider value
	{0x0305,0x02},	// Pre PLL clock output system divder value
	{0x0306,0x00},	// PLL video timing system multiplier value
	{0x0307,0x3A},
	
	{0x0309,0x08},	// Output pixel clock divider value
	//~{0x030B,0x01}, 	// Output system clock divider value
	{0x030C,0x00},	// PLL output system multiplier value
	{0x030D,0x32},             
					

	{CAM_TABLE_END, 0x00 }
};
struct cam_reg imx129_miscellaneous[] = {
	{ 0x30EB, 0x05 }, /* Access Code for address over 0x3000 */
	{ 0x30EB, 0x0C }, /* Access Code for address over 0x3000 */
	{ 0x300A, 0xFF }, /* Access Code for address over 0x3000 */
	{ 0x300B, 0xFF }, /* Access Code for address over 0x3000 */
	{ 0x30EB, 0x05 }, /* Access Code for address over 0x3000 */
	{ 0x30EB, 0x09 }, /* Access Code for address over 0x3000 */
	
	{ 0x0114, 0x01 }, /* CSI_LANE_MODE[1:0} */
	{ 0x0128, 0x01 }, /* DPHY_CNTRL */
	{ 0x012A, 0x0C }, /* EXCK_FREQ[15:8] */
	{ 0x012B, 0x00 }, /* EXCK_FREQ[7:0] */
	{ 0x0160, 0x0A }, /* FRM_LENGTH_A[15:8] */
	{ 0x0161, 0xA8 }, /* FRM_LENGTH_A[7:0] */
	{ 0x0162, 0x0D }, /* LINE_LENGTH_A[15:8] */
	{ 0x0163, 0x78 }, /* LINE_LENGTH_A[7:0] */
	{ 0x0170, 0x01 }, /* X_ODD_INC_A[2:0] */
	{ 0x0171, 0x01 }, /* Y_ODD_INC_A[2:0] */
	{ 0x0174, 0x00 }, /* BINNING_MODE_H_A */
	{ 0x0175, 0x00 }, /* BINNING_MODE_V_A */
	{ 0x018C, 0x08 }, /* CSI_DATA_FORMAT_A[15:8] */ 
	{ 0x018D, 0x08 }, /* CSI_DATA_FORMAT_A[7:0] */
	{ 0x0301, 0x05 }, /* VTPXCK_DIV */
	{ 0x0303, 0x01 }, /* VTSYCK_DIV */
	{ 0x0304, 0x02 }, /* PREPLLCK_VT_DIV[3:0] */
	{ 0x0305, 0x02 }, /* PREPLLCK_OP_DIV[3:0] */
	{ 0x0306, 0x00 }, /* PLL_VT_MPY[10:8] */
	{ 0x0307, 0x75 }, /* PLL_VT_MPY[7:0] */
	{ 0x0309, 0x0A }, /* OPPXCK_DIV[4:0] */
	{ 0x030C, 0x00 }, /* PLL_OP_MPY[10:8] */
	{ 0x030D, 0x75 }, /* PLL_OP_MPY[7:0] */
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
	{ CAM_TABLE_END, 0x00 }
};
void i2c_open(){
	//TODO: decide which adapter you want to access (default: i2c-2)
	g_i2cFile = open("/dev/i2c-2", O_RDWR);
	if (g_i2cFile < 0) {
		perror("Opening I2C device node\n");
		exit(1);
	}
}

// set the I2C slave address for all subsequent I2C device transfers
void i2c_set_address(uint8_t address){
	if (ioctl(g_i2cFile, I2C_SLAVE_FORCE, address) < 0) {
		perror("Selecting I2C device\n");
		exit(1);
	}
}

// close the Linux device
void i2c_close(){
	close(g_i2cFile);
}

void MIPI_RIIC_INIT(){
	i2c_open();
	i2c_set_address(CLIENT);
}
void MIPI_RIIC_CLOSE(){
	i2c_close();
}
// Write 8-bit data to 16-bit data register addr
int MIPI_RIIC_SEND(uint8_t c_addr, uint16_t reg, uint8_t val)
{
	struct i2c_rdwr_ioctl_data xfer_queue;
	struct i2c_msg msgs;
	uint8_t buf[3];
	int ret;

	buf[0] = reg >> 8;
	buf[1] = reg & 0xff;
	buf[2] = val;

	memset(&xfer_queue, 0, sizeof(xfer_queue));
	msgs.addr = c_addr;
	msgs.len = sizeof(buf);
	msgs.flags = 0;
	msgs.buf = buf;
	
	xfer_queue.msgs = &msgs;
	xfer_queue.nmsgs = 1;

	ret = ioctl(g_i2cFile, I2C_RDWR, &xfer_queue);
	if (ret < 0)
	{
		printf("send error: 0x%04X\n", reg);
		perror("Write I2C");
		exit(1);
	}
	return 0;
}

int MIPI_RIIC_SEND_TABLE(uint8_t c_addr, struct cam_reg table[]){
	struct cam_reg *reg;
	int ret;
	for (reg = table; reg->addr != CAM_TABLE_END; reg++) {
		ret = MIPI_RIIC_SEND(c_addr, reg->addr, reg->val);
		if (ret < 0)
			return ret;
	}
	return 0;
}

int R_MIPI_CameraPowOn(int cam_type){
	int ret;
	switch(cam_type){
	case CAM_IU233:

		ret = MIPI_RIIC_SEND_TABLE(CLIENT, iu233);
		break;

    case CAM_IMX219_B:

		ret = MIPI_RIIC_SEND_TABLE(CLIENT, imx219_b);
		break;

    case CAM_IMX219_Q:
        
        ret = MIPI_RIIC_SEND_TABLE(CLIENT, imx219_q);
        break;
    default:
		break;
    }
    if (ret < 0)
		return ret;
	return 0;
}
int R_MIPI_CameraClkStart(void){
    int ret;
    ret = MIPI_RIIC_SEND(CLIENT, 0x0100, 0x01);
	if (ret < 0)
		return ret;
	return 0;
}
int R_MIPI_CameraClkStop(void){
    int ret;
    ret = MIPI_RIIC_SEND(CLIENT, 0x0100,0x00);
	if (ret < 0)
		return ret;
	return 0;
}
int R_MIPI_CameraReset(void){
	int ret;
	
    ret = MIPI_RIIC_SEND(CLIENT, 0x0100,0x00);
	if (ret < 0)
		return ret;
    ret = MIPI_RIIC_SEND(CLIENT, 0x0103,0x01);
	if (ret < 0)
		return ret;
		
	return 0;
}
int R_MIPI_Miscellaneous(void){
	int ret;
    
    ret = MIPI_RIIC_SEND_TABLE(CLIENT, imx129_miscellaneous);
	if (ret < 0)
		return ret;
	return 0;
}

int main(int argc, char *argv[]){
	int i;
	int cam;
	if (argc < 2)
	{
		printf(
		"mipi_cam_send_data: Choose the type of camera\n"\
		"Usage: mipi_cam_send_data [-i] [-b] [-q]\n"\
		"	-i: IU233\n"\
		"	-b: IM219_B\n"\
		"	-q: IM219_Q\n"\
		);
		return -1;
	}
	// Read in command line options
	for (i=1; i<argc; i++)
	{
		if (!strcmp(argv[i],"-i"))
		{
			cam = CAM_IU233;
		}
		else if (!strcmp(argv[i],"-b"))
		{
			cam = CAM_IMX219_B;
		}
		else if (!strcmp(argv[i],"-q"))
		{
			cam = CAM_IMX219_Q;
		}
	}
	MIPI_RIIC_INIT();
	R_MIPI_CameraReset();
	R_MIPI_Miscellaneous();
	R_MIPI_CameraPowOn(cam);
	sleep(3);
	R_MIPI_CameraClkStart();
	//~MIPI_RIIC_CLOSE();
	return 0;
}
