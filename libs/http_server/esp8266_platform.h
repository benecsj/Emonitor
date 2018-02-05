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
#include "spiffs_manager.h"

#define httpd_printf(format, ...) os_printf(format, ##__VA_ARGS__)
typedef struct espconn* ConnTypePtr;


#define DBG_HTTPS(...) printf(__VA_ARGS__)
//#define DBG_HTTPS(...)

#define HTTPD_MAX_CONNECTIONS 3

#define HTTPD_MAX_FILE_READ_BLOCK 1000

#define HTTPD_ALIGNMENT 4

#define HTTPD_REDIRECT_TO_HOSTNAME_BUFFER 1024
#define HTTPD_MAX_POSTBUFF_LEN 1024

extern void* aligned_malloc(size_t required_bytes);
extern void aligned_free(void *p);
