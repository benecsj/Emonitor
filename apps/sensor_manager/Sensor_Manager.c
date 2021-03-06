/*
 * APP_Sensor_Manager.c
 *
 *  Created on: Aug 13, 2012
 *      Author: Jooo
 */

/**********************************************************************************
 * Includes
 **********************************************************************************/
#include "project_config.h"
#include "Sensor_Manager.h"
#include "D18_DS18B20_Temp_Sensor.h"
#include "OWP_One_Wire_Protocol_Driver.h"
#include "CRC_crc8.h"
#include "pins.h"


/**********************************************************************************
 * Defines
 **********************************************************************************/

#define APP_REPORT_FAIL() (Sensor_Manager_ErrorCounters[Sensor_Manager_ErrorCounter_index]++)
#define APP_REPORT_PASS() Sensor_Manager_ErrorCounters[Sensor_Manager_ErrorCounter_index] = 0

#if DEBUG_SENSOR_MANAGER
#define DBG_SENSOR(...) printf(__VA_ARGS__)
#else
#define DBG_SENSOR(...)
#endif

#define DBG_SENSOR2(...)

/**********************************************************************************
 * Types
 **********************************************************************************/
typedef enum
{
	SENSOR_MANAGER_NORMAL,
	SENSOR_MANAGER_NEW_TEMP_DETECTED,
	SENSOR_MANAGER_WAIT_FOR_FIRST_CONVERSION,
}Sensor_Manager_Status;

/**********************************************************************************
 * Defines
 **********************************************************************************/
#define ERROR_AVERAGES 5
#define Sensor_Manager_Change_State(a) 	Sensor_Manager_State = a
#define DIGITAL_PULSE_COUNTERS			(SENSOR_MANAGER_PULSE_COUNTERS - SENSOR_MANAGER_ANALOGCHANNELS_COUNT)
/**********************************************************************************
 * Variables
 **********************************************************************************/
uint8 SENSOR_MANAGER_DS18B20Count;
uint16 Sensor_Manager_Timing = 0;
uint8 Sensor_Manager_RetryCount = 0;
uint8 Sensor_Manager_sensorIDs[SENSOR_MANAGER_DS18B20MAXCOUNT*OWP_CONST_ROMCODE_SIZE];
uint8 Sensor_Manager_sensorChannels[SENSOR_MANAGER_DS18B20MAXCOUNT];
uint8 Sensor_Manager_sensorIDsTEMP[SENSOR_MANAGER_DS18B20MAXCOUNT*OWP_CONST_ROMCODE_SIZE];
uint8 Sensor_Manager_sensorChannelsTEMP[SENSOR_MANAGER_DS18B20MAXCOUNT];
sint16 SENSOR_MANAGER_DS18B20TempList[SENSOR_MANAGER_DS18B20MAXCOUNT];

#if SENSOR_MANAGER_ANALOGCHANNELS_COUNT > 0
uint16 APP_PortMon_analogValues[SENSOR_MANAGER_ANALOGCHANNELS_COUNT];
uint16 APP_PortMon_analogWindow[ANALOG_WINDOW] = {0};
uint16 APP_PortMon_analogWindowIndex = 0;
uint16 APP_PortMon_analogAverage = 0;
uint16 APP_PortMon_analogMax = 0;
#endif

uint32 Sensor_Manager_PulseCounters[SENSOR_MANAGER_PULSE_COUNTERS] = {};

uint32 Sensor_Manager_ErrorCounter=0;
uint32 Sensor_Manager_ErrorCounters[ERROR_AVERAGES];
uint8 Sensor_Manager_ErrorCounter_index = 0;

uint8 Sensor_Manager_analogToPulseState = 1;
uint8 Sensor_Manager_lastInputLevel[DIGITAL_PULSE_COUNTERS] = {1};

Sensor_Manager_Status Sensor_Manager_State = SENSOR_MANAGER_NORMAL;

/**********************************************************************************
 * Function defines
 **********************************************************************************/
