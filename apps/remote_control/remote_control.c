/******************************************************************************
* Includes
\******************************************************************************/

#include "remote_control.h"
#include "esp_common.h"
#include "uart.h"

#include "Shell.h"

#include "NVM_NonVolatileMemory.h"
#include "esp_fs.h"
#include "Wifi_Manager.h"
#include "Sensor_Manager.h"
#include "Emonitor.h"
#include "http_server.h"

/******************************************************************************
* Defines
\******************************************************************************/
#define PRINT_TEMPID "%02X%02X%02X%02X%02X"
#define PRINT_TEMP	 "%c%d.%d"
#define READ_ID		 ids[(i*8)+1],ids[(i*8)+2],ids[(i*8)+3],ids[(i*8)+4],ids[(i*8)+7]
#define READ_TEMP	 sign,abs(temperatures[i]/10),abs(temperatures[i]%10)
/******************************************************************************
* Variables
\******************************************************************************/

uint8 remote_readIndex = 0;
uint8 remote_writeIndex = 0;
uint8 remote_recBuffer[REMOTE_MAX_INPUT];

/******************************************************************************
* Primitives
\******************************************************************************/

void Remote_RegisterCommands(void);
void Remote_UnregisterCommands(void);

/******************************************************************************
* Implementations
\******************************************************************************/
LOCAL void ICACHE_FLASH_ATTR
Remote_Control_UART0_Handler(void *para)
{
    uint8 received;
    uint8 uart_no = UART0;
    uint8 buf_idx = 0;
    uint8 recLength;

    //Read status
    uint32 uart_intr_status = READ_PERI_REG(UART_INT_ST(uart_no)) ;
    //Process while have interrupts
    while (uart_intr_status != 0x0) {
    	//UART ERROR
        if (UART_FRM_ERR_INT_ST == (uart_intr_status & UART_FRM_ERR_INT_ST)) {
            WRITE_PERI_REG(UART_INT_CLR(uart_no), UART_FRM_ERR_INT_CLR);
        //UART RX
        } else if ( (UART_RXFIFO_TOUT_INT_ST == (uart_intr_status & UART_RXFIFO_TOUT_INT_ST)) ||
        		    (UART_RXFIFO_FULL_INT_ST == (uart_intr_status & UART_RXFIFO_FULL_INT_ST)) )  {
        	//Get number of received bytes
        	recLength = (READ_PERI_REG(UART_STATUS(UART0)) >> UART_RXFIFO_CNT_S)&UART_RXFIFO_CNT;
        		//Read out all received bytes
				for (buf_idx = 0 ; buf_idx < recLength ; buf_idx++) {
					//Get it from Uart FIFO
					received = (READ_PERI_REG(UART_FIFO(UART0)) & 0xFF);
					remote_recBuffer[remote_writeIndex]= received;
					remote_writeIndex = (remote_writeIndex +1) % REMOTE_MAX_INPUT;
				}
        	//Clear all interrupta
            WRITE_PERI_REG(UART_INT_CLR(UART0), UART_RXFIFO_TOUT_INT_CLR);
            WRITE_PERI_REG(UART_INT_CLR(UART0), UART_RXFIFO_FULL_INT_CLR);
        //UART TX
        } else if (UART_TXFIFO_EMPTY_INT_ST == (uart_intr_status & UART_TXFIFO_EMPTY_INT_ST)) {
            WRITE_PERI_REG(UART_INT_CLR(uart_no), UART_TXFIFO_EMPTY_INT_CLR);
            CLEAR_PERI_REG_MASK(UART_INT_ENA(UART0), UART_TXFIFO_EMPTY_INT_ENA);
        //OTHER
        } else {
            //Clear all unused flags
        	 CLEAR_PERI_REG_MASK(UART_INT_ENA(UART0), 0x1FF);
        }
        //Read status
        uart_intr_status = READ_PERI_REG(UART_INT_ST(uart_no)) ;
    }
}

/**
 * Function to read data from serial port
 * Functions to read from physical media should use this prototype:
 * int my_reader_function(char * data)
 */
int uart0_read_char(char * data)
{
  // Wrapper for Serial.read() method
  if (remote_readIndex != remote_writeIndex) {
    *data = remote_recBuffer[remote_readIndex];
    remote_readIndex = (remote_readIndex +1) % REMOTE_MAX_INPUT;
    return 1;
  }
  return 0;
}

/******************************************************************************
* Commands
\******************************************************************************/

