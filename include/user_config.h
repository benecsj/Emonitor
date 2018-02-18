/*
 * ESPRSSIF MIT License
 *
 * Copyright (c) 2015 <ESPRESSIF SYSTEMS (SHANGHAI) PTE LTD>
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

#ifndef __USER_CONFIG_H__
#define __USER_CONFIG_H__

//WIFI
#define DEFAULT_STA_SSID 		"BOCI"
#define DEFAULT_STA_PASSWORD 	"fucking6"
#define DEFAULT_SERVER_ADDRESS 	"http://v9.emonitor.hu"
#define DEFAULT_API_KEY 		"97d3e42a841ea6c219582211313d5051"
#define WIFI_SCAN_TIMING 		60

//NvM
#define EMONITOR_PARAM_START_SEC		0x8D

//SPIFFS
#define FS1_FLASH_SIZE      (256*1024)
#define FS2_FLASH_SIZE      (256*1024)

#define FS1_FLASH_ADDR      (1024*1024)
#define FS2_FLASH_ADDR      (1280*1024)

#define SECTOR_SIZE         (4*1024)
#define LOG_BLOCK           (SECTOR_SIZE)
#define LOG_PAGE            (128)

#define FD_BUF_SIZE         32*4
#define CACHE_BUF_SIZE      (LOG_PAGE + 32)*8

//Pins
#define LED_BUILTIN		D0
#define ONEWIRE_BUS_0	D1     //D1
#define ONEWIRE_BUS_1	D2     //D2
#define PULSE_INPUT0    D7     //D7
#define PULSE_INPUT1    D6     //D6
#define PULSE_INPUT2    D5
#define PULSE_INPUT3    D4
#define FLASH_BUTTON    D3

//Emonitor
#define TIMEOUT_TURNON_AP 		43200
#define TIMEOUT_RESET_NO_COMM 	43200*2
#define TIMEOUT_TURNOFF_APP 	43200*2
#define LED_TIMING_NORMAL		500
#define LED_TIMING_RESET		100
#define DEFAULT_SEND_TIMING		10

//Sensor manager
#define OWP_CHANNELS_COUNT 	2
#define TEMP_RESCAN_PERIOD	2

//Web server
#define HTTPD_MAX_HEAD_LEN		1024
#define HTTPD_MAX_POST_LEN		512
#define HTTPD_MAX_SENDBUFF_LEN	1024
#define HTTPD_MAX_FILE_READ_BLOCK HTTPD_MAX_SENDBUFF_LEN -24
#define HTTPD_MAX_CONNECTIONS 5


//Development flags
#define EMONITOR_TIMING_TEST    0
#define DEBUG_SENSOR_MANAGER 	0
#define DEBUG_EMONITOR 			0
#define DEBUG_HTTP_CLIENT 		0
#define DEBUG_HTTP_SERVER 		1
#define DEBUG_WIFI_MANAGER		0


#endif

