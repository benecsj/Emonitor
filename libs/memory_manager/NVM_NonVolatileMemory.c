/*
 * NVM_NonVolatileMemory.c
 *
 *  Created on: Aug 13, 2012
 *      Author: Jooo
 */


/*Includes*/
#include "NVM_NonVolatileMemory.h"
#include "user_config.h"
/*Defines*/


#define NVM_DEBUG printf

#define NVM_DATA(a,b)	NvM_DataInfoList[NvM_DataBlockCount].VariablePointer=(uint8*)&a;NvM_DataInfoList[NvM_DataBlockCount].Size=b;NvM_DataBlockCount++;

/*Variables*/
uint8 NVM_FrameBuffer[NVM_CFG_BLOCK_SIZE];
NvM_Data_Block	NvM_DataInfoList[NVM_CFG_MAX_DATA_COUNT];
uint16 NvM_DataBlockCount;
uint8 NvM_Request = NVM_IDLE;


#if(NVM_CFG_CYCLIC_SAVE == 1)
uint16 NvM_SaveTimer = 0;
#endif

/*Function definitions*/
void NvM_MemCopy(uint8* destination, uint8* source, uint16 length);
void NvM_Store(void);
void NvM_Restore(void);
void NvM_StoreVariables(void);
void NvM_RestoreVariables(void);
//void NvM_Clear(void);

/*Functions*/
void NVM_Init(void)
{
	//Init variables info
	NvM_DataBlockCount = 0;
	NVM_CFG_STORAGE

	//Restore configuration
	NvM_Restore();

}

void NvM_Restore(void)
{
	uint8 NVM_Crc;
	uint16 NvM_Counter;
	uint16 NvM_Restored = 0;

	NVM_DEBUG("(NVM)<<< NvM_Restore>>>\r\n");

	//Try to restore NvM from valid blocks
	if(system_param_load(EMONITOR_PARAM_START_SEC, 0, (void*)&NVM_FrameBuffer, sizeof(NVM_FrameBuffer)))
	{
		NvM_RestoreVariables();
	}
	else
	{
		NVM_DEBUG("(NVM) No valid block were found\r\n");
	}

}

void NvM_RequestSave(void)
{
	NvM_Request = NVM_SAVE;
}

void NvM_RequestLoad(void)
{
	NvM_Request = NVM_LOAD;
}

void NvM_RequestClear(void)
{
    NvM_Request = NVM_CLEAR;
}

void NVM_Main(void)
{
#if(NVM_CFG_CYCLIC_SAVE == 1)
	//Increment save counter
	NvM_SaveTimer++;
	//When period reached then trigger save
	if(((NVM_CFG_SAVE_TIMING*NVM_CFG_MAIN_CYCLE_MS)/1000)<NvM_SaveTimer)
	{
		//Register save request
		NvM_Request = NVM_SAVE;
	}
#endif

	//If save was requested then store variables to NvM
	if(NvM_Request == NVM_SAVE)
	{
#if(NVM_CFG_CYCLIC_SAVE == 1)
		//Reset counter
		NvM_SaveTimer = 0;
#endif
		//Store to NvM
		NvM_Store();
        //Clear request
		NvM_Request = NVM_IDLE;
	}
	else if(NvM_Request == NVM_LOAD)
	{
		//Restore from NvM
		NvM_Restore();
		//Clear request
		NvM_Request = NVM_IDLE;
	}
	else if(NvM_Request == NVM_CLEAR)
	{
#if(NVM_CFG_CYCLIC_SAVE == 1)
		//Reset counter
		NvM_SaveTimer = 0;
#endif
		//Restore from NvM
		//NvM_Clear();
		//Clear request
		NvM_Request = NVM_IDLE;
	}
}

void NvM_Store(void)
{

	NVM_DEBUG("(NVM)<<< NvM_Store>>>\r\n");

	//Store variables
	NvM_StoreVariables();

	//Store complete datablock
	system_param_save_with_protect(EMONITOR_PARAM_START_SEC, (void*)&NVM_FrameBuffer, sizeof(NVM_FrameBuffer));
}


void NvM_RestoreVariables(void)
{
	uint16 NvM_Counter;
	uint16 NvM_ReadPosition = NVM_DATA_POS_DATA_START;

	NVM_DEBUG("(NVM) Restoring variables (%d)\r\n",NvM_DataBlockCount);
	//Restore all registered data blocks
	for(NvM_Counter = 0; NvM_Counter<NvM_DataBlockCount;NvM_Counter++)
	{
		//Restore variable from buffer
		NvM_MemCopy((uint8*)NvM_DataInfoList[NvM_Counter].VariablePointer,(uint8*)&NVM_FrameBuffer[NvM_ReadPosition],NvM_DataInfoList[NvM_Counter].Size);
		//Increment read position
		NvM_ReadPosition+=NvM_DataInfoList[NvM_Counter].Size;
	}
	NVM_DEBUG("(NVM) Restore done\r\n");
}

void NvM_StoreVariables(void)
{
	uint16 NvM_Counter;
	uint16 NvM_WritePosition = NVM_DATA_POS_DATA_START;

	NVM_DEBUG("(NVM) Storing variables\r\n");
	//Store all registered variables
	for(NvM_Counter = 0; NvM_Counter<NvM_DataBlockCount;NvM_Counter++)
	{
		//Store variable to buffer
		NvM_MemCopy((uint8*)&NVM_FrameBuffer[NvM_WritePosition],(uint8*)NvM_DataInfoList[NvM_Counter].VariablePointer,NvM_DataInfoList[NvM_Counter].Size);
		//Increment write position
		NvM_WritePosition+=NvM_DataInfoList[NvM_Counter].Size;
	}
	NVM_DEBUG("(NVM) Store done\r\n");
}


void NvM_MemCopy(uint8* destination, uint8* source, uint16 length)
{
	uint16 NvM_Counter;
	//Copy all bytes
	for(NvM_Counter = 0;NvM_Counter<length;NvM_Counter++)
	{
		//Copy from source to destination
		destination[NvM_Counter]=source[NvM_Counter];
	}
}
