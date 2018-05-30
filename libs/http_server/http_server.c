
#include <esp8266_platform.h>
#include "httpd.h"
#include "httpdespfs.h"
#include "httpdespjson.h"
#include "esp_system.h"

#include "Wifi_Manager.h"
#include "Sensor_Manager.h"
#include "Emonitor.h"
#include "http_server.h"

int Http_Server_TokenProcessor(HttpdConnData *connData, char *token, void **arg);
uint8 Http_Server_GetDefaultLanguage();
uint16 Http_Server_FormId = 0;
uint8 Http_Language = SERVER_LANG_UNDEFINED;

HttpdBuiltInUrl builtInUrls[]={
	{"*", cgiRedirectApClientToHostname, "esp8266.nonet"},
	{"/", cgiRedirect, "/index.html"},
	{"/index.html", cgiEspFsTemplate, Http_Server_TokenProcessor},
	{"/wait.html", cgiEspFsTemplate, Http_Server_TokenProcessor},
	{"/status.html", cgiEspFsTemplate, Http_Server_TokenProcessor},
	{"/status.json", cgiEspJsonTemplate, NULL},
	{"/wifi", cgiRedirect, "/wifi/wifi.tpl"},
	{"/wifi/", cgiRedirect, "/wifi/wifi.tpl"},
	{"/test", cgiRedirect, "/test/index.html"},
	{"/test/", cgiRedirect, "/test/index.html"},

	{"*", cgiEspFsHook, NULL}, //Catch-all cgi function for the filesystem
	{NULL, NULL, NULL}
};

void ICACHE_FLASH_ATTR Http_Server_Init(void)
{
	DBG_HTTPS("(HS) Http_Server_Init START\n");
	httpdInit(builtInUrls, 80);
	Http_Server_FormId = os_random();
	//Load default language if not config found
	if(Http_Language == SERVER_LANG_UNDEFINED)
	{
		Http_Language = Http_Server_GetDefaultLanguage();
	}
	DBG_HTTPS("(HS) Http_Server_Init FINISH (Lang:%d)\n",Http_Language);
}

void ICACHE_FLASH_ATTR Http_Server_Main(void)
{
	//Monitor connection status
	httpdMonitorConnections();
}

uint8 ICACHE_FLASH_ATTR Http_Server_GetDefaultLanguage()
{
	esp_fs_file file;
	uint8 langText[3];
	uint8 returnValue = SERVER_LANG_EN;

	//Get language from filestorage
	esp_fs_OpenFile(&file,"/lang");

	esp_fs_ReadFile(&file, (u8_t *)&langText[0], 2);
	esp_fs_CloseFile(&file);
	//Close string
	langText[2] = 0;
	//Convert language string to id
	return Http_Server_Language_TextToId(langText);
}


void ICACHE_FLASH_ATTR Http_Server_Language_IdToText(int id, char* langText)
{
	//Based on id output proper text
	switch(id)
	{
	case SERVER_LANG_HU:
		langText[0]='h';langText[1]='u';
		break;
	case SERVER_LANG_EN:
		langText[0]='e';langText[1]='n';
		break;
	default: langText[0]=0;
	}
	langText[2]=0;
}

int ICACHE_FLASH_ATTR Http_Server_Language_TextToId(char* langText)
{
	//Init with default language
	int returnValue = SERVER_LANG_EN;
	//Process text
	if(strcmp((char *)&langText[0],"hu")==0)
	{
		returnValue = SERVER_LANG_HU;
	}
	else if(strcmp((char *)&langText[0],"en")==0)
	{
		returnValue = SERVER_LANG_EN;
	}
	//Return id
	return returnValue;
}

