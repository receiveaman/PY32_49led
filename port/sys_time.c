#include "sys_time.h"
#include "py32f0xx.h"

static volatile uint32_t s_ms = 0;

void sys_time_init(void)
{
    /* LL_Init1msTick() 只开了计数器，这里补上每 1ms 一次的中断 */
    SysTick->CTRL |= SysTick_CTRL_TICKINT_Msk;
}

void sys_time_on_tick(void)
{
    s_ms++;
}

uint32_t sys_time_ms(void)
{
    return s_ms;
}

void sys_time_delay_ms(uint32_t ms)
{
    uint32_t start = s_ms;
    while ((s_ms - start) < ms) {
    }
}
