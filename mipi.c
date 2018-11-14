#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>

#include "mipi.h"
static void *mipi_base;
static void *vin_base;

//#define INTS 1

/* Mipi Driver Status */
static uint8_t Mipi_State = MIPI_RESET;

/* Mipi Capture Mode */
static uint8_t Mipi_CaptureMode = CONTINUOUS_MODE;
static inline uint32_t mipi_read32(int offset)
{
	return *(uint32_t *)(mipi_base + offset);
}

static inline void mipi_write32(int offset, uint32_t data)
{
	*(uint32_t *)(mipi_base + offset) = data;
}

static inline uint32_t vin_read32(int offset)
{
	return *(uint32_t *)(vin_base + offset);
}

static inline void vin_write32(int offset, uint32_t data)
{
	*(uint32_t *)(vin_base + offset) = data;
}

void mipi_set_base_addr(void *base_addr)
{
#ifdef DEBUG
	printf("mipi_set_base_addr \n");
#endif
	mipi_base = base_addr;
}

void vin_set_base_addr(void *base_addr)
{
#ifdef DEBUG
	printf("vin_set_base_addr \n");
#endif
	vin_base = base_addr;
}

/* @brief       R_MIPI_StandbyOut DONE*/
mipi_error_t R_MIPI_StandbyOut(void){
	mipi_error_t merr = MIPI_OK;
#ifdef DEBUG
	printf("R_MIPI_StandbyOut \n");
#endif
	/* Check MIPI State */
	if( ( Mipi_State != MIPI_RESET ) && ( Mipi_State != MIPI_STANDBY ) ){
        merr = MIPI_STATUS_ERR;
		printf("R_MIPI_StandbyOut MIPI_STATUS_ERR\n");
    }
	
	if( merr == MIPI_OK ){
	/* MIPI Clock On */
	/* VIN Clock On */
	}
	
	if( merr == MIPI_OK ){
        /* MIPI State Update */
        Mipi_State = MIPI_POWOFF;
		printf("R_MIPI_StandbyOut MIPI_OK => MIPI_POWOFF\n");
    }

	return merr;
}

/* @brief       R_MIPI_StandbyIn DONE*/
mipi_error_t R_MIPI_StandbyIn( void ){
    mipi_error_t merr = MIPI_OK;

#ifdef DEBUG
	printf("R_MIPI_StandbyIn \n");
#endif	
	/* Check MIPI State */
    if( Mipi_State != MIPI_POWOFF ){
        merr = MIPI_STATUS_ERR;
		printf("R_MIPI_StandbyIn MIPI_STATUS_ERR\n");
    }
	
    if( merr == MIPI_OK ){
	/* MIPI Clock Off */
	/* VIN Clock Off */
	}
	
	if( merr == MIPI_OK ){
        /* MIPI State Update */
        Mipi_State = MIPI_STANDBY;
		printf("R_MIPI_StandbyIn MIPI_OK => MIPI_STANDBY\n");
    }
	
	return merr;
}

