/* proto.c 主机侧协议联调：喂 RS485_Protocol_Spec_V1.0.md §6 示例帧，对拍应答，
 * 覆盖 CRC 对拍、模式矩阵、异常帧、广播、半帧重同步、看门狗、Flash 掉电重载。
 * 移植到新 MCU 后建议先在 PC 上把这套用例跑绿，再上板。
 * 在本目录下跑：
 *   python -m ziglang cc -std=c11 -Wall -I..\..\core -I..\..\port test_main.c ..\..\core\proto.c -o proto_test.exe
 *   proto_test.exe          （全绿输出 "N passed, 0 failed"）
 * 桩只替换 rs485 收发 / 配置区 Flash / 时间 / led_matrix / led_effect，
 * 其余头文件用工程原件（core/、port/），保证和上板代码是同一份。
 */
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "proto.h"
#include "rs485.h"
#include "port_flash.h"
#include "led_matrix.h"
#include "led_effect.h"
#include "sys_time.h"

/* ------------------------- flash 桩（配置区一页 128B） ------------------------- */
uint8_t g_fake_flash[128];

bool port_flash_load(void *buf, uint16_t len)
{
    if (buf == NULL || len == 0u || len > sizeof(g_fake_flash)) {
        return false;
    }
    memcpy(buf, g_fake_flash, len);
    return true;
}

bool port_flash_save(const void *buf, uint16_t len)
{
    if (buf == NULL || len == 0u || len > sizeof(g_fake_flash)) {
        return false;
    }
    memset(g_fake_flash, 0xFF, sizeof(g_fake_flash));
    memcpy(g_fake_flash, buf, len);
    return true;
}

/* ------------------------- 时间桩 ------------------------- */
static uint32_t g_ms = 1000;
uint32_t sys_time_ms(void) { return g_ms; }

/* ------------------------- rs485 桩 ------------------------- */
static uint8_t g_tx[64];
static int     g_tx_len = -1;
static uint8_t g_rx[512];
static int     g_rx_n = 0;

int rs485_receive(uint8_t *data, size_t max_len, uint32_t timeout_ms)
{
    (void)timeout_ms;
    if (g_rx_n == 0 || max_len == 0) {
        return 0;
    }
    *data = g_rx[0];
    memmove(g_rx, g_rx + 1, (size_t)--g_rx_n);
    return 1;
}

rs485_err_t rs485_send(const uint8_t *data, size_t len)
{
    if (len > sizeof(g_tx)) {
        len = sizeof(g_tx);
    }
    memcpy(g_tx, data, len);
    g_tx_len = (int)len;
    return RS485_OK;
}

/* ----------------------- led_matrix 桩 ----------------------- */
static uint8_t g_fb[7];
static int     g_clear_cnt = 0;
static uint8_t g_bright = 100;

void led_matrix_clear(void)
{
    memset(g_fb, 0, sizeof(g_fb));
    g_clear_cnt++;
}

void led_matrix_set_index(uint16_t index, bool on)
{
    if (index >= LED_MATRIX_COUNT) {
        return;
    }
    if (on) {
        g_fb[index >> 3] |= (uint8_t)(0x80u >> (index & 7u));
    } else {
        g_fb[index >> 3] &= (uint8_t)~(0x80u >> (index & 7u));
    }
}

bool led_matrix_get_index(uint16_t index)
{
    if (index >= LED_MATRIX_COUNT) {
        return false;
    }
    return (g_fb[index >> 3] & (uint8_t)(0x80u >> (index & 7u))) != 0u;
}

void led_matrix_set_brightness(uint8_t percent)
{
    g_bright = percent > 100u ? 100u : percent;
}

uint8_t led_matrix_get_brightness(void) { return g_bright; }

/* ------------------------ led_effect 桩 ------------------------ */
static led_effect_t g_fx = LED_FX_WALK;
void led_effect_set(led_effect_t fx) { g_fx = fx; }
led_effect_t led_effect_get(void) { return g_fx; }

/* --------------------------- 测试 --------------------------- */
static int g_fail = 0;
static int g_pass = 0;