#if REMOTE_LOGIN_NEEDED == ON
int ICACHE_FLASH_ATTR Command_Login(int argc, char** argv) {
	char* text = "login error";
	char* parameter;

	//Check if password is needed
	if(strlen(Wifi_Manager_GetAP_PASSWORD()) != 0)
	{
		//Check argument count
		if(argc == 2)
		{
			// Process parameters
			parameter = argv[1];

			//Check password
			if(strcmp(parameter,Wifi_Manager_GetAP_PASSWORD())==0)
			{
				Remote_RegisterCommands();
				UART_SetPrintPort(UART0);
				text="Welcome";
			}
		}
	}
	else
	{
		Remote_RegisterCommands();
		UART_SetPrintPort(UART0);
		text="Welcome";
	}
    //Generate response
	prj_printf("%s\n", text);
    return SHELL_RET_SUCCESS;
}

int ICACHE_FLASH_ATTR Command_Exit(int argc, char** argv) {
	char* text = "Bye"+0;

	Remote_UnregisterCommands();
    //Generate response
	prj_printf("%s\n", text);
	UART_SetPrintPort(UART_OFF);
    return SHELL_RET_SUCCESS;
}
#endif

int ICACHE_FLASH_ATTR Command_Reset(int argc, char** argv) {
	//Trigger reset
	Emonitor_Request(EMONITOR_REQ_RESTART);
	return SHELL_RET_SUCCESS;
}

int ICACHE_FLASH_ATTR Command_PrintAll(int argc, char** argv) {
	//Show commands
	prj_printf("command list:\n");
	shell_print_commands();
	return SHELL_RET_SUCCESS;
}

int ICACHE_FLASH_ATTR Command_Config(int argc, char** argv) {
	char* text = "error, use parameter: clear | save | load";
	char* parameter;

	//Check argument count
	if(argc == 2)
	{
		// Process parameters
		parameter = argv[1];

		//Do stuff
		if(strcmp(parameter,"clear")==0)
		{
			NvM_RequestClear();
			text="config cleared";
		}
		else if(strcmp(parameter,"save")==0)
		{
			NvM_RequestSave();
			text="config stored";
		}
		else if(strcmp(parameter,"load")==0)
		{
			NvM_RequestLoad();
			text="config reloaded";
		}
	}
    //Generate response
    prj_printf("%s\n", text);
    return SHELL_RET_SUCCESS;
}

