/***********************************************************************************************************************
Includes
 ***********************************************************************************************************************/
#include "CPI_Command_Processer.h"

#include "esp_common.h"
#include "user_config.h"

#include "NVM_NonVolatileMemory.h"
#include "spiffs_manager.h"
#include "Wifi_Manager.h"
#include "Sensor_Manager.h"
#include "Emonitor.h"

/***********************************************************************************************************************
Defines
 ***********************************************************************************************************************/


/***********************************************************************************************************************
Global variables and functions
 ***********************************************************************************************************************/

void Cpi_Reset(uint8* params,uint8 lenght, uint8* response) {
	//Trigger reset
	Emonitor_Request(EMONITOR_REQ_RESTART);
}

void Cpi_NvM(uint8* params, uint8 lenght, uint8* response) {
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

void Cpi_Wifi(uint8* params, uint8 lenght, uint8* response) {
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
		Wifi_Manager_GetIp(temp);
		sprintf(text,"/IP:%d.%d.%d.%d/",temp[0],temp[1],temp[2],temp[3]);
		break;

	default:
		text="/NOT_OK/"+0;
	}

    //Generate response
    lenght = sprintf((char*) response, text);
    Cpi_SendResponseFrame(lenght, response);
}

void Cpi_Spiffs(uint8* params, uint8 lenght, uint8* response) {
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
	case 'w':
		spiffs_test_write();
		text="/spiffs_test_write/";
		break;

	default:
		text="/NOT_OK/"+0;
	}

    //Generate response
    lenght = sprintf((char*) response, text);
    Cpi_SendResponseFrame(lenght, response);
}


void Cpi_Sensor(uint8* params,uint8 lenght, uint8* response) {
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
