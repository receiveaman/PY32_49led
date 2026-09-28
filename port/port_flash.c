/**
 * 配置区掉电保存 —— PY32F002B 实现（Flash 最后一页，128 字节）
 * 换 MCU 时只需重写本文件，port/port_flash.h 与 core/proto.c 保持不动。
 */
#include <string.h>
#include "port_flash.h"
#include "py32f002b_ll_flash.h"
#include "sys_time.h"

#define CFG_FLASH_ADDR ((uintptr_t)(FLASH_BASE + FLASH_SIZE - FLASH_PAGE_SIZE))
#define CFG_WORDS      (FLASH_PAGE_SIZE / 4u)

static bool flash_idle(void)
{
    uint32_t start = sys_time_ms();

    while (LL_FLASH_IsActiveFlag_BUSY(FLASH) != 0u) {
        if ((sys_time_ms() - start) > 100u) {
            return false;
        }
    }
    return true;
}

bool port_flash_load(void *buf, uint16_t len)
{
    if (buf == 0 || len == 0u || len > PORT_FLASH_CFG_SIZE) {
        return false;
    }
    memcpy(buf, (const void *)CFG_FLASH_ADDR, len);
    return true;
}

bool port_flash_save(const void *buf, uint16_t len)
{
    static uint32_t page[CFG_WORDS];
    bool ok;

    if (buf == 0 || len == 0u || len > sizeof(page)) {
        return false;
    }
    memset(page, 0xFF, sizeof(page));
    memcpy(page, buf, len);

    if (!flash_idle()) {
        return false;
    }
    LL_FLASH_Unlock(FLASH);
    LL_FLASH_TIMMING_SEQUENCE_CONFIG_24M();

    LL_FLASH_ClearFlag_EOP(FLASH);
    LL_FLASH_EnablePageErase(FLASH);
    LL_FLASH_SetEraseAddress(FLASH, CFG_FLASH_ADDR);
    ok = flash_idle();
    LL_FLASH_DisablePageErase(FLASH);

    if (ok) {
        LL_FLASH_ClearFlag_EOP(FLASH);
        LL_FLASH_EnablePageProgram(FLASH);
        LL_FLASH_PageProgram(FLASH, CFG_FLASH_ADDR, page);
        ok = flash_idle();
        LL_FLASH_DisablePageProgram(FLASH);
    }
    LL_FLASH_Lock(FLASH);
    return ok;
}
