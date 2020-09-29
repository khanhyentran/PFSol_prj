/*
 * Copyright (c) 2020, Renesas Electronics Corporation
 * All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 * RZ/G2M: UART driver
 */
 
#include <errno.h>
#include <stdint.h>
#include <string.h>

#include "FreeRTOS.h"
#include "semphr.h"

#include "soc_common.h"
#include "uartdrv.h"
#include "interrupts.h"
#include "lifec.h"

#define SCSMR		0x0000	/* Serial Mode */
#define SCBRR		0x0004	/* Bit Rate */
#define SCSCR		0x0008	/* Serial Control */
#define SCFTDR		0x000C	/* Transmit FIFO Data */
#define SCFSR		0x0010	/* Serial Status */
#define SCFRDR		0x0014	/* Receive FIFO Data */
#define SCFCR		0x0018	/* FIFO Control */
#define SCFDR		0x001C	/* FIFO Data Count */
#define SCSPTR		0x0020	/* Serial Port */
#define SCLSR		0x0024	/* Line Status */
#define DL			0x0030	/* Frequency Division */
#define CKS			0x0034	/* Clock Select */

/* SCSMR (Serial Mode Register) */
#define SCSMR_CHR	BIT(6)	/* 7-bit Character Length */
#define SCSMR_PE	BIT(5)	/* Parity Enable */
#define SCSMR_ODD	BIT(4)	/* Odd Parity */
#define SCSMR_STOP	BIT(3)	/* 2 Stop Bit Length */
#define SCSMR_CKS(x)	(((uint16_t)(x) & 0x3) << 0)	/* Clock Select */
/* SCSMR_CKS (Clock Select 1 and 0) */
#define CKS_PCK_1	0x0000	/* B’00: PCK */
#define CKS_PCK_4	0x0001	/* B’01: PCK/4 */
#define CKS_PCK_16	0x0002	/* B’10: PCK/16 */
#define CKS_PCK_64	0x0003	/* B’11: PCK/64 */

/* SCSCR (Serial Control Register) */
#define SCSCR_TEIE	BIT(11)	/* Transmit End Interrupt Enable */
#define SCSCR_TIE	BIT(7)	/* Transmit Interrupt Enable */
#define SCSCR_RIE	BIT(6)	/* Receive Interrupt Enable */
#define SCSCR_TE	BIT(5)	/* Transmit Enable */
#define SCSCR_RE	BIT(4)	/* Receive Enable */
#define SCSCR_REIE	BIT(3)	/* Receive Error Interrupt Enable */
#define SCSCR_TOIE	BIT(2)	/* Timeout Interrupt Enable */
#define SCSCR_CKE(x)	(((uint16_t)(x) & 0x3) << 0)	/* Clock Enable */
/* SCSCR_CKE (Clock Enable 1 and 0) */
#define CKE_ASYNC_1	0x0000	/* The SCK pin is not used */
#define CKE_ASYNC_2	0x0001	/* The SCK pin outputs the clock (with a frequency 16 times the bit rate) */
#define CKE_ASYNC_3	0x0002	/* Baud rate generator output for external clock or SCK */

/* SCFSR (Serial Status Register) */
#define SCFSR_ER	BIT(7)	/* Receive Error */
#define SCFSR_TEND	BIT(6)	/* Transmission End */
#define SCFSR_TDFE	BIT(5)	/* Transmit FIFO Data Empty */
#define SCFSR_BRK	BIT(4)	/* Break Detect */
#define SCFSR_RDF	BIT(1)	/* Receive FIFO Data Full */
#define SCFSR_DR	BIT(0)	/* Receive Data Ready */
#define SCFSR_PER	BIT(2)	/* Parity Error */
#define SCFSR_FER	BIT(3)	/* Framing Error */

/* SCFCR (FIFO Control Register) */
#define SCFCR_RSTRG(x)	(((uint16_t)(x) & 0x7) << 8)	/* RTS Output Active Trigger */
/* SCFCR_RSTRG (RTS Output Active Trigger) */
#define RSTRG_15	0x0000	/* B’000: 15 */
#define RSTRG_1		0x0001	/* B’001: 1 */
#define RSTRG_4		0x0002	/* B’010: 4 */
#define RSTRG_6 	0x0003	/* B’011: 6 */
#define RSTRG_8 	0x0004	/* B’100: 8 */
#define RSTRG_10	0x0005	/* B’101: 10 */
#define RSTRG_12	0x0006	/* B’110: 12 */
#define RSTRG_14	0x0007	/* B’111: 14 */