/* @brief       R_MIPI_Open DONE*/
mipi_error_t R_MIPI_Open(void){
    mipi_error_t merr = MIPI_OK;
	int cnt;
#ifdef DEBUG
	printf("R_MIPI_Open \n");
#endif
	/* Check MIPI State */
    if( Mipi_State != MIPI_POWOFF ){
        merr = MIPI_STATUS_ERR;
		printf("R_MIPI_Open MIPI_STATUS_ERR\n");
    }

	/* Lane Num == 1 or 2 ? */

	/* Virtual Channel == 0 to 3 ? */

	/* Interlace or Progressive ? */

	/* Mipi Lane Swap Setting ? */
	
	/* Mipi Data Send Speed ? */
	if( merr == MIPI_OK ){
		printf("R_MIPI_Open Setup MIPI registers\n");
		/* MIPI SW-Reset */
		mipi_write32(TREF_REG, 0x00000001);
		mipi_write32(SRST_REG, 0x00000001);
		for( cnt = (528000/2)*5 ; cnt > 0 ; cnt-- ){}  //5us wait
		mipi_write32(SRST_REG, 0x00000000);
	
		/*PHY Timing Register 1, 2, 3*/
		mipi_write32(PHYTIM1_REG, 0x0000338F);
		mipi_write32(PHYTIM2_REG, 0x00030F0A);
		mipi_write32(PHYTIM3_REG, 0x00000E09);
		/*Field Detection Control Register FLD*/
		mipi_write32(FLD_REG, 0x00000000); /*no need?*/
		/*Checksum Control Register (CHKSUM)*/ 
		mipi_write32(CHKSUM_REG, 0x00000003);/*no need?*/
		/*Channel Data Type Select Register (VCDT)*/
		mipi_write32(VCDT_REG, 0x011E802A);	
		/*Frame Data Type Select Register (FRDT)*/
		mipi_write32(FRDT_REG, 0x00010000); /*no need?*/
		/*LINK Operation Control Register (LINKCNT)*/
		mipi_write32(LINKCNT_REG, 0x82000000);
		/*PHY Operation Control Register (PHYCNT)*/
		mipi_write32(PHYCNT_REG, 0x00030013);
		
		for( cnt = (528000/2)*25 ; cnt > 0 ; cnt-- ){}  //25us wait
		
		mipi_write32(INTSTATE_REG, 0xC0);//From Linux
		/* MIPI State Update */
        Mipi_State = MIPI_STOP;
	}
	
	return merr;
}

/* @brief       R_MIPI_Close DONE*/
mipi_error_t R_MIPI_Close( void ){
	mipi_error_t merr = MIPI_OK;
#ifdef DEBUG
	printf("R_MIPI_Close \n");
#endif
	/* Check MIPI State */
    if( ( Mipi_State != MIPI_STOP ) && ( Mipi_State != MIPI_CAPTURE ) ){
        merr = MIPI_STATUS_ERR;
		printf("R_MIPI_Close MIPI_STATUS_ERR\n");
    }

    if( merr == MIPI_OK ){
		printf("R_MIPI_Close Setup MIPI registers 1\n");
		/* Interrupt Disable and Mask */
		mipi_write32(INTCLOSE_REG, 0x181FFCDD);
		vin_write32(VNIE_REG, 0x00000000);
		/* Capture Stop */
		vin_write32(VNFC_REG, 0);// Capture mode off
		vin_write32(VNMC_REG, vin_read32(VNMC_REG) & ~VNMC_ME); // VIN Stop
		if((vin_read32(VNMS_REG) & VNMS_CA) != 0)
			merr = MIPI_FATAL_ERR;
	}

	if( merr == MIPI_OK ){
		printf("R_MIPI_Close Setup MIPI registers 2\n");
		/* MIPI Reset */
		mipi_write32(PHYCNT_REG, 0x00000000);
		mipi_write32(SRST_REG, 0x00000001);
		mipi_write32(SRST_REG, 0x00000000);
	}
	return merr;
}

