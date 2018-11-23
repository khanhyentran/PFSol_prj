#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/time.h>
#include <sys/ioctl.h>
#include "mipi.h"
#include "camera_mipi_imx219.h"

static unsigned int cap_buf_addr1 = 0x40200000;/* offset in internal RAM or HyperRAM*/
static unsigned int cap_buf_addr2 = 0x40380000;/* offset in internal RAM*/
static unsigned int cap_buf_size = 384000; 	/* Capture buffer size */

struct mem_map {
	char name[20];
	void *base;
	uint32_t addr;
	uint32_t size;
};

struct mem_map map_mipi, map_vin, map_ram1;
int mem_fd;

static void save_image(void)
{
	int fd;
	char *filename;
	int ret = 0;

	filename = malloc(12);
	if (filename == NULL)
		return;

#ifdef DEBUG
	printf("Saving image to file /tmp/capture.bin\n");
#endif

	sprintf(filename, "/tmp/capture.bin");

	fd = open(filename, O_CREAT | O_WRONLY | O_TRUNC,
				S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH);
	if (fd == -1)
		printf("open fd failed \n");;

	ret =  write(fd, map_ram1.base, 384000);
	if (ret < 0){
		printf("write error \n");
		}
	else if(ret != (384000))
		printf("write error: only %d bytes written instead of\n", ret);

	close(fd);
}

int close_resources(void)
{
#ifdef DEBUG
	printf("close_resources \n");
#endif
	if( map_mipi.base ) {
		munmap(map_mipi.base, map_mipi.size);
		map_mipi.base = 0;
	}
	
	if( map_vin.base ) {
		munmap(map_vin.base, map_vin.size);
		map_vin.base = 0;
	}
		
	if( map_ram1.base ) {
		munmap(map_ram1.base, map_ram1.size);
		map_ram1.base = 0;
	}
}

int open_resources(void)
{
	map_mipi.addr = 0xe8209000;
	map_mipi.size = 0x26C;
#ifdef DEBUG
	printf("open_resources \n");
#endif
	/* Open a handle to all memory */
	mem_fd = open("/dev/mem", O_RDWR);
	if (mem_fd < 0) {
		perror("Can't open /dev/mem");
		return -1;
	}

	/* Map for MIPI registers */
	map_mipi.base = mmap(NULL, map_mipi.size,
			  PROT_READ | PROT_WRITE,
			  MAP_SHARED, mem_fd, 
			  map_mipi.addr );

	if (map_mipi.base == MAP_FAILED) {
		map_mipi.base = NULL;
		perror("Could not map MIPI registers");
		close(mem_fd);
		return -1;
	}
	mipi_set_base_addr(map_mipi.base);

#ifdef DEBUG
	printf("map_mipi.size=0x%lx\n", map_mipi.size);
	printf("map_mipi.addr=0x%lx\n", map_mipi.addr);
	printf("map_mipi.base=%p\n", map_mipi.base);
#endif
	
	map_vin.addr = 0xe803f000;
	map_vin.size = 0x30C;

	/* Map for VIN register */
	map_vin.base = mmap(NULL, map_vin.size,
			  PROT_READ | PROT_WRITE,
			  MAP_SHARED, mem_fd, 
			  map_vin.addr );

	if (map_vin.base == MAP_FAILED) {
		map_vin.base = NULL;
		perror("Could not map VIN registers");
		close(mem_fd);
		return -1;
	}

	vin_set_base_addr(map_vin.base);
#ifdef DEBUG
	printf("map_vin.size=0x%lx\n", map_vin.size);
	printf("map_vin.addr=0x%lx\n", map_vin.addr);
	printf("map_vin.base=%p\n", map_vin.base);

#endif
	/* Map RAM used for capturing frames from VIN */
	map_ram1.addr = cap_buf_addr1;
	map_ram1.size = cap_buf_size;

	map_ram1.base = mmap(NULL, map_ram1.size,
			  PROT_READ | PROT_WRITE,
			  MAP_SHARED, mem_fd, 
			  map_ram1.addr );
	if (map_ram1.base == MAP_FAILED)
	{
		map_ram1.base = NULL;
		perror("Could not map memory for captured frames");
		close(mem_fd);
		return -1;
	}
#ifdef DEBUG
	printf("map_ram1.size=0x%lx\n", map_ram1.size);
	printf("map_ram1.addr=0x%lx\n", map_ram1.addr);
	printf("map_ram1.base=%p\n", map_ram1.base);
#endif

	return 0;
}

int timeval_subtract (struct timeval *result, struct timeval *x, struct timeval *y)
{
#ifdef DEBUG
	printf("timeval_subtract \n");
#endif
	/* Perform the carry for the later subtraction by updating y. */
	if (x->tv_usec < y->tv_usec) {
		int nsec = (y->tv_usec - x->tv_usec) / 1000000 + 1;
		y->tv_usec -= 1000000 * nsec;
		y->tv_sec += nsec;
	}
	if (x->tv_usec - y->tv_usec > 1000000) {
		int nsec = (x->tv_usec - y->tv_usec) / 1000000;
		y->tv_usec += 1000000 * nsec;
		y->tv_sec -= nsec;
	}

	/* Compute the time remaining to wait.
	tv_usec is certainly positive. */
	result->tv_sec = x->tv_sec - y->tv_sec;
	result->tv_usec = x->tv_usec - y->tv_usec;

	/* Return 1 if result is negative. */
	return x->tv_sec < y->tv_sec;
}

void main(void)
{
	int32_t retval;
	struct timeval tv1,tv2,tv3;
#ifdef DEBUG
	printf("main \n");
#endif	

	if(open_resources())
		printf("open resource failed\n");
	/* Open I2C device for camera setting */
	MIPI_RIIC_INIT();
	/* Camera reset */
	retval = R_MIPI_CameraReset();
	/* Start MIPI clock supply */
	retval = R_MIPI_StandbyOut();
	/* Camera setting */
	retval = R_MIPI_CameraPowOn();
	/* Initial setting of MIPI / VIN */
   	retval = R_MIPI_Open();
	/* Camera clock start */
	retval = R_MIPI_CameraClkStart();


	retval = R_MIPI_Setup();
	/*Set the capture mode*/
	retval = R_MIPI_SetMode(0); /*0: sigle mode, 1 continuous mode*/

	/* Set the address of our capture buffer */
	retval = R_MIPI_SetBufferAdr(0, map_ram1.addr);

	gettimeofday(&tv1, NULL);	/* record start time */	
    	
	/* Start capture */
    	retval = R_MIPI_CaptureStart();

	gettimeofday(&tv2, NULL);	/* record end time */
	timeval_subtract(&tv3,&tv2,&tv1);
	printf("capture time = %ld:%06ld sec\n", (long)tv3.tv_sec, (long)tv3.tv_usec);
	save_image();

app_exit:
	printf("app_exit \n");
	MIPI_RIIC_CLOSE();
	close_resources();
}