#define SCFCR_RTRG(x)	(((uint16_t)(x) & 0x3) << 6)	/* Receive FIFO Data Count Trigger */
/* SCFCR_RTRG (Receive FIFO Data Count Trigger) */
#define RTRG_1		0x0000
#define RTRG_4		0x0001
#define RTRG_8		0x0002
#define RTRG_14		0x0003

#define SCFCR_TTRG(x)	(((uint16_t)(x) & 0x3) << 4)	/* Transmit FIFO Data Count Trigger */
/* SCFCR_TTRG (Transmit FIFO Data Count Trigger) */
#define TTRG_8_8	0x0000	/* B’00: 8 (8) */
#define TTRG_4_12	0x0001	/* B’01: 4 (12) */
#define TTRG_2_14	0x0002	/* B’10: 2 (14) */
#define TTRG_0_16	0x0003	/* B’11: 0 (16) */

#define SCFCR_MCE	BIT(3)	/* Modem Control Enable */
#define SCFCR_TFRST	BIT(2)	/* Transmit FIFO Data Register Reset */
#define SCFCR_RFRST	BIT(1)	/* Receive FIFO Data Register Reset */
#define SCFCR_LOOP	BIT(0)	/* Loopback Test */

/* SCSPTR (Serial Port Register) */
#define SCSPTR_RTSIO	BIT(7)	/* Serial Port RTS# Pin Input/Output */
#define SCSPTR_RTSDT	BIT(6)	/* Serial Port RTS# Pin Data */
#define SCSPTR_CTSIO	BIT(5)	/* Serial Port CTS# Pin Input/Output */
#define SCSPTR_CTSDT	BIT(4)	/* Serial Port CTS# Pin Data */
#define SCSPTR_SCKIO	BIT(3)	/* Serial Port Clock Pin Input/Output */
#define SCSPTR_SCKDT	BIT(2)	/* Serial Port Clock Pin Data */
#define SCSPTR_SPB2IO	BIT(1)	/* Serial Port Break Input/Output */
#define SCSPTR_SPB2DT	BIT(0)	/* Serial Port Break Data */

/* SCLSR (Line Status Register) */
#define SCLSR_TO	BIT(2)	/* Timeout */
#define SCLSR_ORER	BIT(0)	/* Overrun Error */

/* CKS (Clock Select Register */
#define CKS_CKS	BIT(15) /* Switch between SC_CLK and SCK */
#define CKS_XIN	BIT(14) /* Select SCIF_CLK or SCKi */

#define SCFDR_T_MASK		(0x1f << 8)
#define SCFDR_R_MASK		(0x1f << 0)

/* Absolute value */
#define abs(x) (x < 0 ? (-x) : x)

#define UART_FIFO_DEPTH	16
#define MAX_WAIT_MS		10
#define QUEUE_LENGTH	5000

typedef enum UART_Channel_Status {
    UART_CHANNEL_NOT_OPENED,
    UART_CHANNEL_OPENED,
} UART_Channel_Status_t; 

typedef struct UART_Data {
	uint32_t	base;
	uint16_t	mstpb;
	uint16_t	lifec;
	uint16_t	irq_id;
	UART_Channel_Status_t	channel_status;
	R_UART_Config_t *p_cfg;
	QueueHandle_t uart_queue_rx;
	QueueHandle_t uart_queue_tx;
} UART_Data_t;

R_UART_Config_t g_uart_config = {
	115200,
	R_UART_FCK,
	R_UART_DATA_8BIT,
	R_UART_EVEN_PARITY,
	R_UART_STOPBIT_1,
	R_UART_LOOPBACK_DISABLE,
	R_UART_MODEM_CONTROL_DISABLE,
	R_UART_RTS_ACTIVE_TRIGGER_15,
	R_UART_RX_FIFO_TRIGGER_8,
	R_UART_TX_FIFO_TRIGGER_8,
};

static uint8_t uart_rx_error = 0;