static int hex2bytes(const char *s, uint8_t *out, int max)
{
    int n = 0;
    while (*s != '\0' && n < max) {
        unsigned v = 0;
        while (*s == ' ') {
            s++;
        }
        if (*s == '\0') {
            break;
        }
        if (sscanf(s, "%2x", &v) != 1) {
            break;
        }
        out[n++] = (uint8_t)v;
        s += 2;
    }
    return n;
}

static void check_hex(const char *tag, const uint8_t *buf, int len, const char *want)
{
    uint8_t w[64];
    int wn = hex2bytes(want, w, (int)sizeof(w));

    if (len == wn && memcmp(buf, w, (size_t)wn) == 0) {
        g_pass++;
        return;
    }
    g_fail++;
    printf("FAIL %s\n  got :", tag);
    if (len < 0) {
        printf(" <no reply>");
    }
    for (int i = 0; i < len; i++) {
        printf(" %02X", buf[i]);
    }
    printf("\n  want: %s\n", want);
}

static void check_int(const char *tag, long got, long want)
{
    if (got == want) {
        g_pass++;
        return;
    }
    g_fail++;
    printf("FAIL %s: got %ld want %ld\n", tag, got, want);
}

static int build(uint8_t *out, uint8_t station, uint8_t func, const uint8_t *data, int n)
{
    uint16_t crc;
    out[0] = station;
    out[1] = func;
    out[2] = (uint8_t)((n >> 8) & 0xFF);
    out[3] = (uint8_t)(n & 0xFF);
    if (n > 0) {
        memcpy(&out[4], data, (size_t)n);
    }
    crc = proto_crc16(out, (uint16_t)(4 + n));
    out[4 + n] = (uint8_t)(crc >> 8);
    out[5 + n] = (uint8_t)(crc & 0xFF);
    return 6 + n;
}

/* 逐字节喂给 proto_poll，帧间留 3ms 静默（§8 帧间隔） */
static void send_raw(const uint8_t *frame, int len, int split_at, int stall_ms)
{
    g_ms += 3u;
    g_tx_len = -1;
    for (int i = 0; i < len; i++) {
        if (i == split_at) {
            g_ms += (uint32_t)stall_ms;
        }
        g_rx[g_rx_n++] = frame[i];
        proto_poll();
    }
    proto_poll();
}

static void send_hex(const char *s)
{
    uint8_t f[64];
    int n = hex2bytes(s, f, (int)sizeof(f));
    send_raw(f, n, -1, 0);
}

static void send_build(const char *tag, uint8_t st, uint8_t fn, const uint8_t *data, int n)
{
    uint8_t f[64];
    int len = build(f, st, fn, data, n);
    send_raw(f, len, -1, 0);
    if (g_tx_len < 0) {
        g_fail++;
        printf("FAIL %s: no reply\n", tag);
    } else {
        g_pass++;
    }
}

static void send_build_quiet(const char *tag, uint8_t st, uint8_t fn, const uint8_t *data, int n)
{
    uint8_t f[64];
    int len = build(f, st, fn, data, n);
    send_raw(f, len, -1, 0);
    if (g_tx_len >= 0) {
        g_fail++;
        printf("FAIL %s: unexpected reply len=%d\n", tag, g_tx_len);
    } else {
        g_pass++;
    }
}

