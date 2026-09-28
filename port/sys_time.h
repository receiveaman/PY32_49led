#ifndef SYS_TIME_H
#define SYS_TIME_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* 1ms 时基：LL_Init1msTick() 配好 SysTick 后调一次，补开 SysTick 中断 */
void sys_time_init(void);

/* SysTick 中断里调用 */
void sys_time_on_tick(void);

/* 开机以来的毫秒数，会回绕 */
uint32_t sys_time_ms(void);

/* 非阻塞式忙等，基于 sys_time_ms() */
void sys_time_delay_ms(uint32_t ms);

#ifdef __cplusplus
}
#endif

#endif /* SYS_TIME_H */
