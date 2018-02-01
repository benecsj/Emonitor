#include "esp_common.h"
#include "user_config.h"
#include "Wifi_Manager.h"
#include "spiffs_manager.h"

LOCAL void ICACHE_FLASH_ATTR on_wifi_connect(){
    os_printf("Connected\n");
    //Store current ip address

}

LOCAL void ICACHE_FLASH_ATTR on_wifi_disconnect(uint8 reason){
    os_printf("Disconnect %d\n", reason);
}

uint8 WifiManager_enableHotspot = 1;
char WifiManager_STA_SSID[33] = {0xFF};
char WifiManager_STA_PASSWORD[65] = {0xFF};
char WifiManager_AP_SSID[33] = {0};
char WifiManager_AP_PASSWORD[65] = {0xFF};

void Wifi_Manager_Init(void)
{
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
		Wifi_Manager_GetDefaultValues(WifiManager_AP_SSID,NULL);

	}
	textLength = strlen(WifiManager_AP_PASSWORD);
	if(textLength>64 || WifiManager_AP_PASSWORD[0]== 0xFF )
	{
		Wifi_Manager_GetDefaultValues(NULL,WifiManager_AP_PASSWORD);

	}

	DBG_WM("(WF) STA SSID: [%s]\n",WifiManager_STA_SSID);
	DBG_WM("(WF) STA PASS: [%s]\n",WifiManager_STA_PASSWORD);
	DBG_WM("(WF) AP  SSID: [%s]\n",WifiManager_AP_SSID);
	DBG_WM("(WF) AP  PASS: [%s]\n",WifiManager_AP_PASSWORD);

	//Register eventhandlers
    set_on_station_connect(on_wifi_connect);
    set_on_station_disconnect(on_wifi_disconnect);
    //Init Wifi
    init_esp_wifi();
    start_wifi_station(WifiManager_STA_SSID, WifiManager_STA_PASSWORD);
    start_wifi_ap(WifiManager_AP_SSID, WifiManager_AP_PASSWORD,1);
    stop_wifi_station();
    stop_wifi_ap();


    start_wifi_station(WifiManager_STA_SSID, WifiManager_STA_PASSWORD);

    //Set hotspot value
    if(WifiManager_enableHotspot == 0xFF)
    {
    	WifiManager_enableHotspot = 1;
    }

	if(WifiManager_enableHotspot == 1)
	{
		DBG_WM("(WF) AP  ENABLED\n",WifiManager_AP_PASSWORD);
	}

   	start_wifi_ap(WifiManager_AP_SSID, WifiManager_AP_PASSWORD,(WifiManager_enableHotspot==0));

}


void Wifi_Manager_GetIp(uint8 ip[4])
{
	wifi_get_ip_address(ip);
}

void Wifi_Manager_Main()
{

}

void Wifi_Manager_EnableHotspot(uint8 enable)
{
	//Check if state was changed
	if(WifiManager_enableHotspot != enable)
	{
		//Perform execution of the operation
		if(enable == 1)
		{
			//start_wifi_ap(WifiManager_AP_SSID, WifiManager_AP_PASSWORD);
		}
		else
		{
			//stop_wifi_ap();
		}
	}
	//Store request
	WifiManager_enableHotspot = enable;

}

void Wifi_Manager_GetDefaultValues(char* id, char * pass)
{
	spiffs* fs = spiffs_get_fs();
	spiffs_file fd;
	s32_t length = 0;
	if(id != NULL)
	{
		fd = SPIFFS_open(fs, "/id", SPIFFS_RDONLY, 0);
		length = SPIFFS_read(fs, fd, (u8_t *)id, 32);
		SPIFFS_close(fs, fd);
		id[length-1] = 0;
	}
	if(pass != NULL)
	{
		fd = SPIFFS_open(fs, "/pass", SPIFFS_RDONLY, 0);
		length = SPIFFS_read(fs, fd, (u8_t *)pass, 64);
		SPIFFS_close(fs, fd);
		pass[length-1] = 0;
	}
}