int ICACHE_FLASH_ATTR Command_Wifi(int argc, char** argv) {
	char* text = "error, use parameter: init | ip | hotspot | connect | level";
	char* parameter;
	char* parameter2;
    uint8 temp[4];

	//Check argument count
	if(argc >= 2)
	{
		// Process parameters
		parameter = argv[1];

		//Do stuff
		if(strcmp(parameter,"init")==0)
		{
			Wifi_Manager_Init();
			text="wifi reinitialized";
		}
		else if(strcmp(parameter,"ip")==0)
		{
			Wifi_Manager_GetIp(temp,IP);
			printf("ip:[%d.%d.%d.%d]",temp[0],temp[1],temp[2],temp[3]);
			text="";
		}
		else if(strcmp(parameter,"hotspot")==0)
		{
			parameter = argv[2];
			//Check if second parameter received
			if(argc == 3)
			{
				 if(strcmp(parameter,"on")==0)
				 {
					Wifi_Manager_EnableHotspot(TRUE);
					text="wifi hotspot enabled";
				 }
				 else if(strcmp(parameter,"off")==0)
				 {
					Wifi_Manager_EnableHotspot(FALSE);
					text="wifi hotspot disabled";
				 }
				 else
				 {
					if(strlen(parameter)<=32)
					{
						Wifi_Manager_SetAP_SSID(parameter);
						Wifi_Manager_SetAP_PASSWORD("");
						printf("new SSID:[%s] PASS:[]\n",parameter);
						text="wifi hotspot reconfigured";
					}
					else
					{
						text="Error: too long SSID must be smaller than 33 character";
					}
				 }
			}
			else if(argc == 4)
			{
				parameter = argv[2];
				parameter2 = argv[3];
				if((strlen(parameter2)<=63) && (strlen(parameter)<=32))
				{
					Wifi_Manager_SetAP_SSID(parameter);
					Wifi_Manager_SetAP_PASSWORD(parameter2);
					printf("new SSID:[%s] PASS:[%s]\n",parameter,parameter2);
					text="wifi hotspot reconfigured";
				}
				else
				{
					text="Error: too long parameter";
				}
			}
			else
			{
				printf("current SSID:[%s] PASS:[%s] Enable:",Wifi_Manager_GetAP_SSID(),Wifi_Manager_GetAP_PASSWORD());
				if(Wifi_Manager_GetEnableHotspot())
				{
					printf("[on]");
				}
				else
				{
					printf("[off]");
				}
				text = "";
			}
		}
		else if(strcmp(parameter,"connect")==0)
		{
			parameter = argv[2];
			//Check if second parameter received
			if(argc == 3)
			{
				if(strlen(parameter)<=32)
				{
					Wifi_Manager_SetSTA_SSID(parameter);
					Wifi_Manager_SetSTA_PASSWORD("");
					printf("new SSID:[%s] PASS:[]\n",parameter);
					text="wifi connection reconfigured";
				}
				else
				{
					text="Error: too long SSID muss be smaller than 33 character";
				}
			}
			else if(argc == 4)
			{
				parameter2 = argv[3];
				if((strlen(parameter2)<=63) && (strlen(parameter)<=32))
				{

					Wifi_Manager_SetSTA_SSID(parameter);
					Wifi_Manager_SetSTA_PASSWORD(parameter2);
					printf("new SSID:[%s] PASS:[%s]\n",parameter,parameter2);
					text="wifi connection reconfigured";
				}
				else
				{
					text="Error: too long parameter";
				}
			}
			else
			{
				printf("current SSID:[%s] PASS:[%s]",Wifi_Manager_GetSTA_SSID(),Wifi_Manager_GetSTA_PASSWORD());
				text = "";
			}
		}
		else if(strcmp(parameter,"level")==0)
		{
			printf("RSSI:[%d]",Wifi_Manager_GetSignalLevel());
			text = "";
			Wifi_Manager_UpdateLevel();
		}
	}
    //Generate response
	prj_printf("%s\n", text);
    return SHELL_RET_SUCCESS;
}

int ICACHE_FLASH_ATTR Command_Emon(int argc, char** argv) {
	char* text = "error, use parameter: key | url | nodeid | timing | lang";
	char* parameter;
	char* parameter2;
	int32_t number;
    uint8 temp[4];

	//Check argument count
	if(argc >= 2)
	{
		// Process parameters
		parameter = argv[1];

		//Do stuff
		if(strcmp(parameter,"key")==0)
		{
			parameter = argv[2];
			//Check if second parameter received
			if(argc == 3)
			{
				if(strlen(parameter)<=32)
				{
					Emonitor_SetKey(parameter);
					printf("new key:[%s]\n",parameter);
					text="emoncms key reconfigured";
				}
				else
				{
					text="Error: too long key must be 32 character";
				}
			}
			else
			{
				printf("current emoncms key:[%s]",Emonitor_GetKey());
				text = "";
			}
		}
		else if(strcmp(parameter,"url")==0)
		{
			parameter = argv[2];
			//Check if second parameter received
			if(argc == 3)
			{
				if(strlen(parameter)<=100)
				{
					Emonitor_SetUrl(parameter);
					printf("new url:[%s]\n",parameter);
					text="emoncms url reconfigured";
				}
				else
				{
					text="Error: too long key must be smaller than 100 character";
				}
			}
			else
			{
				printf("current emoncms url:[%s]",Emonitor_GetUrl());
				text = "";
			}
		}
		else if(strcmp(parameter,"nodeid")==0)
		{
			parameter = argv[2];
			//Check if second parameter received
			if(argc == 3)
			{
				number = strtol(parameter,NULL,10);
				if((number > 0) && (number <=999999))
				{
					Emonitor_SetNodeId(number);
					printf("new nodeid:[%d]\n",number);
					text="emoncms nodeid reconfigured";
				}
				else
				{
					text="Error: emoncms nodeid must be between 1 and 999999";
				}
			}
			else
			{
				printf("current emoncms nodeid:[%d]",Emonitor_GetNodeId());
				text = "";
			}
		}
		else if(strcmp(parameter,"timing")==0)
		{
			parameter = argv[2];
			//Check if second parameter received
			if(argc == 3)
			{
				number = strtol(parameter,NULL,10);
				if((number > 0) && (number <=3600))
				{
					Emonitor_SetSendPeriod(number);
					printf("new timing:[%d]\n",number);
					text="emoncms timing reconfigured";
				}
				else
				{
					text="Error: emoncms timing must be between 1 and 3600";
				}
			}
			else
			{
				printf("current emoncms timing:[%d]",Emonitor_GetSendPeriod());
				text = "";
			}
		}
		else if(strcmp(parameter,"lang")==0)
		{
			parameter = argv[2];
			//Check if second parameter received
			if(argc == 3)
			{
				Http_Server_SetLanguage(Http_Server_Language_TextToId(parameter));
				Http_Server_Language_IdToText(Http_Server_GetLanguage(),(char*)temp);
				printf("new language:[%s]\n",temp);
				text="imre language reconfigured";

			}
			else
			{
				Http_Server_Language_IdToText(Http_Server_GetLanguage(),(char*)temp);
				printf("current language:[%s]",(char*)temp);
				text = "";
			}
		}
	}
    //Generate response
	prj_printf("%s\n", text);
    return SHELL_RET_SUCCESS;
}

