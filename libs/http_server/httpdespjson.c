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

#include "spiffs_manager.h"

int ICACHE_FLASH_ATTR cgiEspJsonTemplate(HttpdConnData *connData) {
	DBG_HTTPS("(HS) cgiEspJsonTemplate START\n");
	spiffs* fs = spiffs_get_fs();
	int len;
	char buff[1025];
	int returnValue = HTTPD_CGI_DONE;
	//return HTTPD_CGI_NOTFOUND;
	if (connData->conn==NULL) {
		//Connection aborted. Clean up.
		SPIFFS_close(fs, connData->file);
		connData->file = -1;
	}
	else
	{
		//First call to this cgi.
		if (connData->file < 0) {
			if (connData->cgiArg != NULL) {
				//Open a different file than provided in http request.
				//Common usage: {"/", cgiEspFsHook, "/index.html"} will show content of index.html without actual redirect to that file if host root was requested
				connData->file = SPIFFS_open(fs, (char*)connData->cgiArg, SPIFFS_O_RDONLY, 0);
				DBG_HTTPS("(HS) SPIFFS_open [%d][%s]\n",connData->file,(char*)connData->cgiArg);

			} else {
				//Open the file so we can read it.
				connData->file = SPIFFS_open(fs, (char*)connData->url, SPIFFS_O_RDONLY, 0);
				DBG_HTTPS("(HS) SPIFFS_open [%d][%s]\n",connData->file,(char*)connData->url);
			}

			if (connData->file < 0) {
				returnValue = HTTPD_CGI_NOTFOUND;
			}
			else
			{
				httpdStartResponse(connData, 200);
				httpdHeader(connData, "Content-Type", httpdGetMimetype(connData->url));
				httpdHeader(connData, "Cache-Control", "max-age=3600, must-revalidate");
				httpdEndHeaders(connData);
				returnValue = HTTPD_CGI_MORE;
			}
		}
		else
		{
			len = SPIFFS_read(fs, connData->file, (u8_t *)buff, HTTPD_MAX_FILE_READ_BLOCK);
			len = sprintf(buff,"{\"st_wifi\":\"HELLO\"}");
			DBG_HTTPS("(HS) SPIFFS_read  [%d]\n",len);
			if (len>0)
			{
				httpdSend(connData, buff, len);
			}
			if (len!=HTTPD_MAX_FILE_READ_BLOCK) {
				//We're done.
				SPIFFS_close(fs, connData->file);
				connData->file = -1;
			} else {
				//Ok, till next time.
				returnValue = HTTPD_CGI_MORE;
			}
		}
	}

	DBG_HTTPS("(HS) cgiEspJsonTemplate END\n");
	return returnValue;
}



