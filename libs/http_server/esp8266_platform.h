// Combined include file for esp8266
// Actually misnamed, as it also works for ESP32.
// ToDo: Figure out better name

/*
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <user_interface.h>


#include <c_types.h>
#include "limits.h"
#include "espconn.h"
//#include "esp_misc.h"
#include <ets_sys.h>
#include <mem.h>
//#include <osapi.h>

#include "platform.h"



//#include "osapi.h"

*/

#include "c_types.h"
#include "limits.h"
#include "lwip/tcpip.h"
#include "lwip/ip.h"
#include "lwip/ip_addr.h"
#include "espconn.h"
#include "esp_misc.h"
#include "esp_wifi.h"
#include "esp_libc.h"
#include "esp_fs.h"
#include "project_config.h"

typedef struct espconn* ConnTypePtr;


#if DEBUG_HTTP_SERVER == 1
#define DBG_HTTPS(...) printf(__VA_ARGS__)
#define DBG2_HTTPS(...) printf(__VA_ARGS__)
#else
#define DBG_HTTPS(...)
#endif

#ifndef DBG2_HTTPS
#if DEBUG_HTTP_SERVER == 2
#define DBG2_HTTPS(...) printf(__VA_ARGS__)
#else
#define DBG2_HTTPS(...)
#endif
#endif

