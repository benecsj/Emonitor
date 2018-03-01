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
#include "c_types.h"
#include "esp_libc.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"


#include "user_config.h"

/******************************************************************************
* Defines
\******************************************************************************/
#define NULL_PTR	NULL
#define DBG printf

#define prj_malloc(a)		os_malloc(a)
#define prj_free(a)			os_free(a)
//#define prj_malloc(a)		NULL_PTR
//#define prj_free(a)			NULL_PTR

#define prj_Delay(a)  	vTaskDelay((a) / portTICK_RATE_MS)
//#define prj_Delay(a)

#define	prj_ENTER_CRITICAL()	taskENTER_CRITICAL()
#define	prj_EXIT_CRITICAL()		taskEXIT_CRITICAL()
//#define	prj_ENTER_CRITICAL()
//#define	prj_EXIT_CRITICAL()

#define	prj_DisableInt()		PortDisableInt_NoNest()
#define prj_EnableInt()			PortEnableInt_NoNest()
//#define	prj_DisableInt()
//#define prj_EnableInt()

#define prj_createTask			xTaskCreate
//#define prj_createTask(a,...)		a(NULL_PTR)

#define prj_TaskDelete			vTaskDelete
//#define prj_TaskDelete(a)


#endif

