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


int g_i2cFile;
uint8_t chip_addr = 0x10;
int Mipi_Camera = CAM_IU233;
// open the Linux device
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
	i2c_set_address(chip_addr);
}
// Write 8-bit data to 8-bit data register addr (DONE)
int MIPI_RIIC_Send(char* s_data, uint8_t len){	
	uint8_t c_addr = 0x10;
	struct i2c_rdwr_ioctl_data xfer_queue;
	struct i2c_msg msgs;
	int ret;

	memset(&xfer_queue, 0, sizeof(xfer_queue));
	
	msgs.addr = c_addr;
	msgs.len = len;
	msgs.flags = 0;
	msgs.buf = s_data;
	xfer_queue.msgs = &msgs;
	xfer_queue.nmsgs = 1;

	ret = ioctl(g_i2cFile, I2C_RDWR, &xfer_queue);
	if (ret < 0)
	{
		perror("Write I2C");
		exit(1);
	}
	return 0;
}
int R_MIPI_CameraPowOn( void ){
	MIPI_RIIC_INIT();
	
	if (Mipi_Camera == CAM_IU233 ) {
        char snd1[3] = {0x01,0x03,0x01};        //software reset
        char snd2[3] = {0x03,0x05,0x04};        //PREPLL DIV
        char snd3[4] = {0x02,0x02,0x03,0x20};
        char snd4[3] = {0x02,0x05,0xAF};
        char snd5[3] = {0x34,0x18,0x06};        //30fps
    
        MIPI_RIIC_Send( snd1 , 3);
        MIPI_RIIC_Send( snd2 , 3);
        MIPI_RIIC_Send( snd3 , 4);
        MIPI_RIIC_Send( snd4 , 3);
        MIPI_RIIC_Send( snd5 , 3);
    } else if ( Mipi_Camera == CAM_IMX219_B ) {

    // SW standby 
        char snd1_1[3]   = {0x01,0x00,0x00};
        char snd1_2[3]   = {0x01,0x03,0x01};

        char snd3_1[3] = {0x01,0x14,0x01};                  // 2-Lane
        char snd3_2[3] = {0x01,0x28,0x01};                  // timing setting:manual mode
        char snd3_3[4] = {0x01,0x2A,0x0C,0x00};             // INCK = 12MHz
        char snd3_4[6] = {0x01,0x60,0x04,0x38,0x07,0x80};   // frame length(c) = 1080(=438H)Aline length(¡) = 1920(=780H)
        char snd3_5[6] = {0x01,0x64,0x02,0xA8,0x0A,0x27};   // XÌÍÍÍA340`98FH
        char snd3_6[6] = {0x01,0x68,0x02,0xB4,0x06,0xEB};   // YÌÍÍÍA2EC`6B3H
        char snd3_7[6] = {0x01,0x6C,0x05,0x00,0x04,0x38};   // MoÍÍÍÍAXª640HAYª3C0Hª
        char snd3_8[4] = {0x01,0x8C,0x08,0x08};             // oÍtH[}bgÍRAW8
        char snd3_9[6] = {0x01,0x74,0x01,0x01,0x01,0x01};   // binning(digital) mode
        char snd3_10[3] = {0x01,0x57,0xC0};                 // Aigain(LONG)Rg[
        char snd3_11[5] = {0x01,0x89,0xc0,0x03,0xe8};       // Aigain(SHORT)Rg[+IõÔðQ{ÉÎ·
    
        // clock setting
        char snd4_1[3] = {0x03,0x01,0x05};   
        char snd4_2[6] = {0x03,0x04,0x02,0x02,0x00,0x3A};
        char snd4_3[3] = {0x03,0x09,0x08};
        char snd4_4[4] = {0x03,0x0C,0x00,0x32};

        MIPI_RIIC_Send( snd1_1 ,  3);
        MIPI_RIIC_Send( snd1_2 ,  3);
        MIPI_RIIC_Send( snd3_1 ,  3);
        MIPI_RIIC_Send( snd3_2 ,  3);
        MIPI_RIIC_Send( snd3_3 ,  4);
        MIPI_RIIC_Send( snd3_1 ,  3);
        MIPI_RIIC_Send( snd3_5 ,  6);
        MIPI_RIIC_Send( snd3_6 ,  6);
        MIPI_RIIC_Send( snd3_7 ,  6);
        MIPI_RIIC_Send( snd3_8 ,  4);
        MIPI_RIIC_Send( snd3_9 ,  6);
        MIPI_RIIC_Send( snd3_10 ,  3);
        MIPI_RIIC_Send( snd4_1 ,  3);
        MIPI_RIIC_Send( snd4_2 ,  6);
        MIPI_RIIC_Send( snd4_3 ,  3);
        MIPI_RIIC_Send( snd4_4 ,  4);

    } else if ( Mipi_Camera == CAM_IMX219_Q ) {
        // SW standby 
        char snd1_1[3]   = {0x01,0x00,0x00};
        char snd1_2[3]   = {0x01,0x03,0x01};

        // image setting
        char snd3_1[3] = {0x01,0x14,0x01};                  // 2-Lane
        char snd3_2[3] = {0x01,0x28,0x01};                  // timing setting:manual mode
        char snd3_3[4] = {0x01,0x2A,0x0C,0x00};             // INCK = 12MHz
        char snd3_4[6] = {0x01,0x60,0x07,0x80,0x0A,0x00};   // frame length(height) = 1920(=780H),line length(width) = 2560(=A00H)
        char snd3_5[6] = {0x01,0x64,0x01,0x68,0x0B,0x67};   // X 168-B67FH
        char snd3_6[6] = {0x01,0x68,0x01,0x10,0x08,0x8F};   // Y 110-88FH
        char snd3_7[6] = {0x01,0x6C,0x05,0x00,0x03,0xC0};   // X 640H, Y 3C0H
        char snd3_8[4] = {0x01,0x8C,0x08,0x08};             // format RAW8
        char snd3_9[6] = {0x01,0x74,0x01,0x01,0x01,0x01};   // binning(digital) mode
        char snd3_10[3] = {0x01,0x57,0x80};                 // gain(LONG)
        char snd3_11[4] = {0x01,0x5A,0x07,0x70};            // storage time
    
        // clock setting
        char snd4_1[3] = {0x03,0x01,0x05};   
        char snd4_2[6] = {0x03,0x04,0x02,0x02,0x00,0x76};
        char snd4_3[3] = {0x03,0x09,0x08};
        char snd4_4[4] = {0x03,0x0C,0x00,0x4E};

        MIPI_RIIC_Send( snd1_1 ,  3);
        MIPI_RIIC_Send( snd1_2 ,  3);
        MIPI_RIIC_Send( snd3_1 ,  3);
        MIPI_RIIC_Send( snd3_2 ,  3);
        MIPI_RIIC_Send( snd3_3 ,  4);
        MIPI_RIIC_Send( snd3_1 ,  3);
        MIPI_RIIC_Send( snd3_5 ,  6);
        MIPI_RIIC_Send( snd3_6 ,  6);
        MIPI_RIIC_Send( snd3_7 ,  6);
        MIPI_RIIC_Send( snd3_8 ,  4);
        MIPI_RIIC_Send( snd3_9 ,  6);
        MIPI_RIIC_Send( snd3_10 , 3);
        MIPI_RIIC_Send( snd3_11 , 4);
        MIPI_RIIC_Send( snd4_1 ,  3);
        MIPI_RIIC_Send( snd4_2 ,  6);
        MIPI_RIIC_Send( snd4_3 ,  3);
        MIPI_RIIC_Send( snd4_4 ,  4);
    }
	i2c_close();
	return 0;
}
int R_MIPI_CameraClkStart(void){
	char snd1[3] = {0x01,0x00,0x01};
    
    MIPI_RIIC_Send( snd1 ,3);

    return 0;
}
int main(){
	R_MIPI_CameraPowOn();
	return 0;
}
