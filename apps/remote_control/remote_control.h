#ifndef __REMOTE_CONTROL_H__
#define __REMOTE_CONTROL_H__

#include "project_config.h"

/******************************************************************************
* Prototypes
\******************************************************************************/
extern void Remote_Control_Init(void);
extern void Remote_Control_Main(void);


#if DEBUG_REMOTE_CONTROL == ON
#define REMOTE_DBG(...) prj_printf(__VA_ARGS__)
#else
#define REMOTE_DBG(...)
#endif

#endif

