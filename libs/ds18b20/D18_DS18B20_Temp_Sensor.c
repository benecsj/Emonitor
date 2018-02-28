/*
 * D18_DS18B20_Temp_Sensor.c
 *
 *  Created on: Aug 13, 2012
 *      Author: Jooo
 */

/*Includes*/
#include "c_types.h"
#include "stdint.h"
#include "D18_DS18B20_Temp_Sensor.h"
#include "OWP_One_Wire_Protocol_Driver.h"
#include "CRC_crc8.h"
/*Functions*/

/*
   convert raw value from DS18x20 to Celsius
   input is:
   - familycode fc (0x10/0x28 see header)
   - scratchpad-buffer
   output is:
   - cel full celsius
   - fractions of celsius in millicelsius*(10^-1)/625 (the 4 LS-Bits)
   - subzero =0 positiv / 1 negativ
   always returns  D18_DS18B20_OK
   TODO invalid-values detection (but should be covered by CRC)
 */
uint8 D18_DS18B20_MeasuretoCel(uint8 fc, uint8 *sp,
        uint8* subzero, uint8* cel, uint8* cel_frac_bits) {
    uint16 meas;
    uint8 i;

    meas = sp[0]; // LSB
    meas |= ((uint16) sp[1]) << 8; // MSB
    //meas = 0xff5e; meas = 0xfe6f;

    //  only work on 12bit-base
    if (fc == DS18S20_ID) { // 9 -> 12 bit if 18S20
        /* Extended measurements for DS18S20 contributed by Carsten Foss */
        meas &= (uint16) 0xfffe; // Discard LSB , needed for later extended precicion calc
        meas <<= 3; // Convert to 12-bit , now degrees are in 1/16 degrees units
        meas += (16 - sp[6]) - 4; // Add the compensation , and remember to subtract 0.25 degree (4/16)
    }

    // check for negative
    if (meas & 0x8000) {
        *subzero = 1; // mark negative
        meas ^= 0xffff; // convert to positive => (twos complement)++
        meas++;
    } else *subzero = 0;

    // clear undefined bits for B != 12bit
    if (fc == DS18B20_ID) { // check resolution 18B20
        i = sp[DS18B20_CONF_REG];
        if ((i & DS18B20_12_BIT) == DS18B20_12_BIT);
        else if ((i & DS18B20_11_BIT) == DS18B20_11_BIT)
            meas &= ~(DS18B20_11_BIT_UNDF);
        else if ((i & DS18B20_10_BIT) == DS18B20_10_BIT)
            meas &= ~(DS18B20_10_BIT_UNDF);
        else { // if ( (i & DS18B20_9_BIT) == DS18B20_9_BIT ) {
            meas &= ~(DS18B20_9_BIT_UNDF);
        }
    }
    *cel = (uint8) (meas >> 4);
    *cel_frac_bits = (uint8) (meas & 0x000F);

    return D18_DS18B20_OK;
}

/* converts to decicelsius
   input is ouput from meas_to_cel
   returns absolute value of temperatur in decicelsius
        i.e.: sz=0, c=28, frac=15 returns 289 (=28.9�C)
0	0	0
1	625	625	1
2	1250	250
3	1875	875	3
4	2500	500	4
5	3125	125
6	3750	750	6
7	4375	375
8	5000	0
9	5625	625	9
10	6250	250
11	6875	875	11
12	7500	500	12
13	8125	125
14	8750	750	14
15	9375	375	*/
sint16 D18_DS18B20_TemptoDecicel(uint8 subzero, uint8 cel, uint8 cel_frac_bits) {
    sint16 h;
    uint8 i;
    uint8 need_rounding[] = {1, 3, 4, 6, 9, 11, 12, 14};

    h = cel_frac_bits * D18_DS18B20_FRACCONV / 1000;
    h += cel * 10;
    //If value is positive
    if (!subzero) {
        for (i = 0; i<sizeof (need_rounding); i++) {
            if (cel_frac_bits == need_rounding[i]) {
                h++;
                break;
            }
        }
        //Value is negative
    } else {
        //Set return value to negative
        h = -h;
    }
    return h;
}

