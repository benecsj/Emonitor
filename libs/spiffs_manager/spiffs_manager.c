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

spiffs* fs;
struct esp_spiffs_config config;

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
	uint8 i;
	uint8 result = 0;
	BOOL created = FALSE;
	uint8 buf[128];

	uint8 fileName[10] = {0};
	fileName[0] = 'd';
	fileName[1] = 'a';
	fileName[2] = 't';
	fileName[3] = 'a';
	fileName[4] = '0';
	fileName[5] = '0';

	for (i = 0; i < 128; i++) buf[i] = i;


    config.phys_size = FS1_FLASH_SIZE;
    config.phys_addr = FS1_FLASH_ADDR;
    config.phys_erase_block = SECTOR_SIZE;
    config.log_block_size = LOG_BLOCK;
    config.log_page_size = LOG_PAGE;
    config.fd_buf_size = FD_BUF_SIZE * 2;
    config.cache_buf_size = CACHE_BUF_SIZE;

    result = esp_spiffs_init(&config);

    uint32 total, used;
    fs = esp_spiffs_get_fs();
    result = SPIFFS_info(fs, &total, &used);
    SPIFFSM_DBG("(SPIFFS) %d T:%d  U:%d \n",result,total,used );


    //---------------TEST WRITE-------------------------------
    i = 0;
    do
    {
		spiffs_stat status;
		result = SPIFFS_stat(fs, (char *)fileName, &status);
		SPIFFSM_DBG("(SPIFFS) %d %d  %s %d \n",result, status.obj_id, status.name, status.size);
		// create a file with some data in it
		if(result != OK)
		{
			//spiffs_file fd = SPIFFS_open(fs, (char *)fileName, SPIFFS_CREAT | SPIFFS_RDWR, 0);
			//result = SPIFFS_write(fs, fd, buf, 128);
			//SPIFFS_close(fs, fd);
			created = TRUE;
		}
		i++;
		fileName[4] = '0'+(i/10);
		fileName[5] = '0'+(i%10);
    }while (created == FALSE);
    //--------------------TEST READ-----------------------------
    {
    	spiffs_stat status;
    	result = SPIFFS_stat(fs, (char *)"test", &status);
		if(result = OK)
		{
			SPIFFSM_DBG("(SPIFFS) %d %d  %s %d \n",result, status.obj_id, status.name, status.size);
		}
    }

    //List dir
    spiffs_DIR spiffsDir;
    SPIFFS_opendir(fs, "/", &spiffsDir);
    struct spiffs_dirent spiffsDirEnt;
    while(SPIFFS_readdir(&spiffsDir, &spiffsDirEnt) != 0) {
      printf("Got a directory entry: %s\n", spiffsDirEnt.name);
    }
    SPIFFS_closedir(&spiffsDir);

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

    result = SPIFFS_info(fs, &total, &used);
    SPIFFSM_DBG("(SPIFFS) %d T:%d  U:%d \n",result,total,used );

}
