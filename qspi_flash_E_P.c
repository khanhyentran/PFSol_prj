/*
 * Sample driver for erase/program/read of QSPI flash while executing an XIP
 * kernel out of the QSPI flash.
 * While communicating with the SPI flash, system will be off and the QSPI
 * peripheral will be in SPI mode meaning no kernel functions can be called.
 */

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/fs.h>		/* for struct file_operations */
#include <linux/errno.h>	/* Error codes */
#include <linux/slab.h>		/* for kzalloc */
#include <linux/device.h>	/* for sysfs interface */
#include <linux/mm.h>
#include <linux/interrupt.h>	/* for requesting interrupts */
#include <linux/sched.h>
#include <linux/ctype.h> 
#include <linux/uaccess.h>
#include <linux/io.h>
#include <asm/cacheflush.h>	/* for cache flush */
#include <linux/miscdevice.h>	/* Misc Driver */
#include "qspi_flash.h"

/* Options */
#define DEBUG

#define RVC_DEBUG	1

/*** Choose only one of the following Flash devices ***/
#if 0
  /* Spansion S25FL512S */
  #define S25FL512S_512_256K
  #define FLASH_PAGE_SIZE 512
  #define FLASH_ERASE_SIZE (256*1024)
#endif

#if 1
  /* Macronix MX25L51245G */
  #define MX25L51245G
  #define FLASH_PAGE_SIZE 256
  #define FLASH_ERASE_SIZE (64*1024)
#endif


#define DRIVER_VERSION	"2017-11-20"
#define DRIVER_NAME	"qspi_flash"

#ifdef DEBUG
#define DPRINTK(fmt, args...) printk("%s: " fmt, DRIVER_NAME, ## args)
#else
#define DPRINTK(fmt, args...)
#endif


#ifdef DEBUG
const char ASCII_table[] = "0123456789ABCDEF";
#define ASCII_UPPER(val) (ASCII_table[ val >> 4])
#define ASCII_LOWER(val) (ASCII_table[ val & 0x0F ])
#define debugout_PRINT_HEX(val) { debugout( ASCII_UPPER(val) ); debugout( ASCII_LOWER(val) ); }
#define debugout_NEWLINE() { debugout('\r'); debugout('\n'); }
static void debugout(u8 text)
{
	static void *scif_base = NULL;
#if 0 /* NOTE 1: Only needed if not using current console SCIF (SCIF1 on RSK) */
	static void *scif_clk = NULL;
#endif
	/* SCIF2 */
	if(!scif_base) {
#if 0 /* (See NOTE 1 above) */
		/* SCIF 1 pinmux */
		r7s72100_pfc_pin_assign(P4_12, ALT7, DIIO_PBDC_DIS);	/* SCIF1 TX */
		r7s72100_pfc_pin_assign(P4_13, ALT7, DIIO_PBDC_DIS);	/* SCIF1 RX */

		/* SCIF 1 clock */
		scif_clk = clk_get_sys("sh-sci.1", "sci_fck");
		clk_enable(scif_clk);
#endif
		/* SCIF Registers */
		/* Map registers so we can get at them */
		/* Please choose your correct SCIF channel */
//		scif_base = ioremap_nocache(0xE8007800, 0x30); /* SCIF1 */
		scif_base = ioremap_nocache(0xe8008000, 0x30); /* SCIF2 */
//		scif_base = ioremap_nocache(0xe8008800, 0x30); /* SCIF3 */
		#define SCFTDR *(volatile uint8_t *)(scif_base + 0x0C)
		#define SCSCR *(volatile uint16_t *)(scif_base + 0x08)
		#define SCBRR *(volatile uint8_t *)(scif_base + 0x04)
		#define SCFSR *(volatile uint16_t *)(scif_base + 0x10)
		#define SCFCR *(volatile uint16_t *)(scif_base + 0x18)
		#define SCFDR *(volatile uint16_t *)(scif_base + 0x1C)

#if 0 /* (See NOTE 1 above) */
		/* trigger TDFE on 16 bytes */
		SCFCR = 0x30;

		/* Minimum SCIF setup */
		SCBRR = 0x11;		// Baud 115200
		SCSCR = 0x0030;	// Enable TX an RX, but no interrupts
#endif

		/* NOTE that the first time this is called, we still
		 * need to be runing from XIP QSPI flash */
		return;
	}

	// wait for TDFE to be set
	while((SCFSR & (0x1 << 5)) == 0);

	SCFTDR = text;
	
	// clear TDFE and wait for reset (meaning data has been sent)
	SCFSR = SCFSR & ~(1<<5);
	while((SCFSR & (0x1 << 5)) == 0);
}
#else
#define debugout(t) { }
#define debugout_PRINT_HEX(val) { }
#define debugout_NEWLINE() { }
#endif


/* Global variables */
static int major_num, minor_num;
static struct qspi_flash my_qf;
static struct miscdevice my_dev;

u8 prog_buffer[ FLASH_PAGE_SIZE * 2];
int prog_buffer_cnt = 0;

const u8 SPIDE_for_single[5] = {0x0,
		0x8,	// 8-bit transfer (1 bytes)
		0xC,	// 16-bit transfer (2 bytes)
		0x0,	// 24-bit transfers are invalid!
		0xF};	// 32-bit transfer (3-4 bytes)
const u8 SPIDE_for_dual[9] = {0,
		0x0,	// 8-bit transfers are invalid!
		0x8,	// 16-bit transfer (1 byte)
		0x0,	// 24-bit transfers are invalid!
		0xC,	// 32-bit transfer (4 bytes)
		0x0,	// 40-bit transfers are invalid!
		0x0,	// 48-bit transfers are invalid!
		0x0,	// 56-bit transfers are invalid!
		0xF};	// 64-bit transfer (8 bytes)

static inline int qspi_is_ssl_negated(struct qspi_flash *qf)
{
	return !(qspi_read32(qf, QSPI_CMNSR) & CMNSR_SSLF);
}
static int qspi_flash_wait_for_tend(struct qspi_flash *qf)
{
	volatile u32 cnt = 100000;
	do{
		if(qspi_read32(qf, QSPI_CMNSR) & CMNSR_TEND)
			return 0;
		asm("nop");
		asm("nop");
		asm("nop");
		asm("nop");
		cnt--;
	}while( cnt );

	return -1;
}

