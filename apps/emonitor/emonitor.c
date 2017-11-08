/******************************************************************************
* Includes
\******************************************************************************/

#include "project_config.h"
#include "user_config.h"

#include "Emonitor.h"

#include "esp_common.h"
#include "uart.h"
#include "hw_timer.h"
#include "pins.h"
#include "pwm.h"
#include "gpio.h"

#include "httpclient.h"
#include "Sensor_Manager.h"


/******************************************************************************
* Defines
\******************************************************************************/

#define Append(length,buffer,format,...) length += sprintf(&buffer[length],format,__VA_ARGS__)

#define PWM_0_OUT_IO_MUX PERIPHS_IO_MUX_MTDI_U
#define PWM_0_OUT_IO_NUM 12
#define PWM_0_OUT_IO_FUNC FUNC_GPIO12

#define PWM_1_OUT_IO_MUX PERIPHS_IO_MUX_MTDO_U
#define PWM_1_OUT_IO_NUM 15
#define PWM_1_OUT_IO_FUNC FUNC_GPIO15

#define PWM_2_OUT_IO_MUX PERIPHS_IO_MUX_MTCK_U
#define PWM_2_OUT_IO_NUM 13
#define PWM_2_OUT_IO_FUNC FUNC_GPIO13

/******************************************************************************
* Variables
\******************************************************************************/

uint32 pulse_Counter = 0;

uint32 Emonitor_counter = 0;
uint32 Emonitor_statusCounter = 0;

uint32 Emonitor_timing = 0;

uint32 Emonitor_uptime = 0;

uint8 Emonitor_ledControl = 0;

uint32 pwm_info[][3] = {   {PWM_0_OUT_IO_MUX,PWM_0_OUT_IO_FUNC,PWM_0_OUT_IO_NUM}   };

u32 pwm_duty[1] = {500};

/******************************************************************************
* Implementations
\******************************************************************************/

void pulseCounter(void)
{
	pulse_Counter++;
}
/******************************************************************************
 * FunctionName : Emonitor_init
 * Description  : Init emonitor application
 * Parameters   : none
 * Returns      : none
 *******************************************************************************/
void Emonitor_Init(void) {

	//Init UART
	UART_SetBaudrate(UART0, BIT_RATE_115200);
	//Init Status LED
	pinMode(LED2_BUILTIN,OUTPUT);
	//Init Pwm out
	pwm_init(10000, pwm_duty, 1, pwm_info);
	pwm_start();
	//Pulse counter input
	pinMode(D2,INPUT);
	attachInterrupt(D2,pulseCounter,RISING);

	//Init timer for fast task
    hw_timer_init();
    hw_timer_set_func(Emonitor_Main_1ms);
    hw_timer_arm(1000,1);
}

/******************************************************************************
 * FunctionName : Emonitor_Main_1ms
 * Description  : Emonitor Fast Main
 * Parameters   : none
 * Returns      : none
 *******************************************************************************/
void Emonitor_Main_1ms(void) {
	digitalWrite(LED2_BUILTIN, (Emonitor_statusCounter | Emonitor_ledControl));
	Emonitor_statusCounter= (Emonitor_statusCounter+1)%2;
}

/******************************************************************************
 * FunctionName : Emonitor_Main_1000ms
 * Description  : Emonitor Slow Main
 * Parameters   : none
 * Returns      : none
 *******************************************************************************/
void Emonitor_Main_1000ms(void) {
	uint8 i;
	uint16 length = 0;
	char buffer[500];

	uint8 nodeId = 2;
	char url[100] = {"http://v9.emonitor.hu"};
	char apyKey[33] = {"97d3e42a841ea6c219582211313d5051"};

	uint8 tempCount;
	uint8* ids;
	sint16* temperatures;

	//Uptime counter
	Emonitor_uptime++;
	//Free ram
	uint32 freeRam = system_get_free_heap_size();


	Emonitor_timing++;
	if(Emonitor_timing == 10){
		DBG("CYCLE(%d) PULSE(%d) HEAP:(%d)\n", Emonitor_counter,pulse_Counter,freeRam);
		taskENTER_CRITICAL();
		Emonitor_counter = 0;
		pulse_Counter = 0;
		taskEXIT_CRITICAL();
		Emonitor_timing = 0;

		DBG("----------------Emonitor Send Data-------------\n");

		Sensor_Manager_Get_TempSensorData(&tempCount,&ids,&temperatures);
	    //Start of Emoncsm send Url
	    Append(length,buffer,"%s/input/post.json?node=%d&json={",url,nodeId);
	    //Add freeram
	    Append(length,buffer,"freeram:%d,",freeRam);
	    //Add temperatures
	    for(i=0;i<tempCount;i++)
	    {
	    	 Append(length,buffer,"Temp_%02X%02X%02X%02X%02X:%d.%d,",ids[(i*8)+1],ids[(i*8)+2],ids[(i*8)+3],ids[(i*8)+4],ids[(i*8)+7],temperatures[i]/10,abs(temperatures[i]%10));
	    }
	    //Add Uptime
	    Append(length,buffer,"uptime:%d",Emonitor_uptime);
	    //End of Emoncsm send Url
	    Append(length,buffer,"}&apikey=%s",apyKey);


	    //Send out Emoncsm Data
		http_get(buffer, "", http_callback_example);

	}
}

/******************************************************************************
 * FunctionName : Emonitor_Main_Background
 * Description  : Emonitor Background Main
 * Parameters   : none
 * Returns      : none
 *******************************************************************************/
void Emonitor_Main_Background(void) {
	taskENTER_CRITICAL();
	Emonitor_counter++;
	taskEXIT_CRITICAL();
}


