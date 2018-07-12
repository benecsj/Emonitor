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
#include "esp_fs.h"

/******************************************************************************
* Defines
\******************************************************************************/
#if DEBUG_EMONITOR == 1
#define DBG_EMON(...) printf(__VA_ARGS__)
#define DBG2_EMON(...) printf(__VA_ARGS__)
#else
#define DBG_EMON(...)
#endif

#ifndef DBG2_EMON
#if DEBUG_EMONITOR == 2
#define DBG2_EMON(...) printf(__VA_ARGS__)
#else
#define DBG2_EMON(...)
#endif
#endif

#define APPEND(...)				  length += sprintf(&buffer[length],__VA_ARGS__)

#define PRINT_TEMPID "%02X%02X%02X%02X%02X"
#define PRINT_TEMP	 "%c%d.%d"
#define JSON_START	 "&json={"
#define JSON_NEXT	 ","
#define JSON_DIV	 ":"
#define JSON_END	 "}"
#define READ_ID		 ids[(i*8)+1],ids[(i*8)+2],ids[(i*8)+3],ids[(i*8)+4],ids[(i*8)+7]
#define READ_TEMP	 sign,abs(temperatures[i]/10),abs(temperatures[i]%10)

#define EMONITOR_EMPTYCOUNT 550000
#define EMONITOR_TOTALRAM	81920

/******************************************************************************
* Variables
\******************************************************************************/

//Timing and counters
uint32 Emonitor_backgroundCounter = 0;
uint32 Emonitor_LEDCounter = 0;
uint32 Emonitor_sendTimer = 0;
uint32 Emonitor_responseTimer = 0;
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
char Emonitor_version[20] = {0};
uint32 Emonitor_SendPeroid = 0;

/******************************************************************************
* Primitives
\******************************************************************************/
extern void ICACHE_FLASH_ATTR Emonitor_callback(char * response_body, int http_status, char * response_headers, int body_size);
extern uint32_t Emonitor_GetDefaultId(void);
extern void Emonitor_GetDefaultUrl(char* buffer);
extern void task_1ms(void);
extern void task_500us(void);
extern uint32 Emonitor_GetBackgroundRuntime(void);
extern void ICACHE_FLASH_ATTR Emonitor_UpdateVersion(char* version);
/******************************************************************************
* Implementations
\******************************************************************************/

/******************************************************************************
 * FunctionName : Emonitor_Preinit
 * Description  : Init emonitor application
 * Parameters   : none
 * Returns      : none
 *******************************************************************************/
void ICACHE_FLASH_ATTR Emonitor_Preinit(void) {

	//Init UART
	UART_SetBaudrate(UART_CHANNEL,UART_BAUDRATE);
	UART_SetPrintPort(UART_CHANNEL);
	prj_printf("\n");
	//Init flash button
	pinMode(FLASH_BUTTON,INPUT);
    //Disable Wifi
    init_esp_wifi();
}

void ICACHE_FLASH_ATTR Emonitor_EnableStatusLed(void) {
	pinMode(LED_BUILTIN,OUTPUT);
	digitalWrite(LED_BUILTIN,1);
}

