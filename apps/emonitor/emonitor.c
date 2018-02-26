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

#define Append(length,buffer,...) length += sprintf(&buffer[length],__VA_ARGS__)

#if DEBUG_EMONITOR
#define DBG_EMON(...) printf(__VA_ARGS__)
#else
#define DBG_EMON(...)
#endif


#define PRINT_EMON(...) printf(__VA_ARGS__)

/******************************************************************************
* Variables
\******************************************************************************/

uint32 Emonitor_counter = 0;
uint32 Emonitor_counterMirror = 0;
uint32 Emonitor_LEDCounter = 0;

uint32 Emonitor_resetReason = 0;

uint32 Emonitor_timing = 0;

uint32 Emonitor_uptime = 0;
uint32 Emonitor_flashButtonCounter = 0;
Emonitor_Request Emonitor_requestState = EMONITOR_REQ_NONE;
Emonitor_Connection_Status Emonitor_connectionStatus = EMONITOR_NOT_CONNECTED;

sint32 Emonitor_connectionCounter = 0;

uint32 Emonitor_freeRam = 0;

// Emonitor user parameters
uint32 Emonitor_nodeId = INVALID_ID;
char Emonitor_url[100] = {0};
char Emonitor_key[33] = {0xFF};
uint32 Emonitor_SendPeroid = 0;

/******************************************************************************
* Primitives
\******************************************************************************/
extern void ICACHE_FLASH_ATTR Emonitor_callback(char * response_body, int http_status, char * response_headers, int body_size);
extern uint32_t Emonitor_GetDefaultId(void);
extern void task_1ms(void);
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
	//Init flash button
	pinMode(FLASH_BUTTON,INPUT);
    //Disable Wifi
    init_esp_wifi();
}

void Emonitor_EnableStatusLed(void) {
	pinMode(LED_BUILTIN,OUTPUT);
	digitalWrite(LED_BUILTIN,1);
}

void Emonitor_Init(void){
	PRINT_EMON("(EM) Emonitor_Init\n");
    //Check if has valid url and key
	uint8 urlLength = strlen(Emonitor_url);
	uint8 apiKeyLength = strlen(Emonitor_key);
	if ((urlLength <= URL_MIN_LENGTH) || (urlLength == URL_MAX_LENGTH))
	{
		sprintf(Emonitor_url,"%s",DEFAULT_SERVER_ADDRESS);
	}
	if (apiKeyLength > API_KEY_LENGTH || Emonitor_key[0] == 0xFF )
	{
		sprintf(Emonitor_key,"%s",DEFAULT_API_KEY);
	}
	if(Emonitor_nodeId == INVALID_ID)
	{
		Emonitor_nodeId = Emonitor_GetDefaultId();
	}
	if((Emonitor_SendPeroid == 0) || (Emonitor_SendPeroid >3600))
	{
		Emonitor_SendPeroid = DEFAULT_SEND_TIMING;
	}
	//Restore timing
	Emonitor_timing = Emonitor_RestoreTiming();
}

void Emonitor_StartTimer(void){
	//Init timer for fast task
    hw_timer_init();
    hw_timer_set_func(task_1ms);
    hw_timer_arm(1000,1);
}

void Emonitor_StoreTiming(uint32_t timingValue){
	  uint8 buffer[4];
	  buffer[0] = (timingValue>>24) & 0xF;
	  buffer[1] = (timingValue>>16) & 0xF;
	  buffer[2] = (timingValue>>8) & 0xF;
	  buffer[3] = (timingValue) & 0xF;
	  system_rtc_mem_write(64, buffer, 4);
}

uint32_t Emonitor_RestoreTiming(void){
	  uint8 buffer[4];
	  uint32_t timingValue = 0;
	  system_rtc_mem_read(64, buffer, 4);
	  timingValue |=  (((uint32_t) buffer[0])<<24);
	  timingValue |=  (((uint32_t) buffer[1])<<16);
	  timingValue |=  (((uint32_t) buffer[2])<<8);
	  timingValue |=  (((uint32_t) buffer[3]));
	  //plausibity check
	  if(timingValue > Emonitor_SendPeroid*2)
	  {
		  timingValue = Emonitor_SendPeroid/2;
	  }

	  return timingValue;
}

/******************************************************************************
 * FunctionName : Emonitor_Main_1ms
 * Description  : Emonitor Fast Main
 * Parameters   : none
 * Returns      : none
 *******************************************************************************/
