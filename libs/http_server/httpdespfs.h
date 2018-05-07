#ifndef HTTPDESPFS_H
#define HTTPDESPFS_H

#include "httpd.h"

int cgiEspFsHook(HttpdConnData *connData);
int ICACHE_FLASH_ATTR cgiEspFsTemplate(HttpdConnData *connData);

extern uint8 Http_Language;

#define SERVER_LANG_HU	1
#define SERVER_LANG_EN  0
#define SERVER_LANG_UNDEFINED 255

#endif
