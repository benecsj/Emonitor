
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
	{"/status.html", cgiEspFsTemplate, Http_Server_TokenProcessor},
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
	char buff[64];
	int len = 0;
	int i,j,k,temp;
	uint8 tokenizer;
	char name[32];

	//Check if has token
	if (token==NULL)
	{
		//Page finished... last callback
		DBG_HTTPS("(HS) Page finished.. Process ARGS \n");

		// WAIT PAGE
		if(strcmp(connData->url,"/wait.html")==0)
		{
			//Check if args received on wait page
			if(connData->getArgs != NULL)
			{
				DBG_HTTPS("(HS) %s \n",connData->getArgs);

				//Process all tokens
				tokenizer = 0;
				i = -1;
				j = 0;
				name[0] = 0;
				buff[0] = 0;
				do
				{
					//Jump to next char
					i++;

					switch(tokenizer)
					{
					case 0:
						//Check if end of name token received
						if(connData->getArgs[i] == '=')
						{
							//Close name string
							name[j] = 0;
							//Process value next
							tokenizer = 1;
							//Reset char pos for value
							j = 0;
						}
						else
						{
							name[j] = connData->getArgs[i];
							//Next char pos in name
							j++;
						}
						break;

					case 1:
						//Check if end of value token received
						if(connData->getArgs[i] == '&' || connData->getArgs[i] == 0)
						{
							//Close name string
							buff[j] = 0;
							//Process request
							tokenizer = 2;
							//Reset char pos for next name
							j = 0;
						}
						else
						{
							buff[j] = connData->getArgs[i];
							//Next char pos in name
							j++;
						}


						break;

					}

					//Check if has token and value to process
					if(tokenizer == 2)
					{
						tokenizer = 0;
						//Process tokens
						DBG_HTTPS("(HS) [%s][%s] \n",name,buff);


						//STA SSID
						if (strcmp(name, "sta_ssid")==0) {
							//Check length
							if(strlen(buff)<=32)
							{
								Wifi_Manager_SetSTA_SSID(buff);
							}
						}

						//STA PASSWORD
						else if (strcmp(name, "sta_pass")==0) {
							//Check length
							if(strlen(buff)<=64)
							{
								//Check if it not dumy string
								if(buff[0] != '*')
								{
									Wifi_Manager_SetSTA_PASSWORD(buff);
								}
							}
						}

						//EMONCMS KEY
						else if (strcmp(name, "emon_key")==0) {
							//Check length
							if(strlen(buff)<=32)
							{
								Emonitor_SetKey(buff);
							}
						}

						//EMONCMS URL
						else if (strcmp(name, "emon_url")==0) {
							//Check length
							len = strlen(buff);
							if(len<=99)
							{
								httpdUrlDecode(buff,len,buff,len);
								Emonitor_SetUrl(buff);
							}
						}

						//EMONCMS NODE ID
						else if (strcmp(name, "emon_id")==0) {
							temp = strtol(buff,NULL,10);
							if((temp > 0) && (temp <=999999))
							{
								Emonitor_SetNodeId(temp);
							}
						}

						//ACCESS POINT SSID
						else if (strcmp(name, "ap_ssid")==0) {
							//Check length
							if(strlen(buff)<=32)
							{
								Wifi_Manager_SetAP_SSID(buff);
							}
						}

						//ACCESS POINT PASSWORD
						else if (strcmp(name, "ap_pass")==0) {
							//Check length
							if(strlen(buff)<=64)
							{
								Wifi_Manager_SetAP_PASSWORD(buff);
							}
						}

						//ENABLE HOTSPOT
						else if (strcmp(name, "ap_en")==0) {
							if(strcmp(buff,"on")==0)
							{
								Wifi_Manager_EnableHotspot(1);
							}
							else
							{
								Wifi_Manager_EnableHotspot(0);
							}
						}
					}
				}
				while(connData->getArgs[i] != 0);

				//request save after update
				Emonitor_Request(4);

			}
		}
	}
	else
	{
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

		//Send out processed token
		buff[len] = 0;
		DBG_HTTPS("(HS) Fetch token [%s][%s]\n",token,buff);
		httpdSend(connData, buff, -1);
	}

	return HTTPD_CGI_DONE;
}
