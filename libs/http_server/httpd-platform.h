#ifndef HTTPD_PLATFORM_H
#define HTTPD_PLATFORM_H

#include "project_config.h"

int httpdPlatSendData(ConnTypePtr conn, char *buff, int len);
void httpdPlatDisconnect(ConnTypePtr conn);
void httpdPlatDisableTimeout(ConnTypePtr conn);
void httpdPlatInit(int port, int maxConnCt);
void httpdPlatLock();
void httpdPlatUnlock();

#endif