/* compare temperature values (full celsius only)
   returns -1 if param-pair1 < param-pair2
            0 if ==
                        1 if >    */
int8 D18_DS18B20_TempCmp(uint8 subzero1, uint16 cel1,
        uint8 subzero2, uint16 cel2) {
    sint16 t1 = (subzero1) ? (cel1 * (-1)) : (cel1);
    sint16 t2 = (subzero2) ? (cel2 * (-1)) : (cel2);

    if (t1 < t2) return -1;
    if (t1 > t2) return 1;
    return 0;
}

/* find DS18X20 Sensors on 1-Wire-Bus
   input/ouput: diff is the result of the last rom-search
   output: id is the rom-code of the sensor found */
void D18_DS18B20_FindSensor(uint8 *diff, uint8 id[]) {
    for (;;) {
        *diff = OWP_Rom_Search(*diff, &id[0]);
        if (*diff == OWP_CONST_PRESENCE_ERR || *diff == OWP_CONST_DATA_ERR ||
                *diff == OWP_CONST_LAST_DEVICE) return;
        if (id[0] == DS18B20_ID || id[0] == DS18S20_ID) return;
    }
}

/* get power status of DS18x20
   input  : id = rom_code
   returns: D18_DS18B20_POWER_EXTERN or D18_DS18B20_POWER_PARASITE */
uint8 D18_DS18B20_GetPowerStatus(uint8 id[]) {
    uint8 pstat;
    OWP_Reset();
    OWP_Send_Command(D18_DS18B20_READ_POWER_SUPPLY, id);
    pstat = OWP_Bit_IO(1); // pstat 0=is parasite/ !=0 ext. powered
    OWP_Reset();
    return (pstat) ? D18_DS18B20_POWER_EXTERN : D18_DS18B20_POWER_PARASITE;
}

/* start measurement (CONVERT_T) for all sensors if input id==NULL
   or for single sensor. then id is the rom-code */
uint8 D18_DS18B20_StartMeasure(uint8 with_power_extern, uint8 id[]) {
    OWP_Reset(); //**
    if (OWP_Read_Bus()) { // only send if bus is "idle" = high
        OWP_Send_Command(D18_DS18B20_CONVERT_T, id);
        if (with_power_extern != D18_DS18B20_POWER_EXTERN)
            OWP_Parasite_Enable();
        return D18_DS18B20_OK;
    } else {
        return D18_DS18B20_START_FAIL;
    }
}

/* reads temperature (scratchpad) of sensor with rom-code id
   output: subzero==1 if temp.<0, cel: full celsius, mcel: frac
   in millicelsius*0.1
   i.e.: subzero=1, cel=18, millicel=5000 = -18,5000�C */
uint8 D18_DS18B20_ReadMeasure(uint8 id[], uint8 *subzero,
        uint8 *cel, uint8 *cel_frac_bits) {
    uint8 i;
    uint8 sp[D18_DS18B20_SP_SIZE];

    OWP_Reset(); //**
    OWP_Send_Command(D18_DS18B20_READ, id);
    for (i = 0; i < D18_DS18B20_SP_SIZE; i++) sp[i] = OWP_Byte_Read();
    if (CRC_crc8(&sp[0], D18_DS18B20_SP_SIZE, 0))
        return D18_DS18B20_ERROR_CRC;

    for (i = 0; i < D18_DS18B20_SP_SIZE; i++)
        if (sp[i] != 0xff) break;
    if (i == D18_DS18B20_SP_SIZE)
        return D18_DS18B20_ERROR_CRC;

    D18_DS18B20_MeasuretoCel(id[0], sp, subzero, cel, cel_frac_bits);
    return D18_DS18B20_OK;
}

/* reads temperature (scratchpad) of a single sensor (uses skip-rom)
   output: subzero==1 if temp.<0, cel: full celsius, mcel: frac
   in millicelsius*0.1
   i.e.: subzero=1, cel=18, millicel=5000 = -18,5000�C */
