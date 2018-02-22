/*
 * MHZ14.h
 */

#ifndef MHZ14_H_
#define MHZ14_H_

#define MHZ14_INVALID_VALUE			10000
#define MHZ14_MIN_VALID_TIME		100

#define MHZ14_ENTER_CRITICAL		taskENTER_CRITICAL
#define MHZ14_EXIT_CRITICAL			taskEXIT_CRITICAL

extern void MHZ14_Init(void);
extern void MHZ14_Main(void);
extern void MHZ14_Feed(uint8 level);
extern uint32 MHZ14_GetMeasurement(void);

#define MHZ14_IsValid()	(MHZ14_GetMeasurement()!=MHZ14_INVALID_VALUE)


#endif
