/*
 * APP_Sensor_Manager.c
 *
 *  Created on: Aug 13, 2012
 *      Author: Jooo
 */

/**********************************************************************************
 * Includes
 **********************************************************************************/
#include "Sensor_Manager.h"
#include "D18_DS18B20_Temp_Sensor.h"
#include "OWP_One_Wire_Protocol_Driver.h"
#include "CRC_crc8.h"


/**********************************************************************************
 * Defines
 **********************************************************************************/
//#define APP_VERBOSE
//#define APP_VERBOSE_PIC32

#define APP_ENTER_CRITICAL()   //non preemtive task
#define APP_EXIT_CRITICAL()    //non preemtive task

#define APP_REPORT_FAIL()
#define APP_REPORT_PASS()

#define APP_REPORT_COMM_FAIL()
#define APP_REPORT_COMM_PASS()

#define Sensor_Manager_RESCAN_PERIOD 2   //10
#define Sensor_Manager_INVALID_TEMP 0
#define Sensor_Manager_MAX_RETRY_COUNT 2   //6

//#define DBG_SENSOR(...) printf(__VA_ARGS__)
#define DBG_SENSOR(...)

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

uint16 APP_PortMon_analogValues[SENSOR_MANAGER_ANALOGCHANNELS_COUNT];

uint32 Sensor_Manager_PulseCounters[SENSOR_MANAGER_PULSE_COUNTERS] = {};

/**********************************************************************************
 * Function defines
 **********************************************************************************/
uint8 SENSOR_MANAGER_DS18B20_Search(void);
void SENSOR_MANAGER_DS18B20Measure(void);
void Sensor_Manager_UpdateSensors(void);
void Sensor_Manager_PulseCounter0(void);
/**********************************************************************************
 * Functions
 **********************************************************************************/

void Sensor_Manager_Init() {
	//Pulse counter input
	pinMode(PULSE_INPUT0,INPUT);
	attachInterrupt(PULSE_INPUT0,Sensor_Manager_PulseCounter0,RISING);
	//Init pulse counters
	Sensor_Manager_ResetPulseCounters();

	//Init temp sensors
    /*Load default invalid temp values*/
    uint8 CS_i;
    for (CS_i = 0; CS_i < SENSOR_MANAGER_DS18B20MAXCOUNT; CS_i++) {
        //Set temp sensor init value
        SENSOR_MANAGER_DS18B20TempList[CS_i] = Sensor_Manager_INVALID_TEMP;
    }
    //Search for sensors
    Sensor_Manager_UpdateSensors();
}

void Sensor_Manager_Fast() {
    //Local variables
    uint8 CS_i;

    //Scan Adc channels
    for (CS_i = 0; CS_i < SENSOR_MANAGER_ANALOGCHANNELS_COUNT; CS_i++) {
        //Read Adc channel and store it in buffer
        APP_PortMon_analogValues[CS_i] = 0;//ADC_ReadChannel(CS_i);
    }
}

void Sensor_Manager_Main() {
	uint8 i;
    /*Read DS18B20 sensors*/
    SENSOR_MANAGER_DS18B20Measure();

    //Temp sensor rescan timing
    Sensor_Manager_Timing = (Sensor_Manager_Timing + 1) % Sensor_Manager_RESCAN_PERIOD;
    //Is it time to scan
    if (Sensor_Manager_Timing == 0) {
        //Search for sensors again
        Sensor_Manager_UpdateSensors();
    }
    //Report comm pass
    APP_REPORT_COMM_PASS();
    DBG_SENSOR("(SensMan)Main\n");
    for(i=0;i<SENSOR_MANAGER_DS18B20Count;i++)
    {
    	DBG_SENSOR("(SensMan)%d- %d.%dC - %x%x%x%x%x%x%x%x\n",i,SENSOR_MANAGER_DS18B20TempList[i]/10,SENSOR_MANAGER_DS18B20TempList[i]%10,Sensor_Manager_sensorIDs[(i*8)+0],Sensor_Manager_sensorIDs[(i*8)+1],Sensor_Manager_sensorIDs[(i*8)+2],Sensor_Manager_sensorIDs[(i*8)+3],Sensor_Manager_sensorIDs[(i*8)+4],Sensor_Manager_sensorIDs[(i*8)+5],Sensor_Manager_sensorIDs[(i*8)+6],Sensor_Manager_sensorIDs[(i*8)+7]);
    }

}

void Sensor_Manager_PulseCounter0(void)
{
	Sensor_Manager_PulseCounters[0]++;
}

uint32 Sensor_Manager_GetPulseCount(uint8 id)
{
	taskENTER_CRITICAL();
	uint32 tempValue = Sensor_Manager_PulseCounters[id];
	taskEXIT_CRITICAL();
	return tempValue;
}