/* @brief       R_MIPI_Setup DONE*/
mipi_error_t R_MIPI_Setup(void)
{
	mipi_error_t merr = MIPI_OK;
#ifdef DEBUG
	printf("R_MIPI_Setup debug\n");
#endif
	uint32_t vnmc, value;
	
	/* Check MIPI State */
	if( Mipi_State != MIPI_STOP ){
        merr = MIPI_STATUS_ERR;
		printf("R_MIPI_Setup MIPI_STATUS_ERR\n");
    }
	/*Many RANGE CHECK ???*/
    if( merr == MIPI_OK ){	
		/* VIN Initial Setting */
		printf("R_MIPI_Setup Setup VIN registers\n");
		/*Video n Main Control Register (VnMC)*/
		//vnmc = vin_read32(VNMC_REG);
		//vin_write32(VNMC_REG, vnmc | VNMC_CLP_NO_CLIP );
		//vin_write32((VNMC_REG, vnmc | VNMC_INF_RAW8);
		vin_write32(VNMC_REG, 0x3004000B);
		/*Video n CSI2 Interface Mode Register (VnCSI_IFMD)*/
		vin_write32(VNCSI_IFMD_REG, 0x02000000);/*no need ?*/
		/*Video n Data Mode Register 2 (VnDMR2)*/
		vin_write32(VNDMR2_REG, 0x08021000);/*60020000*/
		/*Video n Image Stride Register (VnIS)*/
		vin_write32(VNIS_REG, 0x00000190);/*1A0*/
		/*Video n Start Line Pre-Clip Register (VnSLPrC)*/
		vin_write32(VNSLPRC_REG, 0x00000018);
		/*Video n End Line Pre-Clip Register (VnELPrC)*/
		vin_write32(VNELPRC_REG, 0x000001F7);
		/*Video n Start Pixel Pre-Clip Register (VnSPPrC)*/
		vin_write32(VNSPPRC_REG, 0x00000064);
		/*Video n End Pixel Pre-Clip Register (VnEPPrC)*/
		vin_write32(VNEPPRC_REG, 0x00000383);
		/*Video n Scaling Control Registers (VnUDS_CTRL)*/
		vin_write32(VNUDS_CTRL_REG, 0x40000000);
		/*Video n Scaling Factor Registers (VnUDS_SCALE)*/
		vin_write32(VNUDS_SCALE_REG, 0x00000000);
		/*Video n Passband Registers (VnUDS_PASS_BWIDTH)*/
		vin_write32(VNUDS_PASS_BWIDTH_REG, 0x00400040);
		/*Video n UDS Output Size Clipping Registers (VnUDS_CLIP_SIZE)*/
		vin_write32(VNUDS_CLIP_SIZE_REG, 0x00000000);
		/*Video n Data Mode Register (VnDMR)*/
		vin_write32(VNDMR_REG, 0x00000000);
		/*Video n UV Address Offset Register (VnUVAOF)*/
		vin_write32(VNUVAOF_REG, 0x00000000);
		
		/* MIPI CAPTURE MODE*/
		Mipi_CaptureMode = CONTINUOUS_MODE;
#if 0	
		/* Ack interrupts */ /*From Linux*/ //10030000
		//printf("R_MIPI_Setup, before write VNINTS_REG %x \n",vin_read32(VNINTS_REG));
		vin_write32(VNINTS_REG, 0x10);
		printf("R_MIPI_Setup, After write VNINTS_REG %x \n",vin_read32(VNINTS_REG));
		/* Enable interrupts */ /*From Linux*/
		printf("R_MIPI_Setup, before write VNIE_REG %x \n",vin_read32(VNIE_REG));
		vin_write32(VNIE_REG, 0x10);
		printf("R_MIPI_Setup, after write VNIE_REG %x \n",vin_read32(VNIE_REG));
		/* Enable module */ //From Linux
		printf("R_MIPI_Setup, before write VNMC_REG %x \n",vin_read32(VNMC_REG));
		//vin_write32(VNMC_REG, 0x4000b);
		//printf("R_MIPI_Setup, after write VNMC_REG %x \n",vin_read32(VNMC_REG));
#endif
	}

	return merr;
}

/* @brief       R_MIPI_SetMode DONE*/
mipi_error_t R_MIPI_SetMode( uint8_t CaptureMode ){
    mipi_error_t merr = MIPI_OK;
#ifdef DEBUG
	printf("R_MIPI_SetMode ...\n");
#endif
	/* Check MIPI State */
	if( Mipi_State != MIPI_STOP ){
		merr = MIPI_STATUS_ERR;
        printf("R_MIPI_SetMode Wrong state\n");
    }
	if (CaptureMode > 1){
		printf("R_MIPI_SetMode Wrong mode\n");
		merr = MIPI_PARAM_ERR;
	}
	if( merr == MIPI_OK ){
		Mipi_CaptureMode = CaptureMode;
		printf("R_MIPI_SetMode %d\n", CaptureMode);
	}
	
	return merr;
}

