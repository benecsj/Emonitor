/******************************************************************************
* Includes
\******************************************************************************/

#include "project_config.h"

#include "remote_control.h"

#include "esp_common.h"
#include "uart.h"

#include "CPI_Command_Processer.h"

/******************************************************************************
* Defines
\******************************************************************************/

/******************************************************************************
* Variables
\******************************************************************************/

uint8 remote_recLength = 0;
uint8 remote_recBuffer[100];


/******************************************************************************
* Implementations
\******************************************************************************/
LOCAL void
Remote_Control_UART0_Handler(void *para)
{
    uint8 received;
    uint8 uart_no = UART0;
    uint8 buf_idx = 0;
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
        	remote_recLength = (READ_PERI_REG(UART_STATUS(UART0)) >> UART_RXFIFO_CNT_S)&UART_RXFIFO_CNT;

            for (buf_idx = 0 ; buf_idx < remote_recLength ; buf_idx++) {
                received = (READ_PERI_REG(UART_FIFO(UART0)) & 0xFF);
                remote_recBuffer[buf_idx]= received;
                printf("%c",received);
            }

            Cpi_RxHandler();

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

/******************************************************************************
 * FunctionName : Remote_Control_Init
 * Description  : Init Remote Control application
 * Parameters   : none
 * Returns      : none
 *******************************************************************************/
void Remote_Control_Init(void) {
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
    Cpi_Init();
#endif
}

/******************************************************************************
 * FunctionName : Remote_Control_Main
 * Description  : Remote Control Main
 * Parameters   : none
 * Returns      : none
 *******************************************************************************/
void Remote_Control_Main(void) {
#if(REMOTE_CONTROL_ENABLE ==1)
	Cpi_Main();
#endif
}




