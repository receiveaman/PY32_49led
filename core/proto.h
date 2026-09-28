#ifndef PROTO_H
#define PROTO_H

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * L5 协议层（从站 = 本板）
 * 依据 RS485_Protocol_Spec_V1.0.md 第 9 章 TODO 与第 11 章六条决策实现。
 * 用法：
 *   proto_init();                       // rs485_init() 之后
 *   while (1) { led_effect_update(); led_matrix_scan(); proto_poll(); }
 * proto_poll() 非阻塞：逐字节喂状态机（3ms 判头 → 按长度收满 → CRC → 分发），
 * 只在回帧时阻塞约 1.3ms（§11.3-5，靠 led_matrix_scan() 补扫）。
 */

/* 出厂默认值（§5.3） */
#define PROTO_STATION_DEFAULT    1u
#define PROTO_BRIGHTNESS_DEFAULT 100u
#define PROTO_WD_TIMEOUT_DEFAULT 500u   /* 0 = 关闭，100~60000 ms */
#define PROTO_WD_ACTION_DEFAULT  0u     /* 0 恢复演示 / 1 全灭 / 2 保持 */

/* 帧固定上限（§3：N ≤ 8 → 帧长 ≤ 14） */
#define PROTO_MAX_DATA  8u
#define PROTO_FRAME_MAX 14u

typedef enum {
    PROTO_MODE_DEMO      = 0, /* 本地演示 */
    PROTO_MODE_REMOTE    = 1, /* RS485 受控，效果层停摆 */
    PROTO_MODE_REMOTE_WD = 2  /* 受控，通信看门狗超时全灭 */
} proto_mode_t;

/* 载入 Flash 配置、复位状态机与看门狗（默认本地演示） */
void proto_init(void);

/* 主循环里调用：收帧分发 + 通信看门狗，非阻塞 */
void proto_poll(void);

uint8_t proto_station(void);
uint8_t proto_mode(void);

/* CRC16/MODBUS，多项式 0xA001、初值 0xFFFF；结果高字节在前写入帧（§3/§6 对拍用） */
uint16_t proto_crc16(const uint8_t *data, uint16_t len);

#ifdef __cplusplus
}
#endif

#endif /* PROTO_H */
