#include "esp_common.h"
#include "user_config.h"


LOCAL void ICACHE_FLASH_ATTR on_wifi_connect(){
    os_printf("Connected\n");
}

LOCAL void ICACHE_FLASH_ATTR on_wifi_disconnect(uint8 reason){
    os_printf("Disconnect %d\n", reason);
}


void Wifi_Manager_Init(void)
{
    set_on_station_connect(on_wifi_connect);
    set_on_station_disconnect(on_wifi_disconnect);
    init_esp_wifi();
    stop_wifi_station();
    stop_wifi_ap();
    //start_wifi_station("BOCI", "fucking6");
    start_wifi_ap(SSID, PASSWORD);


}
