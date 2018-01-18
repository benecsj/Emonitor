#include "esp_common.h"
#include "user_config.h"


LOCAL void ICACHE_FLASH_ATTR on_wifi_connect(){
    os_printf("Connected\n");
    //Store current ip address

}

LOCAL void ICACHE_FLASH_ATTR on_wifi_disconnect(uint8 reason){
    os_printf("Disconnect %d\n", reason);
}


void Wifi_Manager_Init(void)
{
	//Register eventhandlers
    set_on_station_connect(on_wifi_connect);
    set_on_station_disconnect(on_wifi_disconnect);
    //Init Wifi
    init_esp_wifi();
    start_wifi_station(STA_SSID, STA_PASSWORD);
    start_wifi_ap(AP_SSID, AP_PASSWORD);
    stop_wifi_station();
    stop_wifi_ap();

}


void Wifi_Manager_GetIp(uint8 ip[4])
{
	wifi_get_ip_address(ip);
}
