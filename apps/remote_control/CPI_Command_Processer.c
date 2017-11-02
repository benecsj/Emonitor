/***********************************************************************************************************************
* File:         CPI_Command_Processer.c
* Revision:
* Author:
* Date:         27.07.2012
* Description:  Command Processor and Interpreter module
***********************************************************************************************************************/

/***********************************************************************************************************************
Includes
***********************************************************************************************************************/
#include "CPI_Command_Processer.h"

/***********************************************************************************************************************
Global variables and functions
***********************************************************************************************************************/

uint8 Cpi_yRxBuffer_mau8[CPI_RX_MAX_FRAME_SIZE];
uint8 Cpi_yRxBufferWritePos_mdu8;
uint8 Cpi_yRxBufferReadPos_mdu8;

uint8 Cpi_yRxParamBuffer_mau8[CPI_RX_MAX_FRAME_SIZE-6-CPI_COMMAND_NAME_LENGHT];
uint8 Cpi_yRxParamPos_mdu8;
uint8 Cpi_yRxParamRecRawSize_mdu8;
uint8 Cpi_yExceptedByteArrayLenght;

uint8 Cpi_yTxBuffer_mau8[CPI_TX_MAX_FRAME_SIZE];
uint8 Cpi_yTxBufferReadPos_mdu8;
uint8 Cpi_yTxMessageSize_mdu8;

Cpi_StatusType  Cpi_Status = CPI_UNINIT;
Cpi_ParamType   Cpi_ParamStatus = CPI_PARAM_NONE_SPEC;
uint8		Cpi_RecCommandId = 0;
Cpi_CommandType Cpi_CommandTable_mas[]={CPI_COMMANDS_TABLE};

const uint32 Cpi_DecimalTable[]={
1,
10,
100,
1000,
10000,
100000,
1000000,
10000000,
100000000,
1000000000,
};

// Private functions declaration
uint8 Cpi_IsEqual( uint8 * data1, uint8 * data2, uint8 lenght );
void Cpi_MemCopy(void* dest, const void* src, uint8 count);
void Cpi_ProcessParamArray(void);
void Cpi_ProcessDecimalValue(void);
void Cpi_ProcessString(void);

extern void Cpi_MainCallout(void);
extern void Cpi_UnsupportedFunctionResponse(uint8* params,uint8 lenght, uint8* response);
/**************************************************************************************************
@brief         Cpi_Init 
@details       Initializes Cpi module
    
@param[in]     None.
   
@return        None.

****************************************************************************************************/
void Cpi_Init(void)
{
  uint8 Cpi_nCounterldu8;
  for (Cpi_nCounterldu8=0;Cpi_nCounterldu8<CPI_RX_MAX_FRAME_SIZE;Cpi_nCounterldu8++)
  {
    Cpi_yRxBuffer_mau8[Cpi_nCounterldu8]=0;
  }
  Cpi_yRxBufferWritePos_mdu8 = 0;
  Cpi_yRxBufferReadPos_mdu8 = 0;
  
  for (Cpi_nCounterldu8=0;Cpi_nCounterldu8<CPI_TX_MAX_FRAME_SIZE;Cpi_nCounterldu8++)
  {
    Cpi_yTxBuffer_mau8[Cpi_nCounterldu8]=0;
  }
  Cpi_yTxBufferReadPos_mdu8 = 0;  
   Cpi_Status = CPI_IDLE;
}

/**************************************************************************************************
@brief         Cpi_Main 
@details       Cpi main function
    
@param[in]     None.
   
@return        None.

****************************************************************************************************/
 uint8 testval =3;
