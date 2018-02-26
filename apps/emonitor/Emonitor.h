#ifndef __EMONITOR_H__
#define __EMONITOR_H__


#include "c_types.h"

/******************************************************************************
* Typedefs
\******************************************************************************/

typedef enum
{
	EMONITOR_NOT_CONNECTED =0,
	EMONITOR_CONNECTED =1
}Emonitor_Connection_Status;

typedef enum
{
	EMONITOR_REQ_NONE = 0,
	EMONITOR_REQ_HOTSPOT = 1,
	EMONITOR_REQ_CLEAR = 2,
	EMONITOR_REQ_RESTART = 3,
	EMONITOR_REQ_SAVE = 4

}Emonitor_Request;

/******************************************************************************
* Prototypes
\******************************************************************************/

extern void Emonitor_Preinit(void);
extern void Emonitor_Init(void);
extern void Emonitor_Main_1ms(void);
extern void Emonitor_Main_1000ms(void);
extern void Emonitor_Main_Background(void);
extern void Emonitor_StartTimer(void);
extern uint32_t Emonitor_RestoreTiming(void);
extern void Emonitor_StoreTiming(uint32_t timingValue);
extern void Emonitor_EnableStatusLed(void);

extern uint32 Emonitor_nodeId;
extern uint32 Emonitor_SendPeroid;
extern char Emonitor_url[100];
extern char Emonitor_key[33];
extern Emonitor_Request Emonitor_requestState;
extern uint32 Emonitor_uptime;
extern uint32 Emonitor_timing;
extern sint32 Emonitor_connectionCounter;
extern uint32 Emonitor_freeRam;
extern uint32 Emonitor_counterMirror;
extern uint32 Emonitor_resetReason;

/******************************************************************************
* Defines
\******************************************************************************/

#define Emonitor_SetResetReason(a) (Emonitor_resetReason = a)
#define Emonitor_GetResetReason() (Emonitor_resetReason)
#define Emonitor_GetUptime()	(Emonitor_uptime)
#define Emonitor_GetSendTiming()	(Emonitor_timing)
#define Emonitor_GetConnectionCounter()	(Emonitor_connectionCounter)
#define Emonitor_GetFreeRam()	(Emonitor_freeRam)
#define Emonitor_GetBackgroundCount()	(Emonitor_counterMirror)

#define Emonitor_GetNodeId()  (Emonitor_nodeId)
#define Emonitor_SetNodeId(a)  (Emonitor_nodeId = a)
#define Emonitor_GetSendPeriod()  (Emonitor_SendPeroid)
#define Emonitor_SetSendPeriod(a)  (Emonitor_SendPeroid = a)

#define Emonitor_GetUrl()	(Emonitor_url)
#define Emonitor_SetUrl(a)	Emonitor_url[sprintf(Emonitor_url,"%s",a)]=0;
#define Emonitor_GetKey()	(Emonitor_key)
#define Emonitor_SetKey(a)	Emonitor_key[sprintf(Emonitor_key,"%s",a)]=0;

#define Emonitor_Request(a)	Emonitor_requestState = a

#endif

