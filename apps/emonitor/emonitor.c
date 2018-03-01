/******************************************************************************
* Includes
\******************************************************************************/

#include "project_config.h"

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
#if DEBUG_EMONITOR
#define DBG_EMON(...) printf(__VA_ARGS__)
#else
#define DBG_EMON(...)
#endif

#define PRINT_EMON(...) printf(__VA_ARGS__)

#define APPEND(...)				  length += sprintf(&buffer[length],__VA_ARGS__)

#define PRINT_TEMPID "%02X%02X%02X%02X%02X"
#define PRINT_TEMP	 "%c%d.%d"
#define JSON_START	 "&json={"
#define JSON_NEXT	 ","
#define JSON_DIV	 ":"
#define JSON_END	 "}"
#define READ_ID		 ids[(i*8)+1],ids[(i*8)+2],ids[(i*8)+3],ids[(i*8)+4],ids[(i*8)+7]
#define READ_TEMP	 sign,abs(temperatures[i]/10),abs(temperatures[i]%10)
/******************************************************************************
* Variables
\******************************************************************************/

//Timing and counters
uint32 Emonitor_backgroundCounter = 0;
uint32 Emonitor_LEDCounter = 0;
uint32 Emonitor_sendTimer = 0;
uint32 Emonitor_buttonCounter = 0;

//statuses
Emonitor_Connection_Status Emonitor_connectionStatus = EMONITOR_NOT_CONNECTED;
sint32 Emonitor_connectionCounter = 0;
uint32 Emonitor_freeRam = 0;
uint32 Emonitor_uptime = 0;
uint32 Emonitor_resetReason = 0;
uint32 Emonitor_backgroundRuntime = 0;

