
/*Includes*/
#include "project_config.h"
#include "MHZ14.h"


/*Defines*/

#if DEBUG_MHZ14
#define DBG_MHZ14(...) os_printf(__VA_ARGS__)
#else
#define DBG_MHZ14(...)
#endif

#define MHZ14_STATE_UNINITALIZED	3
#define MHZ14_STATE_LOW_SIG			0
#define MHZ14_STATE_HIGH_SIG		1

#define MHZ14_WINDOW_SIZE			5
#define MHZ14_MAX_PEROID_VARIANCE	100
#define MHZ14_MAX_PPM_VARIANCE		20
#define MHZ14_PERIOD				1004

#define MHZ14_LOW_INDEX				1
#define MHZ14_HIGH_INDEX			0
#define MIN							0
#define MAX							1

#define STATE_MIN_COUNT				10


/*Functions*/

uint32	MHZ14_managedValue;
uint8 MHZ14_ValueState;

uint8 MHZ14_signalState;
uint32 MHZ14_lowCount;
uint32 MHZ14_highCount;

uint32 MHZ14_Window[MHZ14_WINDOW_SIZE][2]={0};

uint32 MHZ14_writeIndex = MHZ14_WINDOW_SIZE;

void ICACHE_FLASH_ATTR MHZ14_Init(void)
{
	MHZ14_managedValue = MHZ14_INVALID_VALUE;
	MHZ14_signalState = MHZ14_STATE_UNINITALIZED;
	MHZ14_lowCount = 0;
	MHZ14_highCount = 0;
	MHZ14_ValueState = 0;
}


void ICACHE_FLASH_ATTR MHZ14_Main(void)
{
	uint32 i;
	uint32 low[2] ={5000,0};
	uint32 high[2]={5000,0};
	uint32 lowAverage = 0;
	uint32 highAverage = 0;
	uint32 period;
	uint32 temp;
	uint32 lowRead;
	uint32 highRead;
	//Check measurement integrity
	for(i=0;i<MHZ14_WINDOW_SIZE;i++)
	{
		//Read value from window
		MHZ14_ENTER_CRITICAL();
		lowRead = MHZ14_Window[i][MHZ14_LOW_INDEX];
		highRead = MHZ14_Window[i][MHZ14_HIGH_INDEX];
		MHZ14_EXIT_CRITICAL();
		//Check min max values
		if (low[MIN] > lowRead) 	{low[MIN]  = lowRead;}
		if (low[MAX] < lowRead)		{low[MAX]  = lowRead;}
		if (high[MIN] > highRead)	{high[MIN] = highRead;}
		if (high[MAX] < highRead)	{high[MAX] = highRead;}
		//Sum for average
		lowAverage = lowAverage + lowRead;
		highAverage = highAverage + highRead;
	}
	//Caclulate average
	lowAverage = lowAverage / MHZ14_WINDOW_SIZE;
	highAverage = highAverage / MHZ14_WINDOW_SIZE;
	//Calculate period
	period = lowAverage + highAverage;
	//Check integrity
	if(
		(period > (MHZ14_PERIOD-(MHZ14_MAX_PEROID_VARIANCE/2))) &&
		(period < (MHZ14_PERIOD+(MHZ14_MAX_PEROID_VARIANCE/2))) &&
		((low[MAX]-low[MIN]) < MHZ14_MAX_PPM_VARIANCE) &&
		((high[MAX]-high[MIN]) < MHZ14_MAX_PPM_VARIANCE) &&
		(highAverage > MHZ14_MIN_VALID_TIME) &&
		(highAverage < (MHZ14_PERIOD+MHZ14_MAX_PPM_VARIANCE))
	)
	{
		//Calculate PPM from PWM signal
		temp = (2000 * (highAverage -2)) / (highAverage + lowAverage -4);
		//Valid signal detected
		if (MHZ14_ValueState < STATE_MIN_COUNT)	{MHZ14_ValueState = STATE_MIN_COUNT;}
	}
	else
	{
		temp = MHZ14_INVALID_VALUE;
		//Cancel invalidation
		if(MHZ14_ValueState > 0)
		{
			MHZ14_ValueState--;
			MHZ14_ENTER_CRITICAL();
			temp = MHZ14_managedValue;
			MHZ14_EXIT_CRITICAL();
		}
	}
	//Detect no signal
	if((MHZ14_highCount > MHZ14_INVALID_VALUE) ||
	   (MHZ14_lowCount > MHZ14_INVALID_VALUE))
	{
		MHZ14_ENTER_CRITICAL();
		MHZ14_Window[MHZ14_writeIndex][MHZ14_HIGH_INDEX] = MHZ14_highCount;
		MHZ14_Window[MHZ14_writeIndex][MHZ14_LOW_INDEX] = MHZ14_lowCount;
		MHZ14_EXIT_CRITICAL();
	}

	//Store calculate value
	MHZ14_ENTER_CRITICAL();
	MHZ14_managedValue = temp;
	MHZ14_EXIT_CRITICAL();
	DBG_MHZ14("(MHZ14) %d %d : %d  >  %d  (%d) \n",MHZ14_Window[0][0],MHZ14_Window[0][1], (MHZ14_Window[0][0]+MHZ14_Window[0][1]),MHZ14_managedValue,MHZ14_ValueState);
}


void IRAM0 MHZ14_Feed(uint8 level)
{
	//Detect rising edge event
	if((MHZ14_signalState == MHZ14_STATE_LOW_SIG) && (level == 1))
	{
		//Store count values
		if(MHZ14_writeIndex != MHZ14_WINDOW_SIZE)
		{
			MHZ14_ENTER_CRITICAL();
			MHZ14_Window[MHZ14_writeIndex][MHZ14_HIGH_INDEX] =MHZ14_highCount;
			MHZ14_Window[MHZ14_writeIndex][MHZ14_LOW_INDEX] =MHZ14_lowCount;
			MHZ14_EXIT_CRITICAL();
			MHZ14_writeIndex = (MHZ14_writeIndex +1) % MHZ14_WINDOW_SIZE;
		}
		else
		{
			MHZ14_writeIndex = 0;
		}
		//Reset count values
		MHZ14_highCount = 0;
		MHZ14_lowCount = 0;

	}
	//Count time in level
	if(level == 1)
	{
		MHZ14_highCount++;
	}
	else
	{
		MHZ14_lowCount++;
	}
	//Change state based on level
	MHZ14_signalState = level;
}

uint32 ICACHE_FLASH_ATTR MHZ14_GetMeasurement(void)
{
	uint32 value;
	MHZ14_ENTER_CRITICAL();
	value = MHZ14_managedValue;
	MHZ14_EXIT_CRITICAL();
	return value;
}