void ICACHE_FLASH_ATTR Emonitor_Init(void){
	DBG2_EMON("(EM) Emonitor_Init\n");
    //Check if has valid url and key
	uint8 urlLength = strlen(Emonitor_url);
	uint8 apiKeyLength = strlen(Emonitor_key);
	if ((urlLength <= URL_MIN_LENGTH) || (urlLength == URL_MAX_LENGTH))
	{
		Emonitor_GetDefaultUrl(Emonitor_url);
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
	//Get SW version
	Emonitor_UpdateVersion((char*)&Emonitor_version);
	//Start with a send to update connection status right away
	Emonitor_sendTimer = Emonitor_SendPeroid;
}

void ICACHE_FLASH_ATTR Emonitor_StartTimer(void){
	//Init timer for fast task
    hw_timer_init(FRC1_SOURCE,TRUE);
    hw_timer_set_func(task_500us);
    hw_timer_arm(500,1);
}

/******************************************************************************
 * FunctionName : Emonitor_Main_1ms
 * Description  : Emonitor Fast Main
 * Parameters   : none
 * Returns      : none
 *******************************************************************************/
void IRAM0 Emonitor_Main_1ms(void) {
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
		ledValue = (Emonitor_LEDCounter <(LED_TIMING_NORMAL-LED_STRENGTH));
		if((Emonitor_LEDCounter <= (LED_TIMING_NORMAL-150)) && (Emonitor_LEDCounter >= (LED_TIMING_NORMAL-150 - LED_STRENGTH)) && (Emonitor_connectionStatus == EMONITOR_CONNECTED))
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
	digitalWrite(LED_BUILTIN, (ledValue) == STATUS_LED_INVERT);

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
void ICACHE_FLASH_ATTR Emonitor_Main_1000ms(void) {
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
	Wifi_Manager_GetIp(ip,IP);
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
	DBG2_EMON("(EM) U(%d) T(%d) I(%d) C(%d) H(%d) S(%d) R(%d) S(%d) H(%d)\n", Emonitor_uptime,Emonitor_sendTimer,ip[3],Emonitor_connectionCounter,Emonitor_freeRam,freeStack,Emonitor_backgroundRuntime,Wifi_Manager_GetSignalLevel(),Sensor_Manager_GetTempHealth());

	//Send timing
	if((Emonitor_sendTimer+1) >= Emonitor_SendPeroid){
		//Check if got network connection
		if(Wifi_Manager_IsConnected())
		{
			//Increment response timer
			Emonitor_responseTimer++;
			//Check if not waiting for a repsonse
			if(Emonitor_responseTimer == 1)
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
					DBG_EMON("(EM) Report Data\n");
					Sensor_Manager_Get_TempSensorData(&tempCount,&ids,&temperatures);
					//Start of Emoncsm send Url
					APPEND("%s/input/post.json?node=%d",Emonitor_url,Emonitor_nodeId);
					//\\\\\\\\\\\\\\\\\\
					//Start Json frame
					APPEND(JSON_START);
					//\\\\\\\\\\\\\\\\\\\\\\\\\\\
					//Add pulse counters
					for(i = 0 ; i < SENSOR_MANAGER_PULSE_COUNTERS ; i++ ){
						APPEND("Pulse_%02X" 		JSON_DIV 	"%d" 		JSON_NEXT	,(i+1),Sensor_Manager_GetPulseCount(i));
						APPEND("Level_%02X" 		JSON_DIV 	"%d" 		JSON_NEXT	,(i+1),Sensor_Manager_GetPulseLevel(i));}


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
					//Add ip (only single time per connection)
					if (Emonitor_connectionStatus == EMONITOR_NOT_CONNECTED) {
						APPEND("Sys_Ip" 			JSON_DIV 	"%d" 		JSON_NEXT	,ip[3]);}
					//Add ram usage
						APPEND("Sys_Ram" 			JSON_DIV 	"%d"		JSON_NEXT	,Emonitor_GetRAMUsage());
					//Add cpu usage
						APPEND("Sys_Cpu" 			JSON_DIV 	"%d"		JSON_NEXT	,Emonitor_GetCpuUsage());
					//Add Uptime
						APPEND("Sys_Uptime" 		JSON_DIV 	"%d"		/*LAST*/	,Emonitor_GetUptime());
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
			else // Waiting for http response
			{
				//Check if not timed out
				if(Emonitor_responseTimer> RESPONSE_TIME_OUT)
				{
					Emonitor_responseTimer = 0;
					//Clear last http request
					httpclient_Cleanup();
				}
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
			Emonitor_Request(EMONITOR_REQ_RESTART_DELAY);
		}
		break;
	case EMONITOR_REQ_RESTART_DELAY:
		DBG_EMON("(EM) RESTART REQUESTED\n");
		DBG_EMON("(EM) ###################################\n");
		DBG_EMON("(EM) ###################################\n");
		//Disable status led, this will trigger EXT reset
		pinMode(LED_BUILTIN,INPUT);
		//Do nothing
		while(1){}
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
	// other case limit max
	default:
		if(TIMEOUT_TURNOFF_APP < Emonitor_connectionCounter)
		{
			Emonitor_connectionCounter = (TIMEOUT_TURNOFF_APP);
		}
		break;
	}
}

/******************************************************************************
 * FunctionName : Emonitor_Main_Background
 * Description  : Emonitor Background Main
 * Parameters   : none
 * Returns      : none
 *******************************************************************************/
void ICACHE_FLASH_ATTR Emonitor_Main_Background(void) {
	//Just measure background runtime
	prj_ENTER_CRITICAL();
	Emonitor_backgroundCounter++;
	prj_EXIT_CRITICAL();
}

uint32 ICACHE_FLASH_ATTR Emonitor_GetBackgroundRuntime(void)
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

uint32_t ICACHE_FLASH_ATTR Emonitor_GetCpuUsage(void)
{
	uint32_t countUsage = EMONITOR_EMPTYCOUNT - Emonitor_GetBackgroundCount();
	countUsage = ((double)countUsage/(double)EMONITOR_EMPTYCOUNT)*100;
	if(countUsage > 100) {countUsage = 100;}
	return countUsage;
}

uint32_t ICACHE_FLASH_ATTR Emonitor_GetRAMUsage(void)
{
	uint32_t usedRAM = (EMONITOR_TOTALRAM -Emonitor_GetFreeRam())/1024;
	uint32_t freeRAM = (Emonitor_GetFreeRam())/1024;
	uint32_t usagePercent = ((double)(usedRAM)/(double)(usedRAM+freeRAM))*100;
	return usagePercent;
}

void ICACHE_FLASH_ATTR Emonitor_callback(char * response_body, int http_status, char * response_headers, int body_size)
{
	DBG_EMON("(EM) Send status=%d\n", http_status);
	//Check for generic HTTP error
	if (http_status != HTTP_STATUS_GENERIC_ERROR) {
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
	//Clear response timer
	Emonitor_responseTimer = 0;
}

uint32_t ICACHE_FLASH_ATTR Emonitor_GetDefaultId(void)
{
	char buffer[64] = {0};
	char *numStart;

	esp_fs_file file;

	//Get id string from filesystem storage
	esp_fs_OpenFile(&file,"/id");
	esp_fs_ReadFile(&file, (u8_t *)buffer, 64);
	esp_fs_CloseFile(&file);

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

void ICACHE_FLASH_ATTR Emonitor_GetDefaultUrl(char* url)
{
	char buffer[100] = {0};
	int bufferLength = 0;

	esp_fs_file file;

	//Get url string from filesystem storage
	esp_fs_OpenFile(&file,"/server");
	esp_fs_ReadFile(&file, (u8_t *)buffer, 100);
	esp_fs_CloseFile(&file);
	//get url length
	bufferLength = strlen(buffer);
	//Check if valid url was extracted
	if ((bufferLength >= URL_MIN_LENGTH) || (bufferLength <= URL_MAX_LENGTH))
	{
		sprintf(url,"%s",buffer);
	}
	else // No valid default url found
	{
		sprintf(url,"http://");
	}

}

void ICACHE_FLASH_ATTR Emonitor_UpdateVersion(char* version)
{
	esp_fs_file file;
	s32_t length = 0;

	//Get sw version from filestorage
	esp_fs_OpenFile(&file,"/version");

	length = esp_fs_ReadFile(&file, (u8_t *)version, 20);
	esp_fs_CloseFile(&file);
	version[length] = 0;
}