void Cpi_Main(void)
{
  //Only in processing state do the callout
  if(Cpi_Status == CPI_PROCESS)
  {
      //Only call if its present for the command
      if(NULL != Cpi_CommandTable_mas[Cpi_RecCommandId].FunctionPolling)
      {
        //Polling callout to handle long operations
        Cpi_CommandTable_mas[Cpi_RecCommandId].FunctionPolling(Cpi_yRxParamBuffer_mau8,Cpi_yRxParamPos_mdu8,&Cpi_yTxBuffer_mau8[CPI_COMMAND_NAME_LENGHT+3]);
      }
  }
  DBG("(CPI) R:%d W:%d S:%d\n",Cpi_yRxBufferReadPos_mdu8,Cpi_yRxBufferWritePos_mdu8, Cpi_Status);
    #if (CPI_OPERATION_MODE == CPI_MODE_POLLING)
       Cpi_RxHandler(); //Read into Rx buffer received bytes
    #endif 
       //DBG("(CPI) Main\n");
	   /* Process input */
           //Process bytes received into Rx buffer and Cpi state is not Process and SendResponse.
	   while ((Cpi_yRxBufferReadPos_mdu8<Cpi_yRxBufferWritePos_mdu8) &&
			   (Cpi_Status != CPI_SENDRESPONE) &&
			   (Cpi_Status != CPI_PROCESS))
	   {
		   DBG("(CPI)||| R:%d W:%d S:%d C:%c\n",Cpi_yRxBufferReadPos_mdu8,Cpi_yRxBufferWritePos_mdu8, Cpi_Status,Cpi_yRxBuffer_mau8[Cpi_yRxBufferReadPos_mdu8]);
		   switch(Cpi_Status)
		   {
		   case CPI_IDLE: //In idle state waiting for start token
			   // If start token received then wait or read for command
			   if (Cpi_yRxBuffer_mau8[Cpi_yRxBufferReadPos_mdu8]== CPI_TOKEN_START_RX)
			   {
				   Cpi_Status = CPI_RECCMD;//Enter wait for command name state
				   DBG("(CPI) Start Received\n");
			   }
			   else
			   {// Not start token received, reseting receiver buffer
				   Cpi_yRxBufferWritePos_mdu8 = 0;
				   Cpi_yRxBufferReadPos_mdu8 = 0;
			   }
			   break;

		   case CPI_RECCMD: //Waiting for command name or ID
			   Cpi_yRxBufferReadPos_mdu8++; //Increment Read counter
			   /* Wait until command fully received*/
			   if (Cpi_yRxBufferReadPos_mdu8== (CPI_COMMAND_NAME_LENGHT +1))
			   {
				   DBG("(CPI) Command Received\n");
				   /* Check command validity*/
				   uint8 Cpi_zCounter_ldu8;
				   for (Cpi_zCounter_ldu8=0;Cpi_zCounter_ldu8<CPI_COMMANDS_COUNT;Cpi_zCounter_ldu8++)
				   {//Check received command with command table
					   if (Cpi_IsEqual(&Cpi_yRxBuffer_mau8[1],Cpi_CommandTable_mas[Cpi_zCounter_ldu8].Name,CPI_COMMAND_NAME_LENGHT) == OK)
					   {//If command found stop search
						   DBG("(CPI) Command found\n");
						   break;
					   }
				   }
                                   //Check if command found
				   if (Cpi_zCounter_ldu8<CPI_COMMANDS_COUNT)
				   {//Command found, saving command ID, enter parameter receive state
					   Cpi_Status = CPI_RECPARAM;
					   Cpi_ParamStatus = CPI_PARAM_NONE_SPEC;
					   Cpi_RecCommandId = Cpi_zCounter_ldu8;
					   Cpi_yRxParamPos_mdu8 = 0;
				   }
				   else
				   {//Command name not found in table, sending error response
					   Cpi_SendFrame(15,(uint8*)"INVALID COMMAND");
				   }
			   }
			   break;
		   case CPI_RECPARAM:// Receiving parameter data bytes
			   switch(Cpi_ParamStatus)
			   {
			   case CPI_PARAM_NONE_SPEC:
				   switch (Cpi_yRxBuffer_mau8[Cpi_yRxBufferReadPos_mdu8])
				   {
				   case CPI_TOKEN_STOP :
					   if ((Cpi_CommandTable_mas[Cpi_RecCommandId].InLenght == Cpi_yRxParamPos_mdu8) | (Cpi_CommandTable_mas[Cpi_RecCommandId].InLenght == CPI_VARIABLE_LENGHT))
					   {
					       Cpi_Status = CPI_PROCESS;
                                               if(NULL != Cpi_CommandTable_mas[Cpi_RecCommandId].Function)
                                               {
						   Cpi_CommandTable_mas[Cpi_RecCommandId].Function(Cpi_yRxParamBuffer_mau8,Cpi_yRxParamPos_mdu8,&Cpi_yTxBuffer_mau8[CPI_COMMAND_NAME_LENGHT+3]);
                                               }
                                               else
                                               {
                                                   Cpi_UnsupportedFunctionResponse(Cpi_yRxParamBuffer_mau8,Cpi_yRxParamPos_mdu8,&Cpi_yTxBuffer_mau8[CPI_COMMAND_NAME_LENGHT+3]);
                                               }
					   }
					   else
					   {
						   Cpi_SendFrame(17,(uint8*)"INVALID PARAMETER");
					   }
					   break;
				   case CPI_TOKEN_DEC_BYTE:
					   Cpi_yRxParamRecRawSize_mdu8 = 0;
					   Cpi_ParamStatus = CPI_PARAM_DEC_BYTE;
					   break;
				   case CPI_TOKEN_DEC_WORD:
					   Cpi_yRxParamRecRawSize_mdu8 = 0;
					   Cpi_ParamStatus = CPI_PARAM_DEC_WORD;
					   break;
				   case CPI_TOKEN_DEC_INT:
					   Cpi_yRxParamRecRawSize_mdu8 = 0;
					   Cpi_ParamStatus = CPI_PARAM_DEC_INT;
					   break;
				   case CPI_TOKEN_BYTE_ARRAY:
					   Cpi_yRxParamRecRawSize_mdu8 = 0;
					   Cpi_ParamStatus = CPI_PARAM_BYTE_ARRAY;
					   break;
				   case CPI_TOKEN_STRING:
					   Cpi_yRxParamRecRawSize_mdu8 = 0;
					   Cpi_ParamStatus = CPI_PARAM_STRING;
					   break;
				   default: break;
				   }
				   break; /*END CPI_PARAM_NONE_SPEC */
			   case CPI_PARAM_BYTE_ARRAY:
				   /* First receive block lenght if its not yet defined*/
				   if (Cpi_yExceptedByteArrayLenght==0)
				   {
					   Cpi_yExceptedByteArrayLenght = Cpi_yRxBuffer_mau8[Cpi_yRxBufferReadPos_mdu8];
				   }
				   /* Count received bytes*/
				   Cpi_yRxParamRecRawSize_mdu8++;
				   /* If defined lenght received save it and return to RECPARAM state */
				   if (Cpi_yRxParamRecRawSize_mdu8>Cpi_yExceptedByteArrayLenght)
				   {
				      Cpi_ProcessParamArray();
					  Cpi_ParamStatus = CPI_PARAM_NONE_SPEC;
				   }
				   break;
			   case CPI_PARAM_STRING:
				   Cpi_yRxParamRecRawSize_mdu8++;
				   /* If defined lenght received save it and return to RECPARAM state */
				   if (Cpi_yRxBuffer_mau8[Cpi_yRxBufferReadPos_mdu8]== CPI_TOKEN_STRING)
				   {
				      Cpi_ProcessString();
					  Cpi_ParamStatus = CPI_PARAM_NONE_SPEC;
				   }
				   break;
			   case CPI_PARAM_DEC_BYTE:
			   case CPI_PARAM_DEC_WORD:
			   case CPI_PARAM_DEC_INT:
				if (((Cpi_yRxBuffer_mau8[Cpi_yRxBufferReadPos_mdu8]>='0')&(Cpi_yRxBuffer_mau8[Cpi_yRxBufferReadPos_mdu8]<='9'))|(Cpi_yRxBuffer_mau8[Cpi_yRxBufferReadPos_mdu8]=='+')|(Cpi_yRxBuffer_mau8[Cpi_yRxBufferReadPos_mdu8]=='-'))
				{
					/* Received byte is valid wait for next one */
					Cpi_yRxParamRecRawSize_mdu8++;
				}
				else
				{
				    Cpi_yRxBufferReadPos_mdu8--; /* Last byte is not intended for the parameter so not processing it */
				    Cpi_ProcessDecimalValue();
					Cpi_ParamStatus = CPI_PARAM_NONE_SPEC;
				}
				break;
			   default: break;
			   }
			   Cpi_yRxBufferReadPos_mdu8++;
			   break; /*END CPI_RECPARAM */
		   default: break;
		   }
	   }


  if (Cpi_Status == CPI_SENDRESPONE)
  {
#if (CPI_OPERATION_MODE == CPI_MODE_POLLING)
     Cpi_TxHandler();
#endif
     if (Cpi_yTxBufferReadPos_mdu8>=Cpi_yTxMessageSize_mdu8)
     {
       Cpi_Status = CPI_IDLE;
       Cpi_yRxBufferWritePos_mdu8 = 0;
       Cpi_yRxBufferReadPos_mdu8 = 0;
       Cpi_yTxBufferReadPos_mdu8 = 0;
     }
  }

}


