

#include "esp_fs.h"
#include <fcntl.h>
#include <stdio.h>


void ICACHE_FLASH_ATTR esp_fs_init(void) {
	SPIFFSM_DBG("(FS) Initializing file system\n");

	int32 result = 0;

	//Init spiffs file system
    result = spiffs_init();
#if DEBUG_SPIFFS	== ON
    esp_fs_status();
#endif
}

void ICACHE_FLASH_ATTR esp_fs_status(void)
{
	spiffs_status();
}

uint32_t ICACHE_FLASH_ATTR esp_fs_GetFileSize(char * fileName)
{
	return spiffs_GetFileSize(fileName);
}

uint32_t ICACHE_FLASH_ATTR esp_fs_OpenFile(esp_fs_file* filePtr,char * fileName)
{
	return  spiffs_OpenFile(filePtr,fileName);
}

void ICACHE_FLASH_ATTR esp_fs_CloseFile(esp_fs_file* filePtr)
{
	spiffs_CloseFile(*filePtr);
}

s32_t ICACHE_FLASH_ATTR esp_fs_ReadFile(esp_fs_file* filePtr,u8_t * buffer,uint32_t length)
{
	return spiffs_ReadFile(*filePtr, buffer, length);
}