uint8 SENSOR_MANAGER_DS18B20_Search(void);
void SENSOR_MANAGER_DS18B20Measure(void);
void Sensor_Manager_UpdateSensors(void);
void SENSOR_MANAGER_DS18B20_StartMeasure(void);
void Sensor_Manager_ProcessAnalog(void);
/**********************************************************************************
 * Functions
 **********************************************************************************/

void ICACHE_FLASH_ATTR Sensor_Manager_Init() {
    uint8 i;

	//Pulse counter input
#if (DIGITAL_PULSE_COUNTERS > 0)
	pinMode(PULSE_INPUT0,INPUT);
#endif
#if (DIGITAL_PULSE_COUNTERS > 1)
	pinMode(PULSE_INPUT1,INPUT);
#endif
#if (DIGITAL_PULSE_COUNTERS > 2)
	pinMode(PULSE_INPUT2,INPUT);
#endif
#if (DIGITAL_PULSE_COUNTERS > 3)
	pinMode(PULSE_INPUT3,INPUT);
#endif

	//Init pulse counters
	Sensor_Manager_ResetPulseCounters();
	//Init CO2 sensor
	MHZ14_Init();
	//Init temp sensors
    /*Load default invalid temp values*/
    for (i = 0; i < SENSOR_MANAGER_DS18B20MAXCOUNT; i++) {
        //Set temp sensor init value
        SENSOR_MANAGER_DS18B20TempList[i] = Sensor_Manager_INVALID_TEMP;
    }
    //Search for sensors
    Sensor_Manager_UpdateSensors();
    //Init SW pulseCounter
    Sensor_Manager_analogToPulseState = (system_adc_read()>512);
}

void IRAM0 Sensor_Manager_1ms() {
    //MHZ14 CO2 Sensor
    MHZ14_Feed(digitalRead(MHZ14_INPUT_PIN));
}

void ICACHE_FLASH_ATTR Sensor_Manager_Fast() {
    //Local variables
    uint8 CS_i;

#if SENSOR_MANAGER_ANALOGCHANNELS_COUNT > 0
    //Scan Adc channels
    for (CS_i = 0; CS_i < SENSOR_MANAGER_ANALOGCHANNELS_COUNT; CS_i++) {
        //Read Adc channel and store it in buffer
        APP_PortMon_analogValues[CS_i] = system_adc_read();
        //Filtering for first adc channel
        if(CS_i == 0)
        {
        	APP_PortMon_analogWindow[APP_PortMon_analogWindowIndex] = APP_PortMon_analogValues[CS_i];
        	APP_PortMon_analogWindowIndex = (APP_PortMon_analogWindowIndex +1)%ANALOG_WINDOW;
        }
    }
    //SW pulse counter
    if((Sensor_Manager_analogToPulseState == 1) && (APP_PortMon_analogValues[0] < 300))
    {
    	Sensor_Manager_analogToPulseState = 0;
    }
    else if((Sensor_Manager_analogToPulseState == 0) && (APP_PortMon_analogValues[0] > 600))
    {
       	Sensor_Manager_analogToPulseState = 1;
    	//Count rising edge
       	Sensor_Manager_PulseCounters[DIGITAL_PULSE_COUNTERS]++;

    }
#endif
}

