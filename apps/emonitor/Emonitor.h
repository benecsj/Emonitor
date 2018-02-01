#ifndef __EMONITOR_H__
#define __EMONITOR_H__


#include "c_types.h"

/******************************************************************************
* Prototypes
\******************************************************************************/

extern void Emonitor_Preinit(void);
extern void Emonitor_Init(void);
extern void Emonitor_Main_1ms(void);
extern void Emonitor_Main_1000ms(void);
extern void Emonitor_Main_Background(void);


extern uint32 Emonitor_nodeId;
extern char Emonitor_url[100];
extern char Emonitor_key[33];
extern uint8 Emonitor_requestState;

#define Emonitor_GetNodeId()  (Emonitor_nodeId)
#define Emonitor_SetNodeId(a)  (Emonitor_nodeId = a)
#define Emonitor_GetUrl()	(Emonitor_url)
#define Emonitor_SetUrl(a)	Emonitor_url[sprintf(Emonitor_url,"%s",a)]=0;
#define Emonitor_GetKey()	(Emonitor_key)
#define Emonitor_SetKey(a)	Emonitor_key[sprintf(Emonitor_key,"%s",a)]=0;

#define Emonitor_Request(a)	Emonitor_requestState = a;

#endif

