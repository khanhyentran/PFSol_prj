#include <errno.h>
#include <stdint.h>
#include <string.h>
#include <FreeRTOS.h>

#include "soc_common.h"
#include "task.h"
#include "common/inc/pinctrl.h"
#include "uart/inc/uartdrv.h"

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

int printf_raw(const char *format, ...);

static TaskHandle_t UartTaskHandle = NULL;

static void UART_Task(void *pvParameters)
{
	R_UART_Error_t status;

	const char renesas[] = "\r\n\r\n\x1B[91m    ...',,,,'..\r\n"
								".''',,,,;;coO0Oxc.\r\n"
								"..          :0XKKl  .,ccccccccc:'  'cllc,      .c;   .,:lllccccccc,  .,:clccc:ccc;.      .;cllc'     .,:clcc::ccc:.\r\n"
								"           'oKK0x' 'xKKx;........  c000K0d'    :0d. .dKKxl;'....... .oKKKx;'......      'xK0O0Kk;   .lKKKx;'......\r\n"
								"        .:d0Kkc'.  cKK0o,''''''.   c0o;o0KOl.  :0d. :0KKd;,''''''.   :k0K0xl:,..       ,kKO:.l0KOc   ;k0K0xl:,..\r\n"
								",dxxxo. .oKX0l.    lKKKdc:::c::,   c0l  ;xKKk;.:0d. cKKKxlccc:cc:,    .':ldk0K0kdc.   :OKk,  .c0X0l.  .':ldk0K0kdc'\r\n"
								":0XKXk'  .;x00o'   :0X0c           c0l   .cOK0xxKd. ;0XKl..                ..,o0KKk,.c0Kk,     :OKKd.      ..,o0KKO,\r\n"
								",kOOOd.    .,okOl. .cxOkoccccccc,  :Ol     'oO0KKo. .:xOkxoccccccc;. ,ccccccccdO0kl;l0Kx'  'ccclkKK0xlccccccccdO0kl.\r\n"
								" .....        .;lol,....''''''''.  .'.       ..''.     ..'''''''''.  .'''''''''... .'''.   .'''''''''''''''''''...\r\n"
								"                 .',,..\r\n\r\n"
								"\r\n\r\n\x1B[92m    ...',,,,'..\r\n"
								".''',,,,;;coO0Oxc.\r\n"
								"..          :0XKKl  .,ccccccccc:'  'cllc,      .c;   .,:lllccccccc,  .,:clccc:ccc;.      .;cllc'     .,:clcc::ccc:.\r\n"
								"           'oKK0x' 'xKKx;........  c000K0d'    :0d. .dKKxl;'....... .oKKKx;'......      'xK0O0Kk;   .lKKKx;'......\r\n"
								"        .:d0Kkc'.  cKK0o,''''''.   c0o;o0KOl.  :0d. :0KKd;,''''''.   :k0K0xl:,..       ,kKO:.l0KOc   ;k0K0xl:,..\r\n"
								",dxxxo. .oKX0l.    lKKKdc:::c::,   c0l  ;xKKk;.:0d. cKKKxlccc:cc:,    .':ldk0K0kdc.   :OKk,  .c0X0l.  .':ldk0K0kdc'\r\n"
								":0XKXk'  .;x00o'   :0X0c           c0l   .cOK0xxKd. ;0XKl..                ..,o0KKk,.c0Kk,     :OKKd.      ..,o0KKO,\r\n"
								",kOOOd.    .,okOl. .cxOkoccccccc,  :Ol     'oO0KKo. .:xOkxoccccccc;. ,ccccccccdO0kl;l0Kx'  'ccclkKK0xlccccccccdO0kl.\r\n"
								" .....        .;lol,....''''''''.  .'.       ..''.     ..'''''''''.  .'''''''''... .'''.   .'''''''''''''''''''...\r\n"
								"                 .',,..\r\n\r\n"
								"\r\n\r\n\x1B[93m    ...',,,,'..\r\n"
								".''',,,,;;coO0Oxc.\r\n"
								"..          :0XKKl  .,ccccccccc:'  'cllc,      .c;   .,:lllccccccc,  .,:clccc:ccc;.      .;cllc'     .,:clcc::ccc:.\r\n"
								"           'oKK0x' 'xKKx;........  c000K0d'    :0d. .dKKxl;'....... .oKKKx;'......      'xK0O0Kk;   .lKKKx;'......\r\n"
								"        .:d0Kkc'.  cKK0o,''''''.   c0o;o0KOl.  :0d. :0KKd;,''''''.   :k0K0xl:,..       ,kKO:.l0KOc   ;k0K0xl:,..\r\n"
								",dxxxo. .oKX0l.    lKKKdc:::c::,   c0l  ;xKKk;.:0d. cKKKxlccc:cc:,    .':ldk0K0kdc.   :OKk,  .c0X0l.  .':ldk0K0kdc'\r\n"
								":0XKXk'  .;x00o'   :0X0c           c0l   .cOK0xxKd. ;0XKl..                ..,o0KKk,.c0Kk,     :OKKd.      ..,o0KKO,\r\n"
								",kOOOd.    .,okOl. .cxOkoccccccc,  :Ol     'oO0KKo. .:xOkxoccccccc;. ,ccccccccdO0kl;l0Kx'  'ccclkKK0xlccccccccdO0kl.\r\n"
								" .....        .;lol,....''''''''.  .'.       ..''.     ..'''''''''.  .'''''''''... .'''.   .'''''''''''''''''''...\r\n"
								"                 .',,..\r\n\r\n"
								"\r\n\r\n\x1B[94m    ...',,,,'..\r\n"
								".''',,,,;;coO0Oxc.\r\n"
								"..          :0XKKl  .,ccccccccc:'  'cllc,      .c;   .,:lllccccccc,  .,:clccc:ccc;.      .;cllc'     .,:clcc::ccc:.\r\n"
								"           'oKK0x' 'xKKx;........  c000K0d'    :0d. .dKKxl;'....... .oKKKx;'......      'xK0O0Kk;   .lKKKx;'......\r\n"
								"        .:d0Kkc'.  cKK0o,''''''.   c0o;o0KOl.  :0d. :0KKd;,''''''.   :k0K0xl:,..       ,kKO:.l0KOc   ;k0K0xl:,..\r\n"
								",dxxxo. .oKX0l.    lKKKdc:::c::,   c0l  ;xKKk;.:0d. cKKKxlccc:cc:,    .':ldk0K0kdc.   :OKk,  .c0X0l.  .':ldk0K0kdc'\r\n"
								":0XKXk'  .;x00o'   :0X0c           c0l   .cOK0xxKd. ;0XKl..                ..,o0KKk,.c0Kk,     :OKKd.      ..,o0KKO,\r\n"
								",kOOOd.    .,okOl. .cxOkoccccccc,  :Ol     'oO0KKo. .:xOkxoccccccc;. ,ccccccccdO0kl;l0Kx'  'ccclkKK0xlccccccccdO0kl.\r\n"
								" .....        .;lol,....''''''''.  .'.       ..''.     ..'''''''''.  .'''''''''... .'''.   .'''''''''''''''''''...\r\n"
								"                 .',,..\x1B[37m\r\n\r\n";
	uint32_t renesas_size = strlen(renesas);

	const char hello[] ="Hello World from Master!";
	uint32_t hello_size = strlen(hello);

	/* Send Data from Master */
	status = R_UART_Send(uart_master_channel, &hello, hello_size);
	MESSAGE(status, "R_UART_Send");

	/* Uninit the UART module */
	status = R_UART_Release(uart_master_channel);
	MESSAGE(status, "R_UART_Release");
}

void UartTaskStart(uint16_t usStackSize, UBaseType_t uxPriority)
{
	R_UART_Error_t status;

	/* Init the UART module */
	status = R_UART_Init(uart_master_channel, &g_uart_config);
	MESSAGE(status, "R_UART_Init");

	xTaskCreate(UART_Task, "UART-Task", usStackSize,
			NULL, uxPriority, &UartTaskHandle);
	configASSERT(UartTaskHandle);
	boot_time_update(BTIME_MARKER_LED_READY);
}