static int qspi_flash_cs_disable(struct qspi_flash *qf)
{
	u32 cmnsr;

	cmnsr = qspi_read32(qf, QSPI_CMNSR);

	/* Already low? */
	if( cmnsr == CMNSR_TEND )
		return 0;

	/* wait for last transfer to complete */
	if( qspi_flash_wait_for_tend(qf) )
		return -1;

	cmnsr = qspi_read32(qf, QSPI_CMNSR);
	if( cmnsr == CMNSR_TEND )
		return 0;
	
	/* Make sure CS goes back low (it might have been left high
	   from the last transfer). It's tricky because basically,
	   you have to disable RD and WR, then start a dummy transfer. */
	qspi_write32(qf, QSPI_SMCR, 1);
	qspi_write32(qf, QSPI_SMCR, 0);
	qspi_flash_wait_for_tend(qf);

	/* Check the status of the CS pin */
	cmnsr = qspi_read32(qf, QSPI_CMNSR);
	if( cmnsr == CMNSR_TEND )
		return 0;
	
	return -1;
}

static int qspi_flash_send_data(struct qspi_flash *qf,
	const u8* buf, unsigned int len, unsigned int keep_cs_low)
{
	int ret;
	u32 smcr;
	u32 smenr = 0;
	u32 smwdr0;
	u32 smwdr1 = 0;
	int unit;
	int sslkp = 1;

	/* wait spi transfered */
	if ((ret = qspi_flash_wait_for_tend(qf)) < 0) {
		/* timeout */
		return ret;
	}

	while (len > 0) {

		if( qf->dual ) {
			/* Dual memory */
			if (len >= 8)
				unit = 8;
			else
				unit = len;

			if( unit & 1 ) {
				/* Can't send odd number of bytes in dual memory mode */
				return -1;
			}

			if( unit == 6 )
				unit = 4; /* 6 byte transfers not supported */

			smenr &= ~0xF;	/* clear SPIDE bits */
			smenr |= SPIDE_for_dual[unit];
		}
		else {
			/* Single memory */
			if (len >= 4)
				unit = 4;
			else
				unit = len;
			if(unit == 3)
				unit = 2;	/* 3 byte transfers not supported */

			smenr &= ~0xF;	/* clear SPIDE bits */
			smenr |= SPIDE_for_single[unit];
		}

		if( !qf->dual ) {
			/* Single memory */

			/* set data */
			smwdr0 = (u32)*buf++;
			if (unit >= 2)
			smwdr0 |= (u32)*buf++ << 8;
			if (unit >= 3)
			smwdr0 |= (u32)*buf++ << 16;
			if (unit >= 4)
			smwdr0 |= (u32)*buf++ << 24;
		}
		else
		{
			/* Dual memory */
			if( unit == 8 ) {
				/* Note that SMWDR1 gets sent out first
				   when sending 8 bytes */
				smwdr1 = (u32)*buf++;
				smwdr1 |= (u32)*buf++ << 8;
				smwdr1 |= (u32)*buf++ << 16;
				smwdr1 |= (u32)*buf++ << 24;
			}
			/* sending 2 bytes */
			smwdr0 = (u32)*buf++;
			smwdr0 |= (u32)*buf++ << 8;

			/* sending 4 bytes */
			if( unit >= 4) {
				smwdr0 |= (u32)*buf++ << 16;
				smwdr0 |= (u32)*buf++ << 24;
			}
		}

		/* Write data to send */
		if (unit == 2){
			qspi_write16(qf, (u16)smwdr0, QSPI_SMWDR0);
		}
		else if (unit == 1){
			qspi_write8(qf, (u8)smwdr0, QSPI_SMWDR0);
		}
		else {
			qspi_write32(qf, smwdr0, QSPI_SMWDR0);
		}

		if( unit == 8 ) {
			/* Dual memory only */
			qspi_write32(qf, smwdr1, QSPI_SMWDR1);
		}

		len -= unit;
		if (len <= 0) {
			sslkp = 0;
		}

		/* set params */
		qspi_write32(qf, 0, QSPI_SMCMR);
		qspi_write32(qf, 0, QSPI_SMADR);
		qspi_write32(qf, 0, QSPI_SMOPR);
		qspi_write32(qf, smenr, QSPI_SMENR);

		/* start spi transfer */
		smcr = SMCR_SPIE|SMCR_SPIWE;
		if (sslkp || keep_cs_low)
			smcr |= SMCR_SSLKP;
		qspi_write32(qf, smcr, QSPI_SMCR);

		/* wait spi transfered */
		if ((ret = qspi_flash_wait_for_tend(qf)) < 0) {
			/* data send timeout */
			return ret;
		}
	}
	return 0;
}