void Cpi_RxHandler(void)
{
    uint8 Cpi_yCount_ldu8;
    uint8 Cpi_yReceiveSize = 0;
    uint8* Cpi_yRecBuffPointer ;
    CPI_Receive(Cpi_yReceiveSize,Cpi_yRecBuffPointer);
    if (Cpi_yRxBufferWritePos_mdu8+Cpi_yReceiveSize <= CPI_RX_MAX_FRAME_SIZE)
    {
    for (Cpi_yCount_ldu8=0 ;Cpi_yCount_ldu8<Cpi_yReceiveSize;Cpi_yCount_ldu8++)
    {
        Cpi_yRxBuffer_mau8[Cpi_yRxBufferWritePos_mdu8] = Cpi_yRecBuffPointer[Cpi_yCount_ldu8];
        Cpi_yRxBufferWritePos_mdu8++;
    }
    }
	else
	{
	    Cpi_SendFrame(20,(uint8*)"ERROR TOO LONG FRAME");
	}
    
}

void Cpi_ProcessParamArray(void)
{
    Cpi_MemCopy(&Cpi_yRxParamBuffer_mau8[Cpi_yRxParamPos_mdu8],&Cpi_yRxBuffer_mau8[Cpi_yRxBufferReadPos_mdu8-Cpi_yExceptedByteArrayLenght+1],Cpi_yExceptedByteArrayLenght);
	Cpi_yRxParamPos_mdu8 += Cpi_yExceptedByteArrayLenght;
}

