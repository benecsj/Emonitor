/*
 * ESPRSSIF MIT License
 *
 * Copyright (c) 2017 <ESPRESSIF SYSTEMS (SHANGHAI) PTE LTD>
 *
 * Permission is hereby granted for use on ESPRESSIF SYSTEMS ESP8266 only, in which case,
 * it is free of charge, to any person obtaining a copy of this software and associated
 * documentation files (the "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the Software is furnished
 * to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all copies or
 * substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
 * FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
 * COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
 * IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 */

#include "Wifi_Manager.h"
#if PRJ_ENV == OS
#include <stddef.h>
#include "espressif/c_types.h"
#include "lwipopts.h"
#include "lwip/ip_addr.h"
#include "espressif/esp_libc.h"
#include "espressif/esp_misc.h"
#include "espressif/esp_common.h"
#include "espressif/esp_wifi.h"
#include "espressif/esp_sta.h"
#include "espressif/esp_softap.h"
#else
#include "mem.h"
#include "user_interface.h"
#endif

typedef void (* wifi_state_cb_t)();

wifi_state_cb_t on_station_first_connect = NULL;
wifi_state_cb_t on_station_connect = NULL;
wifi_disco_cb_t on_station_disconnect = NULL;

wifi_state_cb_t on_client_connect = NULL;
wifi_state_cb_t on_client_disconnect = NULL;

volatile bool wifi_station_static_ip = false;
volatile bool wifi_station_is_connected = false;

