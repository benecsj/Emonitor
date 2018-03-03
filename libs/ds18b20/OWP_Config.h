/*
 * OWP_Config.h
 *
 *  Created on: Aug 12, 2012
 *      Author: Jooo
 */

#ifndef OWP_CONFIG_H_
#define OWP_CONFIG_H_

#include "project_config.h"

/*Critical session*/
#define OWP_ENTER_CRITICAL()  prj_ENTER_CRITICAL()//vTaskSuspendAll()
#define OWP_EXIT_CRITICAL()	prj_EXIT_CRITICAL() //xTaskResumeAll()

/*Port config*/
#define OWP_GET_IN()   digitalRead(OWP_Channels[OWP_Selected_Channel])
#define OWP_OUT_LOW()  digitalWrite(OWP_Channels[OWP_Selected_Channel], 0)
#define OWP_OUT_HIGH() digitalWrite(OWP_Channels[OWP_Selected_Channel], 1)
#define OWP_DIR_IN()   pinMode(OWP_Channels[OWP_Selected_Channel],INPUT);
#define OWP_DIR_OUT()  pinMode(OWP_Channels[OWP_Selected_Channel],OUTPUT_OPEN_DRAIN);

#endif /* OWP_CONFIG_H_ */
