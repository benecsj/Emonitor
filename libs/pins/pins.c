#include "project_config.h"

#include "pins.h"
#include "pwm.h"

#define MODIFY_PERI_REG(reg, mask, val) WRITE_PERI_REG(reg, (READ_PERI_REG(reg) & (~mask)) | (uint32) val)

#define PINCOUNT 16

static const uint32 g_pin_muxes[PINCOUNT] = {
    [0] = PERIPHS_IO_MUX_GPIO0_U,
    [1] = PERIPHS_IO_MUX_U0TXD_U,
    [2] = PERIPHS_IO_MUX_GPIO2_U,
    [3] = PERIPHS_IO_MUX_U0RXD_U,
    [4] = PERIPHS_IO_MUX_GPIO4_U,
    [5] = PERIPHS_IO_MUX_GPIO5_U,

    // These 6 pins are used for SPI flash interface
    [6] = 0,
    [7] = 0,
    [8] = 0,
    [9] = 0,
    [10] = 0,
    [11] = 0,

    [12] = PERIPHS_IO_MUX_MTDI_U,
    [13] = PERIPHS_IO_MUX_MTCK_U,
    [14] = PERIPHS_IO_MUX_MTMS_U,
    [15] = PERIPHS_IO_MUX_MTDO_U,
};

static const uint32 g_pin_funcs[PINCOUNT] = {
    [0] = FUNC_GPIO0,
    [1] = FUNC_GPIO1,
    [2] = FUNC_GPIO2,
    [3] = FUNC_GPIO3,
    [4] = FUNC_GPIO4,
    [5] = FUNC_GPIO5,

    [6] = 0,
    [7] = 0,
    [8] = 0,
    [9] = 0,
    [10] = 0,
    [11] = 0,

    [12] = FUNC_GPIO12,
    [13] = FUNC_GPIO13,
    [14] = FUNC_GPIO14,
    [15] = FUNC_GPIO15,
};


uint32 digitalPinToPort(uint32 pin)
{
    return 0;
}

uint32 digitalPinToBitMask(uint32 pin)
{
    return 1 << pin;
}

volatile uint32* portOutputRegister(uint32 port)
{
    return (volatile uint32*) (PERIPHS_GPIO_BASEADDR + GPIO_OUT_ADDRESS);
}

volatile uint32* portInputRegister(uint32 port)
{
    return (volatile uint32*) (PERIPHS_GPIO_BASEADDR + GPIO_IN_ADDRESS);
}

volatile uint32* portModeRegister(uint32 port)
{
    return (volatile uint32*) (PERIPHS_GPIO_BASEADDR + GPIO_ENABLE_ADDRESS);
}


enum PinFunction { GPIO, PWM };
static uint32 g_gpio_function[PINCOUNT] = {
    GPIO
};

extern void pinMode(uint8 pin, uint8 mode)
{
    if (pin == 16)
    {
        uint32 val = (mode == OUTPUT) ? 1 : 0;
        
        MODIFY_PERI_REG(PAD_XPD_DCDC_CONF, 0x43, 1);
        MODIFY_PERI_REG(RTC_GPIO_CONF, 1, 0);
        MODIFY_PERI_REG(RTC_GPIO_ENABLE, 1, val);
        return;
    }

    uint32 mux = g_pin_muxes[pin];
    uint32 gpio =  g_pin_funcs[pin];
	PIN_FUNC_SELECT(mux,gpio);

    if (mode == INPUT)
    {
        gpio_output_set(0, 0, 0, 1 << pin);
        PIN_PULLUP_DIS(mux);
    }
    else if (mode == INPUT_PULLUP)
    {
        gpio_output_set(0, 0, 0, 1 << pin);
        PIN_PULLUP_EN(mux);
    }
    else if (mode == OUTPUT)
    {
        gpio_output_set(0, 0, 1 << pin, 0);
    }
    else if (mode == OUTPUT_OPEN_DRAIN)
    {
        GPIO_REG_WRITE(
            GPIO_PIN_ADDR(GPIO_ID_PIN(pin)), 
            GPIO_REG_READ(GPIO_PIN_ADDR(GPIO_ID_PIN(pin))) | 
            GPIO_PIN_PAD_DRIVER_SET(GPIO_PAD_DRIVER_ENABLE)
        );

        GPIO_REG_WRITE(GPIO_ENABLE_ADDRESS, GPIO_REG_READ(GPIO_ENABLE_ADDRESS) | (1 << pin));
    }
}

