/******************************************************************************
* Includes
\******************************************************************************/


#include <fcntl.h>
#include <stdio.h>
#include "spiffs_manager.h"

/******************************************************************************
* Defines
\******************************************************************************/

/******************************************************************************
* Variables
\******************************************************************************/

static spiffs fs;

static u8_t *spiffs_work_buf;
static u8_t *spiffs_fd_buf;
static u8_t *spiffs_cache_buf;

static u8_t spiffs_work_buffer[LOG_PAGE*2];
static u8_t spiffs_fd_buffer[FD_BUF_SIZE * 2];
#if CACHE_BUF_ENABLE == ON
static u8_t spiffs_cache_buffer[CACHE_BUF_SIZE];
#endif
#define FLASH_UNIT_SIZE 4

/******************************************************************************
* Implementations
\******************************************************************************/
static s32_t ICACHE_FLASH_ATTR spiffs_readblock(u32_t addr, u32_t size, u8_t *dst)
{
	s32_t status = SPIFFS_OK;

	//Validate parameters
    if (size > LOG_PAGE) {
        SPIFFSM_DBG("(SPIFFS) Invalid size provided to read/write (%d)\n", (int) size);
        status = SPIFFS_ERR_NOT_CONFIGURED;
    }
    else if (addr > (FS1_FLASH_ADDR+FS1_FLASH_SIZE+LOG_PAGE)) {
        SPIFFSM_DBG("(SPIFFS) Invalid address provided (%x)\n", (int) addr);
        status = SPIFFS_ERR_NOT_CONFIGURED;
    }
    else
    {
		char tmp_buf[LOG_PAGE + FLASH_UNIT_SIZE * 2];
		u32_t aligned_addr = addr & (-FLASH_UNIT_SIZE);
		u32_t aligned_size =
			((size + (FLASH_UNIT_SIZE - 1)) & -FLASH_UNIT_SIZE) + FLASH_UNIT_SIZE;
		//Read flash
		status = spi_flash_read(aligned_addr, (u32_t *) tmp_buf, aligned_size);
		//Check read error
		if (status != SPIFFS_OK) {
			SPIFFSM_DBG("(SPIFFS) spi_flash_read failed: %d (%d, %d)\n\r", status, (int) aligned_addr,
				   (int) aligned_size);
		}
		else
		{
			//Only read requested, just copy result
			prj_memcpy(dst, tmp_buf + (addr - aligned_addr), size);
		}
    }

    return status;
}

static s32_t ICACHE_FLASH_ATTR spiffs_writeblock(u32_t addr, u32_t size, u8_t *src)
{
    return SPIFFS_ERR_NOT_CONFIGURED;
}

static s32_t ICACHE_FLASH_ATTR spiffs_eraseblock(u32_t addr, u32_t size)
{
	return SPIFFS_ERR_NOT_CONFIGURED;
}

s32_t ICACHE_FLASH_ATTR spiffs_init(void)
{
    spiffs_config cfg;
    s32_t status = -1;
	//Only mount if not yet mounted
    if (SPIFFS_mounted(&fs) == FALSE) {
    	//Load hal functions
        cfg.hal_read_f = spiffs_readblock;
        cfg.hal_write_f = spiffs_writeblock;
        cfg.hal_erase_f = spiffs_eraseblock;
        //Mount filesystem
        status =  SPIFFS_mount(&fs, &cfg, (u8_t*)&spiffs_work_buffer,
        					(u8_t*)&spiffs_fd_buffer, sizeof(spiffs_fd_buffer),
#if CACHE_BUF_ENABLE == ON
        					(u8_t*)&spiffs_cache_buffer,
           					sizeof(spiffs_cache_buffer),
#else
        					NULL_PTR,
        					0,
#endif
                            0);
    }
    //Return mount status
    return status;
}

void ICACHE_FLASH_ATTR spiffs_status(void)
{
	uint8 result = 0;
	uint32 total, used;

	prj_printf("(SPIFFS) File system files:\n");
	spiffs_DIR spiffsDir;
	SPIFFS_opendir(&fs, "/", &spiffsDir);
	struct spiffs_dirent spiffsDirEnt;
	while(SPIFFS_readdir(&spiffsDir, &spiffsDirEnt) != 0) {
		prj_printf("(SPIFFS) %s : %d \n", spiffsDirEnt.name, spiffsDirEnt.size);
	}
	SPIFFS_closedir(&spiffsDir);

    SPIFFS_info(&fs, (u32_t *)&total, (u32_t*)&used);
    prj_printf("(SPIFFS) Total: %d  Used: %d  Free: %d\n",total,used, total-used);

}

uint32_t ICACHE_FLASH_ATTR spiffs_GetFileSize(char * fileName)
{
	spiffs_stat status;
	int32 result = SPIFFS_stat(&fs, (char *)fileName, &status);
	if(result != SPIFFS_OK)
	{
		status.size = 0;
	}

	return status.size;
}

uint32_t ICACHE_FLASH_ATTR spiffs_OpenFile(spiffs_file* filePtr,char * fileName)
{
	(*filePtr) =  SPIFFS_open(&fs, (const char *)fileName, SPIFFS_RDONLY, 0);
	return ((*filePtr) > 0) ? OK : FAIL;
}

void ICACHE_FLASH_ATTR spiffs_CloseFile(spiffs_file fileHandler)
{
	SPIFFS_close(&fs, fileHandler);
}

s32_t ICACHE_FLASH_ATTR spiffs_ReadFile(spiffs_file file,u8_t * buffer,s32_t length)
{
	return SPIFFS_read(&fs, file, buffer, length);
}
