#include "main.h"
#include "py32f002b_ll_usart.h"
#include "rs485.h"
#include "rs485_port.h"
#include "sys_time.h"

/* ============ 本板（PY32F002B + MAX3485）外设与引脚绑定 ============
 *   PB4 = USART1_TX  -> MAX3485 DI      (AF1)
 *   PB5 = USART1_RX  <- MAX3485 RO      (AF1)
 *   PB3 = MAX3485_RD -> /RE + DE 短接   (GPIO，高=发送 / 低=接收)
 * PA0/PA1/PA3/PA4/PA5、PB0/PB1/PB2 已被 LED 矩阵占用。
 * 这一段连同下面所有 LL_* 调用，换 MCU 时整体重写。
 */
#define RS485_USARTx            USART1
#define RS485_USARTx_CLK_EN()   LL_APB1_GRP2_EnableClock(LL_APB1_GRP2_PERIPH_USART1)
#define RS485_USARTx_IRQ        USART1_IRQn
#define RS485_GPIOx_CLK_EN()    LL_IOP_GRP1_EnableClock(LL_IOP_GRP1_PERIPH_GPIOB)
#define RS485_AF                LL_GPIO_AF_1

#define RS485_TX_GPIO_PORT      GPIOB
#define RS485_TX_GPIO_PIN       LL_GPIO_PIN_4
#define RS485_RX_GPIO_PORT      GPIOB
#define RS485_RX_GPIO_PIN       LL_GPIO_PIN_5
#define RS485_DIR_GPIO_PORT     GPIOB
#define RS485_DIR_GPIO_PIN      LL_GPIO_PIN_3
/* ===================================================================== */

#define RS485_TX_BYTE_TIMEOUT_MS 100u  /* 单字节等待 TXE 超时 */

static rs485_config_t s_cfg;
static bool s_ready = false;
static volatile bool s_locked = false;

static uint8_t s_rx_buf[RS485_RX_BUF_SIZE];
static volatile uint32_t s_rx_head = 0;   /* 单调递增，取模得下标 */
static volatile uint32_t s_rx_tail = 0;

static uint32_t rx_count(void)
{
    return s_rx_head - s_rx_tail;
}

static void rx_push(uint8_t byte)
{
    if (rx_count() >= s_cfg.rx_buf_size) {
        return;                          /* 满了就丢弃，由上层按协议重传 */
    }
    s_rx_buf[s_rx_head % s_cfg.rx_buf_size] = byte;
    s_rx_head++;
}

static uint8_t rx_pop(void)
{
    uint8_t byte = s_rx_buf[s_rx_tail % s_cfg.rx_buf_size];
    s_rx_tail++;
    return byte;
}

/* USART1 中断：由 py32f002b_it.c 的 USART1_IRQHandler 转发进来 */
void rs485_port_irq_handler(void)
{
    if (LL_USART_IsActiveFlag_RXNE(RS485_USARTx) ||
        LL_USART_IsActiveFlag_ORE(RS485_USARTx)) {
        /* 先读 SR 再读 DR，RXNE / ORE 一并清掉 */
        uint8_t byte = LL_USART_ReceiveData8(RS485_USARTx);
        if (s_ready) {
            rx_push(byte);
        }
    }
}