static int qspi_flash_recv_data(struct qspi_flash *qf,
	u8 *buf, unsigned int len, unsigned int keep_cs_low)
{
	int ret;
	u32 smcr;
	u32 smenr = 0;
	u32 smrdr0;
	u32 smrdr1 = 0;
	int unit;
	int sslkp = 1;
	u8 combine = 0;

	/* wait spi transfered */
	if ((ret = qspi_flash_wait_for_tend(qf)) < 0) {
		/* xmit timeout */
		return ret;
	}

	/* When receiving data from a command, we need to take into account
	   that there are 2 SPI devices (that each will return values) */
	if( qf->dual && !qf->disable_combine)
	{
		switch (qf->this_cmd) {
			case 0x9F: /* Read ID */
			case 0x05: /* Read Status register (CMD_READ_STATUS) */
			case 0x70: /* Read Status register (CMD_FLAG_STATUS) */
			case 0x35: /* Read configuration register */
			case 0x16: /* Read Bank register (CMD_BANKADDR_BRRD) */
			case 0xC8: /* Read Bank register (CMD_EXTNADDR_RDEAR) */
			case 0xB5: /* Read NON-Volatile Configuration register (Micron) */
			case 0x85: /* Read Volatile Configuration register (Micron) */
				combine = 1;
				len *= 2;	// get twice as much data.
				break;
		}
	}

	/* Flash devices with this command wait for bit 7 to go high (not LOW) when
	   erase or writing is done, so we need to AND the results, not OR them,
	   when running in dual SPI flash mode */
	if( qf->this_cmd == 0x70 )
		qf->combine_status_mode = 1; /* AND results (WIP = 1 when ready) */
	else
		qf->combine_status_mode = 0; /* OR results (WIP = 0 when ready) */

	/* Reset after each command */
	qf->disable_combine = 0;

	while (len > 0) {
		if( qf->dual ) {
			/* Dual memory */
			if (len >= 8)
				unit = 8;
			else
				unit = len;

			if( unit & 1 ) {
				/* ERROR: Can't read odd number of bytes in dual memory mode */
				return -1;
			}

			if( unit == 6 )
				unit = 4; /* 6 byte transfers not supported, do 4 then 2 */

			smenr = SPIDE_for_dual[unit];
		}
		else {
			/* Single memory */
			if (len >= 4)
				unit = 4;
			else
				unit = len;
			if(unit == 3)
				unit = 2;	/* 3 byte transfers not supported */

			smenr = SPIDE_for_single[unit];
		}

		len -= unit;
		if (len <= 0) {
			sslkp = 0;	/* Last transfer */
		}

		/* set params */
		qspi_write32(qf, 0, QSPI_SMCMR);
		qspi_write32(qf, 0, QSPI_SMADR);
		qspi_write32(qf, 0, QSPI_SMOPR);
		qspi_write32(qf, smenr, QSPI_SMENR);

		/* start spi transfer */
		smcr = SMCR_SPIE|SMCR_SPIRE|SMCR_SPIWE;
		if (sslkp | keep_cs_low )
			smcr |= SMCR_SSLKP;
		qspi_write32(qf, smcr, QSPI_SMCR);

		/* wait spi transfered */
		if ((ret = qspi_flash_wait_for_tend(qf)) < 0) {
			/* data receive timeout */
			return ret;
		}

		/* Just read both registers. We'll figure out what parts
		   are valid later */
		smrdr0 = qspi_read32(qf, QSPI_SMRDR0);
		smrdr1 = qspi_read32(qf, QSPI_SMRDR1);

		if( !combine ) {
			if (unit == 8) {
				/* Dual Memory */
				/* SMDR1 has the beginning of the RX data (but
				   only when 8 bytes are being read) */
				*buf++ = (u8)(smrdr1 & 0xff);
				*buf++ = (u8)((smrdr1 >> 8) & 0xff);
				*buf++ = (u8)((smrdr1 >> 16) & 0xff);
				*buf++ = (u8)((smrdr1 >> 24) & 0xff);
			}

			*buf++ = (u8)(smrdr0 & 0xff);
			if (unit >= 2) {
				*buf++ = (u8)((smrdr0 >> 8) & 0xff);
			}
			if (unit >= 4) {
				*buf++ = (u8)((smrdr0 >> 16) & 0xff);
				*buf++ = (u8)((smrdr0 >> 24) & 0xff);
			}

		}
		else {
			/* Dual Memory - Combine 2 streams back into 1 */
			/* OR/AND together the data coming back so the WIP bit can be
			   checked for erase/write operations */
			/* Combine results together */
			if ( unit == 8 ) {
				/* SMRDR1 always has the beginning of the RX data stream */
				if( qf->combine_status_mode) { /* AND results together */
					*buf++ = (u8)(smrdr1 & 0xff) & (u8)((smrdr1 >> 8) & 0xff);
					*buf++ = (u8)((smrdr1 >> 16) & 0xff) & (u8)((smrdr1 >> 24) & 0xff);
					*buf++ = (u8)(smrdr0 & 0xff) & (u8)((smrdr0 >> 8) & 0xff);
					*buf++ = (u8)((smrdr0 >> 16) & 0xff) & (u8)((smrdr0 >> 24) & 0xff);
				}
				else {	/* OR results together */
					*buf++ = (u8)(smrdr1 & 0xff) | (u8)((smrdr1 >> 8) & 0xff);
					*buf++ = (u8)((smrdr1 >> 16) & 0xff) | (u8)((smrdr1 >> 24) & 0xff);
					*buf++ = (u8)(smrdr0 & 0xff) | (u8)((smrdr0 >> 8) & 0xff);
					*buf++ = (u8)((smrdr0 >> 16) & 0xff) | (u8)((smrdr0 >> 24) & 0xff);
				}
			}

			if( unit == 2 ) {
				if( qf->combine_status_mode) { /* AND results together */
					*buf++ = (u8)(smrdr0 & 0xff) & (u8)((smrdr0 >> 8) & 0xff);
				}
				else {	/* OR results together */
					*buf++ = (u8)(smrdr0 & 0xff) | (u8)((smrdr0 >> 8) & 0xff);
				}

			}
			if (unit == 4) {
				if( qf->combine_status_mode) { /* AND results together */
					*buf++ = (u8)(smrdr0 & 0xff) & (u8)((smrdr0 >> 8) & 0xff);
					*buf++ = (u8)((smrdr0 >> 16) & 0xff) & (u8)((smrdr0 >> 24) & 0xff);
				}
				else {	/* OR results together */
					*buf++ = (u8)(smrdr0 & 0xff) | (u8)((smrdr0 >> 8) & 0xff);
					*buf++ = (u8)((smrdr0 >> 16) & 0xff) | (u8)((smrdr0 >> 24) & 0xff);
				}
			}
		}
	}

	return 0;
}


static int qspi_flash_send_cmd(struct qspi_flash *qf,
	u8 *buf, unsigned int len, unsigned int keep_cs_low)
{
	u8 dual_cmd[12];
	int ret;

	qf->this_cmd = buf[0];

	/* If this is a dual SPI Flash, we need to send the same
	   command to both chips. */
	if( qf->dual )
	{
		int i,j;
		for(i=0,j=0;i<len;i++) {
			dual_cmd[j++] = buf[i];
			dual_cmd[j++] = buf[i];
		}
		len *= 2;
		buf = dual_cmd;
	}
	ret = qspi_flash_send_data(qf, buf, len, keep_cs_low);

	return ret;
}

