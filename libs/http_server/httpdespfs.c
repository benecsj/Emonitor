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
#include "httpdespfs.h"

#include "spiffs_manager.h"

// The static files marked with FLAG_GZIP are compressed and will be served with GZIP compression.
// If the client does not advertise that he accepts GZIP send following warning message (telnet users for e.g.)
static const char *gzipNonSupportedMessage = "HTTP/1.0 501 Not implemented\r\nServer: esp8266-httpd/"HTTPDVER"\r\nConnection: close\r\nContent-Type: text/plain\r\nContent-Length: 52\r\n\r\nYour browser does not accept gzip-compressed data.\r\n";


//This is a catch-all cgi function. It takes the url passed to it, looks up the corresponding
//path in the filesystem and if it exists, passes the file through. This simulates what a normal
//webserver would do with static files.
int ICACHE_FLASH_ATTR cgiEspFsHook(HttpdConnData *connData) {
	DBG_HTTPS("(HS) cgiEspFsHook START\n");
	spiffs* fs = spiffs_get_fs();
	spiffs_file *file=connData->cgiData;
	int len;
	char buff[1024];
	char acceptEncodingBuffer[64];
	int isGzip;
	
	if (connData->conn==NULL) {
		//Connection aborted. Clean up.
		SPIFFS_close(fs, *file);
		return HTTPD_CGI_DONE;
	}

	//First call to this cgi.
	if (file==NULL) {
		if (connData->cgiArg != NULL) {
			//Open a different file than provided in http request.
			//Common usage: {"/", cgiEspFsHook, "/index.html"} will show content of index.html without actual redirect to that file if host root was requested
			spiffs_file fileTemp = SPIFFS_open(fs, (char*)connData->cgiArg, SPIFFS_O_RDONLY, 0);
			file = &fileTemp;

		} else {
			//Open the file so we can read it.
			spiffs_file fileTemp = SPIFFS_open(fs, (char*)connData->url, SPIFFS_O_RDONLY, 0);
			file = &fileTemp;
		}

		if (file==NULL) {
			return HTTPD_CGI_NOTFOUND;
		}

		connData->cgiData=file;
		httpdStartResponse(connData, 200);
		httpdHeader(connData, "Content-Type", httpdGetMimetype(connData->url));
		httpdHeader(connData, "Cache-Control", "max-age=3600, must-revalidate");
		httpdEndHeaders(connData);
		return HTTPD_CGI_MORE;
	}


	len = SPIFFS_read(fs, *file, (u8_t *)buff, 1024);
	if (len>0) httpdSend(connData, buff, len);
	if (len!=1024) {
		//We're done.
		SPIFFS_close(fs, *file);
		return HTTPD_CGI_DONE;
	} else {
		//Ok, till next time.
		return HTTPD_CGI_MORE;
	}
}


//cgiEspFsTemplate can be used as a template.

typedef struct {
	spiffs_file file;
	void *tplArg;
	char token[64];
	int tokenPos;
} TplData;

typedef void (* TplCallback)(HttpdConnData *connData, char *token, void **arg);

int ICACHE_FLASH_ATTR cgiEspFsTemplate(HttpdConnData *connData) {
	DBG_HTTPS("(HS) cgiEspFsTemplate START\n");
	TplData *tpd=connData->cgiData;
	spiffs* fs = spiffs_get_fs();
	int len;
	int x, sp=0;
	char *e=NULL;
	char buff[1025];

	if (connData->conn==NULL) {
		//Connection aborted. Clean up.
		((TplCallback)(connData->cgiArg))(connData, NULL, &tpd->tplArg);
		SPIFFS_close(fs, tpd->file);
		free(tpd);
		return HTTPD_CGI_DONE;
	}

	if (tpd==NULL) {
		//First call to this cgi. Open the file so we can read it.
		tpd=(TplData *)malloc(sizeof(TplData));
		if (tpd==NULL) return HTTPD_CGI_NOTFOUND;
		DBG_HTTPS("(HS) SPIFFS_open 114\n");
		tpd->file = SPIFFS_open(fs, (char*)connData->url, SPIFFS_O_RDONLY, 0);
		DBG_HTTPS("(HS) SPIFFS_open 116  [%d]\n",tpd->file);
		tpd->tplArg=NULL;
		tpd->tokenPos=-1;
		if (tpd->file<0) {
			free(tpd);
			return HTTPD_CGI_NOTFOUND;
		}

		connData->cgiData=tpd;
		DBG_HTTPS("(HS) httpdStartResponse\n");
		httpdStartResponse(connData, 200);
		DBG_HTTPS("(HS) httpdHeader\n");
		httpdHeader(connData, "Content-Type", httpdGetMimetype(connData->url));
		DBG_HTTPS("(HS) httpdEndHeaders\n");
		httpdEndHeaders(connData);
		return HTTPD_CGI_MORE;
	}
	DBG_HTTPS("(HS) SPIFFS_read 135\n");
	len = SPIFFS_read(fs, tpd->file, (u8_t *)buff, 1024);
	DBG_HTTPS("(HS) SPIFFS_read 137  [%d]\n",len);
	if (len>0) {
		sp=0;
		e=buff;
		for (x=0; x<len; x++) {
			if (tpd->tokenPos==-1) {
				//Inside ordinary text.
				if (buff[x]=='%') {
					//Send raw data up to now
					if (sp!=0) httpdSend(connData, e, sp);
					sp=0;
					//Go collect token chars.
					tpd->tokenPos=0;
				} else {
					sp++;
				}
			} else {
				if (buff[x]=='%') {
					if (tpd->tokenPos==0) {
						//This is the second % of a %% escape string.
						//Send a single % and resume with the normal program flow.
						httpdSend(connData, "%", 1);
					} else {
						//This is an actual token.
						tpd->token[tpd->tokenPos++]=0; //zero-terminate token
						((TplCallback)(connData->cgiArg))(connData, tpd->token, &tpd->tplArg);
					}
					//Go collect normal chars again.
					e=&buff[x+1];
					tpd->tokenPos=-1;
				} else {
					if (tpd->tokenPos<(sizeof(tpd->token)-1)) tpd->token[tpd->tokenPos++]=buff[x];
				}
			}
		}
	}
	//Send remaining bit.
	if (sp!=0) httpdSend(connData, e, sp);
	if (len!=1024) {
		//We're done.
		((TplCallback)(connData->cgiArg))(connData, NULL, &tpd->tplArg);
		SPIFFS_close(fs, tpd->file);
		free(tpd);
		return HTTPD_CGI_DONE;
	} else {
		//Ok, till next time.
		return HTTPD_CGI_MORE;
	}
}

