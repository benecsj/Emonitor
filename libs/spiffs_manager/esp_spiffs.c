
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

    int res = spi_flash_read(aligned_addr, (u32_t *) tmp_buf, aligned_size);

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

int ICACHE_FLASH_ATTR _open_r(struct _reent *r, const char *filename, int flags, int mode)
{
    spiffs_mode sm = 0;
    int res;
    int rw = (flags & 3);

    if (rw == O_RDONLY || rw == O_RDWR) {
        sm |= SPIFFS_RDONLY;
    }

    if (rw == O_WRONLY || rw == O_RDWR) {
        sm |= SPIFFS_WRONLY;
    }

    if (flags & O_CREAT) {
        sm |= SPIFFS_CREAT;
    }

    if (flags & O_TRUNC) {
        sm |= SPIFFS_TRUNC;
    }

    if (flags & O_APPEND) {
        sm |= SPIFFS_APPEND;
    }

    /* Supported in newer versions of SPIFFS. */
    /* if (flags && O_EXCL) sm |= SPIFFS_EXCL; */
    /* if (flags && O_DIRECT) sm |= SPIFFS_DIRECT; */

    res = SPIFFS_open(&fs, (char *) filename, sm, 0);

    if (res >= 0) {
        res += NUM_SYS_FD;
    }

    return res;
}

_ssize_t ICACHE_FLASH_ATTR _read_r(struct _reent *r, int fd, void *buf, size_t len)
{

    ssize_t res;

    if (fd < NUM_SYS_FD) {
        res = -1;
    } else {
        res = SPIFFS_read(&fs, fd - NUM_SYS_FD, buf, len);
    }

    return res;
}

_ssize_t ICACHE_FLASH_ATTR _write_r(struct _reent *r, int fd, void *buf, size_t len)
{

    if (fd < NUM_SYS_FD) {
        return -1;
    }

    int res = SPIFFS_write(&fs, fd - NUM_SYS_FD, (char *) buf, len);
    return res;
}

_off_t ICACHE_FLASH_ATTR _lseek_r(struct _reent *r, int fd, _off_t where, int whence)
{

    ssize_t res;

    if (fd < NUM_SYS_FD) {
        res = -1;
    } else {
        res = SPIFFS_lseek(&fs, fd - NUM_SYS_FD, where, whence);
    }

    return res;
}

int ICACHE_FLASH_ATTR _close_r(struct _reent *r, int fd)
{

    if (fd < NUM_SYS_FD) {
        return -1;
    }

    SPIFFS_close(&fs, fd - NUM_SYS_FD);
    return 0;
}

int ICACHE_FLASH_ATTR _rename_r(struct _reent *r, const char *from, const char *to)
{

    int res = SPIFFS_rename(&fs, (char *) from, (char *) to);
    return res;
}

int ICACHE_FLASH_ATTR _unlink_r(struct _reent *r, const char *filename)
{

    int res = SPIFFS_remove(&fs, (char *) filename);
    return res;
}

int ICACHE_FLASH_ATTR _fstat_r(struct _reent *r, int fd, struct stat *s)
{

    int res;
    spiffs_stat ss;
    prj_memset(s, 0, sizeof(*s));

    if (fd < NUM_SYS_FD) {
        s->st_ino = fd;
        s->st_rdev = fd;
        s->st_mode = S_IFCHR | 0666;
        return 0;
    }

    res = SPIFFS_fstat(&fs, fd - NUM_SYS_FD, &ss);

    if (res < 0) {
        return res;
    }

    s->st_ino = ss.obj_id;
    s->st_mode = 0666;
    s->st_nlink = 1;
    s->st_size = ss.size;
    return 0;
}
