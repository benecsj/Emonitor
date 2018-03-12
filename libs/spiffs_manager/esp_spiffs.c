
#include "spiffs_manager.h"

#include <fcntl.h>
#include <stdio.h>

#define NUM_SYS_FD 3

static spiffs fs;

static u8_t *spiffs_work_buf;
static u8_t *spiffs_fd_buf;
static u8_t *spiffs_cache_buf;

static u8_t spiffs_work_buffer[LOG_PAGE*2];
static u8_t spiffs_fd_buffer[FD_BUF_SIZE * 2];
static u8_t spiffs_cache_buffer[CACHE_BUF_SIZE];

#define FLASH_UNIT_SIZE 4

static s32_t ICACHE_FLASH_ATTR esp_spiffs_readwrite(u32_t addr, u32_t size, u8_t *p, int write)
{
    /*
     * With proper configurarion spiffs never reads or writes more than
     * LOG_PAGE_SIZE
     */

    if (size > LOG_PAGE) {
        SPIFFSM_DBG("Invalid size provided to read/write (%d)\n\r", (int) size);
        return SPIFFS_ERR_NOT_CONFIGURED;
    }

    char tmp_buf[LOG_PAGE + FLASH_UNIT_SIZE * 2];
    u32_t aligned_addr = addr & (-FLASH_UNIT_SIZE);
    u32_t aligned_size =
        ((size + (FLASH_UNIT_SIZE - 1)) & -FLASH_UNIT_SIZE) + FLASH_UNIT_SIZE;
    prj_ENTER_CRITICAL();
    int res = spi_flash_read(aligned_addr, (u32_t *) tmp_buf, aligned_size);
    prj_EXIT_CRITICAL();
    if (res != 0) {
        SPIFFSM_DBG("spi_flash_read failed: %d (%d, %d)\n\r", res, (int) aligned_addr,
               (int) aligned_size);
        return res;
    }

    if (!write) {
        prj_memcpy(p, tmp_buf + (addr - aligned_addr), size);
        return SPIFFS_OK;
    }

    prj_memcpy(tmp_buf + (addr - aligned_addr), p, size);

    res = spi_flash_write(aligned_addr, (u32_t *) tmp_buf, aligned_size);

    if (res != 0) {
//	    SPIFFSM_DBG("spi_flash_write failed: %d (%d, %d)\n\r", res,
//	              (int) aligned_addr, (int) aligned_size);
        return res;
    }

    return SPIFFS_OK;
}

static s32_t ICACHE_FLASH_ATTR esp_spiffs_read(u32_t addr, u32_t size, u8_t *dst)
{
    return esp_spiffs_readwrite(addr, size, dst, 0);
}

static s32_t ICACHE_FLASH_ATTR esp_spiffs_write(u32_t addr, u32_t size, u8_t *src)
{
    return esp_spiffs_readwrite(addr, size, src, 1);
}

static s32_t ICACHE_FLASH_ATTR esp_spiffs_erase(u32_t addr, u32_t size)
{
    /*
     * With proper configurarion spiffs always
     * provides here sector address & sector size
     */
    if (size != SECTOR_SIZE || addr % SECTOR_SIZE != 0) {
        SPIFFSM_DBG("Invalid size provided to esp_spiffs_erase (%d, %d)\n\r",
               (int) addr, (int) size);
        return SPIFFS_ERR_NOT_CONFIGURED;
    }

    return spi_flash_erase_sector(addr / SECTOR_SIZE);
}

spiffs* ICACHE_FLASH_ATTR esp_spiffs_get_fs()
{
    return &fs;
}

s32_t ICACHE_FLASH_ATTR esp_spiffs_init(void)
{
    spiffs_config cfg;
    s32_t ret = -1;
	//Only mount if not yet mounted
    if (SPIFFS_mounted(&fs) == FALSE) {
    	//Load hal functions
        cfg.hal_read_f = esp_spiffs_read;
        cfg.hal_write_f = esp_spiffs_write;
        cfg.hal_erase_f = esp_spiffs_erase;
        //Mount filesystem
        ret =  SPIFFS_mount(&fs, &cfg, (u8_t*)&spiffs_work_buffer,
        					(u8_t*)&spiffs_fd_buffer, sizeof(spiffs_fd_buffer),
        					(u8_t*)&spiffs_cache_buffer, sizeof(spiffs_cache_buffer),
                            0);
    }
    //Return mount status
    return ret;        
}

void ICACHE_FLASH_ATTR esp_spiffs_deinit(u8_t format)
{
	//Check if its mounted
    if (SPIFFS_mounted(&fs)) {
        SPIFFS_unmount(&fs);
        //Check if formating requested
        if (format) {
            SPIFFS_format(&fs);
        }
    }
}

