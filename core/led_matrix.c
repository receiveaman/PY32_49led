#include <stddef.h>
#include "led_matrix.h"
#include "led_map.h"
#include "led_port.h"

/* 显存：每位对应一列，行优先，共 49 bit 存在 7 字节里 */
static uint8_t s_fb[LED_MATRIX_ROWS];
/* 上一拍驱动的灯，从它往后找下一颗 */
static uint8_t s_slot = LED_MATRIX_COUNT - 1;
/* 亮度：0~100 的扫描占空比门控计数 */
static uint8_t s_brightness = 100u;
static uint8_t s_gate = 0u;

static inline void fb_write(uint16_t index, bool on)
{
    uint8_t row = (uint8_t)(index / LED_MATRIX_COLS);
    uint8_t mask = (uint8_t)(1u << (index % LED_MATRIX_COLS));
    if (on) {
        s_fb[row] |= mask;
    } else {
        s_fb[row] &= (uint8_t)~mask;
    }
}

static inline bool fb_read(uint16_t index)
{
    uint8_t row = (uint8_t)(index / LED_MATRIX_COLS);
    uint8_t mask = (uint8_t)(1u << (index % LED_MATRIX_COLS));
    return (s_fb[row] & mask) != 0;
}

void led_matrix_init(void)
{
    led_port_init();
    led_matrix_clear();
}

void led_matrix_clear(void)
{
    for (uint8_t i = 0; i < LED_MATRIX_ROWS; i++) {
        s_fb[i] = 0;
    }
    led_port_release();
    s_slot = LED_MATRIX_COUNT - 1;
}

void led_matrix_fill(bool on)
{
    for (uint8_t i = 0; i < LED_MATRIX_ROWS; i++) {
        s_fb[i] = on ? 0x7Fu : 0x00u;
    }
}

void led_matrix_set(uint8_t row, uint8_t col, bool on)
{
    if (row >= LED_MATRIX_ROWS || col >= LED_MATRIX_COLS) {
        return;
    }
    fb_write((uint16_t)(row * LED_MATRIX_COLS + col), on);
}

bool led_matrix_get(uint8_t row, uint8_t col)
{
    if (row >= LED_MATRIX_ROWS || col >= LED_MATRIX_COLS) {
        return false;
    }
    return fb_read((uint16_t)(row * LED_MATRIX_COLS + col));
}

void led_matrix_set_index(uint16_t index, bool on)
{
    if (index < LED_MATRIX_COUNT) {
        fb_write(index, on);
    }
}

bool led_matrix_get_index(uint16_t index)
{
    if (index >= LED_MATRIX_COUNT) {
        return false;
    }
    return fb_read(index);
}

uint16_t led_matrix_on_count(void)
{
    uint16_t n = 0;
    for (uint16_t i = 0; i < LED_MATRIX_COUNT; i++) {
        if (fb_read(i)) {
            n++;
        }
    }
    return n;
}

void led_matrix_set_brightness(uint8_t percent)
{
    if (percent > 100u) {
        percent = 100u;
    }
    s_brightness = percent;
    s_gate = 0u;
}

uint8_t led_matrix_get_brightness(void)
{
    return s_brightness;
}

void led_matrix_scan(void)
{
    uint16_t index = s_slot;

    /* 占空比门控：亮度 <100 时每 100 拍只驱动 brightness 拍。
     * 门控关断时保持 s_slot 不动，开门后从同一颗灯继续扫 */
    if (s_brightness < 100u) {
        s_gate = (uint8_t)((s_gate + 1u) % 100u);
        if (s_gate >= s_brightness) {
            led_port_release();
            return;
        }
    }

    for (uint16_t i = 0; i < LED_MATRIX_COUNT; i++) {
        index++;
        if (index >= LED_MATRIX_COUNT) {
            index = 0;
        }
        if (fb_read(index)) {
            const led_map_entry_t *entry = led_map_at(index);
            if (entry == NULL) {
                break;
            }
            led_port_drive((led_pin_t)entry->anode, (led_pin_t)entry->cathode);
            s_slot = (uint8_t)index;
            return;
        }
    }

    /* 显存全灭 */
    led_port_release();
    s_slot = LED_MATRIX_COUNT - 1;
}
