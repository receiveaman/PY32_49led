#include "led_port.h"
#include "py32f002b_ll_bus.h"
#include "py32f002b_ll_gpio.h"

typedef struct {
    GPIO_TypeDef *port;
    uint32_t pin;
} led_pin_hw_t;

/* 引脚编号 -> 硬件寄存器 */
static const led_pin_hw_t s_pin_hw[LED_PIN_COUNT] = {
    {GPIOA, LL_GPIO_PIN_0}, /* PA0 */
    {GPIOA, LL_GPIO_PIN_1}, /* PA1 */
    {GPIOA, LL_GPIO_PIN_3}, /* PA3 */
    {GPIOA, LL_GPIO_PIN_4}, /* PA4 */
    {GPIOA, LL_GPIO_PIN_5}, /* PA5 */
    {GPIOB, LL_GPIO_PIN_0}, /* PB0 */
    {GPIOB, LL_GPIO_PIN_1}, /* PB1 */
    {GPIOB, LL_GPIO_PIN_2}, /* PB2 */
};

void led_port_init(void)
{
    LL_IOP_GRP1_EnableClock(LL_IOP_GRP1_PERIPH_GPIOA | LL_IOP_GRP1_PERIPH_GPIOB);

    for (uint8_t i = 0; i < LED_PIN_COUNT; i++) {
        LL_GPIO_SetPinOutputType(s_pin_hw[i].port, s_pin_hw[i].pin, LL_GPIO_OUTPUT_PUSHPULL);
        LL_GPIO_SetPinSpeed(s_pin_hw[i].port, s_pin_hw[i].pin, LL_GPIO_SPEED_FREQ_LOW);
        LL_GPIO_SetPinPull(s_pin_hw[i].port, s_pin_hw[i].pin, LL_GPIO_PULL_NO);
        LL_GPIO_SetPinMode(s_pin_hw[i].port, s_pin_hw[i].pin, LL_GPIO_MODE_INPUT);
    }
}

void led_port_release(void)
{
    for (uint8_t i = 0; i < LED_PIN_COUNT; i++) {
        LL_GPIO_SetPinMode(s_pin_hw[i].port, s_pin_hw[i].pin, LL_GPIO_MODE_INPUT);
    }
}

void led_port_level(led_pin_t pin, bool high)
{
    if (pin >= LED_PIN_COUNT) {
        return;
    }
    if (high) {
        LL_GPIO_SetOutputPin(s_pin_hw[pin].port, s_pin_hw[pin].pin);
    } else {
        LL_GPIO_ResetOutputPin(s_pin_hw[pin].port, s_pin_hw[pin].pin);
    }
}

void led_port_dir(led_pin_t pin, bool output)
{
    if (pin >= LED_PIN_COUNT) {
        return;
    }
    LL_GPIO_SetPinMode(s_pin_hw[pin].port, s_pin_hw[pin].pin,
                       output ? LL_GPIO_MODE_OUTPUT : LL_GPIO_MODE_INPUT);
}

void led_port_drive(led_pin_t anode, led_pin_t cathode)
{
    if (anode >= LED_PIN_COUNT || cathode >= LED_PIN_COUNT || anode == cathode) {
        led_port_release();
        return;
    }

    /* 先全高阻，避免切换过程中点亮错误的灯 */
    led_port_release();
    /* 此时脚仍是输入，写 ODR 不产生输出，先把电平摆好 */
    led_port_level(anode, true);
    led_port_level(cathode, false);
    /* 最后才开输出 */
    led_port_dir(anode, true);
    led_port_dir(cathode, true);
}