rs485_err_t rs485_port_init(const rs485_config_t *cfg)
{
    if (cfg == NULL || cfg->baudrate == 0u ||
        cfg->rx_buf_size == 0u || cfg->rx_buf_size > RS485_RX_BUF_SIZE) {
        return RS485_ERR_PARAM;
    }

    s_ready = false;
    s_cfg = *cfg;
    s_rx_head = 0;
    s_rx_tail = 0;
    s_locked = false;

    RS485_GPIOx_CLK_EN();
    RS485_USARTx_CLK_EN();

    /* PB3 方向脚：先拉低（接收态）再切成推挽输出 */
    LL_GPIO_ResetOutputPin(RS485_DIR_GPIO_PORT, RS485_DIR_GPIO_PIN);
    LL_GPIO_SetPinOutputType(RS485_DIR_GPIO_PORT, RS485_DIR_GPIO_PIN,
                             LL_GPIO_OUTPUT_PUSHPULL);
    LL_GPIO_SetPinSpeed(RS485_DIR_GPIO_PORT, RS485_DIR_GPIO_PIN,
                        LL_GPIO_SPEED_FREQ_LOW);
    LL_GPIO_SetPinPull(RS485_DIR_GPIO_PORT, RS485_DIR_GPIO_PIN, LL_GPIO_PULL_NO);
    LL_GPIO_SetPinMode(RS485_DIR_GPIO_PORT, RS485_DIR_GPIO_PIN,
                       LL_GPIO_MODE_OUTPUT);

    /* PB4 TX */
    LL_GPIO_SetPinSpeed(RS485_TX_GPIO_PORT, RS485_TX_GPIO_PIN,
                        LL_GPIO_SPEED_FREQ_VERY_HIGH);
    LL_GPIO_SetPinPull(RS485_TX_GPIO_PORT, RS485_TX_GPIO_PIN, LL_GPIO_PULL_UP);
    LL_GPIO_SetAFPin_0_7(RS485_TX_GPIO_PORT, RS485_TX_GPIO_PIN, RS485_AF);
    LL_GPIO_SetPinMode(RS485_TX_GPIO_PORT, RS485_TX_GPIO_PIN,
                       LL_GPIO_MODE_ALTERNATE);

    /* PB5 RX */
    LL_GPIO_SetPinSpeed(RS485_RX_GPIO_PORT, RS485_RX_GPIO_PIN,
                        LL_GPIO_SPEED_FREQ_VERY_HIGH);
    LL_GPIO_SetPinPull(RS485_RX_GPIO_PORT, RS485_RX_GPIO_PIN, LL_GPIO_PULL_UP);
    LL_GPIO_SetAFPin_0_7(RS485_RX_GPIO_PORT, RS485_RX_GPIO_PIN, RS485_AF);
    LL_GPIO_SetPinMode(RS485_RX_GPIO_PORT, RS485_RX_GPIO_PIN,
                       LL_GPIO_MODE_ALTERNATE);

    /* USART1: 8-N-1, 无流控 */
    LL_USART_Disable(RS485_USARTx);
    LL_USART_SetBaudRate(RS485_USARTx, SystemCoreClock, LL_USART_OVERSAMPLING_16,
                         s_cfg.baudrate);
    LL_USART_SetDataWidth(RS485_USARTx, LL_USART_DATAWIDTH_8B);
    LL_USART_SetStopBitsLength(RS485_USARTx, LL_USART_STOPBITS_1);
    LL_USART_SetParity(RS485_USARTx, LL_USART_PARITY_NONE);
    LL_USART_SetHWFlowCtrl(RS485_USARTx, LL_USART_HWCONTROL_NONE);
    LL_USART_SetTransferDirection(RS485_USARTx, LL_USART_DIRECTION_TX_RX);
    LL_USART_Enable(RS485_USARTx);
    LL_USART_ClearFlag_TC(RS485_USARTx);

    NVIC_DisableIRQ(RS485_USARTx_IRQ);
    NVIC_ClearPendingIRQ(RS485_USARTx_IRQ);
    NVIC_SetPriority(RS485_USARTx_IRQ, 1U);
    NVIC_EnableIRQ(RS485_USARTx_IRQ);
    LL_USART_EnableIT_RXNE(RS485_USARTx);

    s_ready = true;
    rs485_port_set_tx(false);            /* 上电默认接收 */
    return RS485_OK;
}

