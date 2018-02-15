
#include <esp8266_platform.h>
#include "httpd.h"
#include "httpdespfs.h"

#include "Wifi_Manager.h"
#include "Sensor_Manager.h"
#include "Emonitor.h"

int Http_Server_TokenProcessor(HttpdConnData *connData, char *token, void **arg);


HttpdBuiltInUrl builtInUrls[]={
	{"*", cgiRedirectApClientToHostname, "esp8266.nonet"},
	{"/", cgiRedirect, "/index.html"},
	{"/index.html", cgiEspFsTemplate, Http_Server_TokenProcessor},
	{"/wait.html", cgiEspFsTemplate, Http_Server_TokenProcessor},
	{"/status.html", cgiEspFsTemplate, Http_Server_TokenProcessor},
	{"/hstatus.html", cgiEspFsTemplate, Http_Server_TokenProcessor},
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
				len = sprintf(buff, " %d sec", Emonitor_GetUptime());
			}
			//EMONCMS SENDTIMING
			else if (strcmp(token, "st_timing")==0) {
				len = sprintf(buff, " %d \\ %d", Emonitor_GetSendTiming()+1,Emonitor_GetSendPeriod());
			}
			//EMONCMS CONNECTION COUNTER
			else if (strcmp(token, "st_conn")==0) {
				len = sprintf(buff, " %d", Emonitor_GetConnectionCounter());
			}
			//EMONCMS HEAP
			else if (strcmp(token, "st_heap")==0) {
				uint32_t usedRAM = (HTTP_TOTALRAM -Emonitor_GetFreeRam())/1024;
				uint32_t freeRAM = (Emonitor_GetFreeRam())/1024;
				uint32_t usagePercent = ((double)(usedRAM)/(double)(usedRAM+freeRAM))*100;
				len = sprintf(buff, " %d%% Used: %d kb Free: %d kb ", usagePercent, usedRAM,freeRAM);
			}
			//EMONCMS BACKGROUND COUNT
			else if (strcmp(token, "st_bck")==0) {
				uint32_t countUsage = HTTP_EMPTYCOUNT - Emonitor_GetBackgroundCount();
				double usage = ((double)countUsage/(double)HTTP_EMPTYCOUNT)*100;
				countUsage = usage;
				len = sprintf(buff, " %d%%", countUsage);
			}
			//WIFI CONNECTION
			else if (strcmp(token, "st_wifi")==0) {

				if(Wifi_Manager_IsConnected() == 1)
				{
					len = sprintf(buff, " Connected to %s",Wifi_Manager_GetSTA_SSID());
				}
				else
				{
					len = sprintf(buff, " Searching... (%s)",Wifi_Manager_GetSTA_SSID());
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
					len = sprintf(buff, " %d%% (%d dBm)",signalPercent,signalLevel);
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
					Wifi_Manager_GetIp(ip);
					len = sprintf(buff, " %d.%d.%d.%d",ip[0],ip[1],ip[2],ip[3]);
				}
				else
				{
					len = sprintf(buff, "--");
				}
			}
			//PULSE COUNTERS
			else if (strcmp(token, "pulse_01")==0) {
					uint32 count = Sensor_Manager_GetPulseCount(0);
					uint8 level = Sensor_Manager_GetPulseLevel(0);
					len = sprintf(buff, " %d  (%d)",count,level);
			}
			else if (strcmp(token, "pulse_02")==0) {
					uint32 count = Sensor_Manager_GetPulseCount(1);
					uint8 level = Sensor_Manager_GetPulseLevel(1);
					len = sprintf(buff, " %d  (%d)",count,level);
			}
			else if (strcmp(token, "pulse_03")==0) {
					uint32 count = Sensor_Manager_GetPulseCount(2);
					uint8 level = Sensor_Manager_GetPulseLevel(2);
					len = sprintf(buff, " %d  (%d)",count,level);
			}
			else if (strcmp(token, "pulse_04")==0) {
					uint32 count = Sensor_Manager_GetPulseCount(3);
					uint8 level = Sensor_Manager_GetPulseLevel(3);
					len = sprintf(buff, " %d  (%d)",count,level);
			}
			else if (strcmp(token, "pulse_05")==0) {
					uint32 count = Sensor_Manager_GetPulseCount(4);
					uint32 level = Sensor_Manager_GetPulseLevel(4);
					len = sprintf(buff, " %d  (%d)",count,level);
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
			else if (strncmp(token, "ds18_",5)==0) {
					uint32 id = ((token[5]-'0')*10)+(token[6]-'0')-1;
					uint8* ids;
					uint8 count;
					sint16* temperatures;
					Sensor_Manager_Get_TempSensorData(&count,&ids,&temperatures);
					if(id<count)
					{
						char sign = (temperatures[id]<0 ? '-':' ');
						len = sprintf(buff, "<label>%02X%02X%02X%02X%02X:</label> %c%d.%dC<br>",ids[(id*8)+1],ids[(id*8)+2],ids[(id*8)+3],ids[(id*8)+4],ids[(id*8)+7],sign,abs(temperatures[id]/10),abs(temperatures[id]%10));
					}
			}
		}
		//-----------------------------------------------------------------
		//-----------------------------------------------------------------
		//HSTATUS
		else if(strcmp(connData->url,"/hstatus.html")==0)
		{
			//EMONCMS NODE ID
			if (strcmp(token, "emon_id")==0) {
				len = sprintf(buff, "%d", Emonitor_GetNodeId());
			}
			//DEVICE UPTIME
			else if (strcmp(token, "st_uptime")==0) {
				len = sprintf(buff, " %d sec", Emonitor_GetUptime());
			}
			//EMONCMS SENDTIMING
			else if (strcmp(token, "st_timing")==0) {
				len = sprintf(buff, " %d \\ %d", Emonitor_GetSendTiming()+1,Emonitor_GetSendPeriod());
			}
			//EMONCMS CONNECTION COUNTER
			else if (strcmp(token, "st_conn")==0) {
				len = sprintf(buff, " %d", Emonitor_GetConnectionCounter());
			}
			//EMONCMS HEAP
			else if (strcmp(token, "st_heap")==0) {
				uint32_t usedRAM = (HTTP_TOTALRAM -Emonitor_GetFreeRam())/1024;
				uint32_t freeRAM = (Emonitor_GetFreeRam())/1024;
				uint32_t usagePercent = ((double)(usedRAM)/(double)(usedRAM+freeRAM))*100;
				len = sprintf(buff, " %d%% Used: %d kb Free: %d kb ", usagePercent, usedRAM,freeRAM);
			}
			//EMONCMS BACKGROUND COUNT
			else if (strcmp(token, "st_bck")==0) {
				uint32_t countUsage = HTTP_EMPTYCOUNT - Emonitor_GetBackgroundCount();
				double usage = ((double)countUsage/(double)HTTP_EMPTYCOUNT)*100;
				countUsage = usage;
				len = sprintf(buff, " %d%%", countUsage);
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
					len = sprintf(buff, " %d%% (%d dBm)",signalPercent,signalLevel);
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
					Wifi_Manager_GetIp(ip);
					len = sprintf(buff, " %d.%d.%d.%d",ip[0],ip[1],ip[2],ip[3]);
				}
				else
				{
					len = sprintf(buff, "--");
				}
			}
			//PULSE COUNTERS
			else if (strncmp(token, "pulsc_",6)==0) {
				    uint32 id = (token[7]-'0')-1;
				    if(id<5)
				    {
						uint32 count = Sensor_Manager_GetPulseCount(id);
						uint32 level = Sensor_Manager_GetPulseLevel(id);
						len = sprintf(buff, "<tr><td>0%d</td><td>%d</td><td>%d</td></tr>",(id+1),count,level);
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
			else if (strncmp(token, "ds18_",5)==0) {
					uint32 id = ((token[5]-'0')*10)+(token[6]-'0')-1;
					uint8* ids;
					uint8 count;
					sint16* temperatures;
					Sensor_Manager_Get_TempSensorData(&count,&ids,&temperatures);
					if(id<count)
					{
						char sign = (temperatures[id]<0 ? '-':' ');
						len = sprintf(buff, "<tr><td>%02X%02X%02X%02X%02X</td><td>%c%d.%dC</td></tr>",ids[(id*8)+1],ids[(id*8)+2],ids[(id*8)+3],ids[(id*8)+4],ids[(id*8)+7],sign,abs(temperatures[id]/10),abs(temperatures[id]%10));
					}
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
		DBG_HTTPS("(HS) Fetch token [%s][%s]\n",token,buff);
		httpdSend(connData, buff, -1);
	}

	return HTTPD_CGI_DONE;
}