static UART_Data_t UART_ChannelData[] = {
	[0] = {
		.base = 0xE6E60000U,
		.mstpb = MSTPB(2, 07),
		.lifec = LIFEC_SLAVE_SCIF0,
		.irq_id = 184,
		.channel_status = UART_CHANNEL_NOT_OPENED
	},
	[1] = {
		.base = 0xE6E68000U,
		.mstpb = MSTPB(2, 06),
		.lifec = LIFEC_SLAVE_SCIF1,
		.irq_id = 185,
		.channel_status = UART_CHANNEL_NOT_OPENED
	},
	[2] = {
		.base = 0xE6E88000U,
		.mstpb = MSTPB(3, 10),
		.irq_id = 196,
		.channel_status = UART_CHANNEL_NOT_OPENED
	},
	[3] = {
		.base = 0xE6C50000U,
		.mstpb = MSTPB(2, 04),
		.lifec = LIFEC_SLAVE_SCIF3,
		.irq_id = 55,
		.channel_status = UART_CHANNEL_NOT_OPENED
	},
	[4] = {
		.base = 0xE6C40000U,
		.mstpb = MSTPB(2, 03),
		.lifec = LIFEC_SLAVE_SCIF4,
		.irq_id = 48,
		.channel_status = UART_CHANNEL_NOT_OPENED
	},
	[5] = {
		.base = 0xE6F30000U,
		.mstpb = MSTPB(2, 02),
		.lifec = LIFEC_SLAVE_SCIF5,
		.irq_id = 49,
		.channel_status = UART_CHANNEL_NOT_OPENED
	},
};

static void UART_IrqHandler(void *arg)
{
	uint16_t serial_status;
	uint16_t line_status;
	uint16_t control;
	uint16_t sstatus_err, lstatus_err;
	portBASE_TYPE xHigherPriorityTaskWoken = pdFALSE;
	UART_Data_t *pUart = (UART_Data_t *)arg;

	/* Disable interrupts */
	control = readw(pUart->base + SCSCR);
	writew(control & ~(SCSCR_RIE | SCSCR_TIE), pUart->base + SCSCR);

	serial_status = readw(pUart->base + SCFSR);

	/* Tx FIFO data empty? */
	if (serial_status & SCFSR_TDFE) {
		uint8_t byte;
		int i = UART_FIFO_DEPTH;

		while (--i && xQueueReceiveFromISR(pUart->uart_queue_tx, &byte, &xHigherPriorityTaskWoken) == pdTRUE) {
			/* Send the data retrieved from the queue to the FIFO */
			writeb(byte, pUart->base + SCFTDR);
		}

		/* Disable Transmission interrupt on Tx FIFO data empty */
		if (xQueueIsQueueEmptyFromISR(pUart->uart_queue_tx))
			control &= ~SCSCR_TIE;

		/* Clear Transmit FIFO Data Empty bit */
		serial_status &= ~SCFSR_TDFE;
	}

	/* Are any receive events of interest active? */
	if ((control & ~SCSCR_RIE) != 0) {
		/* While there is data in the FIFO, read it and put it in the queue */
		while (readw(pUart->base + SCFDR) & SCFDR_R_MASK) {

			sstatus_err = readw(pUart->base + SCFSR);
			lstatus_err = readw(pUart->base + SCLSR);

			if ((lstatus_err & (SCLSR_ORER | SCLSR_TO)) ||
				(sstatus_err & (SCFSR_ER | SCFSR_FER))) {
					uart_rx_error = 1;
					break;
			}

			uint8_t byte = readb(pUart->base + SCFRDR);

			xQueueSendFromISR(pUart->uart_queue_rx, &byte, &xHigherPriorityTaskWoken);
		}

		/* Clear the Rx interrupt */
		serial_status &= ~SCFSR_RDF;
	}

	/* Clear the flag */
	writew(serial_status, pUart->base + SCFSR);
	writew(0, pUart->base + SCLSR);

	/* Re-enable interrupts */
	writew(control, pUart->base + SCSCR);

	portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
}

static portBASE_TYPE UART_PutByte(UART_Data_t *pUart, int8_t byte)
{
	if (xQueueSend(pUart->uart_queue_tx, &byte, MAX_WAIT_MS) == pdPASS) {
		/* Enable Transmission interrupt on Tx FIFO data empty */
		taskENTER_CRITICAL();
		uint16_t control = readw(pUart->base + SCSCR);
		writew((control | SCSCR_TIE), pUart->base + SCSCR);
		taskEXIT_CRITICAL();

		return pdTRUE;
	}

	return pdFALSE;
}

