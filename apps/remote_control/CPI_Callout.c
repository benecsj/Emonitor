/***********************************************************************************************************************
Includes
 ***********************************************************************************************************************/
#include "CPI_Command_Processer.h"

#include "project_config.h"

#include "NVM_NonVolatileMemory.h"
#include "spiffs_manager.h"
#include "Wifi_Manager.h"
#include "Sensor_Manager.h"
#include "Emonitor.h"
#include "uart.h"
/***********************************************************************************************************************
Defines
 ***********************************************************************************************************************/


/***********************************************************************************************************************
Global variables and functions
 ***********************************************************************************************************************/

void ICACHE_FLASH_ATTR Cpi_Uart(uint8* params,uint8 lenght, uint8* response) {
	char* text = "/OK/"+0;
	uint8 selection;

    // Process parameters
	selection = params[0];

    //Do stuff
	switch(selection)
	{
	case 'e':
		UART_SetPrintPort(UART0);
		text="/UART_Enabled/";
		break;

	case 'd':
		UART_SetPrintPort(UART_OFF);
		text="/UART_Disabled/";
		break;

	default:
		text="/NOT_OK/"+0;
	}

    //Generate response
    lenght = sprintf((char*) response, text);
    Cpi_SendResponseFrame(lenght, response);
}

void ICACHE_FLASH_ATTR Cpi_Reset(uint8* params,uint8 lenght, uint8* response) {
	//Trigger reset
	Emonitor_Request(EMONITOR_REQ_RESTART);
}

void ICACHE_FLASH_ATTR Cpi_NvM(uint8* params, uint8 lenght, uint8* response) {
	char* text = "/OK/"+0;
	uint8 selection;

    // Process parameters
	selection = params[0];

    //Do stuff
	switch(selection)
	{
	case 'c':
		NvM_RequestClear();
		text="/NvM_RequestClear/";
		break;

	case 'w':
		NvM_RequestSave();
		text="/NvM_RequestSave/";
		break;

	case 'l':
		NvM_RequestLoad();
		text="/NvM_RequestLoad/";
		break;

	default:
		text="/NOT_OK/"+0;
	}

    //Generate response
    lenght = sprintf((char*) response, text);
    Cpi_SendResponseFrame(lenght, response);
}

void ICACHE_FLASH_ATTR Cpi_Wifi(uint8* params, uint8 lenght, uint8* response) {
	char* text = "/OK/"+0;
	uint8 selection;
    uint8 temp[4];
    // Process parameters
	selection = params[0];

    //Do stuff
	switch(selection)
	{
	case 'i':
		Wifi_Manager_Init();
		text="/Wifi_Manager_Init/";
		break;
	case 'a':
		//start_wifi_ap(AP_SSID, AP_PASSWORD);
		text="/start_wifi_ap/";
		break;
	case 's':
		//start_wifi_station(STA_SSID, STA_PASSWORD);
		text="/start_wifi_station/";
		break;
	case 't':
		Wifi_Manager_GetIp(temp,IP);
		sprintf(text,"/IP:%d.%d.%d.%d/",temp[0],temp[1],temp[2],temp[3]);
		break;

	default:
		text="/NOT_OK/"+0;
	}

    //Generate response
    lenght = sprintf((char*) response, text);
    Cpi_SendResponseFrame(lenght, response);
}

void ICACHE_FLASH_ATTR Cpi_Spiffs(uint8* params, uint8 lenght, uint8* response) {
	char* text = "/OK/"+0;
	uint8 selection;

    // Process parameters
	selection = params[0];

    //Do stuff
	switch(selection)
	{
	case 'f':
		spiffs_format();
		text="/spiffs_format/";
		break;
	case 's':
		spiffs_status();
		text="/spiffs_status/";
		break;
	case 'i':
		spiffs_init();
		text="/spiffs_init/";
		break;
	case 'r':
		spiffs_test_read();
		text="/spiffs_test_read/";
		break;
	default:
		text="/NOT_OK/"+0;
	}

    //Generate response
    lenght = sprintf((char*) response, text);
    Cpi_SendResponseFrame(lenght, response);
}


void ICACHE_FLASH_ATTR Cpi_Sensor(uint8* params,uint8 lenght, uint8* response) {
	char* text = "/OK/"+0;
	uint8 selection;

    // Process parameters
	selection = params[0];

    //Do stuff
	switch(selection)
	{
	case 'r':
		Sensor_Manager_ResetPulseCounters();
		text="/Counter Reset/";
		break;

	default:
		text="/NOT_OK/"+0;
	}

    //Generate response
    lenght = sprintf((char*) response, text);
    Cpi_SendResponseFrame(lenght, response);
}

//Process pending operation