//Control
Emonitor_Request Emonitor_requestState = EMONITOR_REQ_NONE;
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
extern uint32 Emonitor_GetBackgroundRuntime(void);
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
	Emonitor_sendTimer = Emonitor_RestoreTiming();
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
	if(Emonitor_buttonCounter > LONG_PRESS_MIN)
	{
		Emonitor_LEDCounter= (Emonitor_LEDCounter+1)%LED_TIMING_RESET;
		ledValue = Emonitor_LEDCounter <(LED_TIMING_RESET/2);
	}
	else if(Emonitor_buttonCounter > SHORT_PRESS_MIN)
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
		Emonitor_buttonCounter++;
	}
	else //Button released
	{
		//Check press time
		if(Emonitor_buttonCounter > LONG_PRESS_MIN)
		{
			Emonitor_Request(EMONITOR_REQ_CLEAR);
		}
		else if(Emonitor_buttonCounter > SHORT_PRESS_MIN)
		{
			Emonitor_Request(EMONITOR_REQ_HOTSPOT);
		}
		//Clear button press time
		Emonitor_buttonCounter = 0;
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
	sint16 temperature;
	char sign;

	//Get local ip address
	Wifi_Manager_GetIp(ip);
	//Get background time
	if(Emonitor_backgroundRuntime ==0)
	{
		Emonitor_backgroundRuntime = Emonitor_GetBackgroundRuntime();
	}
	else
	{
		Emonitor_backgroundRuntime = ((Emonitor_backgroundRuntime*(CPU_USAGE_AVERAGE-1)) + (Emonitor_GetBackgroundRuntime())) / CPU_USAGE_AVERAGE ;
	}
	//Free ram and stack
	Emonitor_freeRam = system_get_free_heap_size();
	uint32 freeStack = uxTaskGetStackHighWaterMark(NULL);

	//Print out status
	PRINT_EMON("(EM) U(%d) T(%d) I(%d) C(%d) H(%d) S(%d) R(%d) S(%d) H(%d)\n", Emonitor_uptime,Emonitor_sendTimer,ip[3],Emonitor_connectionCounter,Emonitor_freeRam,freeStack,Emonitor_backgroundRuntime,Wifi_Manager_GetSignalLevel(),Sensor_Manager_GetTempHealth());

	//Send timing
	if((Emonitor_sendTimer+1) >= Emonitor_SendPeroid){
		//Check if got network connection
		if(Wifi_Manager_IsConnected())
		{
			//Check if strings have a valid length
			uint8 urlLength = strlen(Emonitor_url);
			uint8 apiKeyLength = strlen(Emonitor_key);
			//Check if url already has http:// text if not append it
			if(strncmp(Emonitor_url,"http://",7) != 0)
			{
				APPEND("http://");
			}
			//Check url and user key
			if ((urlLength > URL_MIN_LENGTH) && (apiKeyLength == API_KEY_LENGTH))
			{
				DBG_EMON("----------------Emonitor Send Data-------------\n");
				Sensor_Manager_Get_TempSensorData(&tempCount,&ids,&temperatures);
				//Start of Emoncsm send Url
				APPEND("%s/input/post.json?node=%d",Emonitor_url,Emonitor_nodeId);
				//\\\\\\\\\\\\\\\\\\
				//Start Json frame
				APPEND(JSON_START);
				//\\\\\\\\\\\\\\\\\\\\\\\\\\\
				//Add ip (only single time per connection)
				if (Emonitor_connectionStatus == EMONITOR_NOT_CONNECTED) {
					APPEND("ip" 				JSON_DIV 	"%d" 		JSON_NEXT	,ip[3]);}
				//Add freeram
					APPEND("freeram" 			JSON_DIV 	"%d"		JSON_NEXT	,Emonitor_freeRam);
				//Add pulse counters
				for(i = 0 ; i < SENSOR_MANAGER_PULSE_COUNTERS ; i++ ){
					APPEND("Pulse_%02X" 		JSON_DIV 	"%d" 		JSON_NEXT	,(i+1),Sensor_Manager_GetPulseCount(i));}
				//Add temperatures
				for(i = 0 ; i < tempCount ; i++ ){
					temperature = temperatures[i];
					if(Sensor_Manager_IsTempValid(temperature)) {
						sign = (temperature<0 ? '-':'+');
						APPEND("Temp_" PRINT_TEMPID JSON_DIV 	PRINT_TEMP 	JSON_NEXT	,READ_ID, READ_TEMP);
					}
				}
				//Add analog reads
				for(i = 0 ; i < SENSOR_MANAGER_ANALOGCHANNELS_COUNT ; i++ ){
					APPEND("Analog_%02X" 		JSON_DIV 	"%d" 		JSON_NEXT	,(i+1),Sensor_Manager_GetAnalogValue());}
				//MHZ14 CO2 Sensor
				if(Sensor_Manager_HasCO2Sensor()){
					APPEND("Meter_C02" 			JSON_DIV 	"%d" 		JSON_NEXT	,Sensor_Manager_GetCO2());}
				//Add Uptime
					APPEND("uptime" 			JSON_DIV 	"%d"		/*LAST*/	,Emonitor_uptime);
				//\\\\\\\\\\\\\\\\\\\\\\\\\\\
				//Finish Json frame
				APPEND(JSON_END);
				//\\\\\\\\\\\\\\\
				//End request with apikey
				APPEND("&apikey=%s",Emonitor_key);

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
		Emonitor_sendTimer++;
		//Store timing value
		Emonitor_StoreTiming(Emonitor_sendTimer);
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
	//Just measure background runtime
	prj_ENTER_CRITICAL();
	Emonitor_backgroundCounter++;
	prj_EXIT_CRITICAL();
}

uint32 Emonitor_GetBackgroundRuntime(void)
{
	//Return value
	uint32 returnValue;
	//Get background count value and reset it
	prj_ENTER_CRITICAL();
	returnValue = Emonitor_backgroundCounter;
	Emonitor_backgroundCounter = 0;
	prj_EXIT_CRITICAL();
	//Return background time
	return returnValue;
}

void ICACHE_FLASH_ATTR Emonitor_callback(char * response_body, int http_status, char * response_headers, int body_size)
{
	DBG_EMON("HTTP status=%d\n", http_status);
	//Check for generic HTTP error
	if (http_status != HTTP_STATUS_GENERIC_ERROR) {
		DBG_EMON("HTTP headers (%d)\n", strlen(response_headers));
		DBG_EMON("HTTP body (%d)\n", body_size);
		DBG_EMON("HTTP body=\"%s\"\n", response_body); // FIXME: this does not handle binary data.
		//Check if HTTP OK received
		if(http_status == 200)
		{
			Emonitor_connectionStatus = EMONITOR_CONNECTED;
			//Send was successful clear send timer
			Emonitor_sendTimer = 0;
		}
		else
		{
			Emonitor_connectionStatus = EMONITOR_NOT_CONNECTED;
		}
	}
	else
	{
		Emonitor_connectionStatus = EMONITOR_NOT_CONNECTED;
	}
}

uint32_t Emonitor_GetDefaultId(void)
{
	char buffer[64] = {0};
	char *numStart;
	spiffs* fs;
	spiffs_file fd;

	//Get id string from filesystem storage
	fs = spiffs_get_fs();
	fd = SPIFFS_open(fs, "/id", SPIFFS_RDONLY, 0);
	SPIFFS_read(fs, fd, (u8_t *)buffer, 64);
	SPIFFS_close(fs, fd);

	//Look for spec char if present
	numStart = strchr((const char *)buffer, '_');
    //If char not found then just take start of the buffer
    if(numStart == NULL)
    {
    	numStart = (char*)&buffer[0];
    }
    else // '_' found to step next char
    {
    	numStart = &numStart[1];
    }
    //Return number
	return(atoi(numStart));
}