void ICACHE_FLASH_ATTR Sensor_Manager_Main() {
	uint8 i;
	uint32 temp;

	//Based on state do stuff
	switch(Sensor_Manager_State)
	{
	case SENSOR_MANAGER_NORMAL:
	    /*Read DS18B20 sensors*/
	    SENSOR_MANAGER_DS18B20Measure();
	    //Trigger conversion
	    SENSOR_MANAGER_DS18B20_StartMeasure();
		break;
	case SENSOR_MANAGER_NEW_TEMP_DETECTED:
		//Trigger measurement start
		SENSOR_MANAGER_DS18B20_StartMeasure();
		Sensor_Manager_Change_State(SENSOR_MANAGER_NORMAL);
		break;
	}

    //Temp sensor rescan timing
    Sensor_Manager_Timing = (Sensor_Manager_Timing + 1) % (TEMP_RESCAN_PERIOD+1);
    //Is it time to scan
    if (Sensor_Manager_Timing == 0) {
        //Search for sensors again
        Sensor_Manager_UpdateSensors();
        //Store error counts
        temp = 0;
		for(i=0;i<ERROR_AVERAGES;i++)
		{
			temp = temp + Sensor_Manager_ErrorCounters[i];
		}
		Sensor_Manager_ErrorCounter = temp;
		// Increment index
		Sensor_Manager_ErrorCounter_index = (Sensor_Manager_ErrorCounter_index +1) % ERROR_AVERAGES;
		//Start new error count segment
        Sensor_Manager_ErrorCounters[Sensor_Manager_ErrorCounter_index] = 0;

    }

    // MHZ14 CO2 Sensor
    MHZ14_Main();

    //Analog processing
#if SENSOR_MANAGER_ANALOGCHANNELS_COUNT > 0
    Sensor_Manager_ProcessAnalog();
#endif

    //Debug info
    DBG_SENSOR("(SensMan)");
    for(i=0;i<SENSOR_MANAGER_DS18B20Count;i++)
    {
    	char sign = (SENSOR_MANAGER_DS18B20TempList[i]<0 ? '-':'+');
    	DBG_SENSOR(" (%d)%c%d.%dC",i,Sensor_Manager_sensorChannels[i],sign,abs(SENSOR_MANAGER_DS18B20TempList[i]/10),abs(SENSOR_MANAGER_DS18B20TempList[i]%10));
    	//DBG_SENSOR("(SensMan)(%d)%d- %d.%dC - %x%x%x%x%x%x%x%x\n",i,Sensor_Manager_sensorChannels[i],SENSOR_MANAGER_DS18B20TempList[i]/10,abs(SENSOR_MANAGER_DS18B20TempList[i]%10),Sensor_Manager_sensorIDs[(i*8)+0],Sensor_Manager_sensorIDs[(i*8)+1],Sensor_Manager_sensorIDs[(i*8)+2],Sensor_Manager_sensorIDs[(i*8)+3],Sensor_Manager_sensorIDs[(i*8)+4],Sensor_Manager_sensorIDs[(i*8)+5],Sensor_Manager_sensorIDs[(i*8)+6],Sensor_Manager_sensorIDs[(i*8)+7]);
    }
    DBG_SENSOR("\n");
    DBG_SENSOR("(SensMan)I0:%d I1:%d I2:%d I3:%d  A:%d V:%d M:%d\n",digitalRead(PULSE_INPUT0),digitalRead(PULSE_INPUT1),digitalRead(PULSE_INPUT2),digitalRead(PULSE_INPUT3),APP_PortMon_analogValues[0],APP_PortMon_analogAverage,APP_PortMon_analogMax);


    if(MHZ14_IsValid())
    {
    	DBG_SENSOR("(SensMan)CO2: %d ppm\n",MHZ14_GetMeasurement());
    }
}


void IRAM0 Sensor_Manager_EdgeDetect(void)
{
	uint8 currentLevel;
#if (DIGITAL_PULSE_COUNTERS > 0)
	//Read current pin level
	currentLevel = digitalRead(PULSE_INPUT0);
	//Look for rising edge
	if((currentLevel == 1) && (Sensor_Manager_lastInputLevel[0] ==0))
	{
		prj_DisableInt();
		Sensor_Manager_PulseCounters[0]++;
		prj_EnableInt();
	}
	//Store last level
	Sensor_Manager_lastInputLevel[0] = currentLevel;
#endif
#if (DIGITAL_PULSE_COUNTERS > 1)
	//Read current pin level
	currentLevel = digitalRead(PULSE_INPUT1);
	//Look for rising edge
	if((currentLevel == 1) && (Sensor_Manager_lastInputLevel[1] ==0))
	{
		prj_DisableInt();
		Sensor_Manager_PulseCounters[1]++;
		prj_EnableInt();
	}
	//Store last level
	Sensor_Manager_lastInputLevel[1] = currentLevel;
#endif
#if (DIGITAL_PULSE_COUNTERS > 2)
	//Read current pin level
	currentLevel = digitalRead(PULSE_INPUT2);
	//Look for rising edge
	if((currentLevel == 1) && (Sensor_Manager_lastInputLevel[2] ==0))
	{
		prj_DisableInt();
		Sensor_Manager_PulseCounters[2]++;
		prj_EnableInt();
	}
	//Store last level
	Sensor_Manager_lastInputLevel[2] = currentLevel;
#endif
#if (DIGITAL_PULSE_COUNTERS > 3)
	//Read current pin level
	currentLevel = digitalRead(PULSE_INPUT3);
	//Look for rising edge
	if((currentLevel == 1) && (Sensor_Manager_lastInputLevel[3] ==0))
	{
		prj_DisableInt();
		Sensor_Manager_PulseCounters[3]++;
		prj_EnableInt();
	}
	//Store last level
	Sensor_Manager_lastInputLevel[3] = currentLevel;
#endif
}

