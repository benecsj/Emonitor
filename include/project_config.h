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

#ifndef __PROJECT_CONFIG_H__
#define __PROJECT_CONFIG_H__

/******************************************************************************
* Includes
\******************************************************************************/
#include "user_config.h"

#if PRJ_ENV == OS
#include "c_types.h"
#include "esp_libc.h"

//common
#include "esp_common.h"

//pins
#include "esp8266/ets_sys.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#else

//common
#include "eagle_soc.h"

//pins
#include "ets_sys.h"

#endif
/******************************************************************************
* Defines
\******************************************************************************/
#define NULL_PTR	NULL
#if PRJ_ENV == OS
#define DBG printf
#else
#define DBG os_printf
#endif

#if PRJ_ENV == OS
#define prj_malloc(a)		os_malloc(a)
#define prj_free(a)			os_free(a)
#else
#define prj_malloc(a)		NULL_PTR
#define prj_free(a)			NULL_PTR
#endif

#if PRJ_ENV == OS
#define prj_Delay(a)  	vTaskDelay((a) / portTICK_RATE_MS)
#else
#define prj_Delay(a)
#endif

#if PRJ_ENV == OS
#define	prj_ENTER_CRITICAL()	taskENTER_CRITICAL()
#define	prj_EXIT_CRITICAL()		taskEXIT_CRITICAL()
#else
#define	prj_ENTER_CRITICAL()
#define	prj_EXIT_CRITICAL()
#endif

#if PRJ_ENV == OS
#define	prj_DisableInt()		PortDisableInt_NoNest()
#define prj_EnableInt()			PortEnableInt_NoNest()
#else
#define	prj_DisableInt()
#define prj_EnableInt()
#endif

#if PRJ_ENV == OS
#define prj_createTask			xTaskCreate
#else
#define prj_createTask(a,...)		a(NULL_PTR)
#endif

#if PRJ_ENV == OS
#define prj_TaskDelete			vTaskDelete
#else
#define prj_TaskDelete(a)
#endif

#if PRJ_ENV == OS
#define prj_TaskHandle			xTaskHandle
#else
#define prj_TaskHandle				uint32
#endif

#if PRJ_ENV == OS
#define prj_flash_size_map			flash_size_map
#else
#define prj_flash_size_map			enum flash_size_map
#endif

#if PRJ_ENV == OS
#define LOOP						while(TRUE);
#else
#define LOOP						while(FALSE);
#endif

#endif

