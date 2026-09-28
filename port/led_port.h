#ifndef LED_PORT_H
#define LED_PORT_H

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * L1 引脚层
 * 只负责 8 根查理复用引脚的电平 / 方向，不关心"灯"的概念。
 * 引脚编号与 led_port.c 中的硬件表一一对应。
 */
typedef enum {
    LED_PIN_PA0 = 0,
    LED_PIN_PA1 = 1,
    LED_PIN_PA3 = 2,
    LED_PIN_PA4 = 3,
    LED_PIN_PA5 = 4,
    LED_PIN_PB0 = 5,
    LED_PIN_PB1 = 6,
    LED_PIN_PB2 = 7,
    LED_PIN_COUNT = 8,
} led_pin_t;

/* 时钟 + 8 根脚初始化为高阻（输入、无上下拉），全部灯熄灭 */
void led_port_init(void);

/* 8 根脚全部置高阻 —— 查理复用的"全灭"状态 */
void led_port_release(void);

/* 只写输出寄存器，不改方向（可在输入态下预置电平） */
void led_port_level(led_pin_t pin, bool high);

/* 切换方向：true = 推挽输出，false = 高阻输入 */
void led_port_dir(led_pin_t pin, bool output);

/* 点亮一对脚：anode 输出高、cathode 输出低，其余 6 根脚高阻 */
void led_port_drive(led_pin_t anode, led_pin_t cathode);

#ifdef __cplusplus
}
#endif

#endif /* LED_PORT_H */
