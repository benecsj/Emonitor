#ifndef PLATFORM_H
#define PLATFORM_H


/*
#define sprintf(str, ...) os_sprintf(str, __VA_ARGS__)
#define strcpy(a, b) os_strcpy(a, b)
#define strncpy(a, b, c) os_strncpy(a, b, c)
#define strncmp(a, b, c) os_strncmp(a, b, c)
#define malloc(x) os_malloc(x)
#define free(x) os_free(x)
#define memset(x, a, b) os_memset(x, a, b)
#define memcpy(x, a, b) os_memcpy(x, a, b)
#define strcat(a, b) os_strcat(a, b)
#define strstr(a, b) os_strstr(a, b)
#define strlen(a) os_strlen(a)
#define memcmp(a, b, c) os_memcmp(a, b, c)
*/
#define httpd_printf(format, ...) os_printf(format, ##__VA_ARGS__)
typedef struct espconn* ConnTypePtr;


#endif
