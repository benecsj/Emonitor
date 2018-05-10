/*
 * ----------------------------------------------------------------------------
 * "THE BEER-WARE LICENSE" (Revision 42):
 * Martin d'Allens <martin.dallens@gmail.com> wrote this file. As long as you retain
 * this notice you can do whatever you want with this stuff. If we meet some day,
 * and you think this stuff is worth it, you can buy me a beer in return.
 * ----------------------------------------------------------------------------
 */

#ifndef HTTP_SERVER_H
#define HTTP_SERVER_H

extern void Http_Server_Init(void);
extern void ICACHE_FLASH_ATTR Http_Server_Main(void);

extern void Http_Server_Language_IdToText(int id, char* langText);
extern int  Http_Server_Language_TextToId(char* langText);

extern uint8 Http_Language;

#define Http_Server_GetLanguage()	(Http_Language)
#define Http_Server_SetLanguage(a)	Http_Language=a

#endif
