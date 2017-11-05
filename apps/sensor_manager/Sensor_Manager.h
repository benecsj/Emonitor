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

/**********************************************************************************
 * Interfaces
 **********************************************************************************/
extern void Sensor_Manager_Init();
extern void Sensor_Manager_Main();
extern void Sensor_Manager_Fast();

extern uint8 APP_SensMan_DS18B20Count;
extern sint16 APP_SensMan_DS18B20TempList[];

#endif /* APP_SENSOR_MANAGER_H_ */
