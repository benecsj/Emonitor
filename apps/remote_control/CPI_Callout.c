/***********************************************************************************************************************
Includes
 ***********************************************************************************************************************/
#include "CPI_Command_Processer.h"

#include "esp_common.h"

/***********************************************************************************************************************
Defines
 ***********************************************************************************************************************/


uint8 NVM_test_value;

/***********************************************************************************************************************
Global variables and functions
 ***********************************************************************************************************************/

void Cpi_Test(uint8* params, uint8 lenght, uint8* response) {
	char* text;
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
	default:
		text="/NOT_OK/";
	}
    //Generate response
    lenght = sprintf((char*) response, text);
    Cpi_SendResponseFrame(lenght, response);
}


//Process pending operation