static int qspi_flash_wait_ready(struct qspi_flash *qf)
{
#if defined(S25FL512S_512_256K) || defined(MX25L51245G)
	/* Spansion, Macronix */
	u8 cmd = 0x05;	/* Read Status Register-1 */
#endif
	u8 status[2] = {0,0};
	int timeout = 1000000;

	/* Wait for ready */
	qspi_flash_send_cmd(qf, &cmd, 1, CS_KEEP_LOW);
	while( timeout ) {

		/* Read Status register(s) */
		/* For dual SPI, each SPI flash will have a status register that needs
		 * to be checked. If single SPI, the register is just read twice which
		 * is fine */
		qspi_flash_recv_data(qf, status, 2, CS_KEEP_LOW);

		/* Display results (for debug only) */
		//debugout_PRINT_HEX(status[0]); debugout(','); debugout_PRINT_HEX(status[1]); debugout_NEWLINE();

#if defined(S25FL512S_512_256K) || defined(MX25L51245G)
		/* Spansion: Work in progress is bit 0. Both need to be 0 */
		if( !(status[0] & 1) && !(status[1] & 1) )
			break; /* WIP == 0 */
#endif

		asm("nop");
		asm("nop");
		asm("nop");
		asm("nop");
		timeout--;
	}

	/* End transaction by doing one more read with CS_BACK_HIGH */
	qspi_flash_recv_data(qf, status, 2, CS_BACK_HIGH);

	/* Check the status register for an Erase or Program Error */
#if defined(S25FL512S_512_256K)
	/* Bit 6 is P_ERR for Programming error occurred
	 * Bit 5 is E_ERR for Erase error has occurred */
	if( (status[0] & 0x60) || (status[1] & 0x60) )
	{
		qf->op_err = 1;

		/* Send Clear Status Register command */
		cmd = 0x30;
		qspi_flash_send_cmd(qf, &cmd, 1, CS_BACK_HIGH);
	}
#elif defined(MX25L51245G)
	/* The programming and erase errors are located in Security Register */
        cmd = 0x2B;
	qspi_flash_send_cmd(qf, &cmd, 1, CS_KEEP_LOW);

	qspi_flash_recv_data(qf, status, 2, CS_BACK_HIGH);
	/* Bit 6 is P_ERR for Erase error occurred
	 * Bit 5 is E_ERR for Programming error has occurred */
	if( (status[0] & 0x60) || (status[1] & 0x60) ) // Do we have dual? If not second check to be removed
	{
		qf->op_err = 1;

		/* There is no clear status command needed. */
	}
#endif

	if( timeout == 0 )
		return -1;

	return 0;
}

static int qspi_flash_set_config(struct qspi_flash *qf)
{
	u32 value;

	/* NOTES: Set swap (SFDE) so the order of bytes D0 to D7 in the SPI RX/TX FIFO are always in the
	   same order (LSB=D0, MSB=D7) regardless if the SPI did a byte, word, dwrod fetch */
	value = 
		CMNCR_MD|	       		/* spi mode */
		CMNCR_SFDE|			/* swap */
		CMNCR_MOIIO3(OUT_HIZ)|
		CMNCR_MOIIO2(OUT_HIZ)|
		CMNCR_MOIIO1(OUT_HIZ)|
		CMNCR_MOIIO0(OUT_HIZ)|
		CMNCR_IO3FV(OUT_HIZ)|
		CMNCR_IO2FV(OUT_HIZ)|
		CMNCR_IO0FV(OUT_HIZ)|
		//CMNCR_CPHAR|CMNCR_CPHAT|CMNCR_CPOL| /* spi mode3 */
		CMNCR_CPHAR|
		CMNCR_BSZ(BSZ_SINGLE);

	/* dual memory? */
	if (qf->dual)
		value |= CMNCR_BSZ(BSZ_DUAL);	/* s-flash x 2 */

	/* set common */
	qspi_write32(qf, value, QSPI_CMNCR);

#if 0
	/* setup delay */
	qspi_write32(qf,
		SSLDR_SPNDL(SPBCLK_1_0)|	/* next access delay */
		SSLDR_SLNDL(SPBCLK_1_0)|	/* SPBSSL negate delay */
		SSLDR_SCKDL(SPBCLK_1_0),	/* clock delay */
		QSPI_SSLDR);
#endif

	/* sets transfer bit rate to 1.39 Mbps (for using with slower logic analyzers) */
	//qspi_write32(qf, 0x0603, QSPI_SPBCR);

	return 0;
}
static int qspi_flash_mode_spi(struct qspi_flash *qf)
{
	u32 val;

	/* Turn off all system interrupts */
	local_irq_save(qf->flags);

	/* Flush all the cache (makes things much more stable) */
	flush_cache_all();

	qf->drcr_save = qspi_read32(qf, QSPI_DRCR);

	/* Force SSL low by turning off Burst Read */
	qspi_write32(qf, DRCR_SSLN, QSPI_DRCR);

	/* Disable Read & Write in SPI mode */
	qspi_write32(qf, 0, QSPI_SMCR);	

	qspi_flash_wait_for_tend(qf);

	/* Check if SSL still low */
	if (!qspi_is_ssl_negated(qf))
	{
		debugout_NEWLINE();
		debugout('C');
		debugout('M');
		debugout('N');
		debugout('S');
		debugout('R');
		debugout('=');
		debugout_PRINT_HEX( (u8) qspi_read32(qf, QSPI_CMNSR));
		debugout_NEWLINE();

		local_irq_restore(qf->flags);
		return -EIO;
	}

	/* Save the original value */
	qf->cmncr_save = qspi_read32(qf, QSPI_CMNCR);

	/* Exit XIP */
	val = qspi_read32(qf, QSPI_CMNCR);
	val |= CMNCR_MD;	/* SPI operating mode */
	qspi_write32(qf, val, QSPI_CMNCR);

	/*====================================================*/
	/* NOW IN SPI MODE. CANNOT EXECUTE ANY CODE FROM QSPI */
	/*====================================================*/

	/* Register config */
	qspi_flash_set_config(qf);

	return 0;
}

static int qspi_flash_mode_xip(struct qspi_flash *qf)
{
	u32 val;

	/* End the transfer by putting SSL low (SPI mode) */
	qspi_flash_cs_disable(qf);

	/* Put DRCR back */
	qspi_write32(qf, qf->drcr_save, QSPI_DRCR);

	/* Re-enter XIP mode */
	qspi_write32(qf, qf->cmncr_save, QSPI_CMNCR);

	/* Clear XIP cache */
	val = qspi_read32(qf, QSPI_DRCR) | DRCR_RCF;
	qspi_write32(qf, val, QSPI_DRCR);
	val = qspi_read32(qf, QSPI_DRCR);	// dummy read

	local_irq_restore(qf->flags);

	/*=========================================================*/
	/* NOW IN XIP MODE. ALL KERNEL FUNCTIONS ARE NOW AVAILABLE */
	/*=========================================================*/

	return 0;
}

