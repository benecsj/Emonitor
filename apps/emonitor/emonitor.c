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

#include "httpclient.h"
#include "Sensor_Manager.h"
#include "Wifi_Manager.h"
#include "spiffs_manager.h"

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

sint32 Emonitor_connectionCounter = 0;

uint32 Emonitor_nodeId = 0;
char Emonitor_url[100] = {0};
char Emonitor_key[33] = {0xFF};


/******************************************************************************
* Primitives
\******************************************************************************/
extern void ICACHE_FLASH_ATTR Emonitor_callback(char * response_body, int http_status, char * response_headers, int body_size);
extern uint32_t Emonitor_GetDefaultId(void);
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
	pinMode(LED_BUILTIN,OUTPUT);
	digitalWrite(LED_BUILTIN,1);
	//Init flash button
	pinMode(FLASH_BUTTON,INPUT);
	//Init timer for fast task
    hw_timer_init();
    hw_timer_set_func(Emonitor_Main_1ms);
    hw_timer_arm(1000,1);
    //Disable Wifi
    init_esp_wifi();
}

void Emonitor_Init(void){
    //Check if has valid url and key
	uint8 urlLength = strlen(Emonitor_url);
	uint8 apiKeyLength = strlen(Emonitor_key);
	if ((urlLength <= 4) || (urlLength == 100))
	{
		sprintf(Emonitor_url,"%s",DEFAULT_SERVER_ADDRESS);
	}
	if (apiKeyLength > 32 || Emonitor_key[0] == 0xFF )
	{
		sprintf(Emonitor_key,"%s",DEFAULT_API_KEY);
	}
	if(Emonitor_nodeId == 0)
	{
		Emonitor_nodeId = Emonitor_GetDefaultId();
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
		Emonitor_statusCounter= (Emonitor_statusCounter+1)%1000;
		ledValue = (Emonitor_statusCounter <998);
		if((Emonitor_statusCounter == 800) && (Emonitor_connectionStatus == 0))
		{
			ledValue = 0 ;
		}
		ledValue = ledValue || Emonitor_ledControl;
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
	char http[10] = {0};
	uint8 buttonState;

	uint8 tempCount;
	uint8* ids;
	sint16* temperatures;

	//Uptime counter
	Emonitor_uptime++;
	//Free ram and stack
	uint32 freeRam = system_get_free_heap_size();
	uint32 freeStack = uxTaskGetStackHighWaterMark(NULL);
	DBG("(EM) Uptime(%d) Conn(%d) Heap:(%d) Stack:(%d)\n", Emonitor_uptime,Emonitor_connectionCounter,freeRam,freeStack);
	//DBG("(EM) Pulse0(%d) Pulse1(%d) Pulse2(%d) Pulse3(%d) \n",Sensor_Manager_GetPulseCount(0),Sensor_Manager_GetPulseCount(1),Sensor_Manager_GetPulseCount(2),Sensor_Manager_GetPulseCount(3));
	//Connection status led update
	digitalWrite(LED_BUILTIN,(Emonitor_connectionStatus | Emonitor_ledControl));

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
			if(strncmp(Emonitor_url,"http://",7) != 0)
			{
				sprintf(http,"http://");
			}

			if ((urlLength > 4) && (apiKeyLength == 32))
			{
				DBG("----------------Emonitor Send Data-------------\n");
				Sensor_Manager_Get_TempSensorData(&tempCount,&ids,&temperatures);
				//Start of Emoncsm send Url
				Append(length,buffer,"%s%s/input/post.json?node=%d&json={",http,Emonitor_url,Emonitor_nodeId);
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
				DBG("(EM) ENABLE HOTSPOT\n");
				Wifi_Manager_EnableHotspot(1);
				NvM_RequestSave();
				//Clear processed request
				taskENTER_CRITICAL();
				Emonitor_requestState = 3;
				taskEXIT_CRITICAL();
				break;
			case 2:
				DBG("(EM) FACTORY RESET\n");
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
		Emonitor_connectionCounter = Emonitor_connectionCounter +1;

		if(Emonitor_connectionCounter == TIMEOUT_TURNOFF_APP)
		{
			//Check if it turned on
			if(Wifi_Manager_GetEnableHotspot() == 1)
			{
				DBG("(EM) COMM OK DISABLE HOTSPOT\n");
				//Turn off AccesPoint
				Wifi_Manager_EnableHotspot(0);
				NvM_RequestSave();
				Emonitor_requestState = 3;
			}
		}

	}
	else
	{
		Emonitor_connectionCounter = Emonitor_connectionCounter - 1;

		if(Emonitor_connectionCounter == -TIMEOUT_TURNON_AP)
		{
			//Check if it turned off
			if(Wifi_Manager_GetEnableHotspot() == 0)
			{
				DBG("(EM) COMM TIMEOUT ENABLE HOTSPOT\n");
				//Turn on AccesPoint
				Wifi_Manager_EnableHotspot(1);
				NvM_RequestSave();
				Emonitor_requestState = 3;
			}
		}
		//Check if too long no successfull send was performed
		if(Emonitor_connectionCounter < -TIMEOUT_RESET_NO_COMM)
		{
			DBG("(EM) COMM TIMEOUT RESET\n");
			Emonitor_requestState = 3;
		}
	}

	//Save was requested
	if(Emonitor_requestState == 4)
	{
		DBG("(EM) SAVE REQUESTED\n");
		NvM_RequestSave();
		//Change to reset request
		Emonitor_requestState = 3;
	}

	//In a shutdown state
	if(Emonitor_requestState == 3)
	{
		//Wait until NvM is done
		if(NvM_IsBusy() == FALSE)
		{
			DBG("(EM) ###################################\n");
			DBG("(EM) ###################################\n");
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

uint32_t Emonitor_GetDefaultId(void)
{
	char buffer[64];
	spiffs* fs = spiffs_get_fs();
	spiffs_file fd;
	fd = SPIFFS_open(fs, "/id", SPIFFS_RDONLY, 0);
	SPIFFS_read(fs, fd, (u8_t *)buffer, 64);
	SPIFFS_close(fs, fd);

	char *ret;
    ret = strchr((const char *)buffer, '_');

	return( (((uint32)ret[1]-'0')*1000) + (((uint32)ret[2]-'0')*100) + (((uint32)ret[3]-'0')*10) + ((uint32)ret[4]-'0') );
}