void ICACHE_FLASH_ATTR wifi_event_handler_cb(System_Event_t *event)
{
    static bool station_was_connected = false;
    if (event == NULL) {
        return;
    }

#if PRJ_ENV == OS
    switch (event->event_id) {
#else
    switch (event->event) {
#endif
        case EVENT_STAMODE_DISCONNECTED:
            DBG_WM("(WM) Event STA DIS\n");
            wifi_station_is_connected = false;
            Event_StaMode_Disconnected_t *ev = (Event_StaMode_Disconnected_t *)&event->event_info;
            if(on_station_disconnect){
                on_station_disconnect(ev->reason);
            }
            break;
        case EVENT_STAMODE_CONNECTED:
            DBG_WM("(WM) Event STA CON\n");
            if(wifi_station_static_ip){
                wifi_station_is_connected = true;
                if(!station_was_connected){
                    station_was_connected = true;
                    if(on_station_first_connect){
                        on_station_first_connect();
                    }
                }
                if(on_station_connect){
                    on_station_connect();
                }
            }
            break;
        case EVENT_STAMODE_DHCP_TIMEOUT:
            DBG_WM("(WM) Event DHCP TIMEOUT\n");
            if(wifi_station_is_connected){
                wifi_station_is_connected = false;
                if(on_station_disconnect){
                    on_station_disconnect(REASON_UNSPECIFIED);
                }
            }
            break;
        case EVENT_STAMODE_GOT_IP:
            DBG_WM("(WM) Event GOT IP\n");
            wifi_station_is_connected = true;
            if(!station_was_connected){
                station_was_connected = true;
                if(on_station_first_connect){
                    on_station_first_connect();
                }
            }
            if(on_station_connect){
                on_station_connect();
            }
            break;

        case EVENT_SOFTAPMODE_STACONNECTED:
            DBG_WM("(WM) Event STA CON\n");
            if(on_client_connect){
                on_client_connect();
            }
            break;
        case EVENT_SOFTAPMODE_STADISCONNECTED:
            DBG_WM("(WM) Event STA DIS\n");
            if(on_client_disconnect){
                on_client_disconnect();
            }
            break;
        case EVENT_STAMODE_SCAN_DONE:
            DBG_WM("(WM) Event SCAN DONE\n");
        	break;

        case EVENT_STAMODE_AUTHMODE_CHANGE:
            DBG_WM("(WM) Event AUTH CHG\n");
        	break;

        case EVENT_SOFTAPMODE_PROBEREQRECVED:
            //DBG_WM("(WM) Event PROBE REC\n");
        	break;

        default:
            break;
    }
}

void ICACHE_FLASH_ATTR set_on_station_first_connect(wifi_state_cb_t cb){
    on_station_first_connect = cb;
}

void ICACHE_FLASH_ATTR set_on_station_connect(wifi_state_cb_t cb){
    on_station_connect = cb;
}

void ICACHE_FLASH_ATTR set_on_station_disconnect(wifi_disco_cb_t cb){
    on_station_disconnect = cb;
}

void ICACHE_FLASH_ATTR set_on_client_connect(wifi_state_cb_t cb){
    on_client_connect = cb;
}

void ICACHE_FLASH_ATTR set_on_client_disconnect(wifi_state_cb_t cb){
    on_client_disconnect = cb;
}

bool ICACHE_FLASH_ATTR wifi_set_mode(WIFI_MODE mode){
    if(!mode){
        bool s = wifi_set_opmode_current(mode);
        wifi_fpm_set_sleep_type(MODEM_SLEEP_T);
        wifi_fpm_open();
        wifi_fpm_do_sleep(0xFFFFFFFF);
        return s;
    }
    wifi_fpm_close();
    return wifi_set_opmode_current(mode);
}

WIFI_MODE ICACHE_FLASH_ATTR init_esp_wifi(){
    wifi_set_event_handler_cb(wifi_event_handler_cb);
    WIFI_MODE mode = 0;
    wifi_set_mode(mode);
    //system_phy_set_max_tpw(82);
    return mode;
}

bool ICACHE_FLASH_ATTR start_wifi_station(const char * ssid, const char * pass){
    WIFI_MODE mode = wifi_get_opmode();
    if((mode & STATION_MODE) == 0){
        mode |= STATION_MODE;
        if(!wifi_set_mode(mode)){
        	DBG_WM("(WM) Failed to enable Station mode!\n");
            return false;
        }
    }
    if(!ssid[0] != 0){
    	DBG_WM("(WM) No SSID Given\n");
        return true;
    }

    bool status = wifi_station_set_hostname((char*)ssid);

    struct station_config config;
    prj_memset(&config, 0, sizeof(struct station_config));
    strcpy(config.ssid, ssid);
    if(pass){
        strcpy(config.password, pass);
    }
    if(!wifi_station_set_config_current(&config)){
    	DBG_WM("(WM) Failed to set Station config!\n");
        return false;
    }

    if(!wifi_station_dhcpc_status()){
    	DBG_WM("(WM) DHCP is not started. Starting it...\n");
        if(!wifi_station_dhcpc_start()){
        	DBG_WM("(WM) DHCP start failed!\n");
            return false;
        }
    }
    return wifi_station_connect();
}

bool ICACHE_FLASH_ATTR stop_wifi_station(){
    WIFI_MODE mode = wifi_get_opmode();
    mode &= ~STATION_MODE;
    if(!wifi_set_mode(mode)){
    	DBG_WM("(WM) Failed to disable Station mode!\n");
        return false;
    }
    return true;
}

bool ICACHE_FLASH_ATTR start_wifi_ap(const char * ssid, const char * pass, uint8 hidden){
    WIFI_MODE mode = wifi_get_opmode();
    if((mode & SOFTAP_MODE) == 0){
        mode |= SOFTAP_MODE;
        if(!wifi_set_mode(mode)){
        	DBG_WM("(WM) Failed to enable AP mode!\n");
            return false;
        }
    }
    //Check ssid parameter
    if((ssid == NULL) || (strlen(ssid)==0) )
    {
    	DBG_WM("(WM) No SSID Given. Will start the AP saved in flash\n");
        return true;
    }

    //Create struct for config
    struct softap_config config;
    bzero(&config, sizeof(struct softap_config));
    //Set SSID
    prj_sprintf(config.ssid, ssid);
    //Set PASSWORD
    if((pass != NULL) && (strlen(pass)>0))
    {
        prj_sprintf(config.password, pass);
        config.authmode = AUTH_WPA_WPA2_PSK;
    }
    else
    {
    	config.password[0] = 0;
    	config.authmode = AUTH_OPEN;
    }
    if(hidden == 1)
    {
        config.max_connection = 0;
        config.ssid_hidden = TRUE;
        config.beacon_interval = 60000;
    }
    else
    {
        config.max_connection = 4;
        config.ssid_hidden = FALSE;
        config.beacon_interval = 1000;
    }

    wifi_softap_set_config_current(&config);
    wifi_set_opmode(STATIONAP_MODE);

    wifi_softap_dhcps_stop();
    struct ip_info info;

    IP4_ADDR(&info.ip, 10, 10, 10, 1);
    IP4_ADDR(&info.gw, 10, 10, 10, 1);
    IP4_ADDR(&info.netmask, 255, 255, 255, 0);
    wifi_set_ip_info(SOFTAP_IF, &info);
    wifi_softap_dhcps_start();

    return TRUE;
}

bool ICACHE_FLASH_ATTR stop_wifi_ap(){
    WIFI_MODE mode = wifi_get_opmode();
    mode &= ~SOFTAP_MODE;
    if(!wifi_set_mode(mode)){
    	DBG_WM("(WM) Failed to disable AP mode!\n");
        return false;
    }
    return true;
}

bool ICACHE_FLASH_ATTR wifi_station_connected(){
    if(!wifi_station_is_connected){
        return false;
    }
    WIFI_MODE mode = wifi_get_opmode();
    if((mode & STATION_MODE) == 0){
        return false;
    }
    uint8 wifistate = wifi_station_get_connect_status();
    wifi_station_is_connected = (wifistate == STATION_GOT_IP || (wifi_station_static_ip && wifistate == STATION_CONNECTING));
    return wifi_station_is_connected;
}

bool ICACHE_FLASH_ATTR wifi_ap_enabled(){
    return !!(wifi_get_opmode() & SOFTAP_MODE);
}

void ICACHE_FLASH_ATTR wifi_get_ip_address(uint8 ip[4],Wifi_Manager_Info_Type type){
	bool returnValue;
	struct ip_info data;
	returnValue = wifi_get_ip_info(0x00, &data); // 0x00 for STATION_IF, 0x01 for SOFTAP_IF.
	//If succesfully get ip then extract it
	if(returnValue)
	{
		switch(type)
		{
		case IP:
			ip[0] = (data.ip.addr) & 0xFF;
			ip[1] = (data.ip.addr>>8) & 0xFF;;
			ip[2] = (data.ip.addr>>16) & 0xFF;;
			ip[3] = (data.ip.addr>>24) & 0xFF;;
			break;
		case NETMASK:
			ip[0] = (data.netmask.addr) & 0xFF;
			ip[1] = (data.netmask.addr>>8) & 0xFF;;
			ip[2] = (data.netmask.addr>>16) & 0xFF;;
			ip[3] = (data.netmask.addr>>24) & 0xFF;;
			break;
		case GATEWAY:
			ip[0] = (data.gw.addr) & 0xFF;
			ip[1] = (data.gw.addr>>8) & 0xFF;;
			ip[2] = (data.gw.addr>>16) & 0xFF;;
			ip[3] = (data.gw.addr>>24) & 0xFF;;
			break;
		}
	}
	else
	{
		ip[0] = 0;
		ip[1] = 0;
		ip[2] = 0;
		ip[3] = 0;
	}
}


