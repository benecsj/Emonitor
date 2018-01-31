
#include "user_config.h"
#include <esp8266_platform.h>
#include "httpd.h"
#include "httpdespfs.h"

#include "Wifi_Manager.h"
#include "Emonitor.h"

int Http_Server_TokenProcessor(HttpdConnData *connData, char *token, void **arg);


HttpdBuiltInUrl builtInUrls[]={
	{"*", cgiRedirectApClientToHostname, "esp8266.nonet"},
	{"/", cgiRedirect, "/index.html"},
	{"/index.html", cgiEspFsTemplate, Http_Server_TokenProcessor},
	{"/wait.html", cgiEspFsTemplate, Http_Server_TokenProcessor},

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
	int i,temp;


	if (token==NULL) return HTTPD_CGI_DONE;

	//STA SSID
	if (strcmp(token, "sta_ssid")==0) {
		len = sprintf(buff, "%s", Wifi_Manager_GetSTA_SSID());
	}

	//STA PASSWORD
	else if (strcmp(token, "sta_pass")==0) {
		temp = strlen(Wifi_Manager_GetSTA_PASSWORD());
		for(i = 0 ; i < temp ; i++)
		{
			buff[i] = '*';
		}
		buff[i] = 0;
		len = i;
	}

	//EMONCMS KEY
	else if (strcmp(token, "emon_key")==0) {
		len = sprintf(buff, "%s", Emonitor_GetKey());
	}

	//EMONCMS URL
	else if (strcmp(token, "emon_url")==0) {
		len = sprintf(buff, "%s", Emonitor_GetUrl());
	}

	//EMONCMS NODE ID
	else if (strcmp(token, "emon_id")==0) {
		len = sprintf(buff, "%d", Emonitor_GetNodeId());
	}

	//ACCESS POINT SSID
	else if (strcmp(token, "ap_ssid")==0) {
		len = sprintf(buff, "%s", Wifi_Manager_GetAP_SSID());
	}

	//ACCESS POINT PASSWORD
	else if (strcmp(token, "ap_pass")==0) {
		len = sprintf(buff, "%s", Wifi_Manager_GetAP_PASSWORD());
	}

	//ENABLE HOTSPOT
	else if (strcmp(token, "ap_on")==0) {
		if(Wifi_Manager_GetEnableHotspot() == 1)
		{
		len = sprintf(buff, "%s", "selected");
		}
	}
	else if (strcmp(token, "ap_off")==0) {
		if(Wifi_Manager_GetEnableHotspot() == 0)
		{
		len = sprintf(buff, "%s", "selected");
		}
	}


	buff[len] = 0;
	printf("(HS) Fetch token [%s][%s]\n",token,buff);
	httpdSend(connData, buff, -1);
	return HTTPD_CGI_DONE;
}
