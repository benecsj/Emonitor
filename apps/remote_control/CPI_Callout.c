/***********************************************************************************************************************
Includes
 ***********************************************************************************************************************/
#include "CPI_Command_Processer.h"

#include "esp_common.h"
#include "NVM_NonVolatileMemory.h"

/***********************************************************************************************************************
Defines
 ***********************************************************************************************************************/


uint8 NVM_test_value;

/***********************************************************************************************************************
Global variables and functions
 ***********************************************************************************************************************/

void Cpi_Test(uint8* params, uint8 lenght, uint8* response) {
	char* text = "/OK/"+0;
	uint8 selection;

    // Process parameters
	selection = params[0];

    //Do stuff
	switch(selection)
	{
	case 0:
		stop_wifi_station();
		text="/stop_wifi_station/";
		break;
	case 1:
		stop_wifi_ap();
		text="/stop_wifi_ap/";
		break;

	case 'p':
		printf("(TEST) NVM_test_value: %d \n");
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

	default:
		text="/NOT_OK/"+0;
	}
    //Generate response
    lenght = sprintf((char*) response, text);
    Cpi_SendResponseFrame(lenght, response);
}


//Process pending operation
