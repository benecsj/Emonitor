/******************************************************************************
* Includes
\******************************************************************************/

//Common
#include "project_config.h"

#if PRJ_ENV == NOS
#include "user_interface.h"
#endif

//Libs
#include "pins.h"
#include "MHZ14.h"
#include "uart.h"
#include "spiffs_manager.h"
#include "NVM_NonVolatileMemory.h"
#include "D18_DS18B20_Temp_Sensor.h"
#include "OWP_One_Wire_Protocol_Driver.h"
#include "hw_timer.h"

#if PRJ_ENV == OS
#include "Wifi_Manager.h"
#include "wifi_state_machine.h"



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

#if DEBUG_MAIN
#define DBG_MAIN(...) prj_printf(__VA_ARGS__)
#else
#define DBG_MAIN(...)
#endif

/******************************************************************************
* Implementations
\******************************************************************************/

#if PRJ_ENV == NOS
void Emonitor_Main_1000ms(void){};
void Sensor_Manager_Main(void){};
void Emonitor_IncUptime(void){};
bool Wifi_Manager_IsConnected(void){return 0;};
void Emonitor_Preinit(void){};
void Emonitor_SetResetReason(uint8 a){};
void Emonitor_StartTimer(void){};
void Emonitor_Init(void){};
void Remote_Control_Init(void){};
void Sensor_Manager_Init(void){};
void httpclient_Init(void){};
void Http_Server_Init(void){};
void Emonitor_EnableStatusLed(void){};
void Remote_Control_Main(void){};
void Sensor_Manager_Fast(void){};
void Emonitor_Main_Background(void){};
void Emonitor_Main_1ms(void){};
void Sensor_Manager_VeryFast(void){};

void Wifi_Manager_Init(void){};
void Wifi_Manager_Main(void){};

uint8 Emonitor_ledControl;
uint8 WifiManager_enableHotspot;
uint32 Emonitor_nodeId;
uint32 Emonitor_SendPeroid;
char Emonitor_url[100];
char Emonitor_key[33];
char WifiManager_STA_SSID[33];
char WifiManager_STA_PASSWORD[65];
char WifiManager_AP_SSID[33];
char WifiManager_AP_PASSWORD[65];

#endif

