#ifndef RS485_PORT_H
#define RS485_PORT_H

#include "rs485.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * L1 移植层 —— 485 物理接口（UART + 收发方向脚 + 中断）
 * 本头文件**不包含任何 MCU 厂商头**，core/rs485.c 只依赖这里的 9 个函数。
 * 换 MCU 时：保留本头文件，重写同名 rs485_port.c 即可。
 * 引脚/外设绑定（PB4=USART1_TX、PB5=USART1_RX、PB3=MAX3485_RD）
 * 属于板级私有，全部写在 rs485_port.c 里，不进接口头。
 */
rs485_err_t rs485_port_init(const rs485_config_t *cfg);
rs485_err_t rs485_port_deinit(void);
rs485_err_t rs485_port_set_tx(bool tx_enable);
rs485_err_t rs485_port_lock(uint32_t timeout_ms);
rs485_err_t rs485_port_unlock(void);
int         rs485_port_write(const uint8_t *data, size_t len);
rs485_err_t rs485_port_wait_tx_done(uint32_t timeout_ms);
int         rs485_port_read(uint8_t *data, size_t len, uint32_t timeout_ms);
uint32_t    rs485_port_rx_pending(void);
void        rs485_port_delay_ms(uint32_t ms);
void        rs485_port_delay_us(uint32_t us);
void        rs485_port_irq_handler(void);

#ifdef __cplusplus
}
#endif

#endif /* RS485_PORT_H */