extern void digitalWrite(uint8 pin, uint8 val)
{
    if (pin == 16) 
    {
        MODIFY_PERI_REG(RTC_GPIO_OUT, 1, (val & 1));
        return;
    }

    uint32 mask = 1 << pin;
    if (val)
      GPIO_REG_WRITE(GPIO_OUT_W1TS_ADDRESS, mask);
    else
      GPIO_REG_WRITE(GPIO_OUT_W1TC_ADDRESS, mask);
}

extern int digitalRead(uint8 pin)
{
    if (pin == 16)
        return (READ_PERI_REG(RTC_GPIO_IN_DATA) & 1);
    else
        return ((gpio_input_get() >> pin) & 1);
}

extern void analogWrite(uint8 pin, int val)
{
}


static voidFuncPtr g_handlers[PINCOUNT] = { 0 };


void interrupt_handler(void *arg)
{
	int pin;
    uint32 intr_mask = GPIO_REG_READ(GPIO_STATUS_ADDRESS);
    for (pin = 0; intr_mask; intr_mask >>= 1, ++pin)
    {
        if ((intr_mask & 1) && g_handlers[pin])
        {
            GPIO_REG_WRITE(GPIO_STATUS_W1TC_ADDRESS, 1 << pin);
            (*g_handlers[pin])();
        }
    }
}

extern void attachInterrupt(uint8 pin, voidFuncPtr handler, int mode)
{
    if (pin < 0 || pin > PINCOUNT)
        return;

    g_handlers[pin] = handler;
    
    if (mode == RISING)
    {
        pins_intr_state_set(pin, GPIO_PIN_INTR_POSEDGE);
    }
    else if (mode == FALLING)
    {
        pins_intr_state_set(pin, GPIO_PIN_INTR_NEGEDGE);
    }
    else if (mode == CHANGE)
    {
        pins_intr_state_set(pin, GPIO_PIN_INTR_ANYEDGE);
    }
    else
    {
        pins_intr_state_set(pin, GPIO_PIN_INTR_DISABLE);
    }
}

extern void detachInterrupt(uint8 pin)
{
    g_handlers[pin] = 0;
    pins_intr_state_set(pin, GPIO_PIN_INTR_DISABLE);
}

void Init_Pins(void)
{
	pins_intr_handler_register(&interrupt_handler, NULL);
	_xt_isr_unmask(1<<ETS_GPIO_INUM);
}

void pins_pwm_init(uint32 pin_number,uint32 period, uint32 duty)
{
	uint32 pwm_info[1][3] = {   {0,0,0}   };
	u32 pwm_duty[1];

	pwm_duty[0] = duty;
	pwm_info[0][0] = g_pin_muxes[pin_number];
	pwm_info[0][1] = 3;
	pwm_info[0][2] = pin_number;

	pwm_init(period, pwm_duty, 1, pwm_info);
	pwm_start();
}

void pins_intr_state_set(uint32 i, GPIO_INT_TYPE intr_state)
{
    uint32 pin_reg;

    prj_ENTER_CRITICAL();

    pin_reg = GPIO_REG_READ(GPIO_PIN_ADDR(i));
    pin_reg &= (~GPIO_PIN_INT_TYPE_MASK);
    pin_reg |= (intr_state << GPIO_PIN_INT_TYPE_LSB);
    GPIO_REG_WRITE(GPIO_PIN_ADDR(i), pin_reg);

    prj_EXIT_CRITICAL();
}

void pins_intr_handler_register(void *fn, void *arg)
{
    _xt_isr_attach(ETS_GPIO_INUM, fn, arg);
}