static int qspi_flash_do_read(struct qspi_flash *qf, u8 *buf, int len)
{
#if (RVC_DEBUG == 0)
#if defined(S25FL512S_512_256K) || defined(MX25L51245G)
	/* Spansion: Read Fast (4-byte Address) - (Requires dummy cycles) */
	/* Macronix: FAST READ4B (4-byte Address) - (Requires dummy cycles) */
	u8 cmd[6] = {0x0C};
#endif
	int ret;

	/* Insert address into read command */
	cmd[1] = (qf->op_addr >> 24) & 0xFF;
	cmd[2] = (qf->op_addr >> 16) & 0xFF;
	cmd[3] = (qf->op_addr >> 8)  & 0xFF;
	cmd[4] = (qf->op_addr)       & 0xFF;

#if defined(S25FL512S_512_256K) || defined(MX25L51245G)
	/* Insert dummy cycles into read command */
	cmd[5] = 0xFF;	/* 8 dummy cycles (1 bytes) */
#endif

	/* Enter SPI Mode (exit XIP mode) */
	ret = qspi_flash_mode_spi(qf);
	if( ret ) {
		printk("%s: Cannot enter SPI mode\n",DRIVER_NAME);
		return ret;
	}

	qspi_flash_send_cmd(qf, cmd, 6, CS_KEEP_LOW);
	qspi_flash_recv_data(qf, buf, len, CS_BACK_HIGH);
	
	/* update address for next time */
	if ( qf->dual )
		qf->op_addr += len / 2;	/* 2 bytes per address */
	else
		qf->op_addr += len;

	/* Exit SPI Mode (re-enter XIP mode) */
	qspi_flash_mode_xip(&my_qf);
#else
	qf->op_addr += len;

	memcpy(buf, my_qf.mmio, len);
	
	DPRINTK("\n[RVC_DEBUG] %s %d --> DATA = %X %X %X %X %X %X %X %X %X %X len = %x\n", __func__, __LINE__,
	 buf[0], buf[1], buf[2], buf[3], buf[4], buf[5], buf[6], buf[7], buf[8], buf[9], len);
#endif
	
	return 0;
}
static int qspi_flash_do_erase(struct qspi_flash *qf)
{
#if defined(S25FL512S_512_256K) || defined(MX25L51245G)
	/* Spansion, Macronix */
	u8 cmd[5] = {0xDC}; /* Erase 256 kB (4-byte Address) */
	u8 cmd_wren = 0x06;	/* Write Enable command */
#endif
	int ret;

	/* Insert address into read command */
	cmd[1] = (qf->op_addr >> 24) & 0xFF;
	cmd[2] = (qf->op_addr >> 16) & 0xFF;
	cmd[3] = (qf->op_addr >> 8)  & 0xFF;
	cmd[4] = (qf->op_addr)       & 0xFF;

	/* Enter SPI Mode (exit XIP mode) */
	ret = qspi_flash_mode_spi(qf);
	if( ret ) {
		printk("%s: Cannot enter SPI mode\n",DRIVER_NAME);
		return ret;
	}

	/* Send Write Enable */
	qspi_flash_send_cmd(qf, &cmd_wren, 1, CS_BACK_HIGH);
	/* Send erase command */
	qspi_flash_send_cmd(qf, cmd, 5, CS_BACK_HIGH);
	/* Wait for ready */
	ret = qspi_flash_wait_ready(qf);

	/* Exit SPI Mode (re-enter XIP mode) */
	qspi_flash_mode_xip(&my_qf);
	
#if RVC_DEBUG
	//u8 out_buff[10];

	//memcpy(out_buff, my_qf.mmio /*+ 0x5000000*/, sizeof(u8) * 10);
	
	//DPRINTK("\n[RVC_DEBUG] %s %d --> DATA = %X %X %X %X %X %X %X %X %X %X\n", __func__, __LINE__,
	 //out_buff[0], out_buff[1], out_buff[2], out_buff[3], out_buff[4], out_buff[5], out_buff[6], out_buff[7], out_buff[8], out_buff[9]);
	 
	u8 *debug_buf;
	int length = 256;
	int i;
	
	debug_buf = kmalloc(length, GFP_KERNEL);
	if (!debug_buf) {
		printk("Cannot allocate memory for debug_buf!\n");
		return -ENOMEM;
	} 
	
	memcpy(debug_buf, my_qf.mmio, sizeof(u8) * length);
	DPRINTK("\n[RVC_DEBUG] %s %d --> DATA = ", __func__, __LINE__);
	
	for (i = 0; i < length; i++)
		DPRINTK("%02X", debug_buf[i]);

	DPRINTK("\n");
	
	kfree(debug_buf);
#endif /*RVC_DEBUG*/
	
	return ret;

}
static int qspi_flash_do_program(struct qspi_flash *qf, u8 *buf, int len)
{
#if RVC_DEBUG
	//u8 out_buff[10];

	//memcpy(out_buff, my_qf.mmio /*+ 0x5000000*/, sizeof(u8) * 10);
	
	//DPRINTK("\n[RVC_DEBUG] %s %d --> DATA = %X %X %X %X %X %X %X %X %X %X\n", __func__, __LINE__,
	 //out_buff[0], out_buff[1], out_buff[2], out_buff[3], out_buff[4], out_buff[5], out_buff[6], out_buff[7], out_buff[8], out_buff[9]);
	
	u8 *debug_buf;
	int length = len;
	int i;
	
	DPRINTK("\n[RVC_DEBUG] %s %d len = %d\n", __func__, __LINE__, len);
	
	debug_buf = kmalloc(length, GFP_KERNEL);
	if (!debug_buf) {
		printk("Cannot allocate memory for debug_buf!\n");
		return -ENOMEM;
	} 
	
	memcpy(debug_buf, my_qf.mmio, sizeof(u8) * length);
	DPRINTK("\n[RVC_DEBUG] %s %d --> DATA = ", __func__, __LINE__);
	
	for (i = 0; i < length; i++)
		DPRINTK("%02X", debug_buf[i]);

	DPRINTK("\n");
	
	kfree(debug_buf);
#endif /*RVC_DEBUG*/

#if defined(S25FL512S_512_256K) || defined(MX25L51245G)
	/* Spansion, Macronix */
	u8 cmd[5] = {0x12}; /* Page Program (4-byte Address) */
	u8 cmd_wren = 0x06;	/* Write Enable */
#endif
	int ret;
	int page_size = FLASH_PAGE_SIZE * (qf->dual + 1);

	/* Enter SPI Mode (exit XIP mode) */
	ret = qspi_flash_mode_spi(qf);
	if( ret ) {
		printk("%s: Cannot enter SPI mode\n",DRIVER_NAME);
		return ret;
	}

	while(len)
	{
		/* Insert address into read command */
		cmd[1] = (qf->op_addr >> 24) & 0xFF;
		cmd[2] = (qf->op_addr >> 16) & 0xFF;
		cmd[3] = (qf->op_addr >> 8)  & 0xFF;
		cmd[4] = (qf->op_addr)       & 0xFF;

		/* Send Write Enable */
		qspi_flash_send_cmd(qf, &cmd_wren, 1, CS_BACK_HIGH);
				
		if( prog_buffer_cnt )
		{
			/* There is left over data */
			/* Is enough data to program now? */
			if( (prog_buffer_cnt + len) >= page_size ) {
		
				/* Send program command, address and data */
				qspi_flash_send_cmd(qf, cmd, 5, CS_KEEP_LOW);
				qspi_flash_send_data( qf, prog_buffer, prog_buffer_cnt, CS_KEEP_LOW);
				qspi_flash_send_data( qf, buf, page_size - prog_buffer_cnt, CS_BACK_HIGH);
				len -= page_size - prog_buffer_cnt;
				buf += page_size - prog_buffer_cnt;
				prog_buffer_cnt = 0;
				/* The address always goes up by 1 device page size regardless of single/dual
				 * because each SPI flash only gets 1 PAGE amount of data per program command */
				qf->op_addr += FLASH_PAGE_SIZE;
			}
		}
		else if( len >= page_size ) {

			/* there is enough data to program now */
			/* Send program command, address and data */
			qspi_flash_send_cmd(qf, cmd, 5, CS_KEEP_LOW);
			qspi_flash_send_data( qf, buf, page_size, CS_BACK_HIGH);
			len -= page_size;
			buf += page_size;

			/* The address always goes up by 1 device page size regardless of single/dual
			 * because each SPI flash only gets 1 PAGE amount of data per program command */
			qf->op_addr += FLASH_PAGE_SIZE;

		}
		else {

		}
		/* Wait for ready */
		ret = qspi_flash_wait_ready(qf);
		
		/* Check if an error occured */
		if ( qf->op_err )
			break;
		
		if( len < page_size ) {

			/* we don't have enough for another page. save for next time.  */
			while(len) {
				prog_buffer[prog_buffer_cnt++] = *buf++;
				len--;
			}
		}

		if( ret )
			break;
	}

	/* Exit SPI Mode (re-enter XIP mode) */
	qspi_flash_mode_xip(&my_qf);
	
#if RVC_DEBUG
	//u8 out_buf[10];

	//memcpy(out_buf, my_qf.mmio /*+ 0x5000000*/, sizeof(u8) * 10);
	
	//DPRINTK("\n[RVC_DEBUG] %s %d --> DATA = %X %X %X %X %X %X %X %X %X %X\n", __func__, __LINE__,
	 //out_buf[0], out_buf[1], out_buf[2], out_buf[3], out_buf[4], out_buf[5], out_buf[6], out_buf[7], out_buf[8], out_buf[9]);
	
	debug_buf =  kmalloc(length, GFP_KERNEL);
	if (!debug_buf) {
		printk("Cannot allocate memory for debug_buf!\n");
		return -ENOMEM;
	} 
	
	memcpy(debug_buf, my_qf.mmio, sizeof(u8) * length);
	DPRINTK("\n[RVC_DEBUG] %s %d --> DATA = ", __func__, __LINE__);
	
	for (i = 0; i < length; i++)
		DPRINTK("%02X", debug_buf[i]);

	DPRINTK("\n");
	
	kfree(debug_buf);
#endif /*RVC_DEBUG*/

	return ret;
}

