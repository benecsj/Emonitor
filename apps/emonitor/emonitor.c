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
#include "Sensor_Manager.h"


/******************************************************************************
* Defines
\******************************************************************************/

#define Append(length,buffer,format,...) length += sprintf(&buffer[length],format,__VA_ARGS__)

/******************************************************************************
* Variables
\******************************************************************************/

uint32 Emonitor_counter = 0;
uint32 Emonitor_statusCounter = 0;

uint32 Emonitor_timing = 0;

uint32 Emonitor_uptime = 0;

uint8 Emonitor_ledControl = 0;

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
	digitalWrite(LED2_BUILTIN, (Emonitor_statusCounter | Emonitor_ledControl));
	Emonitor_statusCounter= (Emonitor_statusCounter+1)%2;
}

/******************************************************************************
 * FunctionName : Emonitor_Main_1000ms
 * Description  : Emonitor Slow Main
 * Parameters   : none
 * Returns      : none
 *******************************************************************************/
void Emonitor_Main_1000ms(void) {
	uint8 i;
	uint16 length = 0;
	char buffer[500];

	uint8 nodeId = 2;
	char url[100] = {"http://v9.emonitor.hu"};
	char apyKey[33] = {"97d3e42a841ea6c219582211313d5051"};

	uint8 tempCount;
	uint8* ids;
	sint16* temperatures;

	//Uptime counter
	Emonitor_uptime++;
	//Free ram
	uint32 freeRam = system_get_free_heap_size();


	Emonitor_timing++;
	if(Emonitor_timing == 10){
		DBG("CYCLE(%d) HEAP:(%d)\n", Emonitor_counter,freeRam);
		Emonitor_counter = 0;
		Emonitor_timing = 0;

		DBG("----------------Emonitor Send Data-------------\n");

		Sensor_Manager_Get_TempSensorData(&tempCount,&ids,&temperatures);
	    //Start of Emoncsm send Url
	    Append(length,buffer,"%s/input/post.json?node=%d&json={",url,nodeId);
	    //Add freeram
	    Append(length,buffer,"freeram:%d,",freeRam);
	    //Add temperatures
	    for(i=0;i<tempCount;i++)
	    {
	    	 Append(length,buffer,"Temp_%02X%02X%02X%02X%02X:%d.%d,",ids[(i*8)+1],ids[(i*8)+2],ids[(i*8)+3],ids[(i*8)+4],ids[(i*8)+7],temperatures[i]/10,abs(temperatures[i]%10));
	    }
	    //Add Uptime
	    Append(length,buffer,"uptime:%d",Emonitor_uptime);
	    //End of Emoncsm send Url
	    Append(length,buffer,"}&apikey=%s",apyKey);

	    //Send out Emoncsm Data
		http_get(buffer, "", http_callback_example);

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


