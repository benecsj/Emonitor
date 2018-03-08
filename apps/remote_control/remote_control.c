/******************************************************************************
* Includes
\******************************************************************************/

#include "remote_control.h"
#include "esp_common.h"
#include "uart.h"

#include "Shell.h"

#include "NVM_NonVolatileMemory.h"
#include "spiffs_manager.h"
#include "Wifi_Manager.h"
#include "Sensor_Manager.h"
#include "Emonitor.h"

/******************************************************************************
* Defines
\******************************************************************************/

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
	char* text = "error, use parameter: clear | save | reload";
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
		else if(strcmp(parameter,"reload")==0)
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
	char* text = "error, use parameter: init | ip | hotspot";
	char* parameter;
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
			printf("ip:%d.%d.%d.%d",temp[0],temp[1],temp[2],temp[3]);
			text="";
		}
		else if(strcmp(parameter,"hotspot")==0)
		{
			//Check if second parameter received
			if(argc == 3)
			{
				//Check if its 'on'
				parameter = argv[2];
				if(parameter[1] == 'n')
				{
					Wifi_Manager_EnableHotspot(TRUE);
					text="wifi hotspot enabled";
				}
				else
				{
					Wifi_Manager_EnableHotspot(FALSE);
					text="wifi hotspot disabled";
				}
			}
			else
			{
				text = "error, missing parameter: on | off";
			}
		}
	}
    //Generate response
	prj_printf("%s\n", text);
    return SHELL_RET_SUCCESS;
}

int ICACHE_FLASH_ATTR Command_Dir(int argc, char** argv) {
	uint8 result = 0;
	uint32 total, used;

	prj_printf("Files:\n");
	spiffs_DIR spiffsDir;
	SPIFFS_opendir(spiffs_get_fs(), "/", &spiffsDir);
	struct spiffs_dirent spiffsDirEnt;
	while(SPIFFS_readdir(&spiffsDir, &spiffsDirEnt) != 0) {
		prj_printf("  %s : %d\n", spiffsDirEnt.name, spiffsDirEnt.size);
	}
	SPIFFS_closedir(&spiffsDir);

    SPIFFS_info(spiffs_get_fs(), (u32_t *)&total, (u32_t*)&used);
    prj_printf("Total: %d  Used: %d  Free: %d\n",total,used, total-used);

    return SHELL_RET_SUCCESS;
}

int ICACHE_FLASH_ATTR Command_Print(int argc, char** argv) {
	char* fileName;
	char buf[100] = {0};
	char fileNameBuffer[50] = {0};
	s32_t len;
	spiffs* fs;

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
		//get filesystem
		fs = spiffs_get_fs();
		//Get file status
		spiffs_stat status;
		int32 result = SPIFFS_stat(fs, (char *)&fileNameBuffer, &status);
		//Try to open file
		spiffs_file fd = SPIFFS_open(fs, (const char *)&fileNameBuffer, SPIFFS_RDONLY, 0);
		//Check if file is found
		if(fd>= 0)
		{
			prj_printf("found %s : %d :\n", status.name, status.size);
			do{
				//Read until the end
				len = SPIFFS_read(fs, fd, (u8_t *)buf, sizeof(buf)-1);
				//Print out file content
				buf[len] = 0;
				prj_printf("%s",buf);
			}while(len == sizeof(buf)-1);
			//Close the file
			SPIFFS_close(fs, fd);
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
	char* text = "error, use parameter: reset";
	char* parameter;

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