rs485_err_t rs485_port_deinit(void)
{
    if (!s_ready) {
        return RS485_OK;
    }
    s_ready = false;

    LL_USART_DisableIT_RXNE(RS485_USARTx);
    NVIC_DisableIRQ(RS485_USARTx_IRQ);
    NVIC_ClearPendingIRQ(RS485_USARTx_IRQ);
    LL_USART_Disable(RS485_USARTx);

    LL_GPIO_SetPinMode(RS485_TX_GPIO_PORT, RS485_TX_GPIO_PIN, LL_GPIO_MODE_INPUT);
    LL_GPIO_SetPinMode(RS485_RX_GPIO_PORT, RS485_RX_GPIO_PIN, LL_GPIO_MODE_INPUT);
    LL_GPIO_SetPinMode(RS485_DIR_GPIO_PORT, RS485_DIR_GPIO_PIN,
                       LL_GPIO_MODE_INPUT);

    s_rx_head = 0;
    s_rx_tail = 0;
    s_locked = false;
    return RS485_OK;
}

rs485_err_t rs485_port_set_tx(bool tx_enable)
{
    if (!s_ready) {
        return RS485_ERR_STATE;
    }
    if (tx_enable) {
        LL_GPIO_SetOutputPin(RS485_DIR_GPIO_PORT, RS485_DIR_GPIO_PIN);
    } else {
        LL_GPIO_ResetOutputPin(RS485_DIR_GPIO_PORT, RS485_DIR_GPIO_PIN);
    }
    return RS485_OK;
}

rs485_err_t rs485_port_lock(uint32_t timeout_ms)
{
    uint32_t start = sys_time_ms();

    while (s_locked) {
        if (timeout_ms == 0u) {
            return RS485_ERR_TIMEOUT;
        }
        if (timeout_ms != RS485_TIMEOUT_FOREVER &&
            (sys_time_ms() - start) > timeout_ms) {
            return RS485_ERR_TIMEOUT;
        }
    }
    s_locked = true;
    return RS485_OK;
}

rs485_err_t rs485_port_unlock(void)
{
    s_locked = false;
    return RS485_OK;
}

int rs485_port_write(const uint8_t *data, size_t len)
{
    size_t i;

    if (!s_ready || data == NULL) {
        return -1;
    }
    for (i = 0; i < len; i++) {
        uint32_t start = sys_time_ms();

        while (!LL_USART_IsActiveFlag_TXE(RS485_USARTx)) {
            if ((sys_time_ms() - start) > RS485_TX_BYTE_TIMEOUT_MS) {
                return (int)i;
            }
        }
        LL_USART_TransmitData8(RS485_USARTx, data[i]);
    }
    return (int)i;
}

rs485_err_t rs485_port_wait_tx_done(uint32_t timeout_ms)
{
    uint32_t start = sys_time_ms();

    if (!s_ready) {
        return RS485_ERR_STATE;
    }
    while (!LL_USART_IsActiveFlag_TC(RS485_USARTx)) {
        if (timeout_ms != RS485_TIMEOUT_FOREVER &&
            (sys_time_ms() - start) > timeout_ms) {
            return RS485_ERR_TIMEOUT;
        }
    }
    return RS485_OK;
}

int rs485_port_read(uint8_t *data, size_t len, uint32_t timeout_ms)
{
    uint32_t start = sys_time_ms();
    size_t got = 0;

    if (!s_ready || data == NULL || len == 0u) {
        return -1;
    }

    while (rx_count() == 0u) {
        /* timeout 0 = 非阻塞，拿不到立刻返回 */
        if (timeout_ms == 0u ||
            (timeout_ms != RS485_TIMEOUT_FOREVER &&
             (sys_time_ms() - start) > timeout_ms)) {
            return 0;
        }
    }

    while ((got < len) && (rx_count() != 0u)) {
        data[got++] = rx_pop();
    }
    return (int)got;
}

uint32_t rs485_port_rx_pending(void)
{
    return rx_count();
}

void rs485_port_delay_ms(uint32_t ms)
{
    LL_mDelay(ms);
}

void rs485_port_delay_us(uint32_t us)
{
    uint32_t cycles = us * (SystemCoreClock / 1000000u);

    while (cycles >= 4u) {
        __NOP();
        __NOP();
        __NOP();
        __NOP();
        cycles -= 4u;
    }
    while (cycles != 0u) {
        __NOP();
        cycles--;
    }
}
