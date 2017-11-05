/*
 * OWP_One_Wire_Protocol_Driver.h
 *
 *  Created on: Aug 12, 2012
 *      Author: Jooo
 */

#ifndef OWP_ONE_WIRE_PROTOCOL_DRIVER_H_
#define OWP_ONE_WIRE_PROTOCOL_DRIVER_H_

/*Includes*/
#include "OWP_Config.h"
#include "c_types.h"
#include "stdint.h"

/*Defines*/

#define F_CPU MCU_MAINCLOCKFREQ
#define OWP_CONF_DELAYOFFSET 0

#define OWP_CONST_MATCH_ROM	0x55
#define OWP_CONST_SKIP_ROM	0xCC
#define	OWP_CONST_SEARCH_ROM	0xF0

#define	OWP_CONST_SEARCH_FIRST	0xFF		// start new search
#define	OWP_CONST_PRESENCE_ERR	0xFF
#define	OWP_CONST_DATA_ERR	0xFE
#define OWP_CONST_LAST_DEVICE	0x00		// last device found

#define OWP_CONST_ROMCODE_SIZE 8

/*Interfaces*/
extern void OWP_Parasite_Enable(void);
extern void OWP_Parasite_Disable(void);
extern uint8 OWP_Reset(void);
extern uint8 OWP_Bit_IO( uint8 b );
extern uint8 OWP_Byte_Write( uint8 b );
extern uint8 OWP_Byte_Read( void );
extern uint8 OWP_Rom_Search( uint8 diff, uint8 *id );
extern void OWP_Send_Command( uint8 command, uint8 *id );
extern uint8 OWP_Read_Bus(void);

#endif /* OWP_ONE_WIRE_PROTOCOL_DRIVER_H_ */
