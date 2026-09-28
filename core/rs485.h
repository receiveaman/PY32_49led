#ifndef RS485_H
#define RS485_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ======================= 配置区域 ======================= */
/* 硬件引脚属于板级绑定，全部写在 port/rs485_port.c 里，接口见 port/rs485_port.h：
 *   PB4  USART1_TX  -> MAX3485 DI
 *   PB5  USART1_RX  <- MAX3485 RO
 *   PB3  MAX3485_RD -> MAX3485 /RE 与 DE（两脚短接），高电平发送、低电平接收
 * 说明：PA0/PA1/PA3/PA4/PA5、PB0/PB1/PB2 已被 LED 矩阵占用。
 */
#define RS485_UART_NUM      0
#define RS485_BAUDRATE      115200
#define RS485_RX_BUF_SIZE   1024
#define RS485_TX_END_DELAY_MS 1      /* wait_tx_done 后保持 DE 的时间 */
#define RS485_DE_SETUP_DELAY_US 100  /* DE 拉高到开始发送的建立时间 */
#define RS485_INTER_BYTE_TIMEOUT_MS 10 /* 接收字节间静默超时 */
/* ======================================================= */

#define RS485_TIMEOUT_FOREVER 0xFFFFFFFFu

typedef enum {
    RS485_OK          = 0,
    RS485_ERR_PARAM   = -1,
    RS485_ERR_STATE   = -2,
    RS485_ERR_TIMEOUT = -3,
    RS485_ERR_IO      = -4,
    RS485_ERR_NOMEM   = -5,
} rs485_err_t;

typedef struct {
    int      uart_id;
    uint32_t baudrate;
    uint32_t rx_buf_size;
    uint32_t tx_setup_delay_us;
    uint32_t tx_end_delay_ms;
    uint32_t inter_byte_timeout_ms;
} rs485_config_t;

#define RS485_DEFAULT_CONFIG() {                       \
    .uart_id               = RS485_UART_NUM,          \
    .baudrate              = RS485_BAUDRATE,          \
    .rx_buf_size           = RS485_RX_BUF_SIZE,       \
    .tx_setup_delay_us     = RS485_DE_SETUP_DELAY_US, \
    .tx_end_delay_ms       = RS485_TX_END_DELAY_MS,   \
    .inter_byte_timeout_ms = RS485_INTER_BYTE_TIMEOUT_MS, \
}

rs485_err_t rs485_init(void);
rs485_err_t rs485_init_with_cfg(const rs485_config_t *cfg);
rs485_err_t rs485_deinit(void);
bool        rs485_is_init(void);
const rs485_config_t *rs485_get_config(void);

rs485_err_t rs485_tx_enable(void);
rs485_err_t rs485_rx_enable(void);
rs485_err_t rs485_send(const uint8_t *data, size_t len);
int         rs485_receive(uint8_t *data, size_t max_len, uint32_t timeout_ms);

#ifdef __cplusplus
}
#endif

#endif /* RS485_H */