uint32 ICACHE_FLASH_ATTR Sensor_Manager_GetPulseCount(uint8 id)
{
	prj_ENTER_CRITICAL();
	uint32 tempValue = Sensor_Manager_PulseCounters[id];
	prj_EXIT_CRITICAL();
	return tempValue;
}

uint32 ICACHE_FLASH_ATTR Sensor_Manager_GetPulseLevel(uint8 id)
{
	uint32_t tempValue = 0;
	switch(id)
	{
#if (SENSOR_MANAGER_PULSE_COUNTERS > 0)
	case 0:
		tempValue = digitalRead(PULSE_INPUT0);
	break;
#endif
#if (SENSOR_MANAGER_PULSE_COUNTERS > 1)
	case 1:
		tempValue = digitalRead(PULSE_INPUT1);
	break;
#endif
#if (SENSOR_MANAGER_PULSE_COUNTERS > 2)
	case 2:
		tempValue = digitalRead(PULSE_INPUT2);
	break;
#endif
#if (SENSOR_MANAGER_PULSE_COUNTERS > 3)
	case 3:
		tempValue = digitalRead(PULSE_INPUT3);
	break;
#endif
#if (SENSOR_MANAGER_PULSE_COUNTERS > 4)
	case 4:
		tempValue = Sensor_Manager_GetAnalogValue();
	break;
#endif
	}
	return tempValue;
}

#if SENSOR_MANAGER_ANALOGCHANNELS_COUNT > 0
uint16 ICACHE_FLASH_ATTR Sensor_Manager_GetAnalogValue(void)
{

	return APP_PortMon_analogValues[0];

}
void ICACHE_FLASH_ATTR Sensor_Manager_ProcessAnalog(void)
{
	uint16 analogMax = 0;
	uint32 analogAverage = 0;
	uint16 i;
	//Get max and average values
	for(i=0; i < ANALOG_WINDOW; i++)
	{
		if(analogMax < APP_PortMon_analogWindow[i])
		{
			analogMax = APP_PortMon_analogWindow[i];
		}
		analogAverage = analogAverage + APP_PortMon_analogWindow[i];
	}
	analogAverage = analogAverage / ANALOG_WINDOW;
	//Update average and max
	APP_PortMon_analogAverage = (uint16)analogAverage;
	APP_PortMon_analogMax = analogMax;
}
#endif
void ICACHE_FLASH_ATTR Sensor_Manager_ResetPulseCounters(void)
{
	uint8 i;
	//Init pulse counters
	for(i=0;i<SENSOR_MANAGER_PULSE_COUNTERS;i++)
	{
		Sensor_Manager_PulseCounters[i] = 0;
		Sensor_Manager_lastInputLevel[i] = Sensor_Manager_GetPulseLevel(i);
	}
}

