#ifndef LED_MAP_H
#define LED_MAP_H

#include <stdint.h>
#include "led_port.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * L2 映射层
 * 逻辑灯位 (row, col) -> (阳极脚, 阴极脚)。
 * 行列顺序与 n49-查理复用IO映射标.xlsx 的左表完全一致：
 *   行 1..7 对应表中第 1..7 行，列 1..7 对应表中第 1..7 列。
 *   (row, col) 点亮条件 = anode 输出高、cathode 输出低。
 *
 * 实物几何（2026-09-27 实测）：
 *   表(行1,列1) = 板子左上角，行 1->7 向下，列 1->7 向右，行优先，无 Z 字折返。
 *   与表方向一致，故下面两个开关默认为 0；面板若装反/旋转才需要改，不必动表。
 */
#define LED_MAP_ROW_REVERSE 0
#define LED_MAP_COL_REVERSE 0
#define LED_MAP_ROWS 7
#define LED_MAP_COLS 7

typedef struct {
    uint8_t anode;   /* led_pin_t */
    uint8_t cathode; /* led_pin_t */
} led_map_entry_t;

/* 按行列取映射，越界返回 NULL */
const led_map_entry_t *led_map_get(uint8_t row, uint8_t col);

/* 按线性序号取映射（index = row * LED_MAP_COLS + col），越界返回 NULL */
const led_map_entry_t *led_map_at(uint16_t index);

#ifdef __cplusplus
}
#endif

#endif /* LED_MAP_H */
