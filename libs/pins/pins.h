#ifndef PINS_H_
#define PINS_H_

#include "c_types.h"
#include "stdint.h"

#define D0  16
#define D1	5
#define D2	4
#define D3	0
#define D4	2
#define D5	14
#define D6	12
#define D7	13
#define D8	15
#define D9	3
#define D10 1

#define OUTPUT 0
#define INPUT 1
#define INPUT_PULLUP 2
#define OUTPUT_OPEN_DRAIN 3
#define RISING 0
#define FALLING 1
#define CHANGE 2

typedef void (*voidFuncPtr)(void);

extern void Init_Pins(void);

extern void pinMode(uint8 pin, uint8 mode);
extern void digitalWrite(uint8 pin, uint8 val);
extern int  digitalRead(uint8 pin);
extern void analogWrite(uint8 pin, int val);
extern void attachInterrupt(uint8 pin, voidFuncPtr handler, int mode);
extern void detachInterrupt(uint8 pin);
extern void pins_pwm_init(uint32 pin_number,uint32 period, uint32 duty);

#endif
