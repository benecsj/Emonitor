
#include "user_config.h"
#include <esp8266_platform.h>
#include "httpd.h"
#include "httpdespfs.h"

int Http_Server_TokenProcessor(HttpdConnData *connData, char *token, void **arg);


HttpdBuiltInUrl builtInUrls[]={
	{"*", cgiRedirectApClientToHostname, "esp8266.nonet"},
	{"/", cgiRedirect, "/index.html"},
	{"/index.html", cgiEspFsTemplate, Http_Server_TokenProcessor},

	{"/wifi", cgiRedirect, "/wifi/wifi.tpl"},
	{"/wifi/", cgiRedirect, "/wifi/wifi.tpl"},
	{"/test", cgiRedirect, "/test/index.html"},
	{"/test/", cgiRedirect, "/test/index.html"},

	{"*", cgiEspFsHook, NULL}, //Catch-all cgi function for the filesystem
	{NULL, NULL, NULL}
};

void Http_Server_Init(void)
{
	DBG_HTTPS("(HS) Http_Server_Init START\n");
	httpdInit(builtInUrls, 80);
	DBG_HTTPS("(HS) Http_Server_Init FINISH\n");
}

//Template code for the counter on the index page.
int ICACHE_FLASH_ATTR Http_Server_TokenProcessor(HttpdConnData *connData, char *token, void **arg) {
	char buff[128];
	int len;
	if (token==NULL) return HTTPD_CGI_DONE;

	if (strcmp(token, "test")==0) {
		len = sprintf(buff, "%s", "HELLO");
	}
	httpdSend(connData, buff, len);
	return HTTPD_CGI_DONE;
}
