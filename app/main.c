/**
  ******************************************************************************
  * @file    main.c
  * @author  MCU Application Team
  * @brief   Main program body
  ******************************************************************************
  * @attention
  *
  * <h2><center>&copy; Copyright (c) 2023 Puya Semiconductor Co.
  * All rights reserved.</center></h2>
  *
  * This software component is licensed by Puya under BSD 3-Clause license,
  * the "License"; You may not use this file except in compliance with the
  * License. You may obtain a copy of the License at:
  *                        opensource.org/licenses/BSD-3-Clause
  *
  ******************************************************************************
  * @attention
  *
  * <h2><center>&copy; Copyright (c) 2016 STMicroelectronics.
  * All rights reserved.</center></h2>
  *
  * This software component is licensed by ST under BSD 3-Clause license,
  * the "License"; You may not use this file except in compliance with the
  * License. You may obtain a copy of the License at:
  *                        opensource.org/licenses/BSD-3-Clause
  *
  ******************************************************************************
  */

/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "led_matrix.h"
#include "led_effect.h"
#include "sys_time.h"
#include "rs485.h"
#include "rs485_port.h"
#include "proto.h"

/* Private define ------------------------------------------------------------*/
/* 回显自测：跑协议时必须关闭，会往总线发非协议帧 */
#define APP_ECHO_SELFTEST  0
/* 心跳自测已注释停用（协议运行时不能发 HB 文本帧），启用时打开下面这行 */
/* #define APP_HEARTBEAT  1 */

#if APP_ECHO_SELFTEST
/* RS485 回显自测：攒满或静默 APP_ECHO_QUIET_MS 认为一帧收完，整帧原样发回 */
#define APP_ECHO_MAX       32u
#define APP_ECHO_QUIET_MS  2u
#endif
/* Private variables ---------------------------------------------------------*/
/* Private user code ---------------------------------------------------------*/
/* Private macro -------------------------------------------------------------*/
/* Private function prototypes -----------------------------------------------*/
static void APP_SystemClockConfig(void);
#if APP_ECHO_SELFTEST
static void APP_Rs485Echo(void);
#endif

/**
  * @brief  Application Entry Function.
  * @param  None
  * @retval int
  */
int main(void)
{
  /* Enable SYSCFG and PWR clock */
  LL_APB1_GRP2_EnableClock(LL_APB1_GRP2_PERIPH_SYSCFG);
  LL_APB1_GRP1_EnableClock(LL_APB1_GRP1_PERIPH_PWR);

  /* Configure systemclock */
  APP_SystemClockConfig();

  /* 1ms 时基（SysTick 中断），必须在 LL_Init1msTick() 之后 */
  sys_time_init();

  led_matrix_init();
  led_effect_init();

  /* RS485(MAX3485)：PB4=TX PB5=RX PB3=RE+DE */
  (void)rs485_init();

  /* 协议从站：加载 Flash 配置、复位收帧状态机 */
  proto_init();

  /* infinite loop */
  while (1)
  {
    led_effect_update();
    led_matrix_scan();
    proto_poll();
#if APP_ECHO_SELFTEST
    APP_Rs485Echo();
#endif
    /* 心跳自测已注释停用：APP_Rs485Heartbeat(); */
  }
}

/* ---- 心跳自测：已注释停用（打开时把两处 #if 0 改回 #if APP_HEARTBEAT，
 *       并放开顶部 #define APP_HEARTBEAT 1 与主循环里的调用注释） ---- */
#if 0
/* 心跳：每隔 APP_HB_PERIOD_MS 主动往 485 总线发一帧，验证 MCU->上位机 方向 */
#define APP_HB_PERIOD_MS   1000u

/*
 * 心跳帧：HB <序号> <运行毫秒> ms\r\n，验证 MCU -> 上位机 方向是否通。
 * 自己拼十进制，不用 snprintf —— 3KB RAM 的片子不值得把 printf 机制链进来。
 */
static char *APP_U32ToStr(char *p, uint32_t v)
{
  char tmp[10];
  uint8_t n = 0;

  do {
    tmp[n++] = (char)('0' + (char)(v % 10u));
    v /= 10u;
  } while (v != 0u);
  while (n != 0u) {
    *p++ = tmp[--n];
  }
  return p;
}
#endif

