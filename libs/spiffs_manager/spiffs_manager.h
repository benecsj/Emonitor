
#ifndef __SPIFFS_MANAGER_H__
#define __SPIFFS_MANAGER_H__

/******************************************************************************
* Includes
\******************************************************************************/

#include "project_config.h"
#include "esp_spiffs.h"

/******************************************************************************
* Defines
\******************************************************************************/

#if DEBUG_SPIFFS
#define SPIFFSM_DBG(...) os_printf(__VA_ARGS__)
#else
#define SPIFFSM_DBG(...)
#endif



/******************************************************************************
* Primitives
\******************************************************************************/

extern void spiffs_init(void);
extern void spiffs_format(void);
extern void spiffs_test_read(void);
extern void spiffs_test_write(void);
extern void spiffs_status(void);

extern spiffs* spiffs_get_fs(void);

#endif