void Cpi_ProcessString(void)
{
    Cpi_MemCopy(&Cpi_yRxParamBuffer_mau8[Cpi_yRxParamPos_mdu8],&Cpi_yRxBuffer_mau8[Cpi_yRxBufferReadPos_mdu8-Cpi_yRxParamRecRawSize_mdu8+1],Cpi_yRxParamRecRawSize_mdu8-1);
	Cpi_yRxParamPos_mdu8 += (Cpi_yRxParamRecRawSize_mdu8-1);
}

void Cpi_ProcessDecimalValue(void)
{
        uint8 Cpi_DataTemp1;
        uint16 Cpi_DataTemp2;
	uint8  Cpi_ProcessCounter;
	uint32 Cpi_CalcTemp = 0;
	uint8  Cpi_isNegative = 0;
	uint8* Cpi_DataPointer = &Cpi_yRxBuffer_mau8[Cpi_yRxBufferReadPos_mdu8-Cpi_yRxParamRecRawSize_mdu8+1];

    Cpi_yRxParamRecRawSize_mdu8--;

	if (*Cpi_DataPointer== '-')
	{
		Cpi_isNegative = 1;
		Cpi_yRxParamRecRawSize_mdu8--;
		Cpi_DataPointer++;
	}
	if (Cpi_yRxParamRecRawSize_mdu8 == 255)
        {
         return; 
        }
	for (Cpi_ProcessCounter=0;Cpi_ProcessCounter<=Cpi_yRxParamRecRawSize_mdu8;Cpi_ProcessCounter++)
	{
		Cpi_CalcTemp+= (Cpi_DataPointer[Cpi_ProcessCounter]-'0')* Cpi_DecimalTable[Cpi_yRxParamRecRawSize_mdu8-Cpi_ProcessCounter];
	}


	if (Cpi_isNegative)
	{
	  Cpi_CalcTemp = -Cpi_CalcTemp;
	}
	
	switch(Cpi_ParamStatus)
	{
	case CPI_PARAM_DEC_BYTE:
                Cpi_DataTemp1 = (uint8)Cpi_CalcTemp;
		Cpi_MemCopy(&Cpi_yRxParamBuffer_mau8[Cpi_yRxParamPos_mdu8],&Cpi_DataTemp1,1);
	    Cpi_yRxParamPos_mdu8 += 1;
		break;

	case CPI_PARAM_DEC_WORD:
                Cpi_DataTemp2 = (uint16)Cpi_CalcTemp;
		Cpi_MemCopy(&Cpi_yRxParamBuffer_mau8[Cpi_yRxParamPos_mdu8],&Cpi_DataTemp2,2);
	    Cpi_yRxParamPos_mdu8 += 2;
		break;

	case CPI_PARAM_DEC_INT:
		Cpi_MemCopy(&Cpi_yRxParamBuffer_mau8[Cpi_yRxParamPos_mdu8],&Cpi_CalcTemp,4);
	    Cpi_yRxParamPos_mdu8 += 4;
		break;
	}


}

