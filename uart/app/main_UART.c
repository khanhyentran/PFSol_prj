#include "FreeRTOS.h"
#include "uartdrv.h"
#include "soc_common.h"
#include "task.h"
#include "string.h"

#define DEBUG
#ifdef DEBUG
#define MESSAGE(iVal, pStr)	\
		do {	\
				if (iVal < 0)	\
					printf("\033[0;31m [%s] %s: Error: %d\033[0m \n", __FILE__, (char *)pStr, iVal);	\
				else	\
					printf("\033[0;32m [%s] %s: Success: %d\033[0m \n",__FILE__, (char *)pStr, iVal);	\
		} while (0)
#endif

uint8_t uart_master_channel = 1;
uint8_t uart_slave_channel = 0;

void main_UART_master(void)
{
	/* Function that enables the tracing and creates the control task */
	vTraceEnable(TRC_START);

	/* Setup device PFC (Pin Function Control): check pinctrl.c */
	rcarPFCInit();

	/* Parameters setting for UART module using */
	g_uart_config.tx_trigger = R_UART_TX_FIFO_TRIGGER_0;
	R_UART_Error_t status;
	char string1[] ="Hello World from Master!";
	uint32_t str_size = strlen(string1);

	/* Init the UART module */
	status = R_UART_Init(uart_master_channel, &g_uart_config);
	MESSAGE(status, "R_UART_Init");

	/* Send Data from Master */
	status = R_UART_Send(uart_master_channel, &string1, str_size);
	MESSAGE(status, "R_UART_Send");

	/* Uninit the UART module */
	status = R_UART_Release(uart_master_channel);
	MESSAGE(status, "R_UART_Release");
}

void main_UART_slave(void)
{
	/* Function that enables the tracing and creates the control task */
	vTraceEnable(TRC_START);

	/* Setup device PFC (Pin Function Control): check pinctrl.c */
	rcarPFCInit();

	/* Parameters setting for UART module using */
	R_UART_Error_t status;
	uint32_t length = 24;
	uint32_t received;
	uint8_t *p_data = malloc(length*sizeof(uint8_t));

	/* Init the UART module */
	status = R_UART_Init(uart_slave_channel, &g_uart_config);
	MESSAGE(status, "R_UART_Init");
	
	/* Receive the data from Master */
	R_UART_Receive(uart_slave_channel, p_data, length, &received);
	MESSAGE(status, "R_UART_Receive");
	printf(" The received string from master is : %s\n",p_data);

	/* Uninit the UART module */
	status = R_UART_Release(uart_slave_channel);
	MESSAGE(status, "R_UART_Release");
}