static int qspi_flash_open(struct inode * inode, struct file * filp)
{
	/* Called when someone opens a handle to this device */
	return 0;
}

static int qspi_flash_release(struct inode * inode, struct file * filp)
{
	/* Called when someone closes a handle to this device */
	return 0;
}

/* File Read Operation */
/*

buf: The buffer to fill with you data
length: The size of the buffer that you need to fill

ppos: a pointer to the 'position' index that the file structure is using to know
	where in the file you are. You need to update this to show that you are
	moving your way through the file.

return: how many bytes of the buffer you filled. Note, the userland app might keep
	asking for more data until you return 0.
*/
static ssize_t qspi_flash_read(struct file * file, char __user * buf, size_t length, loff_t *ppos)
{
	u8 *out_buf;
	int err;
	int odd_number_of_bytes = 0;
	int page_size;
	u8 status;

	DPRINTK("%s called. length=%d, *ppos=%ld\n",__func__, length, (long int)*ppos);

	if( my_qf.op == OP_READ) {
		
		/* Odd number of bytes in dual mode */
		if( my_qf.dual && (length & 1) ) {
			odd_number_of_bytes = 1;
			length += 1;
		}

		out_buf = kmalloc(length, GFP_KERNEL);
		if (!out_buf) {
			my_qf.op = OP_IDLE;
			return -ENOMEM;
		}
		
		err = qspi_flash_do_read(&my_qf, out_buf, length);

		if( my_qf.op_err ) {
			my_qf.op = OP_IDLE;
			kfree(out_buf);
			return -EIO;
		}

		/* extra byte at end */
		if( odd_number_of_bytes ) {
			length -= 1;
			
			/* drop out of read mode because next address will not be correct anymore */
			my_qf.op = OP_IDLE;
		}

		err = copy_to_user(buf, out_buf, length);
		kfree(out_buf);
		
		if (err != 0 ) {
			my_qf.op = OP_IDLE;
			return -EFAULT;
		}
		
		*ppos += length;
		return length;
	}

	/* End the write command */
	if( my_qf.op == OP_PROG) {
		
		/* Do we have any pending data? */
		if( prog_buffer_cnt ) {

			/* fill the rest of the data buffer with 0xFF */
			page_size = FLASH_PAGE_SIZE * (my_qf.dual + 1);
			while( prog_buffer_cnt < page_size )
				prog_buffer[prog_buffer_cnt++] = 0xFF;
			prog_buffer_cnt = 0;

			/* Send our final data */
			qspi_flash_do_program(&my_qf, prog_buffer, page_size);
		}

		/* Return programming status */
		my_qf.op = OP_IDLE;
		status = my_qf.op_err;
		my_qf.op_err = 0;
		err = copy_to_user(buf, &status, 1);
		if (err != 0 ) {
			return -EFAULT;
		}

		*ppos += 1;
		return 1;
	}

	if( my_qf.op == OP_ERASE ) {
		/* Return erase status */
		my_qf.op = OP_IDLE;
		status = my_qf.op_err;
		my_qf.op_err = 0;
		err = copy_to_user(buf, &status, 1);
		if (err != 0 ) {
			return -EFAULT;
		}
		*ppos += 1;
		return 1;		
	}

	my_qf.op = OP_IDLE;
	my_qf.op_err = 0;

	return 0;
}

