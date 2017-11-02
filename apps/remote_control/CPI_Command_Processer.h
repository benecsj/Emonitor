/***********************************************************************************************************************
* File:         CPI_Command_Processer.h
* Revision:
* Author:
* Date:         18.02.2013
* Description:  Include file for Command Processor and Interpreter module
*
* Interfaces:
* void              Cpi_Init            (void)
* void              Cpi_Main            (void)
*
***********************************************************************************************************************/

#ifndef CPI_H
#define CPI_H


/***********************************************************************************************************************
Includes
***********************************************************************************************************************/
#include "c_types.h"
#include "stdint.h"
#include "CPI_Config.h"

/***********************************************************************************************************************
Defines 
***********************************************************************************************************************/

#define CPI_MODE_INTERRUPT  (0x00U)
#define CPI_MODE_POLLING    (0x01U)
#define CPI_VARIABLE_LENGHT (0xFFU)

#define CPI_COMMANDS_COUNT  (sizeof(Cpi_CommandTable_mas)/sizeof(Cpi_CommandTable_mas[0]))

/***********************************************************************************************************************
Typedef definitions
***********************************************************************************************************************/

typedef enum
{
  CPI_UNINIT,
  CPI_IDLE,
  CPI_RECCMD,
  CPI_RECPARAM,
  CPI_PROCESS,
  CPI_SENDRESPONE
}Cpi_StatusType;

typedef enum
{
  CPI_PARAM_NONE_SPEC,
  CPI_PARAM_DEC_BYTE,
  CPI_PARAM_DEC_WORD,
  CPI_PARAM_DEC_INT,
  CPI_PARAM_BYTE_ARRAY,
  CPI_PARAM_STRING
}Cpi_ParamType;

typedef void (*Cpi_yFuncPointerType)(uint8* params, uint8 lenght, uint8* response);

typedef struct
{
  uint8 Name[CPI_COMMAND_NAME_LENGHT+1];
  uint8 InLenght;
  Cpi_yFuncPointerType Function;
  Cpi_yFuncPointerType FunctionPolling;
}Cpi_CommandType;


/***********************************************************************************************************************
Global functions
***********************************************************************************************************************/
extern void Cpi_Init(void);
extern void Cpi_Main(void);

extern void Cpi_RxHandler(void);
extern void Cpi_TxHandler(void);

extern uint8 Cpi_SendRaw(uint8 length, uint8* buffer);
extern uint8 Cpi_SendFrame(uint8 length, uint8* buffer);
extern uint8 Cpi_SendResponseFrame(uint8 length,uint8* buffer);

extern uint8 Cpi_TransmitFrame(uint8* command,uint8 length,uint8* buffer);

#endif
