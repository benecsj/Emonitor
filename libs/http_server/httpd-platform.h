#ifndef HTTPD_PLATFORM_H
#define HTTPD_PLATFORM_H

/******************************************************************************
* Includes
\******************************************************************************/
#include "project_config.h"

/******************************************************************************
* Defines
\******************************************************************************/
#define httpdPlatLock()
#define httpdPlatUnlock()

/******************************************************************************
* Primitives
\******************************************************************************/
int httpdPlatSendData(ConnTypePtr conn, char *buff, int len);
void httpdPlatDisconnect(ConnTypePtr conn);
void httpdPlatDisableTimeout(ConnTypePtr conn);
void httpdPlatInit(int port, int maxConnCt);


#endif
