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

#ifndef __ESP_SPIFFS_H__
#define __ESP_SPIFFS_H__

#include "spiffs.h"

#ifdef __cplusplus
extern "C" {
#endif


/**
  * @brief  Initialize spiffs
  *
  * @param  struct esp_spiffs_config *config : ESP8266 spiffs configuration
  *
  * @return 0         : succeed
  * @return otherwise : fail
  */
s32_t esp_spiffs_init(void);

/**
  * @brief  Deinitialize spiffs
  *
  * @param  uint8 format : 0, only deinit; otherwise, deinit spiffs and format.
  *
  * @return null
  */
void esp_spiffs_deinit(uint8 format);

/**
  * @}
  */

#ifdef __cplusplus
}
#endif

#endif /* __ESP_SPIFFS_H__ */