void Sensor_Manager_ResetPulseCounters(void)
{
	uint8 i;
	//Init pulse counters
	for(i=0;i<SENSOR_MANAGER_PULSE_COUNTERS;i++)
	{
		Sensor_Manager_PulseCounters[i] = 0;
	}
}

void Sensor_Manager_UpdateSensors(void) {
    uint16 i;
    /*Search for sensors*/
    uint8 Sensor_Manager_Count = SENSOR_MANAGER_DS18B20_Search();
    if (Sensor_Manager_Count > SENSOR_MANAGER_DS18B20Count) {
        //More found so register new sensors
        Sensor_Manager_RetryCount = Sensor_Manager_MAX_RETRY_COUNT;
    } else if (Sensor_Manager_Count < SENSOR_MANAGER_DS18B20Count) {
        //Less sensor found so retry
        Sensor_Manager_RetryCount++;
    } else {
        //Same sensor count found OK
        Sensor_Manager_RetryCount = 0;
        APP_REPORT_PASS();
    }

    //Check if needs to update temp count
    if (Sensor_Manager_RetryCount >= Sensor_Manager_MAX_RETRY_COUNT) {
        Sensor_Manager_RetryCount = 0;

        APP_REPORT_FAIL();

        DBG_SENSOR("(SensMan)Sensors found: %d\r\n", Sensor_Manager_Count);

        //Block preemption
        //APP_ENTER_CRITICAL();
        //Block all temp data whil still preparing

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
        //Enable preemption
        //APP_EXIT_CRITICAL();

        /*Init sensors*/
            //Broadcast convert message to all temp sensors
            D18_DS18B20_StartMeasure(D18_DS18B20_POWER_EXTERN, 0);

        vTaskDelay(500 / portTICK_RATE_MS);
        //Get temperature  values
        SENSOR_MANAGER_DS18B20Measure();

    }
}

uint8 SENSOR_MANAGER_DS18B20_Search(void) {
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
        //Perform search operation
        D18_DS18B20_FindSensor(&diff, &id[0]);
        //Check if has presence error
        if (diff == OWP_CONST_PRESENCE_ERR) {
            DBG_SENSOR("(SensMan)No sensor found\r\n");
            //Report comm fail
            APP_REPORT_COMM_FAIL();
        }//Check if has data error
        else if (diff == OWP_CONST_DATA_ERR) {
            DBG_SENSOR("(SensMan)Bus error\r\n");
            //Report comm fail
            APP_REPORT_COMM_FAIL();
            //Stop search
            break;
        }//Check if has id error
        else if (id[0] != 0x28) {
            DBG_SENSOR("(SensMan)Id error\r\n");
            //Report comm fail
            APP_REPORT_COMM_FAIL();
            //Stop search
            break;
        }// Check ROM CRC
        else if (CRC_crc8(id, (OWP_CONST_ROMCODE_SIZE - 1), 0) != id[(OWP_CONST_ROMCODE_SIZE - 1)]) {
            DBG_SENSOR("(SensMan)ROM crc error\r\n");
            //Report comm fail
            APP_REPORT_COMM_FAIL();
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
            Sensor_Manager_sensorChannelsTEMP[CS_nSensors] = 0;
            //Increment sensor count
            CS_nSensors++;
        }

        //Check if last sensor found on oneWireChannel
        if ((diff == OWP_CONST_LAST_DEVICE) || (diff == OWP_CONST_PRESENCE_ERR)) {
                //All channels are searched, stop
                break;
        }
    }
    //Return found sensors count
    return CS_nSensors;
}

void SENSOR_MANAGER_DS18B20Measure() {
    //Local variables
    uint8 subzero, cel, cel_frac_bits, i;

        //Broadcast convert message to all temp sensors
        D18_DS18B20_StartMeasure(D18_DS18B20_POWER_EXTERN, 0);

    //Loop all available sensors
    for (i = 0; i < SENSOR_MANAGER_DS18B20Count; i++) {
        //Get sensor id position
        uint16 Sensor_Manager_Offset = i*OWP_CONST_ROMCODE_SIZE;
        //Get temperature data from sensor
        if (D18_DS18B20_ReadMeasure(&Sensor_Manager_sensorIDs[Sensor_Manager_Offset], &subzero, &cel, &cel_frac_bits) == D18_DS18B20_OK) // gSensorIDsOrdered
        {
            //Store received temperature data
            SENSOR_MANAGER_DS18B20TempList[i] = D18_DS18B20_TemptoDecicel(subzero, cel, cel_frac_bits);
        } else {
            //Report comm fail
            APP_REPORT_COMM_FAIL();
        }
    }
}

void Sensor_Manager_Get_TempSensorData(uint8* count, uint8** ids,sint16** temperatures )
{
	(*count) = SENSOR_MANAGER_DS18B20Count;
	(*ids) = (uint8*) &Sensor_Manager_sensorIDs;
	(*temperatures) = (sint16*) &SENSOR_MANAGER_DS18B20TempList;
}

