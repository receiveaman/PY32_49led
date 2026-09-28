#ifndef LED_MATRIX_H
#define LED_MATRIX_H

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * L3 应用层
 * 49 灯显存 + 时分扫描。
 * 使用方式：
 *   led_matrix_init();
 *   led_matrix_set(0, 0, true);   // 只改显存
 *   led_matrix_scan();            // 主循环里尽量频繁调用，负责真正驱动引脚
 *
 * 逐点扫描：任意时刻最多点亮 1 颗灯，点亮颗数越多单颗占空比越低。
 */
#define LED_MATRIX_ROWS 7
#define LED_MATRIX_COLS 7
#define LED_MATRIX_COUNT (LED_MATRIX_ROWS * LED_MATRIX_COLS)

/* 初始化引脚层并清空显存 */
void led_matrix_init(void);

/* 显存全灭 + 引脚高阻 */
void led_matrix_clear(void);

/* 显存全亮 / 全灭（是否真的点亮由 scan 决定） */
void led_matrix_fill(bool on);

/* 按行列写显存，越界调用被忽略 */
void led_matrix_set(uint8_t row, uint8_t col, bool on);
bool led_matrix_get(uint8_t row, uint8_t col);

/* 按线性序号写显存（0..48 = 行优先），越界调用被忽略 */
void led_matrix_set_index(uint16_t index, bool on);
bool led_matrix_get_index(uint16_t index);

/* 当前显存里点亮的灯数 */
uint16_t led_matrix_on_count(void);

/* 扫描占空比亮度 0~100（%），默认 100，越界按边界截断（RS485_Protocol_Spec §11.3-4） */
void led_matrix_set_brightness(uint8_t percent);
uint8_t led_matrix_get_brightness(void);

/* 时分扫描一拍：点亮下一颗需要亮的灯，全灭时释放引脚 */
void led_matrix_scan(void);

#ifdef __cplusplus
}
#endif

#endif /* LED_MATRIX_H */
