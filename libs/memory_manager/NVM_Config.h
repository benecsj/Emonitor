/*
 * NVM_Config.h
 *
 *  Created on: Aug 13, 2012
 *      Author: Jooo
 */

#ifndef NVM_CONFIG_H_
#define NVM_CONFIG_H_


/*DATA ACCESS*/
#include "NVM_NonVolatileMemory.h"

extern uint8 NVM_test_value;
extern uint8 Emonitor_ledControl;
extern uint8 WifiManager_enableHotspot;
extern uint32 Emonitor_nodeId;
extern uint32 Emonitor_SendPeroid;
extern char Emonitor_url[100];
extern char Emonitor_key[33];
extern char WifiManager_STA_SSID[33];
extern char WifiManager_STA_PASSWORD[65];
extern char WifiManager_AP_SSID[33];
extern char WifiManager_AP_PASSWORD[65];


/*DATA BLOCKS*/
#define NVM_CFG_STORAGE	\
	/*	Variable                    Size	   */\
	NVM_DATA(NVM_test_value,       sizeof(NVM_test_value))\
	NVM_DATA(Emonitor_ledControl,       sizeof(Emonitor_ledControl))\
	NVM_DATA(WifiManager_enableHotspot,       sizeof(WifiManager_enableHotspot))\
	NVM_DATA(Emonitor_nodeId,       sizeof(Emonitor_nodeId))\
	NVM_DATA(Emonitor_SendPeroid,       sizeof(Emonitor_SendPeroid))\
	NVM_DATA(Emonitor_url,       100)\
	NVM_DATA(Emonitor_key,       33)\
	NVM_DATA(WifiManager_STA_SSID,       33)\
	NVM_DATA(WifiManager_STA_PASSWORD,       65)\
	NVM_DATA(WifiManager_AP_SSID,       33)\
	NVM_DATA(WifiManager_AP_PASSWORD,       65)\

#define NVM_DATA_POS_DATA_START 16

/*NVM PARAMETERS*/
#define NVM_CFG_STORAGE_SIZE		(8000)
#define NVM_CFG_BLOCK_SIZE		    1000
#define NVM_CFG_MAX_DATA_COUNT		20

/* SAVE TIMING */
#define NVM_CFG_CYCLIC_SAVE			(0)
#define NVM_CFG_MAIN_CYCLE_MS       1000
#define	NVM_CFG_SAVE_TIMING			60

#endif /* NVM_CONFIG_H_ */
