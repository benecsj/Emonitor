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
#include "NVM_NonVolatileMemory.h"
#include "gpio.h"

#include "httpclient.h"
#include "Sensor_Manager.h"
#include "Wifi_Manager.h"

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

uint8 Emonitor_flashButton = 0;
uint32 Emonitor_flashButtonCounter = 0;
uint8 Emonitor_buttonState = 0;
uint8 Emonitor_requestState = 0;
uint8 Emonitor_connectionStatus = 1;

uint32 Emonitor_resetTimeoutCounter = 0;

uint32 Emonitor_nodeId = 1;
char Emonitor_url[100] = {0};
char Emonitor_key[33] = {0};


/******************************************************************************
* Primitives
\******************************************************************************/
extern void ICACHE_FLASH_ATTR Emonitor_callback(char * response_body, int http_status, char * response_headers, int body_size);

/******************************************************************************
* Implementations
\******************************************************************************/

/******************************************************************************
 * FunctionName : Emonitor_Preinit
 * Description  : Init emonitor application
 * Parameters   : none
 * Returns      : none
 *******************************************************************************/
void Emonitor_Preinit(void) {

	//Init UART
	UART_SetBaudrate(UART0, BIT_RATE_115200);
	//Init Status LED
	pinMode(LED2_BUILTIN,OUTPUT);
	gpio16_output_conf();
	gpio16_output_set(1);
	//Init flash button
	pinMode(FLASH_BUTTON,INPUT);
	//Init timer for fast task
    hw_timer_init();
    hw_timer_set_func(Emonitor_Main_1ms);
    hw_timer_arm(1000,1);
}

void Emonitor_Init(void){
    //Check if has valid url and key
	uint8 urlLength = strlen(Emonitor_url);
	uint8 apiKeyLength = strlen(Emonitor_key);
	if ((urlLength <= 4) || (urlLength == 100))
	{
		sprintf(Emonitor_url,"%s",DEFAULT_SERVER_ADDRESS);
	}
	if (apiKeyLength != 32)
	{
		sprintf(Emonitor_key,"%s",DEFAULT_API_KEY);
	}
	if(Emonitor_nodeId == 0xFFFFFFFF)
	{
		Emonitor_nodeId = DEFAULT_NODE_ID;
	}
}

/******************************************************************************
 * FunctionName : Emonitor_Main_1ms
 * Description  : Emonitor Fast Main
 * Parameters   : none
 * Returns      : none
 *******************************************************************************/
