/******************************************************************************
* Includes
\******************************************************************************/

//Common
#include "esp_common.h"
#include "user_config.h"

#include "spiffs_manager.h"

#include "esp_spiffs.h"
extern spiffs* esp_spiffs_get_fs();

/******************************************************************************
* Defines
\******************************************************************************/

/******************************************************************************
* Variables
\******************************************************************************/

spiffs* fs = NULL;
struct esp_spiffs_config config = {
		FS1_FLASH_SIZE,
		FS1_FLASH_ADDR,
		SECTOR_SIZE,
		LOG_BLOCK,
		LOG_PAGE,
		FD_BUF_SIZE * 2,
		CACHE_BUF_SIZE
};

/******************************************************************************
* Implementations
\******************************************************************************/


/******************************************************************************
 * FunctionName : spiffs_init
 * Description  : init spiffs using spiffs_manager
 * Parameters   : none
 * Returns      : none
 *******************************************************************************/
void spiffs_init(void) {
	SPIFFSM_DBG("(SPIFFS) Initializing SPIFFS\n");

	int32 result = 0;
/*
    config.phys_size = FS1_FLASH_SIZE;
    config.phys_addr = FS1_FLASH_ADDR;
    config.phys_erase_block = SECTOR_SIZE;
    config.log_block_size = LOG_BLOCK;
    config.log_page_size = LOG_PAGE;
    config.fd_buf_size = FD_BUF_SIZE * 2;
    config.cache_buf_size = CACHE_BUF_SIZE;
*/
	//Init spiffs file system
    result = esp_spiffs_init(&config);
    //Get File System
    fs = esp_spiffs_get_fs();

    //Show SPIFFS Status
    uint32 total, used;
    result = SPIFFS_info(fs, &total, &used);
    SPIFFSM_DBG("(SPIFFS) Total: %d  Used: %d  Free: %d\n",total,used, total-used);

}

void spiffs_test_write()
{
	int32 result = 0;
	uint8 buf[128];
	uint8 fileName[10] = {0};
	BOOL created = FALSE;
	uint8 i;

	//Data to be written
	for (i = 0; i < 128; i++) buf[i] = i;

	//Setup filename
	fileName[0] = 'd';
	fileName[1] = 'a';
	fileName[2] = 't';
	fileName[3] = 'a';
	fileName[4] = '0';
	fileName[5] = '0';

    i = 0;
    do
    {
    	//Check if file found
		spiffs_stat status;
		result = SPIFFS_stat(fs, (char *)fileName, &status);
		// create a file with some data in it if not found
		if(result != OK)
		{
			spiffs_file fd = SPIFFS_open(fs, (char *)fileName, SPIFFS_CREAT | SPIFFS_RDWR, 0);
			result = SPIFFS_write(fs, fd, buf, 128);
			SPIFFS_close(fs, fd);
			created = TRUE;
			result = SPIFFS_stat(fs, (char *)fileName, &status);
			SPIFFSM_DBG("(SPIFFS) File Created : %d %s %d \n",result, status.name, status.size);
		}
		else
		{
			SPIFFSM_DBG("(SPIFFS) Already Exist: %d %s %d \n",result, status.name, status.size);
		}
		//Generate new filename
		i++;
		fileName[4] = '0'+(i/10);
		fileName[5] = '0'+(i%10);
    }while (created == FALSE);
}

void spiffs_test_read()
{
	char buf[128] = {0};
	spiffs_stat status;
	int32 result = SPIFFS_stat(fs, (char *)"/test"+0, &status);
	SPIFFSM_DBG("(SPIFFS) %d %d  %s %d \n",result, status.obj_id, status.name, status.size);

	spiffs_file fd = SPIFFS_open(fs, "/test", SPIFFS_RDWR, 0);
	SPIFFS_read(fs, fd, (u8_t *)buf, 128);
	SPIFFS_close(fs, fd);
	printf("(SPIFFS) File: %s\n",buf);
}

void spiffs_format()
{
		SPIFFSM_DBG("(SPIFFS) Execute formating\n");
		//Execute formating
		esp_spiffs_deinit(TRUE);
		//Re init Filesystem
		esp_spiffs_init(&config);
}

void spiffs_status()
{
	uint8 result = 0;
	uint32 total, used;

	printf("(SPIFFS) File system files:\n");
	spiffs_DIR spiffsDir;
	SPIFFS_opendir(fs, "/", &spiffsDir);
	struct spiffs_dirent spiffsDirEnt;
	while(SPIFFS_readdir(&spiffsDir, &spiffsDirEnt) != 0) {
	  printf("%s : %d \n", spiffsDirEnt.name, spiffsDirEnt.size);
	}
	SPIFFS_closedir(&spiffsDir);

    SPIFFS_info(fs, &total, &used);
	printf("(SPIFFS) Total: %d  Used: %d  Free: %d\n",total,used, total-used);

}