void Cpi_TxHandler(void)
{
#if (CPI_OPERATION_MODE == CPI_MODE_INTERRUPT)
  if (    Cpi_Status == CPI_SENDRESPONE)
  {
#endif
  uint8 Cpi_ySendSize = 0;
  if (Cpi_yTxBufferReadPos_mdu8<Cpi_yTxMessageSize_mdu8)
  {
#if (CPI_SEND_MAX_SIZE > 1)
   if (Cpi_yTxMessageSize_mdu8-Cpi_yTxBufferReadPos_mdu8>CPI_SEND_MAX_SIZE)
   {
     Cpi_ySendSize = CPI_SEND_MAX_SIZE;
   }
   else
   {
     Cpi_ySendSize = Cpi_yTxMessageSize_mdu8-Cpi_yTxBufferReadPos_mdu8;
   }
#else
   Cpi_ySendSize = CPI_SEND_MAX_SIZE;
#endif
   CPI_Send(Cpi_ySendSize,&Cpi_yTxBuffer_mau8[Cpi_yTxBufferReadPos_mdu8]);
   Cpi_yTxBufferReadPos_mdu8 = Cpi_yTxBufferReadPos_mdu8 + Cpi_ySendSize;
  }
      #if (CPI_OPERATION_MODE == CPI_MODE_INTERRUPT)
  }
#endif
}

uint8 Cpi_SendRaw(uint8 length, uint8* buffer)
{
        Cpi_Status = CPI_SENDRESPONE;
          
        if (&Cpi_yTxBuffer_mau8[0] != buffer)
        {
	    Cpi_MemCopy(&Cpi_yTxBuffer_mau8[0],buffer,length);
        }
        
        Cpi_yTxMessageSize_mdu8 = length;
        
        #if (CPI_OPERATION_MODE == CPI_MODE_INTERRUPT)
        Cpi_TxHandler();
        #endif
	return 0;
}

uint8 Cpi_SendFrame(uint8 length, uint8* buffer)
{
        Cpi_Status = CPI_SENDRESPONE;
  
        Cpi_yTxBuffer_mau8[0]='\r';
        Cpi_yTxBuffer_mau8[1]='\n';
        Cpi_yTxBuffer_mau8[2]=CPI_TOKEN_START_TX;
          
        if (&Cpi_yTxBuffer_mau8[3] != buffer)
        {
	    Cpi_MemCopy(&Cpi_yTxBuffer_mau8[3],buffer,length);
        }
        
        Cpi_yTxMessageSize_mdu8 = length + 3 + 3;
        
        Cpi_yTxBuffer_mau8[length+3]=CPI_TOKEN_STOP;
        Cpi_yTxBuffer_mau8[length+4]='\r';
        Cpi_yTxBuffer_mau8[length+5]='\n';
        
        #if (CPI_OPERATION_MODE == CPI_MODE_INTERRUPT)
        Cpi_TxHandler();
        #endif
	return 0;
}