//Template code for the counter on the index page.
int ICACHE_FLASH_ATTR Http_Server_TokenProcessor(HttpdConnData *connData, char *token, void **arg) {
	char buff[64];
	int len = 0;
	int i,j,k,temp;
	uint8 tokenizer;
	char name[32];
	bool saveNeeded = FALSE;

	//Check if has token
	if (token==NULL)
	{
		//Page finished... last callback
		//DBG_HTTPS("(HS) Page finished.. Process ARGS \n");

		// WAIT PAGE
		if(strcmp(connData->url,"/wait.html")==0)
		{
			//Check if args received on wait page
			if(connData->getArgs != NULL)
			{
				//DBG_HTTPS("(HS) %s \n",connData->getArgs);

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
						//DBG_HTTPS("(HS) [%s][%s] \n",name,buff);


						//STA SSID
						if (strcmp(name, "sta_ssid")==0) {
							//Check length
							if(strlen(buff)<=32)
							{
								Wifi_Manager_SetSTA_SSID(buff);
								saveNeeded = TRUE;
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

						//EMONCMS SEND PEROID
						else if (strcmp(name, "emon_send")==0) {
							temp = strtol(buff,NULL,10);
							if((temp > 0) && (temp <=3600))
							{
								Emonitor_SetSendPeriod(temp);
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
						//CHECK IF FORM ID IS VALID
						else if (strcmp(name, "form_id")==0) {
							temp = strtol(buff,NULL,10);
							if(temp != Http_Server_FormId)
							{
								saveNeeded = FALSE;
								break;
							}
						}
						//RESET REQUEST
						else if (strcmp(name, "restart")==0) {
							Emonitor_Request(EMONITOR_REQ_RESTART);
						}
						//RESET REQUEST
						else if (strcmp(name, "reset")==0) {
							Emonitor_Request(EMONITOR_REQ_CLEAR);
						}
						//LANGUAGE CHANGE REQUEST
						else if (strcmp(name, "lang")==0) {
							//Toggle language
							if(Http_Server_GetLanguage() == SERVER_LANG_HU)
							{
								Http_Server_SetLanguage(SERVER_LANG_EN);
							}
							else
							{
								Http_Server_SetLanguage(SERVER_LANG_HU);
							}
							//Save config
							saveNeeded = TRUE;
						}
					}
				}
				while(connData->getArgs[i] != 0);

				//Only save when needed
				if(saveNeeded == TRUE)
				{
					//request save after update
					Emonitor_Request(EMONITOR_REQ_SAVE);
				}
			}
		}
	}
	else
	{
		//-----------------------------------------------------------------
		//-----------------------------------------------------------------
		// INDEX PAGE
		if(strcmp(connData->url,"/index.html")==0)
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

			//EMONCMS SEND PEROID
			else if (strcmp(token, "emon_send")==0) {
				len = sprintf(buff, "%d", Emonitor_GetSendPeriod());
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
			//FORM ID
			else if (strcmp(token, "form_id")==0) {
				len = sprintf(buff, "%d",Http_Server_FormId);
			}

		}
		//-----------------------------------------------------------------
		//-----------------------------------------------------------------
		// STATUS
		else if(strcmp(connData->url,"/status.html")==0)
		{
			//EMONCMS NODE ID
			if (strcmp(token, "emon_id")==0) {
				len = sprintf(buff, "%d", Emonitor_GetNodeId());
			}
			//DEVICE UPTIME
			else if (strcmp(token, "st_uptime")==0) {
				len = sprintf(buff, "%d", Emonitor_GetUptime());
			}
			//EMONCMS SENDTIMING
			else if (strcmp(token, "st_timing")==0) {
				len = sprintf(buff, "%d | %d", Emonitor_GetSendTiming()+1,Emonitor_GetSendPeriod());
			}
			//EMONCMS CONNECTION COUNTER
			else if (strcmp(token, "st_conn")==0) {
				len = sprintf(buff, "%d", Emonitor_GetConnectionCounter());
			}
			//EMONCMS HEAP
			else if (strcmp(token, "st_heap")==0) {
				uint32_t freeRAM = (Emonitor_GetFreeRam())/1024;
				uint32_t usagePercent = Emonitor_GetRAMUsage();
				len = sprintf(buff, "%d%% Free: %d kb", usagePercent,freeRAM);
			}
			//EMONCMS BACKGROUND COUNT
			else if (strcmp(token, "st_bck")==0) {
				len = sprintf(buff, "%d%", Emonitor_GetCpuUsage());
			}
			//SW version
			else if (strcmp(token, "st_sw")==0) {
				len = sprintf(buff, "%s", Emonitor_GetSWVersion());
			}
			//WIFI CONNECTION
			else if (strcmp(token, "st_wifi")==0) {

				if(Wifi_Manager_IsConnected() == 1)
				{
					len = sprintf(buff, "%s",Wifi_Manager_GetSTA_SSID());
				}
				else
				{
					len = sprintf(buff, "--");
				}
			}
			//WIFI POWER LEVEL
			else if (strcmp(token, "st_signal")==0) {

				if(Wifi_Manager_IsConnected() == 1)
				{
					sint8 signalLevel = Wifi_Manager_GetSignalLevel();
					uint32_t signalPercent = 0;
					if(signalLevel > -50)
					{
						signalPercent = 100;
					}
					else if(signalLevel < -90)
					{
						signalPercent = 0;
					}
					else
					{
						signalPercent = 100+(uint32_t)(((double)signalLevel+50)*(double)2.5);
					}
					len = sprintf(buff, "%d%% (%d dBm)",signalPercent,signalLevel);
				}
				else
				{
					len = sprintf(buff, "--");
				}
			}
			//WIFI IP
			else if (strcmp(token, "st_ip")==0) {

				if(Wifi_Manager_IsConnected() == 1)
				{
					uint8 ip[4];
					Wifi_Manager_GetIp(ip,IP);
					len = sprintf(buff, " %d.%d.%d.%d",ip[0],ip[1],ip[2],ip[3]);
				}
				else
				{
					len = sprintf(buff, "--");
				}
			}
			//TEMP SENSORS
			else if (strcmp(token, "temp_health")==0) {
					uint32 health = Sensor_Manager_GetTempHealth();
					uint32 percent = 100;
					if(health > 10)
					{
						percent = 0;
					}
					else if(health > 1)
					{
						percent = percent - ((health-1) * 10) ;
					}
					len = sprintf(buff, " %d%% (%d)",percent, health);
			}
			else if (strcmp(token, "temp_count")==0) {
					uint8 count = Sensor_Manager_GetTempCount();
					len = sprintf(buff, " %d",count);
			}
			//RESET SOURCE
			else if (strcmp(token, "st_rst")==0) {
				uint32 reset = Emonitor_GetResetReason();
				switch(reset)
				{
				case REASON_DEFAULT_RST:
					len = sprintf(buff, "DEFAULT_RST");
					break;
				case REASON_WDT_RST:
					len = sprintf(buff, "WDT_RST");
					break;
				case REASON_EXCEPTION_RST:
					len = sprintf(buff, "EXCEPTION_RST");
					break;
				case REASON_SOFT_WDT_RST:
					len = sprintf(buff, "SOFT_WDT_RST");
					break;
				case REASON_SOFT_RESTART:
					len = sprintf(buff, "SOFT_RESTART");
					break;
				case REASON_DEEP_SLEEP_AWAKE:
					len = sprintf(buff, "DEEP_SLEEP_AWAKE");
					break;
				case REASON_EXT_SYS_RST:
					len = sprintf(buff, "EXT_SYS_RST");
					break;
				}
			}
			//FORM ID
			else if (strcmp(token, "form_id")==0) {
				len = sprintf(buff, "%d",Http_Server_FormId);
			}
		}

		//-----------------------------------------------------------------
		//-----------------------------------------------------------------
		// WAIT
		else if(strcmp(connData->url,"/wait.html")==0)
		{
			//EMONCMS NODE ID
			if (strcmp(token, "emon_id")==0) {
				len = sprintf(buff, "%d", Emonitor_GetNodeId());
			}

		}
		//Send out processed token
		buff[len] = 0;
		//DBG_HTTPS("(HS) Fetch token [%s][%s]\n",token,buff);
		httpdSend(connData, buff, -1);
	}

	return HTTPD_CGI_DONE;
}
