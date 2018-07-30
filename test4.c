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

int main(int argc, char *argv[])
{
int file;
char *filename = "/dev/i2c-3";
if((file = open(filename, O_RDWR)) < 0)
{
    /* ERROR HANDLING: you can check errno to see what went wrong */
    perror("Failed to open the i2c bus");
    exit(1);
}

int addr = 0x10;          // The I2C address of mipi camera
if (ioctl(file, I2C_SLAVE, addr) < 0) {
    printf("Failed to acquire bus access and/or talk to slave.\n");
    /* ERROR HANDLING; you can check errno to see what went wrong */
    exit(1);
}
/*To write 1 byte*/
char buf[10] = {0};

buf[0] = 0x34;  //register address to write to
buf[1] = 0x18;     //data     0xa5  
if(write(file, buf, 2) != 2)
{
    /* ERROR HANDLING: i2c transaction failed */
    printf("Failed to write to the i2c bus 1.\n");
    printf("\n\n");
}

/*To read random byte*/
if(read(file, buf, 2) != 2)
{
    /* ERROR HANDLING: i2c transaction failed */
    printf("Failed to read from the i2c bus 1.\n");

    printf("\n\n");
}
else
{
    printf("random Byte: 0x%02X, 0x%02X\n",buf[0], buf[1]);
}

/*To read determined byte*/

}