/******************************************************************************
* Includes
\******************************************************************************/

#include "project_config.h"
#include "user_config.h"

#include "Emonitor.h"

#include "esp_common.h"
#include "uart.h"
#include "hw_timer.h"
#include "pins.h"

#include "httpclient.h"

/******************************************************************************
* Defines
\******************************************************************************/

/******************************************************************************
* Variables
\******************************************************************************/

uint32 Emonitor_counter = 0;
uint32 Emonitor_statusCounter = 0;

uint32 Emonitor_timing = 0;

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
	pinMode(LED2_BUILTIN,OUTPUT);
	//Init timer for fast task
    hw_timer_init();
    hw_timer_set_func(Emonitor_Main_1ms);
    hw_timer_arm(1000,1);
}

/******************************************************************************
 * FunctionName : Emonitor_Main_1ms
 * Description  : Emonitor Fast Main
 * Parameters   : none
 * Returns      : none
 *******************************************************************************/
void Emonitor_Main_1ms(void) {
	digitalWrite(LED2_BUILTIN, Emonitor_statusCounter);
	Emonitor_statusCounter= (Emonitor_statusCounter+1)%2;
}

/******************************************************************************
 * FunctionName : Emonitor_Main_1000ms
 * Description  : Emonitor Slow Main
 * Parameters   : none
 * Returns      : none
 *******************************************************************************/
void Emonitor_Main_1000ms(void) {
	Emonitor_timing++;
	if(Emonitor_timing == 10){
		DBG("CYCLE(%d) HEAP:(%d)\n", Emonitor_counter, system_get_free_heap_size());
		Emonitor_counter = 0;
		Emonitor_timing = 0;

		DBG("----------------HTTP GET TEST-------------\n");
		http_get("http://v9.emonitor.hu/input/post.json?node=1&json={power:200}&apikey=97d3e42a841ea6c219582211313d5051", "", http_callback_example);

	}
}

/******************************************************************************
 * FunctionName : Emonitor_Main_Background
 * Description  : Emonitor Background Main
 * Parameters   : none
 * Returns      : none
 *******************************************************************************/
void Emonitor_Main_Background(void) {
	Emonitor_counter++;
}


