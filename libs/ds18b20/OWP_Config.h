/*
 * OWP_Config.h
 *
 *  Created on: Aug 12, 2012
 *      Author: Jooo
 */

#ifndef OWP_CONFIG_H_
#define OWP_CONFIG_H_

#include "c_types.h"
#include "stdint.h"

#include "pins.h"
#include "user_config.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

/*Critical session*/
#define OWP_ENTER_CRITICAL()  taskENTER_CRITICAL()
#define OWP_EXIT_CRITICAL()	taskEXIT_CRITICAL()
//#define OWP_ENTER_CRITICAL()
//#define OWP_EXIT_CRITICAL()
/*Port config*/

#define OWP_GET_IN()   digitalRead(ONEWIRE_BUS)
#define OWP_OUT_LOW()  digitalWrite(ONEWIRE_BUS, 0)
#define OWP_OUT_HIGH() digitalWrite(ONEWIRE_BUS, 1)
#define OWP_DIR_IN()   pinMode(ONEWIRE_BUS,INPUT);
#define OWP_DIR_OUT()  pinMode(ONEWIRE_BUS,OUTPUT_OPEN_DRAIN);

#endif /* OWP_CONFIG_H_ */
