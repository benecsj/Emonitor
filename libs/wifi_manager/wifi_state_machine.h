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

#ifndef _WIFI_STATE_MACHINE_H_
#define _WIFI_STATE_MACHINE_H_

#include "project_config.h"

/******************************************************************************
* Typedefs
\******************************************************************************/

typedef void (* wifi_state_cb_t)();
typedef void (* wifi_disco_cb_t)(uint8_t reason);

typedef enum
{
	IP,
	NETMASK,
	GATEWAY
} Wifi_Manager_Info_Type;

#if PRJ_ENV == NOS
typedef enum {
    NULL_MODE = 0,      /**< null mode */
    STATION_MODE,       /**< WiFi station mode */
    SOFTAP_MODE,        /**< WiFi soft-AP mode */
    STATIONAP_MODE,     /**< WiFi station + soft-AP mode */
    MAX_MODE
} WIFI_MODE;
#endif

/******************************************************************************
* Prototypes
\******************************************************************************/

void set_on_station_first_connect(wifi_state_cb_t cb);
void set_on_station_connect(wifi_state_cb_t cb);
void set_on_station_disconnect(wifi_disco_cb_t cb);
void set_on_client_connect(wifi_state_cb_t cb);
void set_on_client_disconnect(wifi_state_cb_t cb);

WIFI_MODE init_esp_wifi();
bool start_wifi_station(const char * ssid, const char * pass);
bool stop_wifi_station();
bool start_wifi_ap(const char * ssid, const char * pass, uint8 hidden);
bool stop_wifi_ap();

bool wifi_station_connected();
bool wifi_ap_enabled();


void wifi_get_ip_address(uint8 ip[4],Wifi_Manager_Info_Type type);

#endif /* _WIFI_STATE_MACHINE_H_ */
