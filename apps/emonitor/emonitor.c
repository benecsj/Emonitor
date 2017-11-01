/******************************************************************************
* Includes
\******************************************************************************/

#include "project_config.h"

#include "Emonitor.h"

#include "esp_common.h"
#include "uart.h"
#include "gpio.h"

/******************************************************************************
* Defines
\******************************************************************************/

/******************************************************************************
* Variables
\******************************************************************************/

uint32 Emonitor_counter;

/******************************************************************************
* Implementations
\******************************************************************************/

/******************************************************************************
 * FunctionName : Emonitor_init
 * Description  : Init emonitor application
 * Parameters   : none
 * Returns      : none
 *******************************************************************************/
void Emonitor_Init(void) {
	//Init UART
	UART_SetBaudrate(UART0, BIT_RATE_115200);
	//Init Status LED
	PIN_FUNC_SELECT(PERIPHS_IO_MUX_MTDI_U, FUNC_GPIO2);
}

/******************************************************************************
 * FunctionName : Emonitor_Main_10ms
 * Description  : Emonitor 10ms Main
 * Parameters   : none
 * Returns      : none
 *******************************************************************************/
void Emonitor_Main_10ms(void) {
	Emonitor_counter++;
	GPIO_OUTPUT_SET(2, Emonitor_counter % 2);
}

/******************************************************************************
 * FunctionName : Emonitor_Main_1000ms
 * Description  : Emonitor 1000ms Main
 * Parameters   : none
 * Returns      : none
 *******************************************************************************/
void Emonitor_Main_1000ms(void) {
	DBG("Hello World!!!(%d)\n", Emonitor_counter);
	Emonitor_counter = 0;
}



