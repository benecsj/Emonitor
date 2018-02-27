/*
 * D18_DS18B20_Temp_Sensor.h
 *
 *  Created on: Aug 13, 2012
 *      Author: Jooo
 */

#ifndef D18_DS18B20_TEMP_SENSOR_H_
#define D18_DS18B20_TEMP_SENSOR_H_

/*Defines*/
/* return values */
#define D18_DS18B20_OK          0x00
#define D18_DS18B20_ERROR       0x01
#define D18_DS18B20_START_FAIL  0x02
#define D18_DS18B20_ERROR_CRC   0x03
#define D18_DS18B20_POWER_PARASITE 0x00
#define D18_DS18B20_POWER_EXTERN   0x01
/* DS18X20 specific values (see datasheet) */
#define DS18S20_ID 0x10
#define DS18B20_ID 0x28
#define D18_DS18B20_CONVERT_T	0x44
#define D18_DS18B20_READ		0xBE
#define D18_DS18B20_WRITE		0x4E
#define D18_DS18B20_EE_WRITE	0x48
#define D18_DS18B20_EE_RECALL	0xB8
#define D18_DS18B20_READ_POWER_SUPPLY 0xB4
#define DS18B20_CONF_REG    4
#define DS18B20_9_BIT       0
#define DS18B20_10_BIT      (1<<5)
#define DS18B20_11_BIT      (1<<6)
#define DS18B20_12_BIT      ((1<<6)|(1<<5))
// indefined bits in LSB if 18B20 != 12bit
#define DS18B20_9_BIT_UNDF       ((1<<0)|(1<<1)|(1<<2))
#define DS18B20_10_BIT_UNDF      ((1<<0)|(1<<1))
#define DS18B20_11_BIT_UNDF      ((1<<0))
#define DS18B20_12_BIT_UNDF      0
// conversion times in ms
#define DS18B20_TCONV_12BIT      750
#define DS18B20_TCONV_11BIT      DS18B20_TCONV_12_BIT/2
#define DS18B20_TCONV_10BIT      DS18B20_TCONV_12_BIT/4
#define DS18B20_TCONV_9BIT       DS18B20_TCONV_12_BIT/8
#define DS18S20_TCONV            DS18B20_TCONV_12_BIT
// constant to convert the fraction bits to cel*(10^-4)
#define D18_DS18B20_FRACCONV         625
#define D18_DS18B20_SP_SIZE  9
// DS18X20 EEPROM-Support
#define D18_DS18B20_WRITE_SCRATCHPAD  0x4E
#define D18_DS18B20_COPY_SCRATCHPAD   0x48
#define D18_DS18B20_RECALL_E2         0xB8
#define D18_DS18B20_COPYSP_DELAY      10 /* ms */
#define D18_DS18B20_TH_REG      2
#define D18_DS18B20_TL_REG      3

/*Interfaces*/
extern void D18_DS18B20_FindSensor(uint8 *diff,
	uint8 id[]);

extern uint8	D18_DS18B20_GetPowerStatus(uint8 id[]);

extern uint8 D18_DS18B20_StartMeasure( uint8 with_external,
	uint8 id[]);
extern uint8 D18_DS18B20_ReadMeasure(uint8 id[],
	uint8 *subzero, uint8 *cel, uint8 *cel_frac_bits);
extern uint8 D18_DS18B20_ReadMeasureSingle(uint8 familycode,
	uint8 *subzero, uint8 *cel, uint8 *cel_frac_bits);

extern uint8 D18_DS18B20_MeasuretoCel( uint8 fc, uint8 *sp,
	uint8* subzero, uint8* cel, uint8* cel_frac_bits);
extern	sint16 D18_DS18B20_TemptoDecicel(uint8 subzero, uint8 cel,
	uint8 cel_frac_bits);
extern int8 D18_DS18B20_TempCmp(uint8 subzero1, uint16 cel1,
	uint8 subzero2, uint16 cel2);

extern void D18_DS18B20_ShowId( uint8 *id, uint8 n );


//Write th, tl and config-register to scratchpad (config ignored on S20)
uint8 D18_DS18B20_WriteScratchpad( uint8 id[],
	uint8 th, uint8 tl, uint8 conf);
//Read scratchpad into array SP
uint8 D18_DS18B20_ReadScratchpad( uint8 id[], uint8 sp[]);
//Copy values th,tl (config) from scratchpad to DS18x20 eeprom
uint8 D18_DS18B20_CopyScratchpad( uint8 with_power_extern,
	uint8 id[] );
//Copy values from DS18x20 eeprom to scratchpad
uint8 D18_DS18B20_RecallE2( uint8 id[] );

#endif /* D18_DS18B20_TEMP_SENSOR_H_ */
