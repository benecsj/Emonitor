/***********************************************************************************************************************
* File:         CPI_Config.h
* Revision:
* Author:
* Date:         18.02.2013
* Description:  Configuration of Command Processor and Interpreter module
***********************************************************************************************************************/

#ifndef CPI_CFG_H
#define CPI_CFG_H


/* Operation mode*/
#define CPI_OPERATION_MODE  CPI_MODE_INTERRUPT /* CPI_MODE_INTERRUPT / CPI_MODE_POLLING */

/* Message Size */
#define CPI_RX_MAX_FRAME_SIZE   REMOTE_RX_SIZE
#define CPI_TX_MAX_FRAME_SIZE   REMOTE_TX_SIZE

/* Callbacks definitions */
/* Command Events*/
extern void Cpi_NvM(uint8* params,uint8 lenght, uint8* response);
extern void Cpi_Wifi(uint8* params,uint8 lenght, uint8* response);
extern void Cpi_Spiffs(uint8* params,uint8 lenght, uint8* response);
extern void Cpi_Sensor(uint8* params,uint8 lenght, uint8* response);
extern void Cpi_Reset(uint8* params,uint8 lenght, uint8* response);
extern void ICACHE_FLASH_ATTR Cpi_Uart(uint8* params,uint8 lenght, uint8* response);
/* Processing callout */


/* Commands table */
/* Help: You can define variable lenght with CPI_VARIABLE_LENGHT token */
#define CPI_COMMANDS_TABLE \
	/* Name   Rec Bytes, Function Rec, Polling*/\
        {"ctnvm"  ,CPI_VARIABLE_LENGHT  ,Cpi_NvM, NULL},\
        {"cwifi"  ,CPI_VARIABLE_LENGHT  ,Cpi_Wifi, NULL},\
        {"spifs"  ,CPI_VARIABLE_LENGHT  ,Cpi_Spiffs, NULL},\
        {"sense"  ,CPI_VARIABLE_LENGHT  ,Cpi_Sensor, NULL},\
        {"reset"  ,CPI_VARIABLE_LENGHT  ,Cpi_Reset, NULL},\
        {"suart"  ,CPI_VARIABLE_LENGHT  ,Cpi_Uart, NULL},\

/* Communication protocol setup*/
/* Tokens: */
#define CPI_TOKEN_START_RX    '@'
#define CPI_TOKEN_START_TX    '$'
#define CPI_TOKEN_STOP        '#'
#define CPI_TOKEN_DEC_BYTE    'b'
#define CPI_TOKEN_DEC_WORD    'w'
#define CPI_TOKEN_DEC_INT     'i'
#define CPI_TOKEN_BYTE_ARRAY  ':'
#define CPI_TOKEN_STRING      '/'
/* Command name size: */
#define CPI_COMMAND_NAME_LENGHT 5   

/* Communication connections */
#define CPI_SEND_MAX_SIZE       50

extern uint8 remote_recLength;
extern uint8 remote_recBuffer[];

#define CPI_Send(lenght,pointer) printf("%s",pointer);

extern uint8 DES_Cpi_RxSize;
extern uint8* DES_Cpi_RxPointer;

#define CPI_Receive(lenght,pointer) pointer = remote_recBuffer; lenght = remote_recLength; remote_recLength = 0

#endif
