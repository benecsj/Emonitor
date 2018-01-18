/******************************************************************************
* Includes
\******************************************************************************/

//Common
#include "esp_common.h"
#include "project_config.h"
#include "user_config.h"

//OS
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

//Libs
#include "Wifi_Manager.h"
#include "wifi_state_machine.h"
#include "NVM_NonVolatileMemory.h"
#include "spiffs_manager.h"
#include "pins.h"
#include "httpclient.h"
#include "http_server.h"

//Apps
#include "Emonitor.h"
#include "remote_control.h"
#include "Sensor_Manager.h"

/******************************************************************************
* Defines
\******************************************************************************/

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
	flash_size_map size_map = system_get_flash_size_map();
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
	vTaskDelay(100 / portTICK_RATE_MS);
	//Init pins handler
	Init_Pins();
	//Init NvM
	NVM_Init();
	//Init application
    Remote_Control_Init();
	//Init Wifi
	Wifi_Manager_Init();
	start_wifi_station(STA_SSID, STA_PASSWORD);
	//Spiffs init
	spiffs_init();
	//Sensor manager init
	Sensor_Manager_Init();
	//Http client init
	httpclient_Init();
	//Http server init
	Http_Server_Init();
	//Finished
	DBG("Init finished!!!\n-------------------------\n");
	//Exit from the task
	vTaskDelete( NULL );
}

/*
 * Fast task
 */
void task_10ms(void *pvParameters) {
	vTaskDelay(1000 / portTICK_RATE_MS);
	for (;;) {
		Sensor_Manager_Fast();
		vTaskDelay(10 / portTICK_RATE_MS);
	}
}

/*
 * Slow task
 */
void task_1000ms(void *pvParameters) {
	vTaskDelay(1000 / portTICK_RATE_MS);
	for (;;) {

		Emonitor_Main_1000ms();
		Remote_Control_Main();
		NVM_Main();
		Sensor_Manager_Main();

		vTaskDelay(1000 / portTICK_RATE_MS);
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
	//Init application
	Emonitor_Init();

	DBG("SDK version:%s\n", system_get_sdk_version());
	DBG("About to create task\r\n");
	xTaskHandle t;
	xTaskCreate(task_background, "bgnd", 1024, NULL, 0, &t);
	xTaskCreate(task_10ms, "10ms", 1024, NULL, 0, &t);
	xTaskCreate(task_1000ms, "1000ms", 2048, NULL, 0, &t);
	xTaskCreate(task_Init, "init", 2048, NULL, 1, &t);
}