/* @brief       R_MIPI_SetBufferAdr DONE*/
mipi_error_t R_MIPI_SetBufferAdr(uint8_t buffer_no, /*uint8_t*  bufferBase*/ uint32_t phys_addr)
{
	mipi_error_t merr = MIPI_OK;
#ifdef DEBUG
	printf("R_MIPI_SetBufferAdr \n");
#endif	
#ifdef INTS
	uint32_t vin_intf = vin_read32(VNIE_REG);
	printf("R_MIPI_SetBufferAdr 1 VNIE_REG %x\n", vin_intf);
	uint32_t mipi_intf = mipi_read32(INTCLOSE_REG);
	vin_write32(VNIE_REG, 0);
	mipi_write32(INTCLOSE_REG, 0x181FFCDD);
#endif
	/*Video n Interrupt Enable Register (VnIE)*/
	
	/* Check MIPI State */
    if( ( Mipi_State != MIPI_STOP ) && ( Mipi_State != MIPI_CAPTURE ) ){
        merr = MIPI_STATUS_ERR;
		printf("R_MIPI_SetBufferAdr MIPI_STATUS_ERR\n");
    }	
	
	if( merr == MIPI_OK ){
		/*Video n Memory Base 1,2,3 Register (VnMB1, VnMB2, VnMB3)*/
		if( buffer_no > 2 ){
			merr = MIPI_PARAM_ERR;
			printf("R_MIPI_SetBufferAdr error, not support buffer_no\n");
		} else if( ( buffer_no == 0 ) && (phys_addr == NULL) ){
			merr = MIPI_PARAM_ERR;
			printf("R_MIPI_SetBufferAdr error, phys_addr == NULL\n");
		} else if( ( ((uint32_t)phys_addr) & 0x7FUL ) != 0 ){
			 merr = MIPI_PARAM_ERR;
			 printf("R_MIPI_SetBufferAdr error, wrong phys_addr 0x7FUL\n");
		}
	}
	
    if( merr == MIPI_OK ){
		if( buffer_no == 0 ){
				vin_write32(VNMB_REG(0), phys_addr);
				printf("R_MIPI_SetBufferAdr 0 to %x\n", vin_read32(VNMB_REG(0)));
		} else if( buffer_no == 1 ){
				vin_write32(VNMB_REG(1), phys_addr);
				printf("R_MIPI_SetBufferAdr 1 to %x\n", vin_read32(VNMB_REG(1)));
		} else {
				vin_write32(VNMB_REG(2), phys_addr);
				printf("R_MIPI_SetBufferAdr 2 to %x\n", vin_read32(VNMB_REG(2)));
		}	
	}
	/*Restore Interrupt*/
#ifdef INTS
	vin_write32(VNIE_REG, vin_intf);
	printf("R_MIPI_SetBufferAdr 2 VNIE_REG %x\n", vin_intf);
	mipi_write32(INTCLOSE_REG, mipi_intf);
#endif
	return merr;
}

