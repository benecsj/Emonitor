
#include "wifi_state_machine.h"




extern void Wifi_Manager_Init(void);

extern void Wifi_Manager_GetIp(uint8 ip[4]);

extern void Wifi_Manager_EnableHotspot(uint8 enable);


#define Wifi_Manager_Connected	wifi_station_connected
