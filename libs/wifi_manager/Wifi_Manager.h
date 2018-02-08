
#include "wifi_state_machine.h"


#if DEBUG_WIFI_MANAGER
#define DBG_WM(...) printf(__VA_ARGS__)
#else
#define DBG_WM(...)
#endif

extern void Wifi_Manager_Init(void);
extern void Wifi_Manager_CleanUp(void);

extern void Wifi_Manager_GetIp(uint8 ip[4]);

extern void Wifi_Manager_EnableHotspot(uint8 enable);


extern void Wifi_Manager_GetDefaultValues(char* id, char * pass);

#define Wifi_Manager_Connected	wifi_station_connected


extern uint8 WifiManager_enableHotspot;
extern char WifiManager_STA_SSID[33];
extern char WifiManager_STA_PASSWORD[65];
extern char WifiManager_AP_SSID[33];
extern char WifiManager_AP_PASSWORD[65];

#define Wifi_Manager_GetEnableHotspot()  (WifiManager_enableHotspot)
#define Wifi_Manager_SetEnableHotspot(a)  (WifiManager_enableHotspot = a)
#define Wifi_Manager_GetSTA_SSID()	(WifiManager_STA_SSID)
#define Wifi_Manager_SetSTA_SSID(a)	WifiManager_STA_SSID[sprintf(WifiManager_STA_SSID,"%s",a)]=0;
#define Wifi_Manager_GetSTA_PASSWORD()	(WifiManager_STA_PASSWORD)
#define Wifi_Manager_SetSTA_PASSWORD(a)	WifiManager_STA_PASSWORD[sprintf(WifiManager_STA_PASSWORD,"%s",a)]=0;
#define Wifi_Manager_GetAP_SSID()	(WifiManager_AP_SSID)
#define Wifi_Manager_SetAP_SSID(a)	WifiManager_AP_SSID[sprintf(WifiManager_AP_SSID,"%s",a)]=0;
#define Wifi_Manager_GetAP_PASSWORD()	(WifiManager_AP_PASSWORD)
#define Wifi_Manager_SetAP_PASSWORD(a)	WifiManager_AP_PASSWORD[sprintf(WifiManager_AP_PASSWORD,"%s",a)]=0;
