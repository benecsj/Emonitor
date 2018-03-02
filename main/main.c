/******************************************************************************
* Includes
\******************************************************************************/

//Common
#include "esp_common.h"
#include "esp_system.h"
#include "project_config.h"

#if PRJ_ENV == NOS
#include "user_interface.h"
#endif

//Libs
#include "Wifi_Manager.h"
#include "wifi_state_machine.h"
#include "NVM_NonVolatileMemory.h"
#include "spiffs_manager.h"
#include "pins.h"
#include "httpclient.h"
#include "http_server.h"
#include "esp_libc.h"

//Apps
#include "Emonitor.h"
#include "remote_control.h"
#include "Sensor_Manager.h"

/******************************************************************************
* Defines
\******************************************************************************/
#define DELAY_MS(a)  	prj_Delay(a);

/******************************************************************************
* Implementations
\******************************************************************************/

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
	DELAY_MS(10);
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
	//Delay Http server init
	DELAY_MS(5000);
	//--------------------------------
	//Http server init
	Http_Server_Init();
	//--------------------------------
	//Finished
	DBG("Init finished!!!\n-------------------------\n");
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
	DELAY_MS(1000);
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
	DELAY_MS(4000);
	Emonitor_EnableStatusLed();
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
	DELAY_MS(100);
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
	DBG("About to create task\n");
	prj_createTask(task_Init, "init", 1024, NULL, (configMAX_PRIORITIES-1), &t);
	prj_createTask(task_1000ms, "1000ms", 1024, NULL, 1, &t);
	prj_createTask(task_10ms, "10ms", 512, NULL, (configMAX_PRIORITIES-2), &t);
	prj_createTask(task_background, "bgnd", 512, NULL, 0, &t);
}

