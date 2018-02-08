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
	//Init Status LED
	pinMode(LED_BUILTIN,OUTPUT);
	pinMode(PULSE_INPUT3,INPUT);
	digitalWrite(LED_BUILTIN,1);
	//Init flash button
	pinMode(FLASH_BUTTON,INPUT);
    //Disable Wifi
    init_esp_wifi();
}

void Emonitor_Init(void){
	PRINT_EMON("(EM) Emonitor_Init\n");
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
		  timingValue = Emonitor_SendPeroid -10;
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
	switch(Emonitor_buttonState)
	{
	case 0:
		Emonitor_statusCounter= (Emonitor_statusCounter+1)%LED_TIMING_NORMAL;
		ledValue = (Emonitor_statusCounter <(LED_TIMING_NORMAL-2));
		if((Emonitor_statusCounter == (LED_TIMING_NORMAL-150)) && (Emonitor_connectionStatus == 0))
		{
			ledValue = 0 ;
		}
		if(Wifi_Manager_Connected() == 0)
		{
			Emonitor_statusCounter= (Emonitor_statusCounter)%2;
			ledValue = Emonitor_statusCounter == 0;
		}
		ledValue = ledValue || Emonitor_ledControl;
		break;
	case 1:
		Emonitor_statusCounter= (Emonitor_statusCounter+1)%LED_TIMING_NORMAL;
		ledValue = Emonitor_statusCounter <(LED_TIMING_NORMAL/2);
		break;
	case 2:
		Emonitor_statusCounter= (Emonitor_statusCounter+1)%LED_TIMING_RESET;
		ledValue = Emonitor_statusCounter <(LED_TIMING_RESET/2);
		break;
	}
#else
	Emonitor_statusCounter= (Emonitor_statusCounter+1)%2;
	ledValue = Emonitor_statusCounter == 0;
#endif
	//Toggle led
	digitalWrite(LED_BUILTIN, (ledValue));

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
	uint8 ip[4];
	uint8 tempCount;
	uint8* ids;
	sint16* temperatures;

	//Uptime counter
	Emonitor_uptime++;
	//Get local ip address
	Wifi_Manager_GetIp(ip);
	//Free ram and stack
	uint32 freeRam = system_get_free_heap_size();
	uint32 freeStack = uxTaskGetStackHighWaterMark(NULL);
	PRINT_EMON("(EM) U(%d) T(%d) I(%d) C(%d) H(%d) S(%d) R(%d) S(%d) H(%d)\n", Emonitor_uptime,Emonitor_timing,ip[3],Emonitor_connectionCounter,freeRam,freeStack,Emonitor_counter,Wifi_Manager_GetSignalLevel(),Sensor_Manager_GetTempHealth());
	//DBG_EMON("(EM) Pulse0(%d) Pulse1(%d) Pulse2(%d) Pulse3(%d) \n",Sensor_Manager_GetPulseCount(0),Sensor_Manager_GetPulseCount(1),Sensor_Manager_GetPulseCount(2),Sensor_Manager_GetPulseCount(3));
	//Connection status led update

	taskENTER_CRITICAL();
	Emonitor_counter = 0;
	taskEXIT_CRITICAL();

	//Emonitor sending
	taskENTER_CRITICAL();
	Emonitor_timing++;
	taskEXIT_CRITICAL();
	//Store timing value
	Emonitor_StoreTiming(Emonitor_timing);
	if(Emonitor_timing >= Emonitor_SendPeroid){
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
				DBG_EMON("----------------Emonitor Send Data-------------\n");
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
					char sign = (temperatures[i]<0 ? '-':'+');
					 Append(length,buffer,"Temp_%02X%02X%02X%02X%02X:%c%d.%d,",ids[(i*8)+1],ids[(i*8)+2],ids[(i*8)+3],ids[(i*8)+4],ids[(i*8)+7],sign,abs(temperatures[i]/10),abs(temperatures[i]%10));
				}
				//Add analog reads
				for(i=0;i<SENSOR_MANAGER_ANALOGCHANNELS_COUNT;i++)
				{
					Append(length,buffer,"Analog_%02X:%d,",(i+1),Sensor_Manager_GetAnalogValue());
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
				DBG_EMON("(EM) Not valid userkey / server values found [%d][%d]\n",urlLength,apiKeyLength);
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
				DBG_EMON("(EM) ENABLE HOTSPOT\n");
				Wifi_Manager_EnableHotspot(1);
				NvM_RequestSave();
				//Clear processed request
				taskENTER_CRITICAL();
				Emonitor_requestState = 3;
				taskEXIT_CRITICAL();
				break;
			case 2:
				DBG_EMON("(EM) FACTORY RESET\n");
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
				DBG_EMON("(EM) COMM OK DISABLE HOTSPOT\n");
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
				DBG_EMON("(EM) COMM TIMEOUT ENABLE HOTSPOT\n");
				//Turn on AccesPoint
				Wifi_Manager_EnableHotspot(1);
				NvM_RequestSave();
				Emonitor_requestState = 3;
			}
		}
		//Check if too long no successfull send was performed
		if(Emonitor_connectionCounter < -TIMEOUT_RESET_NO_COMM)
		{
			DBG_EMON("(EM) COMM TIMEOUT RESET\n");
			Emonitor_requestState = 3;
		}
	}

	//Save was requested
	if(Emonitor_requestState == 4)
	{
		DBG_EMON("(EM) SAVE REQUESTED\n");
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
			PRINT_EMON("(EM) ###################################\n");
			PRINT_EMON("(EM) ###################################\n");
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
	DBG_EMON("HTTP status=%d\n", http_status);
	if (http_status != HTTP_STATUS_GENERIC_ERROR) {
		DBG_EMON("HTTP headers (%d)\n", strlen(response_headers));
		DBG_EMON("HTTP body (%d)\n", body_size);
		DBG_EMON("HTTP body=\"%s\"\n", response_body); // FIXME: this does not handle binary data.

		if(http_status == 200)
		{
			Emonitor_connectionStatus = 0;
			//Send was successfull clear timer
			taskENTER_CRITICAL();
			Emonitor_timing = 0;
			taskEXIT_CRITICAL();
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