#if 0
static void APP_Rs485Heartbeat(void)
{
  static uint32_t next_ms = 0;
  static uint16_t seq = 0;
  static bool started = false;
  char frame[32];
  char *p = frame;
  uint32_t now = sys_time_ms();

  if (!started) {
    started = true;
    next_ms = now;
  }
  if ((int32_t)(now - next_ms) < 0) {
    return;
  }
  next_ms = now + APP_HB_PERIOD_MS;

  *p++ = 'H';
  *p++ = 'B';
  *p++ = ' ';
  p = APP_U32ToStr(p, ++seq);
  *p++ = ' ';
  p = APP_U32ToStr(p, now);
  *p++ = ' ';
  *p++ = 'm';
  *p++ = 's';
  *p++ = '\r';
  *p++ = '\n';

  (void)rs485_send((const uint8_t *)frame, (size_t)(p - frame));
}
#endif

#if APP_ECHO_SELFTEST
/*
 * 回显自测：一次只取 1 字节且 timeout=0，不触发 inter-byte 超时，
 * 免得把 LED 扫描卡住；静默 APP_ECHO_QUIET_MS 后整帧回发。
 *
 * 静默判定不能只看“最后一次读到字节”的时间：echo 缓冲满得读不进去时
 * 线上仍在收，必须用 rs485_port_rx_pending() 看接收缓冲还在不在涨，
 * 否则会在上位机没发完时提前开吼，半双工总线就撞车丢字节。
 */
static void APP_Rs485Echo(void)
{
  static uint8_t buf[APP_ECHO_MAX];
  static uint8_t len = 0;
  static uint32_t last_ms = 0;
  static uint32_t seen = 0;
  uint32_t pending = rs485_port_rx_pending();
  uint8_t byte;

  if (pending != seen) {            /* 接收缓冲在涨 = 线上还在收 */
    seen = pending;
    last_ms = sys_time_ms();
  }

  while (len < APP_ECHO_MAX && rs485_receive(&byte, 1, 0) == 1) {
    buf[len++] = byte;
    last_ms = sys_time_ms();
    seen = rs485_port_rx_pending();
  }

  if (len != 0 && (sys_time_ms() - last_ms) >= APP_ECHO_QUIET_MS) {
    (void)rs485_send(buf, len);
    len = 0;
  }
}
#endif

/**
  * @brief  Configure systemclock
  * @param  None
  * @retval None
  */
static void APP_SystemClockConfig(void)
{
  /* Enable HSI */
  LL_RCC_HSI_Enable();
  while(LL_RCC_HSI_IsReady() != 1)
  {
  }

  /* Set AHB prescaler: HCLK = SYSCLK */
  LL_RCC_SetAHBPrescaler(LL_RCC_SYSCLK_DIV_1);

  /* Select HSISYS as system clock source */
  LL_RCC_SetSysClkSource(LL_RCC_SYS_CLKSOURCE_HSISYS);
  while(LL_RCC_GetSysClkSource() != LL_RCC_SYS_CLKSOURCE_STATUS_HSISYS)
  {
  }

  /* Set APB prescaler: PCLK = HCLK */
  LL_RCC_SetAPB1Prescaler(LL_RCC_APB1_DIV_1);
  LL_Init1msTick(24000000);

  /* Update the SystemCoreClock global variable(which can be updated also through SystemCoreClockUpdate function) */
  LL_SetSystemCoreClock(24000000);
}

/**
  * @brief  Error handling function
  * @param  None
  * @retval None
  */
void APP_ErrorHandler(void)
{
  /* Infinite loop */
  while (1)
  {
  }
}

#ifdef  USE_FULL_ASSERT
/**
  * @brief  Reports the name of the source file and the source line number
  *         where the assert_param error has occurred.
  * @param  file：Pointer to the source file name
  * @param  line：assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t *file, uint32_t line)
{
  /* User can add His own implementation to report the file name and line number,
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  /* Infinite loop */
  while (1)
  {
  }
}
#endif /* USE_FULL_ASSERT */

/************************ (C) COPYRIGHT Puya *****END OF FILE******************/
