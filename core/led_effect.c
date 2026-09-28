#include "led_effect.h"
#include "led_matrix.h"
#include "sys_time.h"

typedef struct {
    uint16_t period_ms; /* 0 = 只在切效果时画一次 */
    void (*step)(uint16_t phase);
} fx_ops_t;

static led_effect_t s_fx_id = LED_FX_NONE;
static uint16_t s_phase = 0;
static uint32_t s_step_ms = 0;
static uint32_t s_dwell_ms = 0;
static uint32_t s_rand = 0x12345678u;

static void fx_none(uint16_t phase)
{
    (void)phase;
    led_matrix_clear();
}

static void fx_all_on(uint16_t phase)
{
    (void)phase;
    led_matrix_fill(true);
}

static void fx_blink(uint16_t phase)
{
    led_matrix_fill((phase & 1u) != 0u);
}

static void fx_walk(uint16_t phase)
{
    led_matrix_clear();
    led_matrix_set_index(phase % LED_MATRIX_COUNT, true);
}

static void fx_row_scan(uint16_t phase)
{
    uint8_t row = (uint8_t)(phase % LED_MATRIX_ROWS);
    led_matrix_clear();
    for (uint8_t col = 0; col < LED_MATRIX_COLS; col++) {
        led_matrix_set(row, col, true);
    }
}

static void fx_col_scan(uint16_t phase)
{
    uint8_t col = (uint8_t)(phase % LED_MATRIX_COLS);
    led_matrix_clear();
    for (uint8_t row = 0; row < LED_MATRIX_ROWS; row++) {
        led_matrix_set(row, col, true);
    }
}

static void fx_diagonal(uint16_t phase)
{
    uint8_t k = (uint8_t)(phase % (LED_MATRIX_ROWS + LED_MATRIX_COLS - 1));
    led_matrix_clear();
    for (uint8_t row = 0; row < LED_MATRIX_ROWS; row++) {
        for (uint8_t col = 0; col < LED_MATRIX_COLS; col++) {
            if ((uint8_t)(row + col) == k) {
                led_matrix_set(row, col, true);
            }
        }
    }
}

/* 7x7 外框按顺时针展开的序号：0..6 上边，7..12 右边，13..18 下边，19..23 左边 */
static uint16_t border_index(uint8_t step)
{
    if (step < 7) {
        return step;
    }
    if (step < 13) {
        return (uint16_t)((step - 6) * LED_MATRIX_COLS + (LED_MATRIX_COLS - 1));
    }
    if (step < 19) {
        return (uint16_t)((LED_MATRIX_ROWS - 1) * LED_MATRIX_COLS + (5 - (step - 13)));
    }
    return (uint16_t)((5 - (step - 19)) * LED_MATRIX_COLS);
}

static void fx_border(uint16_t phase)
{
    led_matrix_clear();
    led_matrix_set_index(border_index((uint8_t)(phase % 24u)), true);
}

static void fx_ripple(uint16_t phase)
{
    uint8_t ring = (uint8_t)(phase % 7u);
    led_matrix_clear();
    for (uint8_t row = 0; row < LED_MATRIX_ROWS; row++) {
        for (uint8_t col = 0; col < LED_MATRIX_COLS; col++) {
            uint8_t dist = (uint8_t)((row > 3 ? row - 3 : 3 - row) + (col > 3 ? col - 3 : 3 - col));
            if (dist == ring) {
                led_matrix_set(row, col, true);
            }
        }
    }
}

static void fx_checker(uint16_t phase)
{
    uint8_t parity = (uint8_t)(phase & 1u);
    led_matrix_clear();
    for (uint8_t row = 0; row < LED_MATRIX_ROWS; row++) {
        for (uint8_t col = 0; col < LED_MATRIX_COLS; col++) {
            if (((row + col) & 1u) == parity) {
                led_matrix_set(row, col, true);
            }
        }
    }
}

/* 协议层写灯时切到这个“空效果”：既不改显存，也不自动轮播（RS485_Protocol_Spec §9/§11.3-2） */
static void fx_remote(uint16_t phase)
{
    (void)phase;
}

static void fx_random(uint16_t phase)
{
    (void)phase;
    for (uint16_t i = 0; i < LED_MATRIX_COUNT; i++) {
        s_rand = s_rand * 1103515245u + 12345u;
        led_matrix_set_index(i, ((s_rand >> 16) & 1u) != 0u);
    }
}

static const fx_ops_t s_fx_tbl[LED_FX_COUNT] = {
    [LED_FX_NONE]     = {0u,   fx_none},
    [LED_FX_ALL_ON]   = {0u,   fx_all_on},
    [LED_FX_BLINK]    = {500u, fx_blink},
    [LED_FX_WALK]     = {100u, fx_walk},
    [LED_FX_ROW_SCAN] = {200u, fx_row_scan},
    [LED_FX_COL_SCAN] = {200u, fx_col_scan},
    [LED_FX_DIAGONAL] = {130u, fx_diagonal},
    [LED_FX_BORDER]   = {90u,  fx_border},
    [LED_FX_RIPPLE]   = {160u, fx_ripple},
    [LED_FX_CHECKER]  = {400u, fx_checker},
    [LED_FX_RANDOM]  = {70u,  fx_random},
    [LED_FX_REMOTE]  = {0u,   fx_remote},
};

void led_effect_init(void)
{
    led_effect_set(LED_FX_WALK);
}

void led_effect_set(led_effect_t fx)
{
    if (fx >= LED_FX_COUNT) {
        return;
    }
    s_fx_id = fx;
    s_phase = 0;
    s_step_ms = sys_time_ms();
    s_dwell_ms = s_step_ms + LED_FX_DWELL_MS;
    s_fx_tbl[s_fx_id].step(s_phase);
}

led_effect_t led_effect_get(void)
{
    return s_fx_id;
}

void led_effect_next(void)
{
    led_effect_set((led_effect_t)((s_fx_id + 1u) % LED_FX_COUNT));
}

void led_effect_update(void)
{
    uint32_t now = sys_time_ms();
    const fx_ops_t *ops = &s_fx_tbl[s_fx_id];

    if (ops->period_ms != 0u && (now - s_step_ms) >= ops->period_ms) {
        s_step_ms += ops->period_ms;
        if ((now - s_step_ms) >= ops->period_ms) {
            s_step_ms = now; /* 卡顿兜底，避免连跑补偿帧 */
        }
        s_phase++;
        ops->step(s_phase);
    }

    /* 演示模式：到点切下一个效果。受控模式（LED_FX_REMOTE）必须跳过，
     * 否则 5s 后自动轮播会覆盖协议写进显存的内容 */
    if (s_fx_id != LED_FX_REMOTE && (int32_t)(now - s_dwell_ms) >= 0) {
        led_effect_next();
    }
}
