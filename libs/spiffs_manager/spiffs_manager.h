
#ifndef __SPIFFS_MANAGER_H__
#define __SPIFFS_MANAGER_H__

/******************************************************************************
* Defines
\******************************************************************************/

#define SPIFFSM_DBG printf

/******************************************************************************
* Primitives
\******************************************************************************/

extern void spiffs_init(void);
extern void spiffs_format(void);
extern void spiffs_test_read(void);
extern void spiffs_test_write(void);
#endif