/******************************************************************************
* Declarations
\******************************************************************************/
TASK(task_10ms);
TASK(task_1000ms);
TASK(task_background);
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
uint32 ICACHE_FLASH_ATTR user_rf_cal_sector_set(void) {
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
void ICACHE_FLASH_ATTR task_InitA(void)
{
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
}
void ICACHE_FLASH_ATTR task_InitB(void)
{
	prj_TaskHandle t;
	//--------------------------------
	//Http server init
	Http_Server_Init();
	//--------------------------------
	Emonitor_EnableStatusLed();
	//Finished
	DBG_MAIN("Init finished!!!\n-------------------------\n");
	//--------------------------------
	//Start cyclic tasks
	prj_createTask(task_1000ms, "1000ms", 1024, NULL, 1, &t);
	prj_createTask(task_10ms, "10ms", 512, NULL, (configMAX_PRIORITIES-2), &t);
	prj_createTask(task_background, "bgnd", 512, NULL, 0, &t);
}
TASK(task_Init){
	//Init phase A
	task_InitA();

	//Delay init
	DELAY_MS(5000);

	//Init phase B
	task_InitB();

	//Exit from the task
	prj_TaskDelete( NULL );
}

/*
 * Very fast task
 */
void ICACHE_FLASH_ATTR task_1ms(void){
	//------------------
	Emonitor_Main_1ms();
	Sensor_Manager_VeryFast();
	//------------------
}

/*
 * Fast task
 */
TASK(task_10ms) {
	uint32 sysTimeMS;
	do{
		//------------------
		Sensor_Manager_Fast();
		//------------------
		sysTimeMS = system_get_time()/1000;
		DELAY_MS(10-((sysTimeMS)%10));
	}LOOP
}

/*
 * Slow task
 */
TASK(task_1000ms) {
	uint32 sysTimeMS;
	do{
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
	}LOOP
}

/*
 * Background task
 */
TASK(task_background) {
	do{
		Emonitor_Main_Background();
	}LOOP
}

#if PRJ_ENV == NOS

typedef enum
{
	FIRST_CALL,
	INIT_A,
	INIT_B,
	DELAY,
	CYCLIC
} TimingTask_State;

void ICACHE_FLASH_ATTR user_rf_pre_init(void){}

uint32_t a; //TEST
uint32_t testCounter=0; //TEST
os_event_t taskQueue;
uint32_t lastCallTime;
uint32_t taskTimingCounters[3] = {0};
TimingTask_State timingState = FIRST_CALL;

/*
 * NONOS timing task
 */
static void ICACHE_FLASH_ATTR task_timing(os_event_t *events) {
	//Get currentime
	uint32_t currentTime = system_get_time()/1000;
	//Based on state do stuff
	switch(timingState)
	{
	case FIRST_CALL:
		//Do nothing
		timingState = INIT_A;
		break;
	case INIT_A:
		//Call init task
		task_InitA();
		timingState = DELAY;
		break;
	case DELAY:
		//Delay before starting cyclic tasks
		taskTimingCounters[0]+= (currentTime-lastCallTime);
		if(taskTimingCounters[0] >= 5){
			taskTimingCounters[0] = 0;
			timingState = INIT_B;
		}
		break;
	case INIT_B:
		//Call init task
		task_InitB();
		timingState = CYCLIC;
		break;
	case CYCLIC:
		//increment task timing counter
		taskTimingCounters[0]+= (currentTime-lastCallTime);
		taskTimingCounters[1]+= (currentTime-lastCallTime);
		taskTimingCounters[2]+= (currentTime-lastCallTime);
		//Call tasks if needed
		if(taskTimingCounters[0] >= 1){
			taskTimingCounters[0] = 0;
			task_1ms();
		}
		if(taskTimingCounters[1] >= 10){
			taskTimingCounters[1] = 0;
			task_10ms();

		    // TEST
			a++;
			digitalWrite(LED_BUILTIN,(a%2));

		}
		if(taskTimingCounters[2] >= 1000){
			taskTimingCounters[2] = 0;
			task_1000ms();
			DBG("CALLBACK RECEIVED : %d \n" ,testCounter);
			testCounter = 0;
		}
	    //call background task
	    task_background();

		break;
	}
    //Store last call time
    lastCallTime = currentTime;
    //Cyclic task trigger
    system_os_post(0, 0, 0 );
}

void ICACHE_FLASH_ATTR testCallback(void)
{
	testCounter++;
}

#endif

/******************************************************************************
 * FunctionName : user_init
 * Description  : entry of user application, init user function here
 * Parameters   : none
 * Returns      : none
 *******************************************************************************/
void ICACHE_FLASH_ATTR user_init(void) {
	//Local variables
	struct rst_info* resetInfo;
	prj_TaskHandle t;

#if PRJ_ENV == NOS
    uint8 id[OWP_CONST_ROMCODE_SIZE];
    uint8 diff;

	//Init UART
	UART_SetBaudrate(UART0, BIT_RATE_115200);
	//Delay
	os_delay_us(10000);
	DBG("\n----------------------------------------------\n");
	//Init flash button
	pinMode(FLASH_BUTTON,INPUT);

	pinMode(LED_BUILTIN,OUTPUT);
	digitalWrite(LED_BUILTIN,1);

	pinMode(PULSE_INPUT0,INPUT);
	attachInterrupt(PULSE_INPUT0,testCallback,RISING);
/*
	MHZ14_Init();
	MHZ14_Main();
	MHZ14_Feed(0);
	OWP_SelectChannel(0);
	D18_DS18B20_FindSensor(&diff, &id[0]);
	os_printf("ID: %02X%02X%02X%02X%02X%02X%02X%02X\n",id[0],id[1],id[2],id[3],id[4],id[5],id[6],id[7]);
*/
    hw_timer_init(NMI_SOURCE,1);
    hw_timer_set_func(testCallback);
    hw_timer_arm(1000,1);
#endif

	//Pre Init main application
	Emonitor_Preinit();

	//Get general statuses
	DBG_MAIN("\nSDK version:%s\n", system_get_sdk_version());
#if DEBUG_MAIN
	system_print_meminfo();
#endif
	//Get reset cause
	resetInfo = system_get_rst_info();
	Emonitor_SetResetReason(resetInfo->reason);
	DBG_MAIN("Reset exccause:%d reason:%d\n",resetInfo->exccause,resetInfo->reason);

	//Case on not external reset wait for reset
	if(REASON_EXT_SYS_RST != resetInfo->reason)	{return;}

	//Start freeRTOS tasks
	DBG_MAIN("About to create init task\n");
	prj_createTask(task_Init, "init", 1024, NULL, (configMAX_PRIORITIES-1), &t);

#if PRJ_ENV == NOS
	//START Emonitor handling task
    system_os_task(task_timing, 0, &taskQueue, 1);
    system_os_post(0, 0, 0 );
#endif
}

