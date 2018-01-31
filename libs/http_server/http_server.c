
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
	int len = 0;
	if (token==NULL) return HTTPD_CGI_DONE;

	if (strcmp(token, "sta_ssid")==0) {
		len = sprintf(buff, "%s", "BOCI");
	}
	else if (strcmp(token, "sta_pass")==0) {
		len = sprintf(buff, "%s", "********");
	}
	else if (strcmp(token, "emon_key")==0) {
		len = sprintf(buff, "%s", "97d3e42a841ea6c219582211313d5051");
	}
	else if (strcmp(token, "emon_url")==0) {
		len = sprintf(buff, "%s", "http://v9.emonitor.hu");
	}
	else if (strcmp(token, "emon_id")==0) {
		len = sprintf(buff, "%s", "1234");
	}
	else if (strcmp(token, "ap_ssid")==0) {
		len = sprintf(buff, "%s", "Emonitor_1234");
	}
	else if (strcmp(token, "ap_auth_none")==0) {
		len = sprintf(buff, "%s", "");
	}
	else if (strcmp(token, "ap_auth_wep")==0) {
		len = sprintf(buff, "%s", "");
	}
	else if (strcmp(token, "ap_auth_wpa")==0) {
		len = sprintf(buff, "%s", "");
	}
	else if (strcmp(token, "ap_auth_wp2")==0) {
		len = sprintf(buff, "%s", "selected");
	}
	else if (strcmp(token, "ap_pass")==0) {
		len = sprintf(buff, "%s", "12345678");
	}
	else if (strcmp(token, "ap_on")==0) {
		len = sprintf(buff, "%s", "selected");
	}
	else if (strcmp(token, "ap_off")==0) {
		len = sprintf(buff, "%s", "");
	}
	buff[len] = 0;
	printf("(HS) Fetch token [%s][%s]\n",token,buff);
	httpdSend(connData, buff, -1);
	return HTTPD_CGI_DONE;
}