int R_UART_Send(uint8_t channel, void *const p_data, uint32_t const length)
{
	UART_Data_t *pUart;
	uint32_t len = length;
	int8_t *pData = (int8_t *)p_data;
	uint16_t control;
	int i = 0;

	if (channel > 5)
		return R_UART_ERROR_CHANNEL;

	pUart = &UART_ChannelData[channel];

	if (pUart->channel_status == UART_CHANNEL_NOT_OPENED)
		return R_UART_ERROR_CH_NOT_OPENED;

	/* Send each byte in the data, one at a time. */
	while (--len && UART_PutByte(pUart, *(pData++)) == pdTRUE)
		;

	/* Disable Tx interrupt */
	control = readw(pUart->base + SCSCR);
	writew((control & ~SCSCR_TIE), pUart->base + SCSCR);

	if (len > 0)
		return R_UART_ERROR;

	return R_UART_SUCCESS;
}

static portBASE_TYPE UART_GetByte(UART_Data_t *pUart, int8_t *byte)
{
	/* Get the next byte from the buffer. Return false if no bytes
	are available, or arrive before MAX_WAIT_MS expires. */
	if (xQueueReceive(pUart->uart_queue_rx, byte, MAX_WAIT_MS))
		return pdTRUE;

	return pdFALSE;
}

int R_UART_Receive(uint8_t channel, void *p_data, uint32_t length, uint32_t *received)
{
	UART_Data_t *pUart;
	uint32_t rxEd;	/* Data received */
	uint16_t control;
	int8_t *pData = (int8_t *)p_data;

	if (channel > 5)
		return R_UART_ERROR_CHANNEL;

	pUart = &UART_ChannelData[channel];

	if (pUart->channel_status == UART_CHANNEL_NOT_OPENED)
		return R_UART_ERROR_CH_NOT_OPENED;

	/* Enable Rx interrupt */
	control = readw(pUart->base + SCSCR);
	writew((control | SCSCR_RIE), pUart->base + SCSCR);

	rxEd = 0;
	while (UART_GetByte(pUart, pData++) == pdTRUE)
		rxEd++;

	if (received != NULL)
		*received = rxEd;

	/* Disable Rx interrupt */
	control = readw(pUart->base + SCSCR);
	writew((control & ~SCSCR_RIE), pUart->base + SCSCR);

	/* Have any error from Rx */
	if (uart_rx_error) {
		uart_rx_error = 0;
		return R_UART_ERROR;
	}

	return R_UART_SUCCESS;
}

static int UART_CheckParameters(uint8_t channel, R_UART_Config_t const *p_cfg)
{
	if (channel > 5)
		return R_UART_ERROR_CHANNEL;

	if (UART_ChannelData[channel].channel_status == UART_CHANNEL_OPENED) 
		return R_UART_ERROR_ALREADY_OPENED;

	if (p_cfg == NULL)
		return R_UART_ERROR_NULL_PTR;

	if ((p_cfg->clock_source < R_UART_FCK) || (p_cfg->clock_source > R_UART_SCIF_CLK)) 
		return R_UART_ERROR_CLKS;

	if ((p_cfg->data_size != R_UART_DATA_8BIT) && 
		(p_cfg->data_size != R_UART_DATA_7BIT))
		return R_UART_ERROR_DATA_SIZE;

	if ((p_cfg->parity_type < R_UART_NONE_PARITY) || 
		(p_cfg->parity_type > R_UART_ODD_PARITY))
		return R_UART_ERROR_PARITY;

	if ((p_cfg->stop_bits != R_UART_STOPBIT_1) && 
		(p_cfg->stop_bits != R_UART_STOPBIT_2))
		return R_UART_ERROR_STOPBIT;

	if ((p_cfg->loop_test != R_UART_LOOPBACK_DISABLE) && 
		(p_cfg->loop_test != R_UART_LOOPBACK_ENABLE))
		return R_UART_ERROR_LOOP;

	if ((p_cfg->modem_control != R_UART_MODEM_CONTROL_DISABLE) && 
		(p_cfg->modem_control != R_UART_MODEM_CONTROL_ENABLE))
		return R_UART_ERROR_MCE;

	if ((p_cfg->rts_trigger < R_UART_RTS_ACTIVE_TRIGGER_15) || 
		(p_cfg->rts_trigger > R_UART_RTS_ACTIVE_TRIGGER_14))
		return R_UART_ERROR_RSTRG;

	if ((p_cfg->tx_trigger < R_UART_TX_FIFO_TRIGGER_8) || 
		(p_cfg->tx_trigger > R_UART_TX_FIFO_TRIGGER_0))
		return R_UART_ERROR_TX_TRG;

	if ((p_cfg->rx_trigger < R_UART_RX_FIFO_TRIGGER_8) || 
		(p_cfg->rx_trigger > R_UART_RX_FIFO_TRIGGER_0))
		return R_UART_ERROR_RX_TRG;

	return 0;
}

