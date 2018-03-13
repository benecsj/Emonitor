
#ifndef __SPIFFS_MANAGER_H__
#define __SPIFFS_MANAGER_H__

/******************************************************************************
* Includes
\******************************************************************************/

#include "project_config.h"
#include "spiffs.h"

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

extern s32_t spiffs_init(void);
extern void spiffs_format(void);
extern void spiffs_status(void);
extern uint32_t spiffs_GetFileSize(char * fileName);
extern uint32_t spiffs_OpenFile(spiffs_file* filePtr, char * fileName);
extern void spiffs_CloseFile(spiffs_file fileHandler);
extern s32_t ICACHE_FLASH_ATTR spiffs_ReadFile(spiffs_file file,u8_t * buffer,s32_t length);

#endif