int main(void)
{
    static const uint8_t all_on[8] = { 0x01, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF };
    uint8_t d[8];
    uint8_t f[64];
    int len;

    memset(g_fake_flash, 0xFF, sizeof(g_fake_flash));
    proto_init();

    check_int("boot mode", proto_mode(), 0);
    check_int("boot station", proto_station(), 1);
    check_int("boot brightness", g_bright, 100);

    /* T1 资源列表（§6 示例帧） */
    send_hex("01 01 00 00 18 50");
    check_hex("T1 rlist", g_tx, g_tx_len, "01 02 00 07 01 00 00 00 31 00 00 3B A3");

    /* T2 模式 0 拒绝 0x03（§11.3-2） */
    send_hex("01 03 00 08 00 00 00 00 00 00 00 00 45 F1");
    check_hex("T2 mode0 reject", g_tx, g_tx_len, "01 83 00 01 04 17 30");

    /* T3 切模式 1：回显 + 先清屏 + 切 LED_FX_REMOTE */
    send_hex("01 06 00 03 00 00 01 22 CA");
    check_hex("T3 mode1 echo", g_tx, g_tx_len, "01 06 00 03 00 00 01 22 CA");
    check_int("T3 mode", proto_mode(), 1);
    check_int("T3 cleared once", g_clear_cnt, 1);
    check_int("T3 fx remote", g_fx == LED_FX_REMOTE, 1);
    check_int("T3 mode not stored", g_fake_flash[0] | (g_fake_flash[1] << 8), 0xFFFF);

    /* T4 写全亮（§6 示例帧的请求；应答按 §4 只回灯位，D6=0x80 只有灯48） */
    send_hex("01 03 00 08 01 FF FF FF FF FF FF FF 09 3E");
    check_hex("T4 write all", g_tx, g_tx_len, "01 04 00 08 01 FF FF FF FF FF FF 80 9D 65");

    /* T5 只亮灯 0/48（§6 示例帧） */
    send_hex("01 03 00 08 01 80 00 00 00 00 00 80 E1 B0");
    check_hex("T5 write 0/48", g_tx, g_tx_len, "01 04 00 08 01 80 00 00 00 00 00 80 95 AA");

    /* T6 读当前灯（§6 示例帧，回读 T5 结果） */
    send_hex("01 03 00 08 00 00 00 00 00 00 00 00 45 F1");
    check_hex("T6 read", g_tx, g_tx_len, "01 04 00 08 00 80 00 00 00 00 00 80 59 6B");

    /* T7 触发事件非法 */
    memset(d, 0, sizeof(d));
    d[0] = 0x02;
    send_build("T7 bad trigger", 0x01, 0x03, d, 8);
    check_hex("T7 frame", g_tx, g_tx_len, "01 83 00 01 02 15 B0");

    /* T8 触发 00 但位图非 0 */
    memset(d, 0, sizeof(d));
    d[1] = 0xFF;
    send_build("T8 nonzero bitmap", 0x01, 0x03, d, 8);
    check_hex("T8 frame", g_tx, g_tx_len, "01 83 00 01 02 15 B0");

    /* T9 0x03 长度不符 */
    memset(d, 0, sizeof(d));
    send_build("T9 0x03 bad len", 0x01, 0x03, d, 4);
    check_hex("T9 frame", g_tx, g_tx_len, "01 83 00 01 03 D5 71");

    /* T10 亮度 = 100（§6 示例帧） */
    send_hex("01 06 00 03 01 00 64 C9 5B");
    check_hex("T10 bright echo", g_tx, g_tx_len, "01 06 00 03 01 00 64 C9 5B");
    check_int("T10 bright val", g_bright, 100);

    /* T11 亮度越界 → 0x86 + 0x02 */
    d[0] = 0x01;
    d[1] = 0x00;
    d[2] = 0xC8;
    send_build("T11 bright range", 0x01, 0x06, d, 3);
    check_hex("T11 frame", g_tx, g_tx_len, "01 86 00 01 02 D9 B0");

    /* T12 参数地址不存在 → 0x86 + 0x03（§6「异常-参数」示例帧） */
    d[0] = 0x05;
    d[1] = 0x00;
    d[2] = 0x00;
    send_build("T12 bad addr", 0x01, 0x06, d, 3);
    check_hex("T12 frame", g_tx, g_tx_len, "01 86 00 01 03 19 71");

    /* T13 0x06 长度不符 */
    d[0] = 0x00;
    d[1] = 0x01;
    send_build("T13 0x06 bad len", 0x01, 0x06, d, 2);
    check_hex("T13 frame", g_tx, g_tx_len, "01 86 00 01 03 19 71");

    /* T14 改站号 1→7，写入即生效 + 落盘 */
    send_hex("01 06 00 03 04 00 07 E1 0B");
    check_hex("T14 station echo", g_tx, g_tx_len, "01 06 00 03 04 00 07 E1 0B");
    check_int("T14 station", proto_station(), 7);
    check_int("T14 magic", g_fake_flash[0] | (g_fake_flash[1] << 8), 0xA55A);
    check_int("T14 flash station", g_fake_flash[2] | (g_fake_flash[3] << 8), 7);

    send_hex("07 01 00 00 90 50");
    check_hex("T14 new station", g_tx, g_tx_len, "07 02 00 07 01 00 00 00 31 00 00 B3 BD");

    /* 旧站号必须静默 */
    send_build_quiet("T14 old station", 0x01, 0x01, NULL, 0);

    /* T15 不认识的功能码静默丢弃 */
    send_build_quiet("T15 unknown func", 0x07, 0x05, NULL, 0);

    /* T16 CRC 错静默丢弃 */
    send_hex("07 01 00 00 90 51");
    check_int("T16 no reply", g_tx_len < 0, 1);

    /* T17 广播写灯：执行但不回帧 */
    send_build_quiet("T17 broadcast", 0x00, 0x03, all_on, 8);
    check_int("T17 fb all on", g_fb[0], 0xFF);

    /* T18 字节间断 >5ms → 丢半帧（§11.3-1）；主站随后重试（§8 ≤20ms）可恢复 */
    len = build(f, 0x07, 0x01, NULL, 0);
    send_raw(f, len, 3, 10);
    check_int("T18 half frame dropped", g_tx_len < 0, 1);
    g_ms += 20; /* 主站超时后重发，间隔 >5ms → 重新判头 */
    send_hex("07 01 00 00 90 50");
    check_hex("T18 recover", g_tx, g_tx_len, "07 02 00 07 01 00 00 00 31 00 00 B3 BD");

    /* T19 看门狗：100ms 无有效帧 → 恢复本地演示 */
    d[0] = 0x02;
    d[1] = 0x00;
    d[2] = 0x64;
    send_build("T19 wd timeout", 0x07, 0x06, d, 3);
    d[0] = 0x00;
    d[1] = 0x00;
    d[2] = 0x01;
    send_build("T19 mode 1", 0x07, 0x06, d, 3);
    check_int("T19 mode remote", proto_mode(), 1);
    g_ms += 150;
    proto_poll();
    check_int("T19 watchdog -> demo", proto_mode(), 0);
    check_int("T19 demo fx", g_fx, LED_FX_WALK);

    /* T20 模式 2 超时 → 全灭 */
    d[0] = 0x00;
    d[1] = 0x00;
    d[2] = 0x02;
    send_build("T20 mode 2", 0x07, 0x06, d, 3);
    check_int("T20 mode 2", proto_mode(), 2);
    check_int("T20 cleared on 0->2", g_clear_cnt, 2);
    send_build_quiet("T20 write all", 0x00, 0x03, all_on, 8);
    check_int("T20 fb on", g_fb[0], 0xFF);
    g_ms += 200;
    proto_poll();
    check_int("T20 fb cleared", g_fb[0] | g_fb[1] | g_fb[2] | g_fb[3] | g_fb[4] | g_fb[5] | g_fb[6], 0);
    check_int("T20 still remote", proto_mode(), 2);

    /* T21 亮度落盘 + 重启加载 */
    d[0] = 0x01;
    d[1] = 0x00;
    d[2] = 0x28;
    send_build("T21 bright 40", 0x07, 0x06, d, 3);
    check_int("T21 bright", g_bright, 40);
    check_int("T21 flash brightness", g_fake_flash[4] | (g_fake_flash[5] << 8), 40);

    proto_init();
    check_int("T21 reload station", proto_station(), 7);
    check_int("T21 reload brightness", g_bright, 40);
    check_int("T21 reload mode", proto_mode(), 0);

    printf("%d passed, %d failed\n", g_pass, g_fail);
    return g_fail == 0 ? 0 : 1;
}