static int UART_Baudrate(R_UART_Config_t const *p_cfg, uint8_t *scscr_cke, 
					uint8_t *scsmr_cks, uint16_t *cks, uint16_t *val)
{
	uint8_t clk = p_cfg->clock_source;
	uint32_t baud_rate = p_cfg->baud_rate;
	uint32_t FCK = 66666666, SCKi = 266666666, SCIF_CLK = 14745600;
	uint16_t temp;
	uint16_t err, min_err = 0xFFFF;
	uint16_t div;
	uint8_t n;

	if (baud_rate > 115200)
		return -1;

	if (clk == R_UART_FCK) {
		*scscr_cke = 0; /* SCK pin input (not used)*/
		for (n = 0; n <= 3; n++) {
			if (n == 0) {
				div = 64  *0.5;
			} else {
				div = 64 * (1 << (2 * n - 1));
			}
			temp = (float)FCK/(div * baud_rate) + 0.5f;

			if (temp > 256) {
				if (n != 3)
					continue;
				return -1;
			}

			if (temp < 1) {
				if (n != 0)
					continue; 
				return -1;
			}

			err = (float)FCK/(temp * div) + 0.5f;
			err = abs(err - baud_rate);

			if (err >= min_err)
				continue;

			min_err = err;
			*val = temp - 1;
			*scsmr_cks = n;
		}
	}

	if (clk == R_UART_SCK) {
		*scscr_cke = 2;
		*cks = CKS_CKS;
	}

	if (clk == R_UART_BRG_INT) {
		*scscr_cke = 2;
		*cks = CKS_XIN;
		*val = (float)SCKi/(baud_rate * 16) + 0.5f;
		if (*val > 65535)
			return -1;
	}

	if (clk == R_UART_SCIF_CLK) {
		*scscr_cke = 2;
		*cks = 0;
		*val = (float)SCIF_CLK/(baud_rate * 16) + 0.5f;
		if (*val > 65535)
			return -1;
	}

	return 0;
}