int ICACHE_FLASH_ATTR Command_Dir(int argc, char** argv) {
	esp_fs_status();

    return SHELL_RET_SUCCESS;
}

int ICACHE_FLASH_ATTR Command_Print(int argc, char** argv) {
	char* fileName;
	char buf[100] = {0};
	char fileNameBuffer[50] = {0};
	s32_t len;

	//Check argument count
	if(argc == 2)
	{
		// Process parameters
		fileName = argv[1];
		//Check if '/' is missing
		if(fileName[0] != '/')
		{
			sprintf(fileNameBuffer,"/%s",fileName);
		}
		else
		{
			sprintf(fileNameBuffer,"%s",fileName);
		}
		//Try to open file
		esp_fs_file file;
		esp_fs_OpenFile(&file,(char *)&fileNameBuffer);
		//Check if file is found
		if(file>= 0)
		{
			prj_printf("found %s : %d :\n", fileName, esp_fs_GetFileSize(fileName));
			do{
				//Read until the end
				len = esp_fs_ReadFile(&file,(u8_t *)buf, sizeof(buf)-1);
				//Print out file content
				buf[len] = 0;
				prj_printf("%s",buf);
			}while(len == sizeof(buf)-1);
			//Close the file
			esp_fs_CloseFile(&file);
		}
		else
		{
			prj_printf("file not found\n");
		}
	}
	else
	{
		prj_printf("error, provide filename\n");
	}

    return SHELL_RET_SUCCESS;
}

int ICACHE_FLASH_ATTR Command_Counter(int argc, char** argv) {
	char* text = "error, use parameter: reset, read, level";
	char* parameter;
	int i;
	//Check argument count
	if(argc == 2)
	{
		// Process parameters
		parameter = argv[1];

		//Do stuff
		if(strcmp(parameter,"reset")==0)
		{
			Sensor_Manager_ResetPulseCounters();
			text="counters reseted";
		}
		else if(strcmp(parameter,"read")==0)
		{
			for(i = 0 ; i < SENSOR_MANAGER_PULSE_COUNTERS ; i++ ){
				prj_printf("%02X:%d ",(i+1),Sensor_Manager_GetPulseCount(i));
			}
			text="";
		}
		else if(strcmp(parameter,"level")==0)
		{
			for(i = 0 ; i < SENSOR_MANAGER_PULSE_COUNTERS ; i++ ){
				prj_printf("%02X:%d ",(i+1),Sensor_Manager_GetPulseLevel(i));
			}
			text="";
		}

	}
    //Generate response
	prj_printf("%s\n", text);
    return SHELL_RET_SUCCESS;
}

int ICACHE_FLASH_ATTR Command_Status(int argc, char** argv) {
	char text[1025] = {0};
	char* parameter;
	int length;

	length = Http_Server_ReportJson(text);

    //Generate response
	prj_printf("%s\n", text);
    return SHELL_RET_SUCCESS;
}