void ICACHE_FLASH_ATTR Sensor_Manager_UpdateSensors(void) {
    uint16 i;
    uint8 checkOk = OK;
    /*Search for sensors*/
    uint8 Sensor_Manager_Count = SENSOR_MANAGER_DS18B20_Search();
    if (Sensor_Manager_Count > SENSOR_MANAGER_DS18B20Count) {
        //More found so register new sensors
        Sensor_Manager_RetryCount = TEMP_MAX_RETRY_COUNT;
    } else if (Sensor_Manager_Count < SENSOR_MANAGER_DS18B20Count) {
        //Less sensor found so retry
        Sensor_Manager_RetryCount++;
    } else {
        //Same sensor count found check if ids are the same
        for (i = 0; i < OWP_CONST_ROMCODE_SIZE*SENSOR_MANAGER_DS18B20Count; i++) {
        	//Check each byte of the ids
        	if(Sensor_Manager_sensorIDs[i] != Sensor_Manager_sensorIDsTEMP[i])
        	{
        		checkOk = FAIL;
        		break;
        	}
        }
        //Check if error found
        if(OK == checkOk)
        {
        	//Clear retry count all is ok
			Sensor_Manager_RetryCount = 0;
			APP_REPORT_PASS();
        }
        else
        {
        	//there is a change in ids report so retry
            Sensor_Manager_RetryCount++;
        }
    }

    //Check if needs to update temp count
    if (Sensor_Manager_RetryCount >= TEMP_MAX_RETRY_COUNT) {
        Sensor_Manager_RetryCount = 0;

        APP_REPORT_FAIL();

        DBG_SENSOR("(SensMan)Sensors found: %d\n", Sensor_Manager_Count);

        //Set new temp count
        SENSOR_MANAGER_DS18B20Count = Sensor_Manager_Count;
        //Copy Ids
        for (i = 0; i < OWP_CONST_ROMCODE_SIZE * SENSOR_MANAGER_DS18B20MAXCOUNT; i++) {
            Sensor_Manager_sensorIDs[i] = Sensor_Manager_sensorIDsTEMP[i];
        }
        //Copy channels
        for (i = 0; i < SENSOR_MANAGER_DS18B20MAXCOUNT; i++) {
            Sensor_Manager_sensorChannels[i] = Sensor_Manager_sensorChannelsTEMP[i];
        }

        //Clear temperatures
        for (i = 0; i < SENSOR_MANAGER_DS18B20MAXCOUNT; i++) {
        	SENSOR_MANAGER_DS18B20TempList[i] = Sensor_Manager_INVALID_TEMP;
        }
        //Change state because of new temp meters
        Sensor_Manager_Change_State(SENSOR_MANAGER_NEW_TEMP_DETECTED);
    }
}

void ICACHE_FLASH_ATTR SENSOR_MANAGER_DS18B20_StartMeasure(void)
{
	uint8 i;
    for (i = 0; i < OWP_CHANNELS_COUNT; i++) {
        //Select OWP channel
        OWP_SelectChannel(i);
        //Broadcast convert message to all temp sensors
        D18_DS18B20_StartMeasure(D18_DS18B20_POWER_EXTERN, 0);
        //Minor delay for TASK YIELD
        DELAY_MS(1);
    }
}

