/******************************************************************************
* Includes
\******************************************************************************/

#include "remote_control.h"

#include "esp_common.h"
#include "uart.h"

#include "CPI_Command_Processer.h"
#include "Shell.h"

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
				//Process with CPI
				Cpi_RxHandler();
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
 * Test commands: The commands should always use this function prototype.
 * They receive 2 parameters: The total count of arguments (argc) and a pointer
 * to the begining of each one of the null-terminated argument strings.
 *
 * In this example we ignore the parameters passed to the functions
 */
int command_mycommand(int argc, char** argv)
{
	uint8 i;
	  shell_printf("Received %d arguments for test command\r\n",argc);

	  // Print each argument with string lenghts
	  for(i=0; i<argc; i++)
	  {
	    // Print formatted text to terminal
	    shell_printf("%d - \"%s\" [len:%d]\r\n", i, argv[i], strlen(argv[i]) );
	  }
  return SHELL_RET_SUCCESS;
}

int command_othercommand(int argc, char** argv)
{
  shell_println("Running \"othercommand\" now");
  shell_println("Exit...");
  return SHELL_RET_SUCCESS;
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
    //Init command processer
    //Cpi_Init();

    // Initialize command line interface (CLI)
    // We pass the function pointers to the read and write functions that we implement below
    // We can also pass a char pointer to display a custom start message
    shell_init(uart0_read_char, uart0_write_char, 0);

    // Add commands to the shell
    shell_register(command_mycommand, "test");
    shell_register(command_othercommand, "test2");

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
	//Cpi_Main();
	 shell_task();
#endif
}




