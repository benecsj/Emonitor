#ifndef PINS_H_
#define PINS_H_

#include "project_config.h"
#include "c_types.h"
#include "stdint.h"

/******************************************************************************
* Defines
\******************************************************************************/

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

#define GPIO_PIN_ADDR(i)        (GPIO_PIN0_ADDRESS + i*4)

/******************************************************************************
* Typedefs
\******************************************************************************/
#if PRJ_ENV == OS
typedef enum {
    GPIO_PIN_INTR_DISABLE = 0,      /**< disable GPIO interrupt */
    GPIO_PIN_INTR_POSEDGE = 1,      /**< GPIO interrupt type : rising edge */
    GPIO_PIN_INTR_NEGEDGE = 2,      /**< GPIO interrupt type : falling edge */
    GPIO_PIN_INTR_ANYEDGE = 3,      /**< GPIO interrupt type : bothe rising and falling edge */
    GPIO_PIN_INTR_LOLEVEL = 4,      /**< GPIO interrupt type : low level */
    GPIO_PIN_INTR_HILEVEL = 5       /**< GPIO interrupt type : high level */
} GPIO_INT_TYPE;
#endif
typedef void (*voidFuncPtr)(void);

/******************************************************************************
* Primitives
\******************************************************************************/

extern void Init_Pins(void);

extern void pinMode(uint8 pin, uint8 mode);
extern void digitalWrite(uint8 pin, uint8 val);
extern int  digitalRead(uint8 pin);
extern void analogWrite(uint8 pin, int val);
extern void attachInterrupt(uint8 pin, voidFuncPtr handler, int mode);
extern void detachInterrupt(uint8 pin);
extern void pins_pwm_init(uint32 pin_number,uint32 period, uint32 duty);
extern void pins_intr_state_set(uint32 i, GPIO_INT_TYPE intr_state);
extern void pins_intr_handler_register(void *fn, void *arg);


#endif