/* @brief       R_MIPI_CaptureStart DONE*/
mipi_error_t R_MIPI_CaptureStart(void){
	mipi_error_t merr = MIPI_OK;
#ifdef DEBUG
	printf("R_MIPI_CaptureStart \n");
#endif		
#ifdef INTS
	uint32_t intstate;
	uint32_t vin_intf = vin_read32(VNIE_REG);
	printf("R_MIPI_CaptureStart 1 VNIE_REG %x\n", vin_intf);
	uint32_t mipi_intf = mipi_read32(INTCLOSE_REG);
	vin_write32(VNIE_REG, 0);
	mipi_write32(INTCLOSE_REG, 0x181FFCDD);
	printf("Defined INTS VNIE_REG %x\n", vin_read32(VNIE_REG));
#endif	
	/* Check MIPI State */
    if( ( Mipi_State != MIPI_STOP ) && ( Mipi_State != MIPI_CAPTURE ) ){
        merr = MIPI_STATUS_ERR;
		printf("R_MIPI_CaptureStart MIPI_STATUS_ERR\n");
    }
    if( merr == MIPI_OK ){
		printf("R_MIPI_CaptureStart before setting VNMC_ME bit %x\n", vin_read32(VNMC_REG));
		vin_write32(VNMC_REG, vin_read32(VNMC_REG) | (1 << 0));
		printf("R_MIPI_CaptureStart after setting VNMC_ME bit %x\n", vin_read32(VNMC_REG));
		if( Mipi_CaptureMode == SINGLE_MODE ){ //Single Capture Mode 
			vin_write32(VNFC_REG, 0x00000002);
			printf("R_MIPI_CaptureStart SINGLE_MODE %x\n",vin_read32(VNFC_REG));
		}
		else{ //Continuous mode
			vin_write32(VNFC_REG, 0x00000001);
			printf("R_MIPI_CaptureStart Continuous mode %x\n",vin_read32(VNFC_REG));		
		}

		/*Video n Interrupt Enable Register (VnIE)*/
		//vin_write32(VNMC_REG | (1 << 0), 1);//From Linux?
		vin_write32(VNIE_REG, 0x10); //From Linux?
		printf("R_MIPI_CaptureStart from Linux setting VNIE %x\n", vin_read32(VNIE_REG));

		vin_write32(VNINTS_REG, vin_read32(VNINTS_REG));//From Linux?
		printf("R_MIPI_CaptureStart from Linux setting VNINTS %x\n", vin_read32(VNINTS_REG));

#ifdef INTS		
		intstate = mipi_read32(INTSTATE_REG);
		mipi_write32(INTSTATE_REG, intstate);	
		intstate = vin_read32(VNINTS_REG);
		printf("R_MIPI_CaptureStart 2 VNINTS_REG %x\n",intstate);		
		vin_write32(VNINTS_REG, intstate);
#endif	
		Mipi_State = MIPI_CAPTURE;
	}
	/*Restore interrupt*/
#ifdef INTS
printf("R_MIPI_CaptureStart before setting VNIE %x\n", vin_read32(VNIE_REG));
	vin_write32(VNIE_REG, vin_intf);
printf("R_MIPI_CaptureStart after setting VNIE %x\n", vin_read32(VNIE_REG));
	mipi_write32(INTCLOSE_REG, mipi_intf);
#endif
	return merr;
}

/* @brief       R_MIPI_CaptureStop DONE*/
mipi_error_t  R_MIPI_CaptureStop(void){
	mipi_error_t merr = MIPI_OK;
#ifdef DEBUG
	printf("R_MIPI_CaptureStop \n");
#endif	
#ifdef INTS
	uint32_t vin_intf = vin_read32(VNIE_REG);
	uint32_t mipi_intf = mipi_read32(INTCLOSE_REG);
	vin_write32(VNIE_REG, 0);
	mipi_write32(INTCLOSE_REG, 0x181FFCDD);
#endif	
	/* Check MIPI State */
    if( Mipi_State != MIPI_CAPTURE ){
        merr = MIPI_STATUS_ERR;
		printf("R_MIPI_CaptureStop MIPI_STATUS_ERR\n");
    }
	
	/*Video n Interrupt Enable Register (VnIE)*/
	//vin_write32(VNIE_REG, 0x0);//From Linux?

	/*Video n Main Control Register (VnMC)*/
	if( merr == MIPI_OK ){
        vin_write32(VNMC_REG, vin_read32(VNMC_REG) & ~VNMC_IM_MASK);    //Capture Disable
        Mipi_State = MIPI_STOP;
			printf("R_MIPI_CaptureStop setup VnMC %x\n", vin_read32(VNMC_REG));
    }
	
	/*Restore interrupt*/	
#ifdef INTS
	vin_write32(VNIE_REG, vin_intf);
	mipi_write32(INTCLOSE_REG, mipi_intf);
#endif
	return merr;
}

/*void Mipi_interrupt_handler( uint32_t int_sense )*/
/*void Vin_interrupt_handler( uint32_t int_sense )*/
