/*
 * OWP_One_Wire_Protocol_Driver.c
 *
 *  Created on: Aug 12, 2012
 *      Author: Jooo
 */

#include "OWP_One_Wire_Protocol_Driver.h"
#include "pins.h"

uint8 OWP_Channels[OWP_CHANNELS_COUNT]={ONEWIRE_BUS_0,ONEWIRE_BUS_1};
uint8 OWP_Selected_Channel = 0;

/*Functions*/
void OWP_SelectChannel(uint8 channel)
{
    OWP_Selected_Channel = channel;
}

uint8 OWP_GetChannel(void)
{
    return OWP_Selected_Channel;
}


void OWP_Delay_us(uint32 delay)
{

	uint32 startTime = system_get_time();
	uint32 currenTime = startTime;
	do
	{
		currenTime = system_get_time();
	}while(delay>(currenTime-startTime));
}

void OWP_Parasite_Enable(void)
{
    OWP_OUT_HIGH();
	OWP_DIR_OUT();
}

void OWP_Parasite_Disable(void)
{
    OWP_OUT_LOW();
	OWP_DIR_IN();
}

uint8 OWP_Read_Bus(void)
{
    return OWP_GET_IN();
}

uint8 OWP_Reset(void)
{


	uint8 err;

	OWP_OUT_LOW(); // disable internal pull-up (maybe on from parasite)
	OWP_DIR_OUT(); // pull OW-Pin low for 480us
	OWP_Delay_us(480);
    OWP_ENTER_CRITICAL();
	// set Pin as input - wait for clients to pull low
	OWP_DIR_IN(); // input

	OWP_Delay_us(120);//66
	err = OWP_GET_IN();		// no presence detect
	// nobody pulled to low, still high
	OWP_EXIT_CRITICAL();
	// after a delay the clients should release the line
	// and input-pin gets back to high due to pull-up-resistor
	OWP_Delay_us(480-120);//66
	if( OWP_GET_IN() == 0 )		// short circuit
		err = 1;


	return err;
}

/* Timing issue when using runtime-bus-selection (!OWP_CONST_ONE_BUS):
   The master should sample at the end of the 15-slot after initiating
   the read-time-slot. The variable bus-settings need more
   cycles than the constant ones so the delays had to be shortened
   to achive a 15uS overall delay
   Setting/clearing a bit in I/O Register needs 1 cyle in OWP_CONST_ONE_BUS
   but around 14 cyles in configureable bus (us-Delay is 4 cyles per uS) */
uint8 OWP_Bit_IO( uint8 b )
{
        OWP_ENTER_CRITICAL();

	OWP_DIR_OUT(); // drive bus low

	OWP_Delay_us(1); // Recovery-Time wuffwuff was 1
	if ( b ) OWP_DIR_IN(); // if bit is 1 set bus high (by ext. pull-up)

	// wuffwuff delay was 15uS-1 see comment above
	OWP_Delay_us(15-1-OWP_CONF_DELAYOFFSET);

	if( OWP_GET_IN() == 0 ) b = 0;  // sample at end of read-timeslot
        
	OWP_Delay_us(60-15+OWP_CONF_DELAYOFFSET);
	OWP_DIR_IN();

        OWP_EXIT_CRITICAL();

	return b;
}


uint8 OWP_Byte_Write( uint8 b )
{
	//OWP_ENTER_CRITICAL();
	uint8 i = 8, j;

	do {
		j = OWP_Bit_IO( b & 1 );
		b >>= 1;
		if( j ) b |= 0x80;
	} while( --i );
	//OWP_EXIT_CRITICAL();
	return b;
}


uint8 OWP_Byte_Read( void )
{
  // read by sending 0xff (a dontcare?)
  return OWP_Byte_Write( 0xFF );
}


uint8 OWP_Rom_Search( uint8 diff, uint8 *id )
{
	uint8 i, j, next_diff;
	uint8 b;

	if( OWP_Reset() ) return OWP_CONST_PRESENCE_ERR;	// error, no device found

	OWP_Byte_Write( OWP_CONST_SEARCH_ROM );			// ROM search command
	next_diff = OWP_CONST_LAST_DEVICE;			// unchanged on last device

	i = OWP_CONST_ROMCODE_SIZE * 8;					// 8 bytes

	do {
		j = 8;					// 8 bits
		do {
			b = OWP_Bit_IO( 1 );			// read bit
			if( OWP_Bit_IO( 1 ) ) {			// read complement bit
				if( b )					// 11
				return OWP_CONST_DATA_ERR;			// data error
			}
			else {
				if( !b ) {				// 00 = 2 devices
					if( diff > i || ((*id & 1) && diff != i) ) {
					b = 1;				// now 1
					next_diff = i;			// next pass 0
					}
				}
			}
			OWP_Bit_IO( b );     			// write bit
			*id >>= 1;
			if( b ) *id |= 0x80;			// store bit

			i--;

		} while( --j );

		id++;					// next byte

	} while( i );

	return next_diff;				// to continue search
}


void OWP_Send_Command( uint8 command, uint8 *id )
{
	uint8 i;

	OWP_Reset();

	if( id ) {
		OWP_Byte_Write( OWP_CONST_MATCH_ROM );			// to a single device
		i = OWP_CONST_ROMCODE_SIZE;
		do {
			OWP_Byte_Write( *id );
			id++;
		} while( --i );
	}
	else {
		OWP_Byte_Write( OWP_CONST_SKIP_ROM );			// to all devices
	}

	OWP_Byte_Write( command );
}
