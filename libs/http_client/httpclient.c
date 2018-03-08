/*
 * ----------------------------------------------------------------------------
 * "THE BEER-WARE LICENSE" (Revision 42):
 * Martin d'Allens <martin.dallens@gmail.com> wrote this file. As long as you retain
 * this notice you can do whatever you want with this stuff. If we meet some day,
 * and you think this stuff is worth it, you can buy me a beer in return.
 * ----------------------------------------------------------------------------
 */

// FIXME: sHC_DEBUG->snHC_DEBUG everywhere.


#include "c_types.h"
//#include "osapi.h"
#include "limits.h"
#include "httpclient.h"
#include "lwip/tcpip.h"
#include "lwip/ip.h"
#include "lwip/ip_addr.h"
#include "espconn.h"
#include "esp_misc.h"
#include "project_config.h"

// Debug output.
#if DEBUG_HTTP_CLIENT
#define HC_DEBUG(...) HC_DEBUG(__VA_ARGS__)
#else
#define HC_DEBUG(...)
#endif

// Internal state.
typedef struct {
	char * path;
	int port;
	char * post_data;
	char * headers;
	char * hostname;
	char * buffer;
	int buffer_size;
	http_callback user_callback;
} request_args;

//Single client single connection
struct espconn client_connection;
esp_tcp client_tcp;
request_args client_request;

static char * ICACHE_FLASH_ATTR esp_strdup(const char * str)
{
	//New string pointer
	char * new_str = NULL;
	//Check if input parameter is valid
	if (str != NULL) {
		//Create a new buffer for the string
		new_str = (char *)prj_malloc(strlen(str) + 1); // 1 for null character
		//Check if malloc was successful
		if (new_str != NULL) {
			//copy string to new buffer
			strcpy(new_str, str);
		}
	}
	//Return pointer of new string
	return new_str;
}

static int ICACHE_FLASH_ATTR esp_isupper(char c)
{
    return (c >= 'A' && c <= 'Z');
}

