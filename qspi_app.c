#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <stdint.h>
#include <string.h>

#define INDEX_CMD	1
#define INDEX_SPICNT	2
#define INDEX_ADDR	3
#define INDEX_FILE	4
#define INDEX_LENGTH 5

void print_help( void )
{
	printf("** qspi_app: Access QSPI while in XIP mode **\n\
	r {WIDTH} {ADDRESS} {OUT_FILE} {LENGTH}\n\
		Read data to a file\n\
	R {WIDTH} {ADDRESS} OUT_FILE\n\
		Read data to a file. The first 4 bytes at ADDRESS is the length to read\n\
	e {WIDTH} {ADDRESS}\n\
		Erase one sector at address ADRESS\n\
	p {WIDTH} {ADDRESS} {IN_FILE}\n\
		Program data from a file into flash\n\
	P {WIDTH} {ADDRESS} {IN_FILE}\n\
		Program data from a file into flash. The first 4 bytes at ADDRESS will be file size\n\
\n\
	WIDTH = \'s\' for single spi flash, \'d\' for dual spi flash\n\
	ADDRESS is always in HEX\n\
	LENGTH is always in DEC\n\
");
}


#define BUF_SZ 1024 /* must be multiple of program page size */

int main(int argc, char *argv[])
{
	int fd =0, fd_out = 0, fd_in = 0;
	int i,j;
	char cmd;
	uint8_t cmd_string[20];
	int ret;
	uint32_t addr;
	char filepath[120];
	uint32_t length;
	int dual = 0;
	uint8_t status;
	uint8_t data[BUF_SZ];
	int read_amount, write_amount;
	int page_size;

	if( argc == 1 ) {
		print_help();
		exit(1);
	}

	fd = open("/dev/qspi_flash", O_RDWR);
	if( fd == -1) {
		printf("Can't open device node\n");
		exit(1);
	}

	cmd = argv[ INDEX_CMD ][0];

	/* Check correct number of aruments */
	switch( cmd ) {
		case 'e':
			i = 4;
			break;
		case 'R': 
		case 'p':
		case 'P': 
			i = 5;
			break;
		case 'r': 
			i = 6;
			break;
		default: 
			printf("ERROR: Unknown command (%c)\n", cmd);
			exit(1);
			break;
	}
	if( argc != i ) {
		printf("ERROR: Incorrect number of arguments for that command\n"
			"\tRequired: %d\n\tPassed: %d\n",i-1, argc-1);
		exit(1);
	}

	/* single/dual */
	if( argv[INDEX_SPICNT][0] == 'd')
		dual = 1;

	/* address */
	sscanf(argv[INDEX_ADDR],"%x", &addr);

	/* file in/out */
	if( argc >= 5 )
		sscanf(argv[INDEX_FILE],"%s", filepath);

	/* length */
	if( argc >= 6 )
		sscanf(argv[INDEX_LENGTH],"%d", &length);

#if 0
	/* only for debug */
	printf("\tINPUT: cmd=%c, width=%s, addr=0x%08X", cmd, dual?"dual":"single", addr);
	if( argc >= 5 )
		printf(", file=%s", filepath);
	if( argc >= 6 )
		printf(", length=%d", length);
	printf("\n\n");
#endif		

	/* set single/dual */
	ret = write(fd, dual?"d1":"d0", 2);

	/* Read */
	if( (cmd == 'r') || (cmd == 'R') )
	{
		fd_out = open(filepath, O_RDWR | O_CREAT | O_TRUNC,
			S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH );
		if( fd_out == -1)
		{
			printf("Can't open output file\n");
			goto out;
		}

		/* Write command to put into READ mode */
		cmd_string[0] = 'r';
		cmd_string[1] = 'd';
		*(uint32_t *)&cmd_string[2] = addr;
		ret = write(fd,cmd_string,6);

		if (cmd == 'R')
		{
			/* first read size of data (32-bit value) */
			ret = read(fd, &length, 4);
			if( ret < 0 )
			{
				printf("Error Reading from driver\n");
				goto out;
			}
			
			if (length == 0xFFFFFFFF )
			{
				printf("No data at address\n");
				goto out;
			}
		}

		printf("Reading out %d bytes\n", length);

		while( length )
		{
			if ( length >= BUF_SZ )
				read_amount = BUF_SZ;
			else
				read_amount = length;
			length -= read_amount;
	
			/* read from SPI flash and write to file */
			ret = read(fd, data, read_amount);
			if( ret < 0 )
			{
				printf("Error Reading from driver\n");
				goto out;
			}

			ret = write(fd_out, data, read_amount);
			if( ret < 0 )
			{
				printf("Error writting to output file\n");
				goto out;
			}

		}
	}

	/* Program */
	if( (cmd == 'p') || (cmd == 'P') )
	{
		fd_in = open(filepath, O_RDONLY);
		if( fd_in == -1)
		{
			printf("Can't open input file\n");
			goto out;
		}

		/* Write command to put into PROGRAM mode */
		cmd_string[0] = 'p';
		cmd_string[1] = 'r';
		*(uint32_t *)&cmd_string[2] = addr;
		ret = write(fd,cmd_string,6);

		/* Determine the file size of data to program */
		length = lseek(fd_in, 0, SEEK_END);
		lseek(fd_in, 0, SEEK_SET); /* rewind */
		*(uint32_t *)data = length;

		if (cmd == 'P')
		{
			/* first program size of data (32-bit value) */
			printf("Writting out 4 bytes for file size\n");
			ret = write(fd, data, 4);
		}

		printf("Writting out %d bytes\n", length);

		while( length )
		{
			if ( length >= BUF_SZ )
				write_amount = BUF_SZ;
			else
				write_amount = length;
			length -= write_amount;

			/* Read data from input file */
			ret = read( fd_in, data, write_amount);
			if( ret < 0 )
			{
				printf("Error reading from input file\n");
				break;
			}

			/* write to SPI flash */
			ret = write(fd, data, write_amount);
			if( ret < 0 )
			{
				printf("Error writting to device driver\n");
				break;
			}
		}

		/* Do a read in order to finish the operation */
		read(fd, data, 1);
		printf("Programming Complete. Status = %s\n", data[0]?"Fail":"OK");
	}

	/* Erase */
	if( cmd == 'e' )
	{
		/* Write command to put into PROGRAM mode */
		cmd_string[0] = 'e';
		cmd_string[1] = 'r';
		*(uint32_t *)&cmd_string[2] = addr;
		ret = write(fd,cmd_string,6);

		/* Do a read in order to get the result of the operation */
		ret = read(fd, data, 1);
		printf("Erase Complete. Status = %s\n", data[0]?"Fail":"OK");
	}

out:

	if ( fd_in )
		close(fd_in);
	if ( fd_out )
		close(fd_out);
	if ( fd )
		close(fd);

	return 0;
}
