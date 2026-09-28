#ifndef PORT_FLASH_H
#define PORT_FLASH_H

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * L1 移植层 —— 配置区掉电保存（站号 / 亮度 / 看门狗）
 *
 * core/proto.c 只认这两个函数，不关心 Flash 型号、页大小、基地址。
 * 换 MCU 时保留本头文件、重写同名 port_flash.c 即可（本板实现约 60 行）。
 *
 * 约定：
 *  - 配置区 = 一块独立的 Flash 存储，出厂内容为 0xFF（本板取最后一页，128 字节）
 *  - 区域未编程 / 读失败 → load 返回 false，调用方自行退回出厂默认值
 *  - save 负责"先擦后写"，擦写期间核会停顿，只能在主循环上下文调用
 *  - len ≤ PORT_FLASH_CFG_SIZE，且必须一次调用放下整个配置结构
 */
#define PORT_FLASH_CFG_SIZE 128u

/* 读配置区前 len 字节；区域无效/超长返回 false */
bool port_flash_load(void *buf, uint16_t len);

/* 擦除配置区并写入前 len 字节；成功返回 true */
bool port_flash_save(const void *buf, uint16_t len);

#ifdef __cplusplus
}
#endif

#endif /* PORT_FLASH_H */