static int ICACHE_FLASH_ATTR esp_isalpha(char c)
{
    return ((c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z'));
}


static int ICACHE_FLASH_ATTR esp_isspace(char c)
{
    return (c == ' ' || c == '\t' || c == '\n' || c == '\12');
}

static int ICACHE_FLASH_ATTR esp_isdigit(char c)
{
    return (c >= '0' && c <= '9');
}

/*
 * Convert a string to a long integer.
 *
 * Ignores `locale' stuff.  Assumes that the upper and lower case
 * alphabets and digits are each contiguous.
 */
static long ICACHE_FLASH_ATTR
esp_strtol(const char *nptr, char **endptr, int base)
{
	const char *s = nptr;
	unsigned long acc;
	int c;
	unsigned long cutoff;
	int neg = 0, any, cutlim;

	/*
	 * Skip white space and pick up leading +/- sign if any.
	 * If base is 0, allow 0x for hex and 0 for octal, else
	 * assume decimal; if base is already 16, allow 0x.
	 */
	do {
		c = *s++;
	} while (esp_isspace(c));
	if (c == '-') {
		neg = 1;
		c = *s++;
	} else if (c == '+')
		c = *s++;
	if ((base == 0 || base == 16) &&
	    c == '0' && (*s == 'x' || *s == 'X')) {
		c = s[1];
		s += 2;
		base = 16;
	} else if ((base == 0 || base == 2) &&
	    c == '0' && (*s == 'b' || *s == 'B')) {
		c = s[1];
		s += 2;
		base = 2;
	}
	if (base == 0)
		base = c == '0' ? 8 : 10;

	/*
	 * Compute the cutoff value between legal numbers and illegal
	 * numbers.  That is the largest legal value, divided by the
	 * base.  An input number that is greater than this value, if
	 * followed by a legal input character, is too big.  One that
	 * is equal to this value may be valid or not; the limit
	 * between valid and invalid numbers is then based on the last
	 * digit.  For instance, if the range for longs is
	 * [-2147483648..2147483647] and the input base is 10,
	 * cutoff will be set to 214748364 and cutlim to either
	 * 7 (neg==0) or 8 (neg==1), meaning that if we have accumulated
	 * a value > 214748364, or equal but the next digit is > 7 (or 8),
	 * the number is too big, and we will return a range error.
	 *
	 * Set any if any `digits' consumed; make it negative to indicate
	 * overflow.
	 */
	cutoff = neg ? -(unsigned long)LONG_MIN : LONG_MAX;
	cutlim = cutoff % (unsigned long)base;
	cutoff /= (unsigned long)base;
	for (acc = 0, any = 0;; c = *s++) {
		if (esp_isdigit(c))
			c -= '0';
		else if (esp_isalpha(c))
			c -= esp_isupper(c) ? 'A' - 10 : 'a' - 10;
		else
			break;
		if (c >= base)
			break;
		if (any < 0 || acc > cutoff || (acc == cutoff && c > cutlim))
			any = -1;
		else {
			any = 1;
			acc *= base;
			acc += c;
		}
	}
	if (any < 0) {
		acc = neg ? LONG_MIN : LONG_MAX;
//		errno = ERANGE;
	} else if (neg)
		acc = -acc;
	if (endptr != 0)
		*endptr = (char *)(any ? s - 1 : nptr);
	return (acc);
}

static int ICACHE_FLASH_ATTR chunked_decode(char * chunked, int size)
{
	char *src = chunked;
	char *end = chunked + size;
	int i, dst = 0;

	do
	{
		//[chunk-size]
		i = esp_strtol(src, (char **) NULL, 16);
		HC_DEBUG("(HC) Chunk Size:%d\r\n", i);
		if (i <= 0) 
			break;
		//[chunk-size-end-ptr]
		src = (char *)strstr(src, "\r\n") + 2;
		//[chunk-data]
		memmove(&chunked[dst], src, i);
		src += i + 2; /* CRLF */
		dst += i;
	} while (src < end);

	// return decoded size
	return dst;
}

static void ICACHE_FLASH_ATTR receive_callback(void * arg, char * buf, unsigned short len)
{
	struct espconn * conn = (struct espconn *)arg;
	request_args * req = (request_args *)conn->reserve;

	//Check if request buffer is valid
	if (req->buffer != NULL) {
		// Let's do the equivalent of a realloc().
		const int new_size = req->buffer_size + len;
		char * new_buffer;
		//Try to malloc a new buffer
		if (new_size > BUFFER_SIZE_MAX || NULL == (new_buffer = (char *)prj_malloc(new_size))) {
			HC_DEBUG("(HC) HTTP Response too long (%d)\n", new_size);
			req->buffer[0] = '\0'; // Discard the buffer to avoid using an incomplete response.
			espconn_disconnect(conn);
		}
		else
		{
			HC_DEBUG("(HC) HTTP Response received (%d)\n", new_size);
			memcpy(new_buffer, req->buffer, req->buffer_size);
			memcpy(new_buffer + req->buffer_size - 1 /*overwrite the null character*/, buf, len); // Append new data.
			new_buffer[new_size - 1] = '\0'; // Make sure there is an end of string.
			prj_free(req->buffer);
			req->buffer = new_buffer;
			req->buffer_size = new_size;
		}
	}
}

static void ICACHE_FLASH_ATTR sent_callback(void * arg)
{
	struct espconn * conn = (struct espconn *)arg;
	request_args * req = (request_args *)conn->reserve;

	if (req->post_data == NULL) {
		HC_DEBUG("(HC) HTTP All sent\n");
	}
	else {
		// The headers were sent, now send the contents.
		HC_DEBUG("(HC) HTTP Sending request body\n");
		espconn_send(conn, (uint8 *)req->post_data, strlen(req->post_data));
		prj_free(req->post_data);
		req->post_data = NULL;
	}
}

static void ICACHE_FLASH_ATTR connect_callback(void * arg)
{
	HC_DEBUG("(HC) TCP Connected\n");
	struct espconn * conn = (struct espconn *)arg;
	request_args * req = (request_args *)conn->reserve;

	espconn_regist_recvcb(conn, receive_callback);
	espconn_regist_sentcb(conn, sent_callback);

	const char * method = "GET";
	char post_headers[32] = "";

	if (req->post_data != NULL) { // If there is data this is a POST request.
		method = "POST";
		prj_sprintf(post_headers, "Content-Length: %d\r\n", strlen(req->post_data));
	}

	char buf[69 + strlen(method) + strlen(req->path) + strlen(req->hostname) +
			 strlen(req->headers) + strlen(post_headers)];
	int len = prj_sprintf(buf,
						 "%s %s HTTP/1.1\r\n"
						 "Host: %s:%d\r\n"
						 "Connection: close\r\n"
						 "User-Agent: ESP8266\r\n"
						 "%s"
						 "%s"
						 "\r\n",
						 method, req->path, req->hostname, req->port, req->headers, post_headers);

	espconn_send(conn, (uint8 *)buf, len);
	prj_free(req->headers);
	req->headers = NULL;
}

static void ICACHE_FLASH_ATTR disconnect_callback(void * arg)
{
	HC_DEBUG("(HC) TCP Disconnected\n");
	struct espconn *conn = (struct espconn *)arg;
	request_args * req;

	//Check connection parameter
	if(conn != NULL) {
		//Check if connection got user data
		if(conn->reserve != NULL) {
			req = (request_args *)conn->reserve;
			int http_status = -1;
			int body_size = 0;
			char * body = "";
			//Check if buffer is valid
			if (req->buffer == NULL) {
				HC_DEBUG("(HC) Buffer shouldn't be NULL\n");
			}
			//Check if buffer is not empty
			else if (req->buffer[0] != '\0') {
				//Check http version
				const char * version10 = "HTTP/1.0 ";
				const char * version11 = "HTTP/1.1 ";
				if (strncmp(req->buffer, version10, strlen(version10)) != 0
				 && strncmp(req->buffer, version11, strlen(version11)) != 0) {
					HC_DEBUG("(HC) Invalid version in %s\n", req->buffer);
				}
				else {//All ok, get respnse status and body
					http_status = atoi(req->buffer + strlen(version10));
					/* find body and zero terminate headers */
					body = (char *)strstr(req->buffer, "\r\n\r\n") + 2;
					*body++ = '\0';
					*body++ = '\0';

					body_size = req->buffer_size - (body - req->buffer);

					if(strstr(req->buffer, "Transfer-Encoding: chunked"))
					{
						body_size = chunked_decode(body, body_size);
						body[body_size] = '\0';
					}
				}
			}
			//Call user callback if registered
			if (req->user_callback != NULL) {
				req->user_callback(body, http_status, req->buffer, body_size);
			}
			//Free allocated ram
			prj_free(req->buffer);
			prj_free(req->hostname);
			prj_free(req->path);
			//Set user callback back to NULL
			req->user_callback = NULL;
		}
		//Connection clean up
		espconn_delete(conn);
	}
}

static void ICACHE_FLASH_ATTR error_callback(void *arg, sint8 errType)
{
	HC_DEBUG("(HC) TCP Disconnected with error\n");
	disconnect_callback(arg);
}

static void ICACHE_FLASH_ATTR dns_callback(const char * hostname, ip_addr_t * addr, void * arg)
{
	request_args * req = (request_args *)arg;
	sint8 result = -1;

	//Check input parameter
	if (addr == NULL) { // DNS Not found
		HC_DEBUG("(HC) DNS failed for %s\n", hostname);
	}
	else
	{ // DNS found
		HC_DEBUG("(HC) DNS found %s " IPSTR "\n", hostname, IP2STR(addr));
		//Prepare esp connection
		//Malloc connection struct
		struct espconn * conn = (struct espconn *)&client_connection;
		//Setup connection parameters
		conn->type = ESPCONN_TCP;
		conn->state = ESPCONN_NONE;
		conn->proto.tcp->local_port = espconn_port();
		conn->proto.tcp->remote_port = req->port;
		conn->reserve = req;
		//Set remote ip
		memcpy(conn->proto.tcp->remote_ip, addr, 4);
		//Register connection
		result = espconn_connect(conn);
		HC_DEBUG("(HC) TCP Connect Request (%d)\n",result);
		//Check if connection could be registered
		if(result != 0)
		{
			//Connection setup failed do clean up
			espconn_delete(conn);
		}
	}
	//Check if got valid connection
	if(result != 0)
	{
		//Free allocated ram
		prj_free(req->buffer);
		prj_free(req->post_data);
		prj_free(req->headers);
		prj_free(req->path);
		prj_free(req->hostname);
		//Call user callback if registered
		if (req->user_callback != NULL) {
			req->user_callback("", -1, "", 0);
		}
		//Set user callback back to NULL
		req->user_callback = NULL;
	}
}

void ICACHE_FLASH_ATTR http_raw_request(const char * hostname, int port, const char * path, const char * post_data, const char * headers, http_callback user_callback)
{
	HC_DEBUG("(HC) DNS request\n");
	request_args * req = (request_args *)&client_request;
	ip_addr_t addr;
	err_t error;

	//Setup request parameters
	req->hostname = esp_strdup(hostname);
	req->path = esp_strdup(path);
	req->port = port;
	req->headers = esp_strdup(headers);
	req->post_data = esp_strdup(post_data);
	req->buffer_size = 1;
	req->buffer = (char *)prj_malloc(1);
	req->buffer[0] = '\0'; // Empty string.
	req->user_callback = user_callback;

	error = espconn_gethostbyname((struct espconn *)req, // It seems we don't need a real espconn pointer here.
										hostname, &addr, dns_callback);
	//Check dns request response
	if (error == ESPCONN_INPROGRESS) {
		HC_DEBUG("(HC) DNS pending\n");
	}
	// DNS OK
	else if (error == ESPCONN_OK)
	{
		// Already in the local names table (or hostname was an IP address), execute the callback ourselves.
		dns_callback(hostname, &addr, req);
	}
	//DNS ERROR
	else
	{
		//Get error type
		if (error == ESPCONN_ARG)
		{
			HC_DEBUG("(HC) DNS arg error %s\n", hostname);
		}
		else
		{
			HC_DEBUG("(HC) DNS error code %d\n", error);
		}
		// Handle all DNS errors the same way.
		dns_callback(hostname, NULL, req);
	}
}

/*
 * Parse an URL of the form http://host:port/path
 * <host> can be a hostname or an IP address
 * <port> is optional
 */
void ICACHE_FLASH_ATTR http_post(const char * url, const char * post_data, const char * headers, http_callback user_callback)
{
	char hostname[128] = "";
	int port = 80;
	bool is_http;
	char * path;
	char * colon;

	//Only accept new request if http request is not pending
	if(client_request.user_callback == NULL)
	{
		//Check url
		is_http  = strncmp(url, "http://",  strlen("http://"))  == 0;
		//Check if http request
		if (is_http)
		{
			// Get rid of the protocol.
			url += strlen("http://");
			//Get path
			path = (char *)strchr(url, '/');
			if (path == NULL) {
				path = (char *)strchr(url, '\0'); // Pointer to end of string.
			}
			//Get port integer position
			colon = (char *)strchr(url, ':');
			if (colon > path) {
				colon = NULL; // Limit the search to characters before the path.
			}
			//Check if port is defined
			if (colon == NULL) { // The port is not present.
				memcpy(hostname, url, path - url);
				hostname[path - url] = '\0';
			}
			else //Port is present
			{
				//Get port value
				port = atoi(colon + 1);
				//Check if its a valid port
				if (port == 0)
				{
					HC_DEBUG("(HC) IP Port error %s\n", url);
				}
				else
				{
					memcpy(hostname, url, colon - url);
					hostname[colon - url] = '\0';
				}
			}
			//Check path
			if (path[0] == '\0') {
				// Empty path is not allowed.
				path = "/";
			}
			HC_DEBUG("(HC) IP hostname=%s\n", hostname);
			HC_DEBUG("(HC) IP port=%d\n", port);
			HC_DEBUG("(HC) HTTP path=%s\n", path);
			//Send http request
			http_raw_request(hostname, port, path, post_data, headers, user_callback);

		}
		else // Not http request
		{
			HC_DEBUG("(HC) URL is not HTTP or HTTPS %s\n", url);
		}
	}
	else
	{
		HC_DEBUG("(HC) Http client is BUSY\n");
	}
}

void ICACHE_FLASH_ATTR http_get(const char * url, const char * headers, http_callback user_callback)
{
	//Call without post data
	http_post(url, NULL, headers, user_callback);
}

void ICACHE_FLASH_ATTR httpclient_Init(void)
{
	//Call espconn stack init
	espconn_init();

	//Setup tcp struct
	client_connection.proto.tcp = (esp_tcp *)&client_tcp;

	//Set callbacks
	espconn_regist_connectcb(&client_connection, connect_callback);
	espconn_regist_disconcb(&client_connection, disconnect_callback);
	espconn_regist_reconcb(&client_connection, error_callback);

	//Callback used for status handling
	client_request.user_callback = NULL;
}

void ICACHE_FLASH_ATTR httpclient_Cleanup(void)
{
	//Connection setup failed do clean up
	espconn_delete(&client_connection);
	//Call user callback if registered
	if (client_request.user_callback != NULL) {
		client_request.user_callback("", -1, "", 0);
	}
	//Callback used for status handling
	client_request.user_callback = NULL;
}
