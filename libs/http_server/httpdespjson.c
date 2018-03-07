/*
Connector to let httpd use the espfs filesystem to serve the files in it.
*/

/*
 * ----------------------------------------------------------------------------
 * "THE BEER-WARE LICENSE" (Revision 42):
 * Jeroen Domburg <jeroen@spritesmods.com> wrote this file. As long as you retain 
 * this notice you can do whatever you want with this stuff. If we meet some day, 
 * and you think this stuff is worth it, you can buy me a beer in return. 
 * ----------------------------------------------------------------------------
 */

#include "esp8266_platform.h"
#include "httpdespjson.h"

#include "Wifi_Manager.h"
#include "Sensor_Manager.h"
#include "Emonitor.h"
#include "esp_system.h"

int ICACHE_FLASH_ATTR cgiEspJsonTemplate(HttpdConnData *connData) {
	//DBG_HTTPS("(HS) cgiEspJsonTemplate\n");
	int length =0;
	char buffer[1025];
	int returnValue = HTTPD_CGI_DONE;

	if (connData->conn==NULL) {
		connData->file = -1;
	}
	else
	{
		//First call to this cgi.
		if (connData->file < 0) {
			connData->file = 1;
			httpdStartResponse(connData, 200);
			httpdHeader(connData, "Content-Type", httpdGetMimetype(connData->url));
			httpdHeader(connData, "Cache-Control", "max-age=3600, must-revalidate");
			httpdEndHeaders(connData);
			returnValue = HTTPD_CGI_MORE;

		}
		else
		{

			//Open Json
			length += sprintf(&buffer[length],"{");

			//EMONCMS NODE ID
			length += sprintf(&buffer[length],"\"emon_id\":\"%d\",",Emonitor_GetNodeId());
			//DEVICE UPTIME
			length += sprintf(&buffer[length],"\"st_uptime\":\"%d\",",Emonitor_GetUptime());
			//EMONCMS SENDTIMING
			length += sprintf(&buffer[length],"\"st_timing\":\"%d | %d\",",Emonitor_GetSendTiming()+1,Emonitor_GetSendPeriod());
			//EMONCMS CONNECTION COUNTER
			length += sprintf(&buffer[length],"\"st_conn\":\"%d\",",Emonitor_GetConnectionCounter());
			//EMONCMS HEAP
			uint32_t freeRAM = (Emonitor_GetFreeRam())/1024;
			uint32_t usagePercent = Emonitor_GetRAMUsage();
			length += sprintf(&buffer[length],"\"st_heap\":\"%d%% Free: %d kb\",", usagePercent,freeRAM);
			//EMONCMS BACKGROUND COUNT
			length += sprintf(&buffer[length],"\"st_bck\":\" %d%\",",Emonitor_GetCpuUsage());

			//RESET
			uint32 reset = Emonitor_GetResetReason();
			switch(reset)
			{
			case REASON_DEFAULT_RST:
				length += sprintf(&buffer[length],"\"st_rst\":\"DEFAULT_RST\",");
				break;
			case REASON_WDT_RST:
				length += sprintf(&buffer[length],"\"st_rst\":\"WDT_RST\",");
				break;
			case REASON_EXCEPTION_RST:
				length += sprintf(&buffer[length],"\"st_rst\":\"EXCEPTION_RST\",");
				break;
			case REASON_SOFT_WDT_RST:
				length += sprintf(&buffer[length],"\"st_rst\":\"SOFT_WDT_RST\",");
				break;
			case REASON_SOFT_RESTART:
				length += sprintf(&buffer[length],"\"st_rst\":\"SOFT_RESTART\",");
				break;
			case REASON_DEEP_SLEEP_AWAKE:
				length += sprintf(&buffer[length],"\"st_rst\":\"DEEP_SLEEP_AWAKE\",");
				break;
			case REASON_EXT_SYS_RST:
				length += sprintf(&buffer[length],"\"st_rst\":\"EXT_SYS_RST\",");
				break;
			}

			//SSID
			length += sprintf(&buffer[length],"\"st_wifi\":\"%s\",",Wifi_Manager_GetSTA_SSID());
			//WIFISIGNAL
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
				length += sprintf(&buffer[length],"\"st_signal\":\"%d%% (%d dBm)\",",signalPercent,signalLevel);
			}
			else
			{
				length += sprintf(&buffer[length],"\"st_signal\":\"--\",");
			}
			//IPADRESS
			if(Wifi_Manager_IsConnected() == 1)
			{
				uint8 ip[4];
				Wifi_Manager_GetIp(ip,IP);
				length += sprintf(&buffer[length],"\"st_ip\":\"%d.%d.%d.%d\",",ip[0],ip[1],ip[2],ip[3]);
			}
			else
			{
				length += sprintf(&buffer[length],"\"st_ip\":\"--\",");
			}
			//MHZ14 CO2 Sensor
			length += sprintf(&buffer[length],"\"meter_co2\":\"%d\",",Sensor_Manager_GetCO2());
			//PULSE COUNTERS
			uint32 id = 0;
			for(id=0; id < 5; id++)
			{
				uint32 count = Sensor_Manager_GetPulseCount(id);
				uint32 level = Sensor_Manager_GetPulseLevel(id);
				length += sprintf(&buffer[length],"\"pc_%02d\":\"%d|%d\",",(id+1),count,level);
			}
			//TEMP COUNT
			uint8 count = Sensor_Manager_GetTempCount();
			length += sprintf(&buffer[length],"\"temp_count\":%d,",count);
			//TEMP SENSORS
			id = 0;
			for(id=0; id < count; id++)
			{
				uint8* ids;
				uint8 count;
				sint16* temperatures;
				sint16 temperature;
				Sensor_Manager_Get_TempSensorData(&count,&ids,&temperatures);
				if(id<count)
				{
					temperature = temperatures[id];
					if(Sensor_Manager_IsTempValid(temperature)) {
						char sign = (temperature<0 ? '-':' ');
						length += sprintf(&buffer[length],"\"ds18_%02d\":\"%02X%02X%02X%02X%02X|%c%d.%d\",",id+1,ids[(id*8)+1],ids[(id*8)+2],ids[(id*8)+3],ids[(id*8)+4],ids[(id*8)+7],sign,abs(temperature/10),abs(temperature%10));
					}
				}
			}
			//TEMP HEALTH
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
			length += sprintf(&buffer[length],"\"temp_health\":\"%d%% (%d)\"",percent, health);




			//Close Json
			length += sprintf(&buffer[length],"}");



			//Send data
			httpdSend(connData, buffer, length);
			connData->file = -1;
		}
	}
	return returnValue;
}



