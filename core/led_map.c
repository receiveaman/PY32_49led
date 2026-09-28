#include <stddef.h>
#include "led_map.h"

/**
 * 由 n49-查理复用IO映射标.xlsx 左表生成（"引脚翻转 / 二极管不翻转"）。
 * 引脚编号：PA0=0 PA1=1 PA3=2 PA4=3 PA5=4 PB0=5 PB1=6 PB2=7
 * 每格 { anode, cathode }：anode 输出高 + cathode 输出低 时该灯点亮。
 */
static const led_map_entry_t s_map[LED_MAP_ROWS][LED_MAP_COLS] = {
    {{7, 6}, {6, 5}, {5, 0}, {0, 1}, {1, 2}, {2, 3}, {3, 4}},
    {{6, 7}, {5, 6}, {0, 5}, {1, 0}, {2, 1}, {3, 2}, {4, 3}},
    {{4, 6}, {7, 5}, {6, 0}, {5, 1}, {0, 2}, {1, 3}, {2, 4}},
    {{6, 4}, {5, 7}, {0, 6}, {1, 5}, {2, 0}, {3, 1}, {4, 2}},
    {{3, 6}, {4, 7}, {7, 0}, {6, 1}, {5, 2}, {0, 3}, {1, 4}},
    {{6, 3}, {7, 4}, {0, 7}, {1, 6}, {2, 5}, {3, 0}, {4, 1}},
    {{3, 7}, {2, 6}, {4, 5}, {4, 0}, {7, 1}, {7, 2}, {7, 3}},
};

static inline uint8_t map_row(uint8_t row)
{
    return LED_MAP_ROW_REVERSE ? (uint8_t)(LED_MAP_ROWS - 1 - row) : row;
}

static inline uint8_t map_col(uint8_t col)
{
    return LED_MAP_COL_REVERSE ? (uint8_t)(LED_MAP_COLS - 1 - col) : col;
}

const led_map_entry_t *led_map_get(uint8_t row, uint8_t col)
{
    if (row >= LED_MAP_ROWS || col >= LED_MAP_COLS) {
        return NULL;
    }
    return &s_map[map_row(row)][map_col(col)];
}

const led_map_entry_t *led_map_at(uint16_t index)
{
    if (index >= (uint16_t)(LED_MAP_ROWS * LED_MAP_COLS)) {
        return NULL;
    }
    return &s_map[map_row((uint8_t)(index / LED_MAP_COLS))]
                 [map_col((uint8_t)(index % LED_MAP_COLS))];
}
