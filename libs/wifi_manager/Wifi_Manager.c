#include "project_config.h"
#include "Wifi_Manager.h"
#include "spiffs_manager.h"
#include "Emonitor.h"

#if PRJ_ENV == OS
#else
#include "user_interface.h"
#endif

#define GETARRAY(a)		a[0],a[1],a[2],a[3]

uint32 Wifi_Manager_disconnectCount = 0;
uint32 Wifi_Manager_disconnectLastCount = 0;
Wifi_Manager_State Wifi_Manager_status;
uint32 Wifi_Manager_errorCounter = 0;

LOCAL void ICACHE_FLASH_ATTR on_wifi_connect(){
	DBG_WM("(WM) Connected\n");
#if DEBUG_WIFI_MANAGER
	uint8 addr[4];
	uint8 netmask[4];
	uint8 gateway[4];
	Wifi_Manager_GetIp(addr,IP);
	Wifi_Manager_GetIp(netmask,NETMASK);
	Wifi_Manager_GetIp(gateway,GATEWAY);
	DBG_WM("(WM) ip:%d.%d.%d.%d  netmask:%d.%d.%d.%d  gateway:%d.%d.%d.%d\n",GETARRAY(addr),GETARRAY(netmask),GETARRAY(gateway));
#endif
	//Set internal status
	Wifi_Manager_status = WIFI_CONNECTED;
    //Trigger level measurement
	Wifi_Manager_UpdateLevel();
}

LOCAL void ICACHE_FLASH_ATTR on_wifi_disconnect(uint8 reason){
	DBG_WM("(WM) Disconnect %d\n", reason);
	//Count disconnects for monitoring
	Wifi_Manager_disconnectCount++;
	//Set internal status
	Wifi_Manager_status = WIFI_DISCONNECTED;
}

//Configured parameters
uint8 WifiManager_enableHotspot = 1;
char WifiManager_STA_SSID[33] = {0xFF};
char WifiManager_STA_PASSWORD[65] = {0xFF};
char WifiManager_AP_SSID[33] = {0};
char WifiManager_AP_PASSWORD[65] = {0xFF};
//Signal level scanning
struct scan_config WifiManager_ScanConfig;
sint8 WifiManager_SignalLevel = 0;
uint8 WifiManager_ScanRunning = OFF;
uint8 WifiManager_ScanTiming = 0;

void ICACHE_FLASH_ATTR Wifi_Manager_Init(void)
{
	DBG_WM("(WM) Init\n",WifiManager_STA_SSID);
	//Set internal status
	Wifi_Manager_status = WIFI_INIT;
	//Verify Parameters
	uint8 textLength = 0;
	uint8 length;
	textLength = strlen(WifiManager_STA_SSID);
	if((textLength>32) || (WifiManager_STA_SSID[0] == 0xFF))
	{
		Wifi_Manager_SetSTA_SSID(DEFAULT_STA_SSID);
	}
	textLength = strlen(WifiManager_STA_PASSWORD);
	if(textLength>64 || WifiManager_STA_PASSWORD[0]== 0xFF)
	{
		Wifi_Manager_SetSTA_PASSWORD(DEFAULT_STA_PASSWORD);
	}
	textLength = strlen(WifiManager_AP_SSID);
	if((textLength == 0) || (textLength>32))
	{
		Wifi_Manager_GetDefaultId(WifiManager_AP_SSID);

	}
	textLength = strlen(WifiManager_AP_PASSWORD);
	if(textLength>64 || WifiManager_AP_PASSWORD[0]== 0xFF )
	{
		Wifi_Manager_GetDefaultPassword(WifiManager_AP_PASSWORD);

	}
    //Set hotspot value
	if((WifiManager_enableHotspot == 0xFF) || (WifiManager_enableHotspot == 1))
	{
		WifiManager_enableHotspot = 1;
	}

	DBG_WM("(WM) STA SSID: [%s]\n",WifiManager_STA_SSID);
	DBG_WM("(WM) STA PASS: [%s]\n",WifiManager_STA_PASSWORD);
	DBG_WM("(WM) AP  SSID: [%s]\n",WifiManager_AP_SSID);
	DBG_WM("(WM) AP  PASS: [%s]\n",WifiManager_AP_PASSWORD);
	DBG_WM("(WM) AP  %s\n",((WifiManager_enableHotspot ==1) ? "ENABLED" : "DISABLED"));

	//Clear default configs from Wifi if present
	if(wifi_get_opmode() != 0x0)
	{
		wifi_set_opmode(0x0);
	}

	//Register eventhandlers
    set_on_station_connect(on_wifi_connect);
    set_on_station_disconnect(on_wifi_disconnect);

    //Init Wifi
   	start_wifi_ap(WifiManager_AP_SSID, WifiManager_AP_PASSWORD,(WifiManager_enableHotspot==0));
   	start_wifi_station(WifiManager_STA_SSID, WifiManager_STA_PASSWORD);

   	//Check if special state has to be entered
   	if(WifiManager_STA_SSID[0] == 0)
   	{
   		Wifi_Manager_status = WIFI_STA_NOT_CONFIGURED;
   	}
}