/* File Write Operation */
static ssize_t qspi_flash_write(struct file * file, const char __user * buf,  size_t count, loff_t *ppos)
{
	u8 *in_buf;

	DPRINTK("%s called. count=%d, *ppos=%ld\n",__func__, count, (long int)*ppos);

	DPRINTK("(BEFORE) op=%d,addr=0x%X,dual=%d,prog_buffer_cnt=%d,op_err=%d\n",my_qf.op,my_qf.op_addr,my_qf.dual,prog_buffer_cnt,my_qf.op_err);

	/* Operation already had an error. Need to READ to reset */
	if ( my_qf.op_err ) {
		//*ppos += count;
		//return count;
		return -EIO;
	}

	/* any write after a read cancels a read */
	if( my_qf.op == OP_READ)
	{
		my_qf.op = OP_IDLE;
		my_qf.op_addr = 0;
		my_qf.op_err = 0;
	}

	in_buf = memdup_user(buf, count);
	if(IS_ERR(in_buf))
		return PTR_ERR(in_buf);
		
#if RVC_DEBUG
	DPRINTK("\n[RVC_DEBUG] %s %d count = %d\n", __func__, __LINE__, count);
	DPRINTK("\n[RVC_DEBUG] %s %d --> DATA = ", __func__, __LINE__);
	
	int i;
	for (i = 0; i < count; i++)
		DPRINTK("%02X", in_buf[i]);
	DPRINTK("\n");
#endif

	/* Continuation of a write command */
	if( my_qf.op == OP_PROG ) {
		qspi_flash_do_program(&my_qf, in_buf, count);
		goto out;
	}

	if( my_qf.op != OP_IDLE ) {
		/* don't accept new command till last status is read */
		goto err_inval;
	}

	/* new command */
	if( in_buf[0] == 'r' ) /* "rd" */
	{
		if ( count < 6 )
			goto err_inval;;	
		my_qf.op = OP_READ;
		my_qf.op_addr = *(u32 *)(in_buf + 2 );
		/* user will get the data when they do a file read */
	}
	else if( in_buf[0] == 'e' ) /* "er" */
	{
		if ( count < 6 )
			goto err_inval;;	
		my_qf.op = OP_ERASE;
		my_qf.op_addr = *(u32 *)(in_buf + 2 );
		qspi_flash_do_erase(&my_qf);
	}
	else if( in_buf[0] == 'p' ) /* "pr" */
	{
		if ( count < 6 )
			goto err_inval;
		my_qf.op = OP_PROG;
		my_qf.op_addr = *(u32 *)(in_buf + 2 );
		
		if( count > 6 )
			qspi_flash_do_program(&my_qf, in_buf + 6, count - 6);
	}
	else if( in_buf[0] == 'd' ) /* dual mode: "d0  or d1" */
	{
		if ( count < 2 )
			goto err_inval;	
		my_qf.dual = in_buf[1] - '0';
	}
	else {
		/* invalid command */
		goto err_inval;
	}

out:
	kfree(in_buf);
	*ppos += count;

	DPRINTK("(AFTER) op=%d,addr=0x%X,dual=%d,prog_buffer_cnt=%d,op_err=%d\n",my_qf.op,my_qf.op_addr,my_qf.dual,prog_buffer_cnt,my_qf.op_err);

	return count;

err_inval:
	pr_err("invalid command or bad argument\n");
	kfree(in_buf);

	return -EINVAL;	
}

/* Define which file operations are supported */
/* The full list is in include/linux/fs.h */
struct file_operations qspi_flash_fops = {
	.owner		=	THIS_MODULE,
	.llseek		=	NULL,
	.read		=	qspi_flash_read,
	.write		=	qspi_flash_write,
	.unlocked_ioctl	=	NULL,
	.open		=	qspi_flash_open,
	.release	=	qspi_flash_release,
};

/************************* sysfs attribute files ************************/
/* For a misc driver, these sysfs files will show up under:
	/sys/devices/virtual/misc/qspi_flash/
*/
static ssize_t qspi_flash_show_dual(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct qspi_flash *pdata = dev_get_drvdata(dev);
	int count;

	count = sprintf(buf, "%d\n", pdata->dual);

	/* Return the number characters (bytes) copied to the buffer */
	return count;
}

static ssize_t qspi_flash_store_dual(struct device *dev,
		struct device_attribute *attr, const char *buf,
		size_t count)
{
	struct qspi_flash *pdata = dev_get_drvdata(dev);

	/* Scan in our argument(s) */
	sscanf(buf, "%d", &pdata->dual);

	/* Return the number of characters (bytes) we used from the buffer */
	return count;
}

static struct device_attribute qspi_flash_device_attributes[] = {
	__ATTR(	dual,0660,qspi_flash_show_dual,qspi_flash_store_dual),
};

/* This 'init' function of the driver will run when the system is booting.
   It will only run once no matter how many 'devices' this driver will
   control. Its main job to register the driver interface (file I/O).
   Only after the kernel finds a 'device' that needs to use this driver
   will the 'probe' function be called.
   A 'device' is registered in the system after a platform_device_register()
   in your board-xxxx.c is called.
   If this is a simple driver that will only ever control 1 device,
	   you can do the device registration in the init routine and avoid having
   to edit your board-xxx.c file.

  For more info, read:
	https://www.kernel.org/doc/Documentation/driver-model/platform.txt
*/

void to_show_register_value(struct qspi_flash *qf)
{
	printk("CMNSR \t\t= 0x%08X\n", qspi_read32(qf, QSPI_CMNSR));
	printk("CMNCR \t\t= 0x%08X\n", qspi_read32(qf, QSPI_CMNCR));
	printk("SSLDR \t\t= 0x%08X\n", qspi_read32(qf, QSPI_SSLDR));
	printk("DRCR \t\t= 0x%08X\n", qspi_read32(qf, QSPI_DRCR));
	printk("DRCMR \t\t= 0x%08X\n", qspi_read32(qf, QSPI_DRCMR));
	printk("DREAR \t\t= 0x%08X\n", qspi_read32(qf, QSPI_DREAR));
	printk("DROPR \t\t= 0x%08X\n", qspi_read32(qf, QSPI_DROPR));
	printk("DRENR \t\t= 0x%08X\n", qspi_read32(qf, QSPI_DRENR));
	printk("DRDMCR \t\t= 0x%08X\n", qspi_read32(qf, QSPI_DRDMCR));
	printk("DRDRENR \t\t= 0x%08X\n", qspi_read32(qf, QSPI_DRDRENR));
	//printk("PHYADJ1 \t\t= 0x%08X\n", qspi_read32(qf, QSPI_PHYADJ1));
	//printk("PHYADJ2 \t\t= 0x%08X\n", qspi_read32(qf, QSPI_PHYADJ2));
	//printk("PHYCNT \t\t= 0x%08X\n", qspi_read32(qf, QSPI_PHYCNT));
	//printk("PHYINT \t\t= 0x%08X\n", qspi_read32(qf, QSPI_PHYINT));
	//printk("PHYOFFSET1 \t= 0x%08X\n", qspi_read32(qf, QSPI_PHYOFFSET1));
	//printk("PHYOFFSET2 \t= 0x%08X\n", qspi_read32(qf, QSPI_PHYOFFSET2));
	
	printk("--END--\n\n");
}

