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

//HW VARIANT
#define VARIANT		1

//ENVIROMENT
#define NOS			0
#define OS			1
#define PRJ_ENV		OS

//WIFI
#define DEFAULT_STA_SSID 		""
#define DEFAULT_STA_PASSWORD 	""
#define DEFAULT_API_KEY 		""
#define WIFI_SCAN_TIMING 		60

//UART
#define UART_CHANNEL			(UART0)   // UART0 | UART1 | UART_OFF
#define UART_BAUDRATE			BIT_RATE_460800

//NvM
#define EMONITOR_PARAM_START_SEC		0x8D

//SPIFFS
#define FS1_FLASH_SIZE      (256*1024*2)
#define FS1_FLASH_ADDR      (0x100000)
#define SECTOR_SIZE         (4*1024)
#define LOG_BLOCK           (SECTOR_SIZE)
#define LOG_PAGE            (128)
#define FD_BUF_SIZE         32*4
#define CACHE_BUF_ENABLE	(OFF)
#define CACHE_BUF_SIZE      (LOG_PAGE + 32)*8

//Pins
#if VARIANT == 0
	#define LED_BUILTIN		D0
	#define ONEWIRE_BUS_0	D1
	#define ONEWIRE_BUS_1	D2
	#define PULSE_INPUT0    D7
	#define PULSE_INPUT1    D6
	#define PULSE_INPUT2    D5
	#define PULSE_INPUT3    D4
	#define FLASH_BUTTON    D3
#else
	#define LED_BUILTIN		D4
	#define ONEWIRE_BUS_0	D1
	#define ONEWIRE_BUS_1	D2
	#define PULSE_INPUT0    D0
	#define PULSE_INPUT1    D5
	#define PULSE_INPUT2    D6
	#define PULSE_INPUT3    D7
	#define FLASH_BUTTON    D3
#endif

//Remote(Serial) Control
#define REMOTE_CONTROL_ENABLE	(ON)
#define REMOTE_TX_SIZE			150
#define REMOTE_RX_SIZE			50

//Emonitor
#define TIMEOUT_TURNON_AP 		3600
#define TIMEOUT_RESET_NO_COMM 	3600*2
#define TIMEOUT_TURNOFF_APP 	43200*2
#define LED_TIMING_NORMAL		500
#define LED_TIMING_RESET		100
#define LED_STRENGTH			50
#define DEFAULT_SEND_TIMING		10
#define SHORT_PRESS_MIN			100
#define LONG_PRESS_MIN			8000
#define CPU_USAGE_AVERAGE		8
#define RESPONSE_TIME_OUT		20
#define STATUS_LED_INVERT		(ON)

//Sensor manager
#if VARIANT == 0
	#define ANALOG_ENABLE			(ON)
#else
	#define ANALOG_ENABLE			(OFF)
#endif
#define OWP_CHANNELS_COUNT 			2
#define TEMP_RESCAN_PERIOD			1
#define TEMP_MAX_RETRY_COUNT 		5
#define MHZ14_INPUT_PIN				PULSE_INPUT3

//Remote control
#define REMOTE_PRINT_PROMPT			(OFF)
#define REMOTE_LOGIN_NEEDED			(OFF)
#define REMOTE_MAX_COMMANDS			10
#define REMOTE_MAX_INPUT			150
#define REMOTE_MAX_COMMAND_ARGS 	4
#define REMOTE_MAX_OUTPUT			70

//Web server
#define HTTPD_MAX_HEAD_LEN					1024
#define HTTPD_MAX_POST_LEN					256
#define HTTPD_MAX_SENDBUFF_LEN				1000
#define HTTPD_REDIRECT_TO_HOSTNAME_BUFFER 	512
#define HTTPD_MAX_POSTBUFF_LEN  			400
#define HTTPD_MAX_FILE_READ_BLOCK 			(HTTPD_MAX_SENDBUFF_LEN - 24)
#define HTTPD_MAX_CONNECTIONS   			3
#define HTTPD_TCP_TIMEOUT   				300
#define HTTPD_TCP_IDLE_TIMEOUT 				60
#define HTTPD_TCP_NO_CONNECTION_TIMEOUT  	120

//Web client
#define HTTPC_RESPONE_BUFFER_SIZE		500
#define HTTPC_POST_DATA_BUFFER_SIZE		1
#define HTTPC_HOST_NAME_MAX_SIZE		100
#define HTTPC_PATH_MAX_SIZE				600
#define HTTPC_HEADERS_MAX_SIZE			100

//Development flags
#define EMONITOR_TIMING_TEST    (OFF)
#define DEBUG_LIBS				(OFF)
#define DEBUG_SENSOR_MANAGER 	(OFF)
#define DEBUG_EMONITOR 			(OFF)
#define DEBUG_REMOTE_CONTROL	(OFF)
#define DEBUG_HTTP_CLIENT 		(OFF)
#define DEBUG_HTTP_SERVER 		(OFF)
#define DEBUG_WIFI_MANAGER		(OFF)
#define DEBUG_MHZ14				(OFF)
#define DEBUG_SPIFFS 			(OFF)
#define DEBUG_NVM				(ON)
#define DEBUG_MAIN				(ON)

#endif

