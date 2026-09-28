#ifndef LED_EFFECT_H
#define LED_EFFECT_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * L4 效果层
 * 在显存上播放动画，不直接碰引脚。
 * 用法：
 *   led_effect_init();
 *   while (1) { led_effect_update(); led_matrix_scan(); }
 * 演示模式下每个效果停留 LED_FX_DWELL_MS 后自动切到下一个；
 * 外部（RS485）可以用 led_effect_set() 定死某个效果。
 */
typedef enum {
    LED_FX_NONE = 0,    /* 全灭 */
    LED_FX_ALL_ON,      /* 全亮 */
    LED_FX_BLINK,       /* 1Hz 全亮/全灭 */
    LED_FX_WALK,        /* 逐颗点亮 0 -> 48 */
    LED_FX_ROW_SCAN,    /* 逐行下扫 */
    LED_FX_COL_SCAN,    /* 逐列右扫 */
    LED_FX_DIAGONAL,    /* 对角线扫 */
    LED_FX_BORDER,      /* 外框顺时针跑马 */
    LED_FX_RIPPLE,      /* 中心波纹扩散 */
    LED_FX_CHECKER,     /* 棋盘交替 */
    LED_FX_RANDOM,     /* 随机雪花 */
    LED_FX_REMOTE,     /* RS485 受控：不重画显存，也不参与演示自动轮播 */
    LED_FX_COUNT
} led_effect_t;

/* 单个效果的停留时间（演示模式自动切换用） */
#define LED_FX_DWELL_MS 5000u

void led_effect_init(void);
void led_effect_set(led_effect_t fx);
led_effect_t led_effect_get(void);
void led_effect_next(void);

/* 非阻塞，主循环里频繁调用 */
void led_effect_update(void);

#ifdef __cplusplus
}
#endif

#endif /* LED_EFFECT_H */
