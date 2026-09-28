/**
 * RS485 从站协议层 —— 49LED 板（PY32F002B）
 * 实现 RS485_Protocol_Spec_V1.0.md 第 9 章 TODO + 第 11.3 章六条决策。
 */
#include <string.h>
#include "proto.h"
#include "rs485.h"
#include "led_matrix.h"
#include "led_effect.h"
#include "sys_time.h"
#include "port_flash.h" /* L1 移植层：配置区掉电保存，换 MCU 改 port/port_flash.c */

/* ============================ 帧与状态机（§3/§8） ============================ */
#define PROTO_HDR_LEN      4u
#define PROTO_CRC_LEN      2u
#define PROTO_SILENCE_MS   3u  /* 总线静默 ≥3ms 后的第一个字节判为站号 */
#define PROTO_BYTE_GAP_MS  5u  /* 字节间 >5ms 未收满 → 丢半帧重同步（§11.3-1） */
#define PROTO_RX_BUDGET    16u /* 每次 proto_poll 最多喂这么多字节，免卡扫描 */

/* ============================== 功能码（§5） ============================== */
#define FUNC_RLIST_REQ 0x01u
#define FUNC_RLIST_RSP 0x02u
#define FUNC_LED_RW    0x03u
#define FUNC_LED_RD    0x04u
#define FUNC_CFG       0x06u

/* ============================== 错误码（§7） ============================== */
#define ERR_UNSUPPORTED 0x01u
#define ERR_DATA        0x02u
#define ERR_LENGTH      0x03u
#define ERR_MODE        0x04u

/* ========================== 配置参数地址（§5.3） ========================== */
#define ADDR_MODE     0x00u
#define ADDR_BRIGHT   0x01u
#define ADDR_WD_TIME  0x02u
#define ADDR_WD_ACT   0x03u
#define ADDR_STATION  0x04u

/* ============================ Flash 配置区（§11.3-3） ====================== */
#define CFG_MAGIC      0xA55Au

typedef struct {
    uint8_t  mode;
    uint8_t  station;
    uint8_t  brightness;
    uint8_t  wd_action;
    uint16_t wd_timeout;
} proto_cfg_t;

typedef struct {
    uint16_t magic;
    uint16_t station;
    uint16_t brightness;
    uint16_t wd_timeout;
    uint16_t wd_action;
} proto_store_t;

_Static_assert(sizeof(proto_store_t) == 10u, "proto_store_t layout changed");

/* ================================== 状态 ================================== */
static proto_cfg_t s_cfg;
static led_effect_t s_demo_fx = LED_FX_WALK;

static uint8_t  s_rx[PROTO_FRAME_MAX];
static uint8_t  s_rx_len;
static uint8_t  s_rx_expect;
static bool     s_rx_active;
static uint32_t s_rx_last_ms;
static uint32_t s_wd_last_ms;

/* 4 个固定发送缓冲（§9），发送前只改变化字节 + 算 CRC */
static uint8_t s_send_02[PROTO_HDR_LEN + 7u + PROTO_CRC_LEN] = {
    0x00, FUNC_RLIST_RSP, 0x00, 0x07, 0x01, 0x00, 0x00, 0x00, 0x31, 0x00, 0x00, 0x00, 0x00};
