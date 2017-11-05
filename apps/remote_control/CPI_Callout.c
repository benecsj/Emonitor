/***********************************************************************************************************************
Includes
 ***********************************************************************************************************************/
#include "CPI_Command_Processer.h"

#include "esp_common.h"
#include "user_config.h"

#include "NVM_NonVolatileMemory.h"
#include "spiffs_manager.h"
#include "Wifi_Manager.h"

/***********************************************************************************************************************
Defines
 ***********************************************************************************************************************/

uint8 NVM_test_value;

/***********************************************************************************************************************
Global variables and functions
 ***********************************************************************************************************************/

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
		NVM_test_value = params[1];
		NvM_RequestSave();
		text="/NvM_RequestSave/";
		break;

	case 'l':
		NvM_RequestLoad();
		text="/NvM_RequestLoad/";
		break;

	case 'p':
		printf("(TEST) NVM_test_value: %d \n",NVM_test_value);
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
		start_wifi_ap(AP_SSID, AP_PASSWORD);
		text="/start_wifi_ap/";
		break;
	case 's':
		start_wifi_station(STA_SSID, STA_PASSWORD);
		text="/start_wifi_station/";
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


//Process pending operation
