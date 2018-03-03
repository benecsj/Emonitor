/******************************************************************************
* Includes
\******************************************************************************/

//Common
#include "project_config.h"
#if PRJ_ENV == OS
#include "esp_common.h"
#include "esp_system.h"
#else
#include "ets_sys.h"
#include "osapi.h"
#endif

#if PRJ_ENV == NOS
#include "user_interface.h"
#endif

//Libs
#include "pins.h"
#include "uart.h"
#if PRJ_ENV == OS
#include "Wifi_Manager.h"
#include "wifi_state_machine.h"
#include "NVM_NonVolatileMemory.h"
#include "spiffs_manager.h"

#include "httpclient.h"
#include "http_server.h"
#include "esp_libc.h"
#endif

//Apps
#if PRJ_ENV == OS
#include "Emonitor.h"
#include "remote_control.h"
#include "Sensor_Manager.h"
#endif
/******************************************************************************
* Defines
\******************************************************************************/
#define DELAY_MS(a)  	prj_Delay(a);

/******************************************************************************
* Implementations
\******************************************************************************/

#if PRJ_ENV == NOS
void Emonitor_Main_1000ms(void){};
void NVM_Main(void){};
void Sensor_Manager_Main(void){};
void Wifi_Manager_Main(void){};
void Emonitor_IncUptime(void){};
bool Wifi_Manager_IsConnected(void){return 0;};
void Emonitor_Preinit(void){};
void Emonitor_SetResetReason(uint8 a){};
void Emonitor_StartTimer(void){};
void NVM_Init(void){};
void spiffs_init(void){};
void Emonitor_Init(void){};
void Remote_Control_Init(void){};
void Wifi_Manager_Init(void){};
void Sensor_Manager_Init(void){};
void httpclient_Init(void){};
void Http_Server_Init(void){};
void Emonitor_EnableStatusLed(void){};
void Remote_Control_Main(void){};
#endif

/******************************************************************************
* Declarations
\******************************************************************************/
void task_1000ms(void *pvParameters);
void task_10ms(void *pvParameters);
void task_background(void *pvParameters);
void task_1ms(void);
/******************************************************************************
 * FunctionName : user_rf_cal_sector_set
 * Description  : SDK just reversed 4 sectors, used for rf init data and paramters.
 *                We add this function to force users to set rf cal sector, since
 *                we don't know which sector is free in user's application.
 *                sector map for last several sectors : ABCCC
 *                A : rf cal
 *                B : rf init data
 *                C : sdk parameters
 * Parameters   : none
 * Returns      : rf cal sector
 *******************************************************************************/
uint32 user_rf_cal_sector_set(void) {
	prj_flash_size_map size_map = system_get_flash_size_map();
	uint32 rf_cal_sec = 0;

	switch (size_map) {
	case FLASH_SIZE_4M_MAP_256_256:
		rf_cal_sec = 128 - 5;
		break;

	case FLASH_SIZE_8M_MAP_512_512:
		rf_cal_sec = 256 - 5;
		break;

	case FLASH_SIZE_16M_MAP_512_512:
	case FLASH_SIZE_16M_MAP_1024_1024:
		rf_cal_sec = 512 - 5;
		break;

	case FLASH_SIZE_32M_MAP_512_512:
	case FLASH_SIZE_32M_MAP_1024_1024:
		rf_cal_sec = 1024 - 5;
		break;
	case FLASH_SIZE_64M_MAP_1024_1024:
		rf_cal_sec = 2048 - 5;
		break;
	case FLASH_SIZE_128M_MAP_1024_1024:
		rf_cal_sec = 4096 - 5;
		break;
	default:
		rf_cal_sec = 0;
		break;
	}

	return rf_cal_sec;
}

uint32 counter;

/******************************************************************************
* FreeRTOS Tasks
\******************************************************************************/

/*
 * Init task
 */
