#include "esp_common.h"
#include "user_config.h"

static os_timer_t timer;

LOCAL void ICACHE_FLASH_ATTR wait_for_connection_ready(uint8 flag)
{
    os_timer_disarm(&timer);
    if(wifi_station_connected()){
        os_printf("connected\n");
    } else {
        os_printf("reconnect after 2s\n");
        os_timer_setfn(&timer, (os_timer_func_t *)wait_for_connection_ready, NULL);
        os_timer_arm(&timer, 2000, 0);
    }
}

LOCAL void ICACHE_FLASH_ATTR on_wifi_connect(){
    os_timer_disarm(&timer);
    os_timer_setfn(&timer, (os_timer_func_t *)wait_for_connection_ready, NULL);
    os_timer_arm(&timer, 100, 0);
}

LOCAL void ICACHE_FLASH_ATTR on_wifi_disconnect(uint8 reason){
    os_printf("disconnect %d\n", reason);
}


void Wifi_Manager_Init(void)
{
    set_on_station_connect(on_wifi_connect);
    set_on_station_disconnect(on_wifi_disconnect);
    init_esp_wifi();
    //stop_wifi_station();
    //stop_wifi_ap();
    start_wifi_station("BOCI", "fucking6");
    start_wifi_ap(SSID, PASSWORD);
    //wifi_set_opmode(STATION_MODE);
}