uint8 D18_DS18B20_ReadMeasureSingle(uint8 familycode, uint8 *subzero,
        uint8 *cel, uint8 *cel_frac_bits) {
    uint8 i;
    uint8 sp[D18_DS18B20_SP_SIZE];

    OWP_Send_Command(D18_DS18B20_READ, NULL);
    for (i = 0; i < D18_DS18B20_SP_SIZE; i++) sp[i] = OWP_Byte_Read();
    if (CRC_crc8(&sp[0], D18_DS18B20_SP_SIZE, 0))
        return D18_DS18B20_ERROR_CRC;
    D18_DS18B20_MeasuretoCel(familycode, sp, subzero, cel, cel_frac_bits);
    return D18_DS18B20_OK;
}

void D18_DS18B20_ShowId(uint8 *id, uint8 n) {
    size_t i;
    for (i = 0; i < n; i++) {
        if (i == 0) printf("FC:");
        else if (i == n - 1) printf("CRC:");
        if (i == 1) printf("SN: ");
        printf("%0X", id[i]);
        //lcd_putc(' ');
        //if ( i == 0 )
        //{
        //	if ( id[0] == DS18S20_ID ) lcd_puts ("(18S)");
        //	else if ( id[0] == DS18B20_ID ) lcd_puts("(18B)");
        //	else lcd_puts("( ? )");
        //}
        printf("\r\n");
    }
    //if ( crc8( id, OW_ROMCODE_SIZE) )
    //	lcd_puts( " CRC FAIL " );
    //else
    //	lcd_puts( " CRC O.K. " );
}


uint8 D18_DS18B20_WriteScratchpad(uint8 id[],
        uint8 th, uint8 tl, uint8 conf) {
    OWP_Reset(); //**
    if (OWP_Read_Bus()) { // only send if bus is "idle" = high
        OWP_Send_Command(D18_DS18B20_WRITE_SCRATCHPAD, id);
        OWP_Byte_Write(th);
        OWP_Byte_Write(tl);
        if (id[0] == DS18B20_ID) OWP_Byte_Write(conf); // config avail. on B20 only
        return D18_DS18B20_OK;
    } else {
#ifdef D18_DS18B20_VERBOSE
#endif
        return D18_DS18B20_ERROR;
    }
}

uint8 D18_DS18B20_ReadScratchpad(uint8 id[], uint8 sp[]) {
    uint8 i;

    OWP_Reset(); //**
    if (OWP_Read_Bus()) { // only send if bus is "idle" = high
        OWP_Send_Command(D18_DS18B20_READ, id);
        for (i = 0; i < D18_DS18B20_SP_SIZE; i++) sp[i] = OWP_Byte_Read();
        return D18_DS18B20_OK;
    } else {
#ifdef D18_DS18B20_VERBOSE
#endif
        return D18_DS18B20_ERROR;
    }
}

uint8 D18_DS18B20_CopyScratchpad(uint8 with_power_extern,
        uint8 id[]) {
    OWP_Reset(); //**
    if (OWP_Read_Bus()) { // only send if bus is "idle" = high
        OWP_Send_Command(D18_DS18B20_COPY_SCRATCHPAD, id);
        if (with_power_extern != D18_DS18B20_POWER_EXTERN)
            OWP_Parasite_Enable();
        DEL_Delay_ms(D18_DS18B20_COPYSP_DELAY); // wait for 10 ms
        if (with_power_extern != D18_DS18B20_POWER_EXTERN)
            OWP_Parasite_Disable();
        return D18_DS18B20_OK;
    } else {
#ifdef D18_DS18B20_VERBOSE
#endif
        return D18_DS18B20_START_FAIL;
    }
}

uint8 D18_DS18B20_RecallE2(uint8 id[]) {
    OWP_Reset(); //**
    if (OWP_Read_Bus()) { // only send if bus is "idle" = high
        OWP_Send_Command(D18_DS18B20_RECALL_E2, id);
        // TODO: wait until status is "1" (then eeprom values
        // have been copied). here simple delay to avoid timeout
        // handling
        DEL_Delay_ms(D18_DS18B20_COPYSP_DELAY);
        return D18_DS18B20_OK;
    } else {
        return D18_DS18B20_ERROR;
    }
}

