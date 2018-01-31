
#include "wifi_state_machine.h"




extern void Wifi_Manager_Init(void);

extern void Wifi_Manager_GetIp(uint8 ip[4]);

extern void Wifi_Manager_EnableHotspot(uint8 enable);


extern void Wifi_Manager_GetDefaultValues(char* id, char * pass);

#define Wifi_Manager_Connected	wifi_station_connected


extern uint8 WifiManager_enableHotspot;
extern char WifiManager_STA_SSID[32];
extern char WifiManager_STA_PASSWORD[64];
extern char WifiManager_AP_SSID[32];
extern char WifiManager_AP_PASSWORD[64];

#define Wifi_Manager_GetEnableHotspot()  (WifiManager_enableHotspot)
#define Wifi_Manager_SetEnableHotspot(a)  (WifiManager_enableHotspot = a)
#define Wifi_Manager_GetSTA_SSID()	(WifiManager_STA_SSID)
#define Wifi_Manager_SetSTA_SSID(a)	WifiManager_STA_SSID[sprintf("%s",a)]=0;
#define Wifi_Manager_GetSTA_PASSWORD()	(WifiManager_STA_PASSWORD)
#define Wifi_Manager_SetSTA_PASSWORD(a)	WifiManager_STA_PASSWORD[sprintf("%s",a)]=0;
#define Wifi_Manager_GetAP_SSID()	(WifiManager_AP_SSID)
#define Wifi_Manager_SetAP_SSID(a)	WifiManager_AP_SSID[sprintf("%s",a)]=0;
#define Wifi_Manager_GetAP_PASSWORD()	(WifiManager_AP_PASSWORD)
#define Wifi_Manager_SetAP_PASSWORD(a)	WifiManager_AP_PASSWORD[sprintf("%s",a)]=0;

