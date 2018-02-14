/*
 * APP_Sensor_Manager.h
 *
 *  Created on: Aug 13, 2012
 *      Author: Jooo
 */

#ifndef SENSOR_MANAGER_H_
#define SENSOR_MANAGER_H_


/**********************************************************************************
 * Includes
 **********************************************************************************/
#include "c_types.h"
#include "stdint.h"

/**********************************************************************************
 * Defines
 **********************************************************************************/
#define SENSOR_MANAGER_ANALOGCHANNELS_COUNT 1
#define SENSOR_MANAGER_DS18B20MAXCOUNT 16
#define SENSOR_MANAGER_PULSE_COUNTERS 5

/**********************************************************************************
 * Interfaces
 **********************************************************************************/
extern void Sensor_Manager_Init();
extern void Sensor_Manager_Main();
extern void Sensor_Manager_Fast();

extern void Sensor_Manager_Get_TempSensorData(uint8* count, uint8** ids,sint16** temperatures );
extern uint32 Sensor_Manager_GetPulseCount(uint8 id);
extern uint32 Sensor_Manager_GetPulseLevel(uint8 id);
extern void Sensor_Manager_ResetPulseCounters(void);
extern uint16 Sensor_Manager_GetAnalogValue(void);

extern uint32 Sensor_Manager_ErrorCounter;
extern uint8 APP_SensMan_DS18B20Count;
extern sint16 APP_SensMan_DS18B20TempList[];

#define Sensor_Manager_GetTempHealth()	(Sensor_Manager_ErrorCounter)

#endif /* APP_SENSOR_MANAGER_H_ */