void task_Init(void *pvParameters) {
	prj_TaskHandle t;
	//--------------------------------
	//Init pins handler
	Init_Pins();
	//Start 1ms task timer
	Emonitor_StartTimer();
	//Init NvM
	NVM_Init();
	//Spiffs init
	spiffs_init();
	//Emonitor
	Emonitor_Init();
	//Init application
    Remote_Control_Init();
	//Init Wifi
	Wifi_Manager_Init();
	//Sensor manager init
	Sensor_Manager_Init();
	//Http client init
	httpclient_Init();
	//--------------------------------
	//Delay init
	DELAY_MS(5000);
	//--------------------------------
	//Http server init
	Http_Server_Init();
	//--------------------------------
	Emonitor_EnableStatusLed();
	//Finished
	DBG("Init finished!!!\n-------------------------\n");
	//--------------------------------
	//Start cyclic tasks
	prj_createTask(task_1000ms, "1000ms", 1024, NULL, 1, &t);
	prj_createTask(task_10ms, "10ms", 512, NULL, (configMAX_PRIORITIES-2), &t);
	prj_createTask(task_background, "bgnd", 512, NULL, 0, &t);
	//Exit from the task
	prj_TaskDelete( NULL );
}

/*
 * Very fast task
 */
void task_1ms(void){
	//------------------
	Emonitor_Main_1ms();
	Sensor_Manager_VeryFast();
	//------------------
}

/*
 * Fast task
 */
void task_10ms(void *pvParameters) {
	uint32 sysTimeMS;
	for (;;) {
		//------------------
		Sensor_Manager_Fast();
		//------------------
		sysTimeMS = system_get_time()/1000;
		DELAY_MS(10-((sysTimeMS)%10));
	}
}

/*
 * Slow task
 */
void task_1000ms(void *pvParameters) {
	uint32 sysTimeMS;
	for (;;) {
		//------------------
		Emonitor_Main_1000ms();
		Remote_Control_Main();
		NVM_Main();
		Sensor_Manager_Main();
		Wifi_Manager_Main();
		//------------------
		sysTimeMS = system_get_time()/1000;
		DELAY_MS(1000-((sysTimeMS)%1000));
		Emonitor_IncUptime();
		//Let Wifi task running to try to connect
		if(Wifi_Manager_IsConnected() == FALSE)
		{
			DELAY_MS((sysTimeMS%1000)+1000);
			Emonitor_IncUptime();
		}
	}
}

/*
 * Background task
 */
void task_background(void *pvParameters) {
	for (;;) {
		Emonitor_Main_Background();
	}
}

/******************************************************************************
 * FunctionName : user_init
 * Description  : entry of user application, init user function here
 * Parameters   : none
 * Returns      : none
 *******************************************************************************/
void user_init(void) {
	//Local variables
	struct rst_info* resetInfo;
	prj_TaskHandle t;

#if PRJ_ENV == NOS
	//Init UART
	UART_SetBaudrate(UART0, BIT_RATE_115200);
	//Init flash button
	pinMode(FLASH_BUTTON,INPUT);

	pinMode(LED_BUILTIN,OUTPUT);
	digitalWrite(LED_BUILTIN,0);
#endif


	//Pre Init main application
	Emonitor_Preinit();

	//Get general statuses
	DBG("\nSDK version:%s\n", system_get_sdk_version());
	system_print_meminfo();
	//Get reset cause
	resetInfo = system_get_rst_info();
	Emonitor_SetResetReason(resetInfo->reason);
	DBG("Reset exccause:%d reason:%d\n",resetInfo->exccause,resetInfo->reason);

	//Case on not external reset wait for reset
	if(REASON_EXT_SYS_RST != resetInfo->reason)	{return;}

	//Start freeRTOS tasks
	DBG("About to create init task\n");
	prj_createTask(task_Init, "init", 1024, NULL, (configMAX_PRIORITIES-1), &t);
}

#if PRJ_ENV == NOS
void ICACHE_FLASH_ATTR
user_rf_pre_init(void)
{
}
#endif