void Emonitor_Main_1ms(void) {
	uint8 ledValue;

#if (EMONITOR_TIMING_TEST == 0)
	//Calculate led state
	if(Emonitor_flashButtonCounter > LONG_PRESS_MIN)
	{
		Emonitor_LEDCounter= (Emonitor_LEDCounter+1)%LED_TIMING_RESET;
		ledValue = Emonitor_LEDCounter <(LED_TIMING_RESET/2);
	}
	else if(Emonitor_flashButtonCounter > SHORT_PRESS_MIN)
	{
		Emonitor_LEDCounter= (Emonitor_LEDCounter+1)%LED_TIMING_NORMAL;
		ledValue = Emonitor_LEDCounter <(LED_TIMING_NORMAL/2);
	}
	else
	{
		Emonitor_LEDCounter= (Emonitor_LEDCounter+1)%LED_TIMING_NORMAL;
		ledValue = (Emonitor_LEDCounter <(LED_TIMING_NORMAL-2));
		if((Emonitor_LEDCounter == (LED_TIMING_NORMAL-150)) && (Emonitor_connectionStatus == EMONITOR_CONNECTED))
		{
			ledValue = 0 ;
		}
		if(Wifi_Manager_IsConnected() == false)
		{
			Emonitor_LEDCounter= (Emonitor_LEDCounter)%2;
			ledValue = Emonitor_LEDCounter == 0;
		}
	}
#else
	Emonitor_LEDCounter= (Emonitor_LEDCounter+1)%2;
	ledValue = Emonitor_LEDCounter;
#endif
	//Toggle led
	digitalWrite(LED_BUILTIN, (ledValue));

	//Process flash button state
	if(digitalRead(FLASH_BUTTON) == BUTTON_PRESSED)
	{
		//Button being pressed, count press time
		Emonitor_flashButtonCounter++;
	}
	else //Button released
	{
		//Check press time
		if(Emonitor_flashButtonCounter > LONG_PRESS_MIN)
		{
			Emonitor_Request(EMONITOR_REQ_CLEAR);
		}
		else if(Emonitor_flashButtonCounter > SHORT_PRESS_MIN)
		{
			Emonitor_Request(EMONITOR_REQ_HOTSPOT);
		}
		//Clear button press time
		Emonitor_flashButtonCounter = 0;
	}
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
	uint8 ip[4];
	uint8 tempCount;
	uint8* ids;
	sint16* temperatures;

	//Uptime counter
	Emonitor_uptime++;
	//Get local ip address
	Wifi_Manager_GetIp(ip);
	//Free ram and stack
	Emonitor_freeRam = system_get_free_heap_size();
	uint32 freeStack = uxTaskGetStackHighWaterMark(NULL);
	PRINT_EMON("(EM) U(%d) T(%d) I(%d) C(%d) H(%d) S(%d) R(%d) S(%d) H(%d)\n", Emonitor_uptime,Emonitor_timing,ip[3],Emonitor_connectionCounter,Emonitor_freeRam,freeStack,Emonitor_counter,Wifi_Manager_GetSignalLevel(),Sensor_Manager_GetTempHealth());


	taskENTER_CRITICAL();
	Emonitor_counterMirror = Emonitor_counter;
	Emonitor_counter = 0;
	taskEXIT_CRITICAL();

	if((Emonitor_timing+1) >= Emonitor_SendPeroid){
		if(Wifi_Manager_IsConnected())
		{
			//Check if strings have a valid length
			uint8 urlLength = strlen(Emonitor_url);
			uint8 apiKeyLength = strlen(Emonitor_key);
			//Check if url already has http:// text if not append it
			if(strncmp(Emonitor_url,"http://",7) != 0)
			{
				Append(length,buffer,"http://");
			}
			//Check url and user key
			if ((urlLength > URL_MIN_LENGTH) && (apiKeyLength == API_KEY_LENGTH))
			{
				DBG_EMON("----------------Emonitor Send Data-------------\n");
				Sensor_Manager_Get_TempSensorData(&tempCount,&ids,&temperatures);
				//Start of Emoncsm send Url
				Append(length,buffer,"%s/input/post.json?node=%d&json={",Emonitor_url,Emonitor_nodeId);
				//Add freeram
				Append(length,buffer,"freeram:%d,",Emonitor_freeRam);
				//Add pulse counters
				for(i=0;i<SENSOR_MANAGER_PULSE_COUNTERS;i++)
				{
					 Append(length,buffer,"Pulse_%02X:%d,",(i+1),Sensor_Manager_GetPulseCount(i));
				}
				//Add temperatures
				for(i=0;i<tempCount;i++)
				{
					char sign = (temperatures[i]<0 ? '-':'+');
					 Append(length,buffer,"Temp_%02X%02X%02X%02X%02X:%c%d.%d,",ids[(i*8)+1],ids[(i*8)+2],ids[(i*8)+3],ids[(i*8)+4],ids[(i*8)+7],sign,abs(temperatures[i]/10),abs(temperatures[i]%10));
				}
				//Add analog reads
				for(i=0;i<SENSOR_MANAGER_ANALOGCHANNELS_COUNT;i++)
				{
					Append(length,buffer,"Analog_%02X:%d,",(i+1),Sensor_Manager_GetAnalogValue());
				}
				//MHZ14 CO2 Sensor
				if(Sensor_Manager_HasCO2Sensor())
				{
					Append(length,buffer,"Meter_C02:%d,",Sensor_Manager_GetCO2());
				}
				//Add Uptime
				Append(length,buffer,"uptime:%d",Emonitor_uptime);
				//End of Emoncsm send Url
				Append(length,buffer,"}&apikey=%s",Emonitor_key);


				//Send out Emoncsm Data
				http_get(buffer, "", Emonitor_callback);
			}
			else  //Invalid user parameters
			{
				DBG_EMON("(EM) Not valid userkey / server values found [%d][%d]\n",urlLength,apiKeyLength);
			}
		}
		else	//No wifi connection therefore so no user connection possible
		{
			Emonitor_connectionStatus = EMONITOR_NOT_CONNECTED;
		}
	}
	else //Waiting to send
	{
		//Emonitor send timing
		Emonitor_timing++;
		//Store timing value
		Emonitor_StoreTiming(Emonitor_timing);
	}

	//Process requests
	switch(Emonitor_requestState)
	{
	case EMONITOR_REQ_HOTSPOT:
		DBG_EMON("(EM) ENABLE HOTSPOT\n");
		Wifi_Manager_EnableHotspot(TRUE);
		//Clear processed request
		Emonitor_Request(EMONITOR_REQ_SAVE);
		break;
	case EMONITOR_REQ_CLEAR:
		DBG_EMON("(EM) FACTORY RESET\n");
		NvM_RequestClear();
		//Enter shutdown request
		Emonitor_Request(EMONITOR_REQ_RESTART);
		break;
	case EMONITOR_REQ_SAVE:
		DBG_EMON("(EM) SAVE REQUESTED\n");
		NvM_RequestSave();
		//Change to reset request
		Emonitor_Request(EMONITOR_REQ_RESTART);
		break;
	case EMONITOR_REQ_RESTART:
		//Wait until NvM is done
		if(NvM_IsBusy() == FALSE)
		{
			DBG_EMON("(EM) RESTART REQUESTED\n");
			PRINT_EMON("(EM) ###################################\n");
			PRINT_EMON("(EM) ###################################\n");
			//Disable status led, this will trigger EXT reset
			pinMode(LED_BUILTIN,INPUT);
			//Do nothing
			while(1){}
		}
		break;
	}

	//Check connections status and increment counter
	Emonitor_connectionCounter = Emonitor_connectionCounter + ((Emonitor_connectionStatus == EMONITOR_CONNECTED)? 1 : -1);
	//Based on counter value perform task if needed
	switch(Emonitor_connectionCounter)
	{
	//TURN OFF ACCESS POINT
	case TIMEOUT_TURNOFF_APP:
		//Check if hotspot turned on
		if(Wifi_Manager_GetEnableHotspot() == ON)
		{
			DBG_EMON("(EM) COMM OK DISABLE HOTSPOT\n");
			//Turn off AccesPoint
			Wifi_Manager_EnableHotspot(OFF);
			Emonitor_Request(EMONITOR_REQ_SAVE);
		}
		break;
	//TURN ON ACCESS POINT
	case -TIMEOUT_TURNON_AP:
		//Check if hotspot turned off
		if(Wifi_Manager_GetEnableHotspot() == OFF)
		{
			DBG_EMON("(EM) COMM TIMEOUT ENABLE HOTSPOT\n");
			//Turn on AccesPoint
			Wifi_Manager_EnableHotspot(ON);;
			Emonitor_Request(EMONITOR_REQ_SAVE);
		}
		break;
	//NO COMM RESET
	case -TIMEOUT_RESET_NO_COMM:
		DBG_EMON("(EM) COMM TIMEOUT RESET\n");
		Emonitor_Request(EMONITOR_REQ_RESTART);
		break;
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
	DBG_EMON("HTTP status=%d\n", http_status);
	if (http_status != HTTP_STATUS_GENERIC_ERROR) {
		DBG_EMON("HTTP headers (%d)\n", strlen(response_headers));
		DBG_EMON("HTTP body (%d)\n", body_size);
		DBG_EMON("HTTP body=\"%s\"\n", response_body); // FIXME: this does not handle binary data.

		if(http_status == 200)
		{
			Emonitor_connectionStatus = EMONITOR_CONNECTED;
			//Send was successfull clear timer
			Emonitor_timing = 0;
		}
		else
		{
			Emonitor_connectionStatus = EMONITOR_NOT_CONNECTED;
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

