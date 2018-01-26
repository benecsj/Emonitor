/*
ESP8266 web server - platform-dependent routines, nonos version
*/

#include "esp8266_platform.h"
#include "httpd.h"
#include "httpd-platform.h"

#ifndef FREERTOS

//Listening connection data
static struct espconn httpdConn;
static esp_tcp httpdTcp;

//Set/clear global httpd lock.
//Not needed on nonoos.
void ICACHE_FLASH_ATTR httpdPlatLock() {
}
void ICACHE_FLASH_ATTR httpdPlatUnlock() {
}


static void ICACHE_FLASH_ATTR platReconCb(void *arg, sint8 err) {
	//From ESP8266 SDK
	//If still no response, considers it as TCP connection broke, goes into espconn_reconnect_callback.

	ConnTypePtr conn=arg;
	//Just call disconnect to clean up pool and close connection.
	httpdDisconCb(conn, (char*)conn->proto.tcp->remote_ip, conn->proto.tcp->remote_port);
}

static void ICACHE_FLASH_ATTR platDisconCb(void *arg) {
	ConnTypePtr conn=arg;
	httpdDisconCb(conn, (char*)conn->proto.tcp->remote_ip, conn->proto.tcp->remote_port);
}

static void ICACHE_FLASH_ATTR platRecvCb(void *arg, char *data, unsigned short len) {
	ConnTypePtr conn=arg;
	httpdRecvCb(conn, (char*)conn->proto.tcp->remote_ip, conn->proto.tcp->remote_port, data, len);
}

static void ICACHE_FLASH_ATTR platSentCb(void *arg) {
	ConnTypePtr conn=arg;
	httpdSentCb(conn, (char*)conn->proto.tcp->remote_ip, conn->proto.tcp->remote_port);
}

static void ICACHE_FLASH_ATTR platConnCb(void *arg) {
	ConnTypePtr conn=arg;
	if (httpdConnectCb(conn, (char*)conn->proto.tcp->remote_ip, conn->proto.tcp->remote_port)) {
		espconn_regist_recvcb(conn, platRecvCb);
		espconn_regist_reconcb(conn, platReconCb);
		espconn_regist_disconcb(conn, platDisconCb);
		espconn_regist_sentcb(conn, platSentCb);
	} else {
		httpdPlatDisconnect(conn);
	}
}


int ICACHE_FLASH_ATTR httpdPlatSendData(ConnTypePtr conn, char *buff, int len) {
	DBG_HTTPS("(HS) <httpdPlatSendData [%d]>\n",len);
	int r=0;
	r=espconn_send(conn, (uint8_t*)buff, len);
	return (r>=0);
}

void ICACHE_FLASH_ATTR httpdPlatDisconnect(ConnTypePtr conn) {
	espconn_disconnect(conn);
}

void ICACHE_FLASH_ATTR httpdPlatDisableTimeout(ConnTypePtr conn) {
	//Can't disable timeout; set to 2 hours instead.
	espconn_regist_time(conn, 7199, 1);
}

//Initialize listening socket, do general initialization
void ICACHE_FLASH_ATTR httpdPlatInit(int port, int maxConnCt) {
	sint8 ret;
	httpdConn.type=ESPCONN_TCP;
	httpdConn.state=ESPCONN_NONE;
	httpdTcp.local_port=port;
	httpdConn.proto.tcp=&httpdTcp;
	espconn_regist_connectcb(&httpdConn, platConnCb);
	ret = espconn_accept(&httpdConn);
	DBG_HTTPS("(HS)< espconn_accept [%d][%d] >\n", ret,espconn_tcp_get_max_con_allow(&httpdConn));
}


void* aligned_malloc(size_t required_bytes)
{
	DBG_HTTPS("(HS)< malloc [%d] >\n",required_bytes);
    void* p1; // original block
    void** p2; // aligned block
    int offset = HTTPD_ALIGNMENT - 1 + sizeof(void*);
    if ((p1 = (void*)malloc(required_bytes + offset)) == NULL)
    {
       return NULL;
    }
    p2 = (void**)(((size_t)(p1) + offset) & ~(HTTPD_ALIGNMENT - 1));
    p2[-1] = p1;
    return p2;
}

void aligned_free(void *p)
{
	DBG_HTTPS("(HS)< free >\n");
    free(((void**)p)[-1]);
}


#endif
