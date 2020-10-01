/*
 * Copyright (c) 2020, Renesas Electronics Corporation
 * All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 * RZ/G2M: UART driver
 */

#ifndef _UARTDRV_H_
#define _UARTDRV_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include "soc_common.h"

/**
 * \defgroup  R_UARTgroup   Renesas UART driver
 * @{
 */

typedef enum R_UART_Clks {
	R_UART_FCK,
	R_UART_SCK,
	R_UART_BRG_INT,
	R_UART_SCIF_CLK,
} R_UART_Clks_t;

typedef enum R_UART_DataSize {
	R_UART_DATA_8BIT,
	R_UART_DATA_7BIT,
} R_UART_DataSize_t;

typedef enum R_UART_Parity {
	R_UART_NONE_PARITY,
	R_UART_EVEN_PARITY,
	R_UART_ODD_PARITY,
} R_UART_Parity_t;

typedef enum R_UART_StopBit {
	R_UART_STOPBIT_1,
	R_UART_STOPBIT_2,
} R_UART_StopBit_t;

typedef enum R_UART_Loop {
	R_UART_LOOPBACK_DISABLE,
	R_UART_LOOPBACK_ENABLE,
} R_UART_Loop_t;

typedef enum R_UART_Mce {
	R_UART_MODEM_CONTROL_DISABLE,
	R_UART_MODEM_CONTROL_ENABLE,
} R_UART_Mce_t;

typedef enum R_UART_Rstrg {
	R_UART_RTS_ACTIVE_TRIGGER_15,
	R_UART_RTS_ACTIVE_TRIGGER_1,
	R_UART_RTS_ACTIVE_TRIGGER_4,
	R_UART_RTS_ACTIVE_TRIGGER_6,
	R_UART_RTS_ACTIVE_TRIGGER_8,
	R_UART_RTS_ACTIVE_TRIGGER_10,
	R_UART_RTS_ACTIVE_TRIGGER_12,
	R_UART_RTS_ACTIVE_TRIGGER_14,
} R_UART_Rstrg_t;

typedef enum R_UART_Rx_Trigger {
	R_UART_RX_FIFO_TRIGGER_8,
	R_UART_RX_FIFO_TRIGGER_4,
	R_UART_RX_FIFO_TRIGGER_2,
	R_UART_RX_FIFO_TRIGGER_0,
} R_UART_Rx_Trigger_t;

typedef enum R_UART_Tx_Trigger {
	R_UART_TX_FIFO_TRIGGER_8,
	R_UART_TX_FIFO_TRIGGER_4,
	R_UART_TX_FIFO_TRIGGER_2,
	R_UART_TX_FIFO_TRIGGER_0,
} R_UART_Tx_Trigger_t;

typedef enum R_UART_Error {
	R_UART_SUCCESS				= 0,
	R_UART_ERROR				= -1,
	R_UART_ERROR_CHANNEL		= -2,
	R_UART_ERROR_NULL_PTR		= -3,
	R_UART_ERROR_ALREADY_OPENED	= -4,
	R_UART_ERROR_CH_NOT_OPENED	= -5,
	R_UART_ERROR_BAUDRATE		= -6,
	R_UART_ERROR_CLKS			= -7,
	R_UART_ERROR_DATA_SIZE		= -8,
	R_UART_ERROR_PARITY			= -9,
	R_UART_ERROR_STOPBIT		= -10,
	R_UART_ERROR_MCE			= -11,
	R_UART_ERROR_LOOP			= -12,
	R_UART_ERROR_RSTRG			= -13,
	R_UART_ERROR_TX_TRG			= -14,
	R_UART_ERROR_RX_TRG			= -15,
} R_UART_Error_t;

typedef struct R_UART_Config {
	uint32_t				baud_rate;
	R_UART_Clks_t			clock_source;
	R_UART_DataSize_t		data_size;
	R_UART_Parity_t			parity_type;
	R_UART_StopBit_t		stop_bits;
	R_UART_Loop_t			loop_test;		/* Not support now, use default value R_UART_LOOPBACK_DISABLE*/
	R_UART_Mce_t			modem_control;	/* Not support now, use default value R_UART_MODEM_CONTROL_DISABLE*/
	R_UART_Rstrg_t			rts_trigger;	/* Not support now, use default value R_UART_RTS_ACTIVE_TRIGGER_15*/
	R_UART_Rx_Trigger_t		rx_trigger;
	R_UART_Tx_Trigger_t		tx_trigger;
} R_UART_Config_t;

extern R_UART_Config_t g_uart_config;

int R_UART_Send(uint8_t channel, void *const p_data, uint32_t const length);
int R_UART_Receive(uint8_t channel, void *p_data, uint32_t length, uint32_t *received);
int R_UART_Init(uint8_t channel, R_UART_Config_t const *p_cfg);
int R_UART_Release(uint8_t channel);

#ifdef __cplusplus
}
#endif

#endif	/* _UARTDRV_H_ */