int R_UART_Init(uint8_t channel, R_UART_Config_t const *p_cfg)
{
	UART_Data_t *pUart;
	uint16_t data;
	uint16_t cks = 0;
	uint16_t val = 0;
	uint8_t scscr_cke = 0; 
	uint8_t scsmr_cks = 0;
	int error;

	/* Check parameters */
	error = UART_CheckParameters(channel, p_cfg);
	if (error) 
		return error;

	/* Check baud rate */
	error = UART_Baudrate(p_cfg, &scscr_cke, &scsmr_cks, &cks, &val);
	if (error)
		return R_UART_ERROR_BAUDRATE;

	/* Update config */
	UART_ChannelData[channel].p_cfg = (R_UART_Config_t *)p_cfg;
	pUart = &UART_ChannelData[channel];

	/* Enable LifeC */
	R_LifeC_obtain_peripheral(pUart->lifec);

	/* Power On */
	modulePowerOnd(pUart->mstpb);

	/* Disable Transmission/Reception */
	data = readw(pUart->base + SCSCR);
	data &= ~(SCSCR_TE | SCSCR_RE); 
	writew(data, pUart->base + SCSCR);

	/* Enable Transmit/Receive FIFO Data Register reset */
	data = readw(pUart->base + SCFCR);
	data |= SCFCR_TFRST | SCFCR_RFRST;
	writew(data, pUart->base + SCFCR);

	/* Clear Error, Receive Data Ready, Break Detect, Receive FIFO Data Full bit */
	data = readw(pUart->base + SCFSR);
	data &= ~(SCFSR_ER | SCFSR_DR | SCFSR_BRK | SCFSR_RDF);
	writew(data, pUart->base + SCFSR);

	/* Clear Timeout, Overrun Error bit */
	data = readw(pUart->base + SCLSR);
	data &= ~(SCLSR_TO | SCLSR_ORER);
	writew(data, pUart->base + SCLSR);

	/* Choose Clock Source and disable interrupts registers */
	data = readw(pUart->base + SCSCR);
	data |= SCSCR_CKE(scscr_cke);
	data &= ~(SCSCR_TIE | SCSCR_RIE | SCSCR_TOIE);
	writew(data, pUart->base + SCSCR);

	/* Set Serial Mode Register */
	data = (pUart->p_cfg->data_size == R_UART_DATA_7BIT ? SCSMR_CHR : 0);
	switch (pUart->p_cfg->parity_type) {
		case R_UART_NONE_PARITY:
			data &= ~SCSMR_PE;
			break;
		case R_UART_ODD_PARITY:
			data |= SCSMR_PE | SCSMR_ODD;
			break;
		case R_UART_EVEN_PARITY:
			data |= SCSMR_PE;
			data &= ~SCSMR_ODD;
			break;
		default:
			;
	}
	data |= (pUart->p_cfg->stop_bits == R_UART_STOPBIT_2 ? SCSMR_STOP : 0);
	data |= SCSMR_CKS(scsmr_cks);
	writew(data, pUart->base + SCSMR);

	/* Set baud rate register */
	if (pUart->p_cfg->clock_source == R_UART_FCK) { /* internal clock */
		writew((uint8_t)val, pUart->base + SCBRR);
	} else { /* external clock */
		writew(cks, pUart->base + CKS);
		writew(val, pUart->base + DL);
	}

	/* Wait 1-bit interval */
	delay_us((1000000 + pUart->p_cfg->baud_rate - 1)/pUart->p_cfg->baud_rate);

	/* Set Serial Status Register */
	data = (pUart->p_cfg->loop_test == R_UART_LOOPBACK_ENABLE ? SCFCR_LOOP : 0);
	data |= (pUart->p_cfg->modem_control == R_UART_MODEM_CONTROL_ENABLE ? SCFCR_MCE : 0);
	data |= SCFCR_RSTRG(pUart->p_cfg->rts_trigger);
	data |= SCFCR_RTRG(pUart->p_cfg->rx_trigger);
	data |= SCFCR_TTRG(pUart->p_cfg->tx_trigger);
	data &= ~(SCFCR_TFRST | SCFCR_RFRST);
	writew(data, pUart->base + SCFCR);

	/* Enable transmit, receive */
	data = readw(pUart->base + SCSCR); 
	data |= SCSCR_TE | SCSCR_RE; 
	writew(data, pUart->base + SCSCR);

	/* Create the queues used to hold Rx/Tx data. */
	pUart->uart_queue_rx = xQueueCreate(QUEUE_LENGTH, sizeof(uint8_t));
	vTraceSetQueueName(pUart->uart_queue_rx, "uartRx");
	pUart->uart_queue_tx = xQueueCreate(QUEUE_LENGTH + 1, sizeof(uint8_t));
	vTraceSetQueueName(pUart->uart_queue_tx, "uartTx");

	/* Set up interrupt handler */
	Irq_SetupEntry(pUart->irq_id, UART_IrqHandler, pUart);
	IRQ_Enable(pUart->irq_id);

	/* Update channel status */
	UART_ChannelData[channel].channel_status = UART_CHANNEL_OPENED;
	
	return R_UART_SUCCESS;
}

int R_UART_Release(uint8_t channel)
{
	UART_Data_t *pUart;

	/* Check channel error */
	if (channel > 5)
		return R_UART_ERROR_CHANNEL;

	if (UART_ChannelData[channel].channel_status == UART_CHANNEL_NOT_OPENED)
		return R_UART_ERROR_CH_NOT_OPENED;

	/* Update channel status */
	UART_ChannelData[channel].channel_status = UART_CHANNEL_NOT_OPENED;
	pUart = &UART_ChannelData[channel];

	/* Disable interrupt */
	IRQ_Disable(pUart->irq_id);

	/* Power Off */
	modulePowerOffd(pUart->mstpb);

	/* Disable LifeC */
	R_LifeC_release_peripheral(pUart->lifec);

	return R_UART_SUCCESS;
}