void Emonitor_Main_1ms(void) {
	uint8 ledValue;
	//Calculate led state
	switch(Emonitor_buttonState)
	{
	case 0:
		Emonitor_statusCounter= (Emonitor_statusCounter+1)%2;
		ledValue = Emonitor_statusCounter | Emonitor_ledControl;
		break;
	case 1:
		Emonitor_statusCounter= (Emonitor_statusCounter+1)%1000;
		ledValue = Emonitor_statusCounter <500;
		break;
	case 2:
		Emonitor_statusCounter= (Emonitor_statusCounter+1)%100;
		ledValue = Emonitor_statusCounter <50;
		break;
	}
	//Toggle led
	digitalWrite(LED2_BUILTIN, (ledValue));

	//Read flash button
	Emonitor_flashButton = (digitalRead(FLASH_BUTTON) == 0);
	//Process button state
	if(Emonitor_flashButton == 1)
	{
		Emonitor_flashButtonCounter = Emonitor_flashButtonCounter + 1;
	}
	else
	{
		Emonitor_flashButtonCounter = 0;
	}

	taskENTER_CRITICAL();
	if(Emonitor_flashButtonCounter == 0)
	{
		Emonitor_buttonState = 0;
	}
	else if(Emonitor_flashButtonCounter == 100)
	{

		Emonitor_buttonState = 1;
		Emonitor_requestState = 1;

	}
	else if(Emonitor_flashButtonCounter == 8000)
	{
		Emonitor_buttonState = 2;
		Emonitor_requestState = 2;
	}
	taskEXIT_CRITICAL();
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
	uint8 buttonState;

	uint8 tempCount;
	uint8* ids;
	sint16* temperatures;

	//Uptime counter
	Emonitor_uptime++;
	//Free ram and stack
	uint32 freeRam = system_get_free_heap_size();
	uint32 freeStack = uxTaskGetStackHighWaterMark(NULL);
	DBG("(EM) Uptime(%d) Pulse(%d) Heap:(%d) Stack:(%d)\n", Emonitor_uptime,Sensor_Manager_GetPulseCount(0),freeRam,freeStack);

	//Connection status led update
	gpio16_output_set(Emonitor_connectionStatus | Emonitor_ledControl);

	//Emonitor sending
	Emonitor_timing++;
	if(Emonitor_timing == 10){
		taskENTER_CRITICAL();
		Emonitor_counter = 0;
		taskEXIT_CRITICAL();
		Emonitor_timing = 0;

		if(Wifi_Manager_Connected() == 1)
		{
			//Check if strings have a valid length
			uint8 urlLength = strlen(Emonitor_url);
			uint8 apiKeyLength = strlen(Emonitor_key);
			if ((urlLength > 4) && (apiKeyLength == 32))
			{
				DBG("----------------Emonitor Send Data-------------\n");
				Sensor_Manager_Get_TempSensorData(&tempCount,&ids,&temperatures);
				//Start of Emoncsm send Url
				Append(length,buffer,"%s/input/post.json?node=%d&json={",Emonitor_url,Emonitor_nodeId);
				//Add freeram
				Append(length,buffer,"freeram:%d,",freeRam);
				//Add pulse counters
				for(i=0;i<SENSOR_MANAGER_PULSE_COUNTERS;i++)
				{
					 Append(length,buffer,"Pulse_%02X:%d,",(i+1),Sensor_Manager_GetPulseCount(i));
				}
				//Add temperatures
				for(i=0;i<tempCount;i++)
				{
					 Append(length,buffer,"Temp_%02X%02X%02X%02X%02X:%d.%d,",ids[(i*8)+1],ids[(i*8)+2],ids[(i*8)+3],ids[(i*8)+4],ids[(i*8)+7],temperatures[i]/10,abs(temperatures[i]%10));
				}
				//Add Uptime
				Append(length,buffer,"uptime:%d",Emonitor_uptime);
				//End of Emoncsm send Url
				Append(length,buffer,"}&apikey=%s",Emonitor_key);


				//Send out Emoncsm Data
				http_get(buffer, "", Emonitor_callback);
			}
			else
			{
				DBG("(EM) Not valid userkey / server values found [%d][%d]\n",urlLength,apiKeyLength);
			}
		}
		else
		{
			Emonitor_connectionStatus = 1;
		}
	}

	//Handle button action
	taskENTER_CRITICAL();
	buttonState = Emonitor_buttonState;
	taskEXIT_CRITICAL();
	if(buttonState != Emonitor_requestState)
	{
		//Only process when button is released
		if(buttonState == 0)
		{
			switch(Emonitor_requestState)
			{
			case 1:
				DBG("ENABLE HOTSPOT\n");
				Wifi_Manager_EnableHotspot(1);
				NvM_RequestSave();
				//Clear processed request
				taskENTER_CRITICAL();
				Emonitor_requestState = 3;
				taskEXIT_CRITICAL();
				break;
			case 2:
				DBG("FACTORY RESET\n");
				NvM_RequestClear();
				//Enter shutdown request
				taskENTER_CRITICAL();
				Emonitor_requestState = 3;
				taskEXIT_CRITICAL();
				break;
			}
		}
	}

	//Check connections status
	if(Emonitor_connectionStatus == 0)
	{
		Emonitor_resetTimeoutCounter = 0;
	}
	else
	{
		Emonitor_resetTimeoutCounter = Emonitor_resetTimeoutCounter + 1;
		//Check if too long no successfull send was performed
		if(Emonitor_resetTimeoutCounter > 1800)
		{
			Emonitor_requestState = 3;
		}
	}

	//In a shutdown state
	if(Emonitor_requestState == 3)
	{
		//Wait until NvM is done
		if(NvM_IsBusy() == FALSE)
		{
			//Trigger reset
			taskENTER_CRITICAL();
			while(1)
			{

			}
			taskEXIT_CRITICAL();
		}
	}
}

/******************************************************************************
 * FunctionName : Emonitor_Main_Background
 * Description  : Emonitor Background Main
 * Parameters   : none
 * Returns      : none
 *******************************************************************************/
void Emonitor_Main_Background(void) {
	taskENTER_CRITICAL();
	Emonitor_counter++;
	taskEXIT_CRITICAL();
}

void ICACHE_FLASH_ATTR Emonitor_callback(char * response_body, int http_status, char * response_headers, int body_size)
{
	printf("HTTP status=%d\n", http_status);
	if (http_status != HTTP_STATUS_GENERIC_ERROR) {
		printf("HTTP headers (%d)\n", strlen(response_headers));
		printf("HTTP body (%d)\n", body_size);
		printf("HTTP body=\"%s\"\n", response_body); // FIXME: this does not handle binary data.

		if(http_status == 200)
		{
			Emonitor_connectionStatus = 0;
		}
		else
		{
			Emonitor_connectionStatus = 1;
		}
	}
}


