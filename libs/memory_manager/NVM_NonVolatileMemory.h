/*
 * NVM_NonVolatileMemory.h
 *
 *  Created on: Aug 13, 2012
 *      Author: Jooo
 */

#ifndef NVM_NONVOLATILEMEMORY_H_
#define NVM_NONVOLATILEMEMORY_H_

/*Includes*/

#include "c_types.h"
#include "stdint.h"

#include "NVM_Config.h"

/*Defines*/

//NvM requests
#define NVM_IDLE 	0
#define NVM_SAVE 	1
#define NVM_LOAD 	2
#define NVM_CLEAR 	3

/* Types */

typedef struct
{
	uint8* VariablePointer;
	uint16 Size;
} NvM_Data_Block;

/*Defines*/
#define NvM_IsBusy()    (NvM_Request != NVM_IDLE)

/*Interfaces*/
extern uint8 NvM_Request;

extern void NVM_Init(void);
extern void NVM_Main(void);
extern void NvM_RequestSave(void);
extern void NvM_RequestLoad(void);
extern void NvM_RequestClear(void);

extern void NvM_MemCopy(uint8* destination, uint8* source, uint16 length);

#endif /* NVM_NONVOLATILEMEMORY_H_ */