void ICACHE_FLASH_ATTR Wifi_Manager_Main(void)
{
	//Based on wifi state do stuff
	switch(Wifi_Manager_status)
	{
	case WIFI_STA_NOT_CONFIGURED:
		//Do nothing
		break;
	case WIFI_INIT:
	case WIFI_DISCONNECTED:
		//Check if trying to connect
		if(Wifi_Manager_disconnectLastCount != Wifi_Manager_disconnectCount)
		{
			//Clear error count
			Wifi_Manager_errorCounter=0;

		}
		else
		{
			//Check if connecting timeouted
			Wifi_Manager_errorCounter++;
			if(Wifi_Manager_errorCounter == 10)
			{
				//Try to Reactivate reconnect
				wifi_station_set_reconnect_policy(TRUE);
			}
			if(Wifi_Manager_errorCounter>20)
			{
				//Request reset
				Emonitor_Request(EMONITOR_REQ_RESTART);
			}
		}
		//Store last count
		Wifi_Manager_disconnectLastCount = Wifi_Manager_disconnectCount;
		break;

	case WIFI_CONNECTED:
		//Scan timing
		WifiManager_ScanTiming++;
		if(WifiManager_ScanTiming == WIFI_SCAN_TIMING)
		{
			WifiManager_ScanTiming = 0;
			//Trigger scan
			Wifi_Manager_UpdateLevel();
		}

		//Monitor if still connected
		if(Wifi_Manager_IsConnected() == false)
		{
			//Set internal status
			Wifi_Manager_status = WIFI_DISCONNECTED;
		}
		break;
	}
}

void ICACHE_FLASH_ATTR ICACHE_FLASH_ATTR Wifi_Manager_ScanDone(void *arg, STATUS status)
{
	  uint8 ssid[33];
	  char temp[128];
	  if (status == OK)
	  {
	    struct bss_info *bss_link = (struct bss_info *)arg;

	    while (bss_link != NULL)
	    {
	      prj_memset(ssid, 0, 33);
	      if (strlen(bss_link->ssid) <= 32)
	      {
	        prj_memcpy(ssid, bss_link->ssid, strlen(bss_link->ssid));
	      }
	      else
	      {
	        prj_memcpy(ssid, bss_link->ssid, 32);
	      }
	      WifiManager_SignalLevel = bss_link->rssi;

	      DBG_WM("(WM) auth:%d ssid:\"%s\" rssi:%ddBm MAC:\""MACSTR"\" ch:%d\n",
	                 bss_link->authmode, ssid, bss_link->rssi,
	                 MAC2STR(bss_link->bssid),bss_link->channel);
	      bss_link = bss_link->next.stqe_next;
	    }
	  }
	  else
	  {
		  WifiManager_SignalLevel = 0;
		  DBG_WM("(WM) scan fail !!!\n");
	  }
	  WifiManager_ScanRunning = OFF;
}

void ICACHE_FLASH_ATTR Wifi_Manager_UpdateLevel(void)
{
	//Check if scan is not already running
	if(WifiManager_ScanRunning == OFF)
	{
		WifiManager_ScanRunning = ON;
		WifiManager_ScanConfig.ssid = WifiManager_STA_SSID;
		(void)wifi_station_scan(&WifiManager_ScanConfig, Wifi_Manager_ScanDone);
	}
}


void ICACHE_FLASH_ATTR Wifi_Manager_CleanUp(void)
{
    stop_wifi_station();
    stop_wifi_ap();
}


void ICACHE_FLASH_ATTR Wifi_Manager_GetIp(uint8 ip[4],Wifi_Manager_Info_Type type)
{
	wifi_get_ip_address(ip,type);
}

void ICACHE_FLASH_ATTR Wifi_Manager_EnableHotspot(uint8 enable)
{
	WifiManager_enableHotspot = enable;
}

void ICACHE_FLASH_ATTR Wifi_Manager_GetDefaultId(char* id)
{
	spiffs* fs = spiffs_get_fs();
	spiffs_file fd;
	s32_t length = 0;
	//Get id from filestorage
	fd = SPIFFS_open(fs, "/id", SPIFFS_RDONLY, 0);
	length = SPIFFS_read(fs, fd, (u8_t *)id, 32);
	SPIFFS_close(fs, fd);
	id[length-1] = 0;
}

void ICACHE_FLASH_ATTR Wifi_Manager_GetDefaultPassword(char * pass)
{
	spiffs* fs = spiffs_get_fs();
	spiffs_file fd;
	s32_t length = 0;
	//Get password from filestorage
	fd = SPIFFS_open(fs, "/pass", SPIFFS_RDONLY, 0);
	length = SPIFFS_read(fs, fd, (u8_t *)pass, 64);
	SPIFFS_close(fs, fd);
	pass[length-1] = 0;
}