int ICACHE_FLASH_ATTR Command_Sensor(int argc, char** argv) {
#if (ANALOG_ENABLE == ON)
	char* text = "error, use parameter: co2, temp, analog, max, average";
#else
	char* text = "error, use parameter: co2, temp";
#endif
	char* parameter;
	int i;
	uint8 tempCount;
	uint8* ids;
	sint16* temperatures;
	sint16 temperature;
	char sign;
	//Check argument count
	if(argc == 2)
	{
		// Process parameters
		parameter = argv[1];

		//Do stuff
		if(strcmp(parameter,"co2")==0)
		{
			if(Sensor_Manager_HasCO2Sensor())
			{
				prj_printf("CO2: %d ppm",Sensor_Manager_GetCO2());
				text="";
			}
			else
			{
				text="CO2 sensor not found";
			}
		}
		else if(strcmp(parameter,"temp")==0)
		{
			Sensor_Manager_Get_TempSensorData(&tempCount,&ids,&temperatures);

			if(tempCount ==0)
			{
				text="No temp sensors found";
			}
			else
			{
				prj_printf("Temp sensor count: %d\n",tempCount);
				for(i = 0 ; i < tempCount ; i++ ){
					temperature = temperatures[i];
					if(Sensor_Manager_IsTempValid(temperature)) {
						sign = (temperature<0 ? '-':'+');
						prj_printf("   " PRINT_TEMPID " : " PRINT_TEMP "C\n",READ_ID, READ_TEMP);
					}
				}
				text="";
			}
		}
#if (ANALOG_ENABLE == ON)
		else if(strcmp(parameter,"analog")==0)
		{
			prj_printf("Analog_01:[%d]\n",Sensor_Manager_GetAnalogValue());
			text="";
		}
		else if(strcmp(parameter,"average")==0)
		{
			prj_printf("AnalogAverage_01:[%d]\n",Sensor_Manager_GetAnalogAverage());
			text="";
		}
		else if(strcmp(parameter,"max")==0)
		{
			prj_printf("AnalogMax_01:[%d]\n",Sensor_Manager_GetAnalogMax());
			text="";
		}
#endif
	}
    //Generate response
	prj_printf("%s\n", text);
    return SHELL_RET_SUCCESS;
}

/******************************************************************************
 * FunctionName : Remote_Control_Init
 * Description  : Init Remote Control application
 * Parameters   : none
 * Returns      : none
 *******************************************************************************/
void ICACHE_FLASH_ATTR Remote_Control_Init(void) {
#if(REMOTE_CONTROL_ENABLE ==1)
	//Init UART
	//Setup interrupts
    UART_IntrConfTypeDef uart_intr;
    uart_intr.UART_IntrEnMask = UART_RXFIFO_TOUT_INT_ENA | UART_FRM_ERR_INT_ENA | UART_RXFIFO_FULL_INT_ENA | UART_TXFIFO_EMPTY_INT_ENA;
    uart_intr.UART_RX_FifoFullIntrThresh = 10;
    uart_intr.UART_RX_TimeOutIntrThresh = 2;
    uart_intr.UART_TX_FifoEmptyIntrThresh = 20;
    UART_IntrConfig(UART0, &uart_intr);
    //Register interrupthandler
    UART_intr_handler_register(Remote_Control_UART0_Handler, NULL);
    //Enable Uart
    ETS_UART_INTR_ENABLE();

    // Initialize command line interface (CLI)
    // We pass the function pointers to the read and write functions that we implement below
    // We can also pass a char pointer to display a custom start message
    shell_init(uart0_read_char, uart0_write_char, 0);

    // Add commands to the shell
#if REMOTE_LOGIN_NEEDED == ON
    shell_register(Command_Login, "login");
#else
    Remote_RegisterCommands();
#endif

#endif
}

void Remote_RegisterCommands(void)
{
#if REMOTE_LOGIN_NEEDED == ON
    shell_register(Command_Exit, "exit");
#endif
    shell_register(Command_PrintAll, "help");
    shell_register(Command_Reset, "reset");
    shell_register(Command_Config, "config");
    shell_register(Command_Wifi, "wifi");
    shell_register(Command_Dir, "dir");
    shell_register(Command_Counter, "counter");
    shell_register(Command_Print, "print");
    shell_register(Command_Emon, "emon");
    shell_register(Command_Sensor, "sensor");
    shell_register(Command_Status, "status");
}

void Remote_UnregisterCommands(void)
{
	shell_unregister_all();
#if REMOTE_LOGIN_NEEDED == ON
    shell_register(Command_Login, "login");
#endif
}

/******************************************************************************
 * FunctionName : Remote_Control_Main
 * Description  : Remote Control Main
 * Parameters   : none
 * Returns      : none
 *******************************************************************************/
void ICACHE_FLASH_ATTR Remote_Control_Main(void) {
#if(REMOTE_CONTROL_ENABLE ==1)
	 shell_task();
#endif
}




