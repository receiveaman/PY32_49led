#include <string.h>
#include "rs485.h"
#include "rs485_port.h"

static bool s_inited = false;
static rs485_config_t s_cfg;

rs485_err_t rs485_init_with_cfg(const rs485_config_t *cfg)
{
    if (cfg == NULL) {
        return RS485_ERR_PARAM;
    }
    if (s_inited) {
        return RS485_OK;
    }

    rs485_err_t err = rs485_port_init(cfg);
    if (err != RS485_OK) {
        return err;
    }

    s_cfg = *cfg;
    s_inited = true;
    rs485_rx_enable();
    return RS485_OK;
}

rs485_err_t rs485_init(void)
{
    const rs485_config_t def = RS485_DEFAULT_CONFIG();
    return rs485_init_with_cfg(&def);
}

rs485_err_t rs485_deinit(void)
{
    if (!s_inited) {
        return RS485_OK;
    }
    rs485_err_t err = rs485_port_deinit();
    s_inited = false;
    memset(&s_cfg, 0, sizeof(s_cfg));
    return err;
}

bool rs485_is_init(void)
{
    return s_inited;
}

const rs485_config_t *rs485_get_config(void)
{
    return s_inited ? &s_cfg : NULL;
}

rs485_err_t rs485_tx_enable(void)
{
    if (!s_inited) {
        return RS485_ERR_STATE;
    }
    rs485_err_t err = rs485_port_set_tx(true);
    if (err != RS485_OK) {
        return err;
    }
    rs485_port_delay_us(s_cfg.tx_setup_delay_us);
    return RS485_OK;
}

rs485_err_t rs485_rx_enable(void)
{
    if (!s_inited) {
        return RS485_ERR_STATE;
    }
    return rs485_port_set_tx(false);
}

rs485_err_t rs485_send(const uint8_t *data, size_t len)
{
    if (data == NULL || len == 0) {
        return RS485_ERR_PARAM;
    }
    if (!s_inited) {
        return RS485_ERR_STATE;
    }

    rs485_err_t err = rs485_port_lock(RS485_TIMEOUT_FOREVER);
    if (err != RS485_OK) {
        return err;
    }

    err = rs485_tx_enable();
    if (err != RS485_OK) {
        rs485_port_unlock();
        return err;
    }

    int written = rs485_port_write(data, len);
    if (written < 0 || (size_t)written != len) {
        rs485_rx_enable();
        rs485_port_unlock();
        return RS485_ERR_IO;
    }

    rs485_err_t tx_err = rs485_port_wait_tx_done(1000 + (uint32_t)len);
    rs485_port_delay_ms(s_cfg.tx_end_delay_ms);
    rs485_rx_enable();
    rs485_port_unlock();

    if (tx_err != RS485_OK) {
        return RS485_ERR_TIMEOUT;
    }
    return RS485_OK;
}

int rs485_receive(uint8_t *data, size_t max_len, uint32_t timeout_ms)
{
    if (data == NULL || max_len == 0) {
        return -1;
    }
    if (!s_inited) {
        return -1;
    }

    int got = rs485_port_read(data, 1, timeout_ms);
    if (got <= 0) {
        return 0;
    }

    while ((size_t)got < max_len) {
        int n = rs485_port_read(data + got, max_len - got,
                                s_cfg.inter_byte_timeout_ms);
        if (n <= 0) {
            break;
        }
        got += n;
    }
    return got;
}
