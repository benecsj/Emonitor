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

#define SPIFFS_close STUB_SPIFFS_close
#define SPIFFS_open STUB_SPIFFS_open
#define SPIFFS_read STUB_SPIFFS_read
s32_t STUB_SPIFFS_close(spiffs *fs, spiffs_file fh)
{
	return 0;
}
spiffs_file STUB_SPIFFS_open(spiffs *fs, const char *path, spiffs_flags flags, spiffs_mode mode)
{
	return 1;
}
s32_t STUB_SPIFFS_read(spiffs *fs, spiffs_file fh, void *buf, s32_t len)
{
	char * text = "Hello Jooo"+0;
	memcpy(buf, text, 11);
	return 11;
}

//This is a catch-all cgi function. It takes the url passed to it, looks up the corresponding
//path in the filesystem and if it exists, passes the file through. This simulates what a normal
//webserver would do with static files.
int ICACHE_FLASH_ATTR cgiEspFsHook(HttpdConnData *connData) {
	DBG_HTTPS("(HS) cgiEspFsHook START\n");
	spiffs* fs = spiffs_get_fs();
	int len;
	char buff[1024];
	int returnValue = HTTPD_CGI_DONE;
	
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
				//httpdStartResponse(connData, 200);
				//httpdHeader(connData, "Content-Type", httpdGetMimetype(connData->url));
				//httpdHeader(connData, "Cache-Control", "max-age=3600, must-revalidate");
				//httpdEndHeaders(connData);
				returnValue = HTTPD_CGI_MORE;
			}
		}
		else
		{
			len = SPIFFS_read(fs, connData->file, (u8_t *)buff, 1024);
			DBG_HTTPS("(HS) SPIFFS_read  [%d]\n",len);
			if (len>0)
			{
				httpdSend(connData, buff, len);
			}
			if (len!=1024) {
				//We're done.
				SPIFFS_close(fs, connData->file);
				connData->file = -1;
			} else {
				//Ok, till next time.
				returnValue = HTTPD_CGI_MORE;
			}
		}
	}

	DBG_HTTPS("(HS) cgiEspFsHook END\n");
	return returnValue;
}


//cgiEspFsTemplate can be used as a template.

typedef struct {
	spiffs_file file;
	void *tplArg;
	char token[64];
	int tokenPos;
} TplData;

typedef void (* TplCallback)(HttpdConnData *connData, char *token, void **arg);

static TplData tplData[HTTPD_MAX_CONNECTIONS];

int ICACHE_FLASH_ATTR cgiEspFsTemplate(HttpdConnData *connData) {
	DBG_HTTPS("(HS) cgiEspFsTemplate START\n");
	TplData *tpd=connData->cgiData;
	spiffs* fs = spiffs_get_fs();
	int len;
	int returnValue = HTTPD_CGI_DONE;
	int x, sp=0;
	char *e=NULL;
	char buff[1025];

	if (connData->conn==NULL) {
		//Connection aborted. Clean up.
		((TplCallback)(connData->cgiArg))(connData, NULL, &tpd->tplArg);
		SPIFFS_close(fs, tpd->file);
		tpd->file = -1;
	}
	else
	{
		if (tpd==NULL) {
			//First call to this cgi. Open the file so we can read it.
			tpd = &tplData[connData->slot];
			if (tpd==NULL)
			{
				returnValue = HTTPD_CGI_NOTFOUND;
			}
			else
			{
				tpd->file = SPIFFS_open(fs, (char*)connData->url, SPIFFS_O_RDONLY, 0);
				DBG_HTTPS("(HS) SPIFFS_open [%d][%s]\n",tpd->file,(char*)connData->url);
				tpd->tplArg=NULL;
				tpd->tokenPos=-1;
				if (tpd->file < 0) {
					returnValue = HTTPD_CGI_NOTFOUND;
				}
				else
				{
				connData->cgiData=tpd;
				httpdStartResponse(connData, 200);
				httpdHeader(connData, "Content-Type", httpdGetMimetype(connData->url));
				httpdEndHeaders(connData);
				returnValue = HTTPD_CGI_MORE;
				}
			}
		}

		if(returnValue == HTTPD_CGI_DONE)
		{
			len = SPIFFS_read(fs, tpd->file, (u8_t *)buff, 1024);
			DBG_HTTPS("(HS) SPIFFS_read  [%d]\n",len);
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
				tpd->file = -1;
			} else {
				//Ok, till next time.
				returnValue = HTTPD_CGI_MORE;
			}
		}
	}
	DBG_HTTPS("(HS) cgiEspFsTemplate END\n");
	return returnValue;
}