uint8 ICACHE_FLASH_ATTR SENSOR_MANAGER_DS18B20_Search(void) {
    //Local variables
    uint8 i;
    uint8 j;
    uint8 id[OWP_CONST_ROMCODE_SIZE];
    uint8 diff;
    uint8 CS_nSensors = 0;
    uint8 OneWireChannel = 0;

    //Clear temp sensor IDS
    for (i = 0; i < OWP_CONST_ROMCODE_SIZE * SENSOR_MANAGER_DS18B20MAXCOUNT; i++) {
        Sensor_Manager_sensorIDsTEMP[i] = 0;
    }
    //Clear temp sensor channels
    for (i = 0; i < SENSOR_MANAGER_DS18B20MAXCOUNT; i++) {
        Sensor_Manager_sensorChannelsTEMP[i] = 0;
    }

    //Search until not last found or sensor count limit reached
    for (diff = OWP_CONST_SEARCH_FIRST;
            diff != OWP_CONST_LAST_DEVICE && CS_nSensors < SENSOR_MANAGER_DS18B20MAXCOUNT;) {
        
        //Select OWP channel
        OWP_SelectChannel(OneWireChannel);
	//Perform search operation
        D18_DS18B20_FindSensor(&diff, &id[0]);
        //Check if has presence error
        if (diff == OWP_CONST_PRESENCE_ERR) {
            DBG_SENSOR("(SensMan)No sensor found\n");
            //Report comm fail
            APP_REPORT_FAIL();
        }//Check if has data error
        else if (diff == OWP_CONST_DATA_ERR) {
            DBG_SENSOR("(SensMan)Bus error\n");
            //Report comm fail
            APP_REPORT_FAIL();
            //Stop search
            break;
        }//Check if has id error
        else if (id[0] != 0x28) {
            DBG_SENSOR("(SensMan)Id error\n");
            //Report comm fail
            APP_REPORT_FAIL();
            //Stop search
            break;
        }// Check ROM CRC
        else if (CRC_crc8(id, (OWP_CONST_ROMCODE_SIZE - 1), 0) != id[(OWP_CONST_ROMCODE_SIZE - 1)]) {
            DBG_SENSOR("(SensMan)ROM crc error\n");
            //Report comm fail
            APP_REPORT_FAIL();
            //Stop search
            break;
        }

        //Check if has presence error
        if (diff != OWP_CONST_PRESENCE_ERR) {
            //Calculate id offset in temp Ids buffer
            uint16 Sensor_Manager_Offset = CS_nSensors*OWP_CONST_ROMCODE_SIZE;
            //Store found sensor id
            for (j = 0; j < OWP_CONST_ROMCODE_SIZE; j++) {
                Sensor_Manager_sensorIDsTEMP[Sensor_Manager_Offset + j] = id[j];
            }
            //Store found sensor channel
            Sensor_Manager_sensorChannelsTEMP[CS_nSensors] = OneWireChannel;
            DBG_SENSOR2("(SensMan)Sensor found: Ch(%d) Id:(%x%x%x)\n",OneWireChannel,Sensor_Manager_sensorIDsTEMP[0],Sensor_Manager_sensorIDsTEMP[1],Sensor_Manager_sensorIDsTEMP[2]);
            //Increment sensor count
            CS_nSensors++;
        }

        //Check if last sensor found on oneWireChannel
        if ((diff == OWP_CONST_LAST_DEVICE) || (diff == OWP_CONST_PRESENCE_ERR)) {
            //Next channel
            OneWireChannel++;
            //Check if OneWire channel to search on
            if (OneWireChannel < OWP_CHANNELS_COUNT) {
                //Don't stop search
                diff = OWP_CONST_SEARCH_FIRST;
                DBG_SENSOR2("(SensMan)Search on channel: %d\n", OneWireChannel);
            }
            else
            {
                //All channels are searched, stop
                break;
            }
        }
        //Minor delay for TASK YIELD
        DELAY_MS(1);
    }
    //Return found sensors count
    return CS_nSensors;
}

void ICACHE_FLASH_ATTR SENSOR_MANAGER_DS18B20Measure() {
    //Local variables
    uint8 subzero, cel, cel_frac_bits, i;

    //Loop all available sensors
    for (i = 0; i < SENSOR_MANAGER_DS18B20Count; i++) {
        //Get sensor id position
        uint16 Sensor_Manager_Offset = i*OWP_CONST_ROMCODE_SIZE;
        //Select OneWire channel where sensor is
        OWP_SelectChannel(Sensor_Manager_sensorChannels[i]);
        //Get temperature data from sensor
        if (D18_DS18B20_ReadMeasure(&Sensor_Manager_sensorIDs[Sensor_Manager_Offset], &subzero, &cel, &cel_frac_bits) == D18_DS18B20_OK) // gSensorIDsOrdered
        {
            //Store received temperature data
            SENSOR_MANAGER_DS18B20TempList[i] = D18_DS18B20_TemptoDecicel(subzero, cel, cel_frac_bits);
        } else {
            //Report comm fail
        	APP_REPORT_FAIL();
        }
    }
}

void ICACHE_FLASH_ATTR Sensor_Manager_Get_TempSensorData(uint8* count, uint8** ids,sint16** temperatures )
{
	(*count) = SENSOR_MANAGER_DS18B20Count;
	(*ids) = (uint8*) &Sensor_Manager_sensorIDs;
	(*temperatures) = (sint16*) &SENSOR_MANAGER_DS18B20TempList;
}