static int __init qspi_flash_init(void)
{
	int ret;
	int i;
	u8 cmd = 0x9F; /* Read Identification (RDID 9Fh) */
	//u8 id[5];
	u8 id[10];

	my_qf.reg = ioremap(QSPI0_BASE, 0x100);
	if (my_qf.reg == NULL) {
		ret = -ENOMEM;
		printk("ioremap error.\n");
		goto clean_up;
	}
	
	my_qf.mmio = ioremap(0x25000000, 0x1000000);
	if (my_qf.mmio == NULL) {
		ret = -ENOMEM;
		printk("mmio ioremap error.\n");
		goto clean_up;
	}

	my_dev.minor = MISC_DYNAMIC_MINOR;
	my_dev.name = "qspi_flash";	// will show up as /dev/qspi_flash
	my_dev.fops = &qspi_flash_fops;
	ret = misc_register(&my_dev);
	if (ret) {
		printk(KERN_ERR "Misc Device Registration Failed\n");
		goto clean_up;
	}

	/* Save the major/minor number that was assigned */
	major_num = MISC_MAJOR;	/* same for all misc devices */
	minor_num = my_dev.minor;

	/* NOTE: The device_create_file() feature can only be used with GPL drivers */
	for (i = 0; i < ARRAY_SIZE(qspi_flash_device_attributes); i++) {
		ret = device_create_file(my_dev.this_device, &qspi_flash_device_attributes[i]);
		if (ret < 0) {
			printk(KERN_ERR "device_create_file error\n");
			break;
		}
	}

	/* Set defaults */
	my_qf.dual = 0;

	debugout(' ');	/* dummy call to set up ioremap */

#if (RVC_DEBUG == 0)
	/* Enter SPI Mode (exit XIP mode) */
	ret = qspi_flash_mode_spi(&my_qf);
	if( ret ) {
		printk("%s: Cannot enter SPI mode\n",DRIVER_NAME);
		ret = -EIO;
		goto clean_up;
	}

	/* Read the ID of the connected SPI flash */
	/* [0]=Manf, [1]=device, [2]=memory size */
	qspi_flash_send_cmd(&my_qf, &cmd, 1, CS_KEEP_LOW);
	qspi_flash_recv_data(&my_qf, id, 5, CS_BACK_HIGH);

	/* Exit SPI Mode (re-enter XIP mode) */
	qspi_flash_mode_xip(&my_qf);
#endif /* RVC_DEBUG */

#if (RVC_DEBUG == 0)
	to_show_register_value(&my_qf);
	
	//qspi_write32(&my_qf, 0x00fff300, QSPI_CMNCR);
	//qspi_write32(&my_qf, 0x00070707, QSPI_SSLDR);
	//qspi_write32(&my_qf, 0x00010100 | DRCR_SSLN | DRCR_RCF, QSPI_DRCR);
	qspi_write32(&my_qf, 0x009f0000, QSPI_DRCMR);
	//qspi_write32(&my_qf, 0x00000000, QSPI_DREAR);
	//qspi_write32(&my_qf, 0x00000000, QSPI_DROPR);
	//qspi_write32(&my_qf, 0x00004000, QSPI_DRENR);
	//qspi_write32(&my_qf, 0x00000013, QSPI_DRDMCR);
	//qspi_write32(&my_qf, 0x00000000, QSPI_DRDRENR);
	
	memcpy(id, my_qf.mmio + 0, sizeof(u8) * 10);
	
	to_show_register_value(&my_qf);
#endif /* RVC_DEBUG */
	
#if defined(S25FL512S_512_256K)
	/* Spansion S25FL512S */
	DPRINTK("Flash ID = %02X %02X %02X %02X %02X\n",id[0],id[1],id[2],id[3],id[4]);
	if( (id[0] == 0x01) && /* Spansion */
	    (id[1] == 0x02) && (id[2] == 0x20) && /* S25FL512S */
	    (id[3] == 0x4D) && (id[4] == 0x00) ) /* Wrt Page=512B, Ers Sector=256KB */
	{
		printk("%s: Detected Spansion S25FL512S (512B/256kB)\n", DRIVER_NAME);
	}
	else
		printk("%s: Warning - SPI Device is not a Spansion S25FL512S\n", DRIVER_NAME);
#elif defined(MX25L51245G)
	/* Macronix MX25L51245G */
	DPRINTK("Flash ID = %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X\n",id[0],id[1],id[2],id[3],id[4], id[5],id[6],id[7],id[8],id[9]);

	if( (id[0] == 0xC2) && /* Macronix */
	    (id[1] == 0x20) && (id[2] == 0x1A) )
	{
		printk("%s: Detected Macronix MX25L51245G (256/64kB)\n", DRIVER_NAME);
	}
	else
		printk("%s: Warning - SPI Device is not Macronix MX25L51245G\n", DRIVER_NAME);
#endif

	printk("%s: version %s\n", DRIVER_NAME, DRIVER_VERSION);

	return 0;

clean_up:
	if (my_qf.reg)
		iounmap(my_qf.reg);
		
#if RVC_DEBUG		
	if (my_qf.mmio)
		iounmap(my_qf.mmio);
#endif /* RVC_DEBUG */

	/* Remove our sysfs files */
	for (i = 0; i < ARRAY_SIZE(qspi_flash_device_attributes); i++)
		device_remove_file(my_dev.this_device, &qspi_flash_device_attributes[i]);

	misc_deregister(&my_dev);

	
	return ret;
}

static void __exit qspi_flash_exit(void)
{
	int i;

	if (my_qf.reg)
		iounmap(my_qf.reg);
		
#if RVC_DEBUG		
	if (my_qf.mmio)
		iounmap(my_qf.mmio);
#endif /* RVC_DEBUG */
	
	/* Remove our sysfs files */
	for (i = 0; i < ARRAY_SIZE(qspi_flash_device_attributes); i++)
		device_remove_file(my_dev.this_device, &qspi_flash_device_attributes[i]);

	misc_deregister(&my_dev);
}

module_init(qspi_flash_init);
module_exit(qspi_flash_exit);
MODULE_LICENSE("GPL v2");
MODULE_AUTHOR("Chris Brandt");
MODULE_ALIAS("platform:qspi_flash");
MODULE_DESCRIPTION("qspi_flash: Program QSPI in XIP mode");
