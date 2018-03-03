/* please read copyright-notice at EOF */


#include "c_types.h"
#include "stdint.h"
#include "CRC_Config.h"



uint8 ICACHE_FLASH_ATTR CRC_crc8 ( uint8 *CS_CS_data_in, uint16 CS_number_of_bytes_to_read ,uint8 crcInit)
{
	
	uint8  CS_crc;
	uint16 CS_loop_count;
	uint8  CS_bit_counter;
	uint8  CS_data;
	uint8  CS_feedback_bit;
	
	CS_crc = crcInit;

	for (CS_loop_count = 0; CS_loop_count != CS_number_of_bytes_to_read; CS_loop_count++)
	{
		CS_data = CS_CS_data_in[CS_loop_count];
		
		CS_bit_counter = 8;
		do {
			CS_feedback_bit = (CS_crc ^ CS_data) & 0x01;
	
			if ( CS_feedback_bit == 0x01 ) {
				CS_crc = CS_crc ^ CRC_CRC8POLY;
			}
			CS_crc = (CS_crc >> 1) & 0x7F;
			if ( CS_feedback_bit == 0x01 ) {
				CS_crc = CS_crc | 0x80;
			}
		
			CS_data = CS_data >> 1;
			CS_bit_counter--;
		
		} while (CS_bit_counter > 0);
	}
	
	return CS_crc;
}