uint8 Cpi_SendResponseFrame(uint8 length,uint8* buffer)
{
  uint8 Cpi_temp;
        
        Cpi_yTxBuffer_mau8[0]='\r';
        Cpi_yTxBuffer_mau8[1]='\n';
        Cpi_yTxBuffer_mau8[2]=CPI_TOKEN_START_TX;
  
        for(Cpi_temp=0;Cpi_temp<CPI_COMMAND_NAME_LENGHT;Cpi_temp++)
        {
          Cpi_yTxBuffer_mau8[3+Cpi_temp] = Cpi_CommandTable_mas[Cpi_RecCommandId].Name[Cpi_temp];
        }
        
        if (&Cpi_yTxBuffer_mau8[CPI_COMMAND_NAME_LENGHT+3] != buffer)
        {
	    Cpi_MemCopy(&Cpi_yTxBuffer_mau8[CPI_COMMAND_NAME_LENGHT+3],buffer,length);
        }
        
        Cpi_yTxMessageSize_mdu8 = length + CPI_COMMAND_NAME_LENGHT + 3 + 3;
        
        Cpi_yTxBuffer_mau8[Cpi_yTxMessageSize_mdu8-3]=CPI_TOKEN_STOP;
        Cpi_yTxBuffer_mau8[Cpi_yTxMessageSize_mdu8-2]='\r';
        Cpi_yTxBuffer_mau8[Cpi_yTxMessageSize_mdu8-1]='\n';
        
        Cpi_Status = CPI_SENDRESPONE;
        
        #if (CPI_OPERATION_MODE == CPI_MODE_INTERRUPT)
        Cpi_TxHandler();
        #endif
	return 0;
}

uint8 Cpi_IsEqual( uint8 * data1, uint8 * data2, uint8 lenght )
{
while ( lenght -- )
{
if ( * (data1+lenght) != * (data2+lenght) )
{
return FAIL;
}
}
return OK;
}

void Cpi_MemCopy(void* dest, const void* src, uint8 count)
{
        char* dst8 = (char*)dest;
        char* src8 = (char*)src;

        while (count--) {
            *dst8++ = *src8++;
        }
}


uint8 Cpi_TransmitFrame(uint8* command,uint8 length,uint8* buffer)
{
  uint8 Cpi_temp;
        
        Cpi_yTxBuffer_mau8[0]='\r';
        Cpi_yTxBuffer_mau8[1]='\n';
        Cpi_yTxBuffer_mau8[2]=CPI_TOKEN_START_TX;
  
        for(Cpi_temp=0;Cpi_temp<CPI_COMMAND_NAME_LENGHT;Cpi_temp++)
        {
          Cpi_yTxBuffer_mau8[3+Cpi_temp] = command[Cpi_temp];
        }
        
        if (&Cpi_yTxBuffer_mau8[CPI_COMMAND_NAME_LENGHT+3] != buffer)
        {
	    Cpi_MemCopy(&Cpi_yTxBuffer_mau8[CPI_COMMAND_NAME_LENGHT+3],buffer,length);
        }
        
        Cpi_yTxMessageSize_mdu8 = length + CPI_COMMAND_NAME_LENGHT + 3 + 4;

        Cpi_yTxBuffer_mau8[Cpi_yTxMessageSize_mdu8-4]=CPI_TOKEN_STOP;
        Cpi_yTxBuffer_mau8[Cpi_yTxMessageSize_mdu8-3]='\r';
        Cpi_yTxBuffer_mau8[Cpi_yTxMessageSize_mdu8-2]='\n';
        Cpi_yTxBuffer_mau8[Cpi_yTxMessageSize_mdu8-1]=0;
        
        CPI_Send((Cpi_yTxMessageSize_mdu8-Cpi_yTxBufferReadPos_mdu8),&Cpi_yTxBuffer_mau8[0]);
	return 0;
}

void Cpi_UnsupportedFunctionResponse(uint8* params,uint8 lenght, uint8* response)
{
  //Send response
  uint8* Cpi_TextPointer = "/NOT SUPPORTED/";
  uint8 Cpi_Counter_u8;
  lenght = 15;
  //Copy response
  for(Cpi_Counter_u8 = 0 ; Cpi_Counter_u8 < lenght; Cpi_Counter_u8++)
  {
    response[Cpi_Counter_u8]=Cpi_TextPointer[Cpi_Counter_u8];
  }
  Cpi_SendResponseFrame(lenght,response);
}