static uint8_t s_send_04[PROTO_HDR_LEN + 8u + PROTO_CRC_LEN] = {
    0x00, FUNC_LED_RD, 0x00, 0x08, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
static uint8_t s_send_06[PROTO_HDR_LEN + 3u + PROTO_CRC_LEN] = {
    0x00, FUNC_CFG, 0x00, 0x03, 0x00, 0x00, 0x00, 0x00, 0x00};
static uint8_t s_send_err[PROTO_HDR_LEN + 1u + PROTO_CRC_LEN] = {
    0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00};

/* ================================= CRC16 ================================= */
uint16_t proto_crc16(const uint8_t *data, uint16_t len)
{
    uint16_t crc = 0xFFFFu;

    for (uint16_t i = 0; i < len; i++) {
        crc ^= data[i];
        for (uint8_t bit = 0; bit < 8u; bit++) {
            crc = ((crc & 1u) != 0u) ? (uint16_t)((crc >> 1) ^ 0xA001u) : (uint16_t)(crc >> 1);
        }
    }
    return crc;
}

static void frame_send(uint8_t *buf, uint8_t total)
{
    uint16_t crc = proto_crc16(buf, (uint16_t)(total - PROTO_CRC_LEN));

    buf[total - 2u] = (uint8_t)(crc >> 8);   /* 高字节在前，非 Modbus 标准顺序（§3） */
    buf[total - 1u] = (uint8_t)(crc & 0xFFu);
    (void)rs485_send(buf, total);
}

static void send_exception(uint8_t station, uint8_t func, uint8_t code)
{
    s_send_err[0] = station;
    s_send_err[1] = (uint8_t)(func | 0x80u);
    s_send_err[4] = code;
    frame_send(s_send_err, (uint8_t)sizeof(s_send_err));
}

/* ============================== 位图读写（§4） ============================ */
static void bitmap_read(uint8_t out[7])
{
    memset(out, 0, 7);
    for (uint8_t n = 0; n < LED_MATRIX_COUNT; n++) {
        if (led_matrix_get_index(n)) {
            out[n >> 3] |= (uint8_t)(0x80u >> (n & 7u));
        }
    }
}

static void bitmap_write(const uint8_t in[7])
{
    for (uint8_t n = 0; n < LED_MATRIX_COUNT; n++) {
        led_matrix_set_index(n, (in[n >> 3] & (uint8_t)(0x80u >> (n & 7u))) != 0u);
    }
}

/* ============================ 运行模式切换（§11.3-2） ====================== */
static void mode_set(uint8_t mode)
{
    if (mode == s_cfg.mode) {
        return;
    }
    if (mode == (uint8_t)PROTO_MODE_DEMO) {
        if (s_demo_fx == LED_FX_REMOTE) {
            s_demo_fx = LED_FX_WALK;
        }
        led_effect_set(s_demo_fx); /* 切回 0 恢复演示 */
    } else {
        if (s_cfg.mode == (uint8_t)PROTO_MODE_DEMO) {
            led_matrix_clear(); /* 模式 0→1/2 先清屏再生效 */
        }
        if (led_effect_get() != LED_FX_REMOTE) {
            s_demo_fx = led_effect_get();
        }
        led_effect_set(LED_FX_REMOTE); /* 防演示效果覆盖显存 */
    }
    s_cfg.mode = mode;
}

/* ============================== 配置读写（§5.3） ========================== */
static void cfg_get(uint8_t addr, uint16_t *value)
{
    switch (addr) {
    case ADDR_MODE:
        *value = s_cfg.mode;
        break;
    case ADDR_BRIGHT:
        *value = s_cfg.brightness;
        break;
    case ADDR_WD_TIME:
        *value = s_cfg.wd_timeout;
        break;
    case ADDR_WD_ACT:
        *value = s_cfg.wd_action;
        break;
    case ADDR_STATION:
        *value = s_cfg.station;
        break;
    default:
        *value = 0u;
        break;
    }
}

static uint8_t cfg_set(uint8_t addr, uint16_t value)
{
    switch (addr) {
    case ADDR_MODE:
        if (value > 2u) {
            return ERR_DATA;
        }
        mode_set((uint8_t)value);
        return 0u;
    case ADDR_BRIGHT:
        if (value > 100u) {
            return ERR_DATA;
        }
        s_cfg.brightness = (uint8_t)value;
        led_matrix_set_brightness(s_cfg.brightness);
        return 0u;
    case ADDR_WD_TIME:
        if (value != 0u && (value < 100u || value > 60000u)) {
            return ERR_DATA;
        }
        s_cfg.wd_timeout = value;
        return 0u;
    case ADDR_WD_ACT:
        if (value > 2u) {
            return ERR_DATA;
        }
        s_cfg.wd_action = (uint8_t)value;
        return 0u;
    case ADDR_STATION:
        if (value < 1u || value > 127u) {
            return ERR_DATA;
        }
        s_cfg.station = (uint8_t)value;
        return 0u;
    default:
        return ERR_LENGTH; /* 地址不存在 → 0x86 + 0x03（§6「异常-参数」示例帧） */
    }
}

/* ============================== Flash 持久化 ============================== */
static bool cfg_store(void)
{
    proto_store_t st;

    st.magic      = CFG_MAGIC;
    st.station    = s_cfg.station;
    st.brightness = s_cfg.brightness;
    st.wd_timeout = s_cfg.wd_timeout;
    st.wd_action  = s_cfg.wd_action;

    return port_flash_save(&st, (uint16_t)sizeof(st));
}

static void cfg_load(void)
{
    proto_store_t st;
    if (!port_flash_load(&st, (uint16_t)sizeof(st))) {
        return; /* area not programmed / read failed -> keep defaults */
    }

    if (st.magic != CFG_MAGIC) {
        return; /* 上电魔数不符 → 用默认值 */
    }
    if (st.station < 1u || st.station > 127u || st.brightness > 100u || st.wd_action > 2u) {
        return;
    }
    if (st.wd_timeout != 0u && (st.wd_timeout < 100u || st.wd_timeout > 60000u)) {
        return;
    }
    s_cfg.station    = (uint8_t)st.station;
    s_cfg.brightness = (uint8_t)st.brightness;
    s_cfg.wd_timeout = st.wd_timeout;
    s_cfg.wd_action  = (uint8_t)st.wd_action;
}

/* ================================ 帧分发 ================================= */
static void dispatch(void)
{
    const uint8_t *f = s_rx;
    uint8_t station = f[0];
    uint8_t func = f[1];
    uint16_t n = (uint16_t)(((uint16_t)f[2] << 8) | f[3]);
    const uint8_t *data = &f[PROTO_HDR_LEN];
    bool reply = (station != 0x00u); /* 0x00 = 广播，执行不回帧（§8） */

    if (reply) {
        s_wd_last_ms = sys_time_ms();
    }

    switch (func) {
    case FUNC_RLIST_REQ: /* 资源列表（§5.2） */
        if (n != 0u) {
            if (reply) {
                send_exception(station, func, ERR_LENGTH);
            }
            return;
        }
        if (reply) {
            s_send_02[0] = station;
            frame_send(s_send_02, (uint8_t)sizeof(s_send_02));
        }
        return;

    case FUNC_LED_RW: { /* 读/写 49 灯（§5.1） */
        uint8_t err = 0u;

        if (n != 8u) {
            err = ERR_LENGTH;
        } else if (s_cfg.mode == (uint8_t)PROTO_MODE_DEMO) {
            err = ERR_MODE; /* 模式 0 拒绝写灯 */
        } else if (data[0] == 0x01u) {
            if (led_effect_get() != LED_FX_REMOTE) {
                led_effect_set(LED_FX_REMOTE);
            }
            bitmap_write(&data[1]);
        } else if (data[0] == 0x00u) {
            for (uint8_t i = 1u; i < 8u; i++) {
                if (data[i] != 0u) {
                    err = ERR_DATA; /* 触发=00 时位图必须全 0 */
                    break;
                }
            }
        } else {
            err = ERR_DATA;
        }

        if (err != 0u) {
            if (reply) {
                send_exception(station, func, err);
            }
            return;
        }
        if (reply) {
            s_send_04[0] = station;
            s_send_04[4] = data[0];
            bitmap_read(&s_send_04[5]); /* 位图 = 当前显存回读值 */
            frame_send(s_send_04, (uint8_t)sizeof(s_send_04));
        }
        return;
    }

    case FUNC_CFG: { /* 配置参数（§5.3） */
        uint8_t err;
        uint16_t value;

        if (n != 3u) {
            if (reply) {
                send_exception(station, func, ERR_LENGTH);
            }
            return;
        }
        value = (uint16_t)(((uint16_t)data[1] << 8) | data[2]);
        err = cfg_set(data[0], value);
        if (err != 0u) {
            if (reply) {
                send_exception(station, func, err);
            }
            return;
        }
        if (reply) {
            cfg_get(data[0], &value); /* 回显已生效的新值（§11.3-6） */
            s_send_06[0] = station;
            s_send_06[4] = data[0];
            s_send_06[5] = (uint8_t)(value >> 8);
            s_send_06[6] = (uint8_t)(value & 0xFFu);
            frame_send(s_send_06, (uint8_t)sizeof(s_send_06));
        }
        if (data[0] != ADDR_MODE) {
            (void)cfg_store(); /* 站号/亮度/超时/动作 落盘，擦写期间扫描短暂卡顿 */
        }
        return;
    }

    default:
        return; /* 不认识的功能码静默丢弃（§3/§5） */
    }
}

/* ============================== 收帧状态机 =============================== */
static void rx_byte(uint8_t b, uint32_t now)
{
    uint32_t gap = now - s_rx_last_ms;

    if (s_rx_active && gap > PROTO_BYTE_GAP_MS) {
        s_rx_active = false; /* 丢半帧，靠下一次 ≥3ms 静默重新判头（§11.3-1） */
    }

    if (!s_rx_active) {
        if (gap < PROTO_SILENCE_MS) {
            s_rx_last_ms = now; /* 帧内续字节，继续等静默 */
            return;
        }
        if (b != s_cfg.station && b != 0x00u) {
            s_rx_last_ms = now; /* 别的站（含其它从站的应答），整帧跳过 */
            return;
        }
        s_rx_active = true;
        s_rx_len = 0u;
        s_rx_expect = 0u;
    }

    s_rx[s_rx_len++] = b;
    s_rx_last_ms = now;

    if (s_rx_len == 4u) {
        uint16_t n = (uint16_t)(((uint16_t)s_rx[2] << 8) | s_rx[3]);

        if (n > PROTO_MAX_DATA) {
            s_rx_active = false; /* 长度非法 → 丢弃（§3） */
            return;
        }
        s_rx_expect = (uint8_t)(PROTO_HDR_LEN + n + PROTO_CRC_LEN);
    }

    if (s_rx_expect != 0u && s_rx_len == s_rx_expect) {
        uint16_t crc = proto_crc16(s_rx, (uint16_t)(s_rx_expect - PROTO_CRC_LEN));

        s_rx_active = false;
        if (s_rx[s_rx_expect - 2u] == (uint8_t)(crc >> 8) &&
            s_rx[s_rx_expect - 1u] == (uint8_t)(crc & 0xFFu)) {
            dispatch();
        }
    }
}

/* ============================== 通信看门狗 =============================== */
static void watchdog_poll(uint32_t now)
{
    uint8_t action;

    if (s_cfg.mode == (uint8_t)PROTO_MODE_DEMO || s_cfg.wd_timeout == 0u) {
        return;
    }
    if ((now - s_wd_last_ms) < s_cfg.wd_timeout) {
        return;
    }
    s_wd_last_ms = now;

    action = (s_cfg.mode == (uint8_t)PROTO_MODE_REMOTE_WD) ? 1u : s_cfg.wd_action;
    if (action == 0u) {
        mode_set((uint8_t)PROTO_MODE_DEMO);
    } else if (action == 1u) {
        led_matrix_clear();
    }
    /* action == 2 保持：什么都不做 */
}

/* ================================= 对外 API =============================== */
void proto_init(void)
{
    s_cfg.mode      = (uint8_t)PROTO_MODE_DEMO;
    s_cfg.station   = PROTO_STATION_DEFAULT;
    s_cfg.brightness = PROTO_BRIGHTNESS_DEFAULT;
    s_cfg.wd_timeout = PROTO_WD_TIMEOUT_DEFAULT;
    s_cfg.wd_action  = PROTO_WD_ACTION_DEFAULT;

    cfg_load(); /* 站号/亮度/超时/动作，模式不落盘，上电固定本地演示 */

    s_demo_fx    = led_effect_get();
    s_rx_active  = false;
    s_rx_len     = 0u;
    s_rx_expect  = 0u;
    s_rx_last_ms = sys_time_ms() - PROTO_SILENCE_MS; /* 开机总线视为已静默 */
    s_wd_last_ms = sys_time_ms();

    led_matrix_set_brightness(s_cfg.brightness);
}

void proto_poll(void)
{
    uint8_t budget = PROTO_RX_BUDGET;
    uint8_t byte;

    while (budget != 0u && rs485_receive(&byte, 1u, 0u) == 1) {
        rx_byte(byte, sys_time_ms());
        budget--;
    }

    watchdog_poll(sys_time_ms());
}

uint8_t proto_station(void)
{
    return s_cfg.station;
}

uint8_t proto_mode(void)
{
    return s_cfg.mode;
}
