# PY32F002B 49LED 灯板固件

7 × 7 = **49 颗 LED 点阵板**的固件，运行在 **PY32F002BF15**（Cortex-M0+，3KB RAM）上。
本板作为 **RS485 从站**，由另一块控制板（主站）通过 RS485 下发指令控制 49 颗灯的亮灭、亮度与运行模式。

## 硬件

| 项目 | 说明 |
|---|---|
| MCU | PY32F002BF15，24MHz HSI，3KB RAM / 16KB Flash |
| LED 驱动 | 7×7 点阵，**8 个 GPIO 查理复用**逐点扫描（任意时刻最多点亮 1 颗） |
| 灯位编号 | 0~48 行优先，`编号 = row*7 + col`，实物左上角 = 0 |
| RS485 | MAX3485：PB4=TX、PB5=RX、PB3=/RE+DE，**115200 8N1** |
| 接口引脚约束 | PA0/PA1/PA3~PA5、PB0~PB2 已被灯阵占用 |

## 目录结构（分层，core 与 MCU 无关）

```
app/      应用专属：main、中断、系统时钟（移植时重写）
core/     与 MCU 无关的业务逻辑（移植时整体带走）
  proto.c/.h          L5  RS485 协议从站：收帧状态机、CRC16、0x01/0x03/0x04/0x06 分发、异常帧、通信看门狗
  led_effect.c/.h     L4  演示效果（受控模式下停摆，防覆盖显存）
  led_matrix.c/.h     L3  49 灯显存 + 时分扫描 + 亮度占空比
  led_map.c/.h        L3  灯位 → 阳极/阴极引脚映射
  rs485.c/h           L2  485 收发原语（自动收发方向、帧间静默）
port/     MCU 相关的移植层（换芯片只改这 4 对文件）
  port_flash.c/.h     配置掉电保存（站号/亮度/看门狗）
  rs485_port.c/.h     UART + 收发缓冲 + 中断
  led_port.c/.h       8 个推挽 GPIO 的电平/高阻
  sys_time.c/.h       1ms 时基（SysTick）
Startup/ Ld/  启动文件与链接脚本
tools/
  proto_test/         PC 端协议单元测试（test_main.c）
  rs485_regression.ps1  上位机 7 条指令回归脚本
```

## RS485 协议速览

完整定义见 [`RS485_Protocol_Spec_V1.0.md`](RS485_Protocol_Spec_V1.0.md)，可直接抄帧的速查见 [`RS485_Command_Reference.md`](RS485_Command_Reference.md)。

**帧格式**（固定长度，最长 14 字节，不依赖任何协议库）：

```
站号(1) | 功能码(1) | 数据长度H(1) | 数据长度L(1) | 数据区(N≤8) | CRC16H(1) | CRC16L(1)
```

CRC16/MODBUS（多项式 0xA001，初值 0xFFFF），覆盖站号→数据区，**高字节在前发送**；帧间隔 ≥3ms。

| 功能码 | 数据区 | 用途 |
|---|---|---|
| 0x01 → 0x02 | 空 → 7B | 查询资源列表（输出点数=49） |
| 0x03 → 0x04 | 8B = 触发事件(1) + 49 位位图(7) | 触发=0x00 只回读，=0x01 写入并回读 |
| 0x06 → 0x06 回显 | 3B = 地址(1) + 值(2) | 写配置：模式/亮度/看门狗/超时动作/站号 |
| 0x83 / 0x86 | 1B 错误码 | 异常帧（原功能码 \| 0x80） |

**常用帧（可直接发）**：

```
切入 RS485 受控模式:  01 06 00 03 00 00 01 22 CA      ← 上电默认模式 0，必须先发这条
49 灯全亮:            01 03 00 08 01 FF FF FF FF FF FF FF 09 3E
只亮灯 0 和灯 48:     01 03 00 08 01 80 00 00 00 00 00 80 E1 B0
读当前灯状态:         01 03 00 08 00 00 00 00 00 00 00 00 45 F1
```

> **上电默认停在本地演示模式（模式 0），写灯会被拒绝**（回 `01 83 00 01 04 ...`），
> 所以主站每次上电先发受控模式帧；看门狗默认 500ms，受控期间需周期发帧保活。

## 快速开始

环境准备（VSCode + Embedded IDE 插件 + GCC ARM + OpenOCD/pyOCD）详见 [`环境配置教程.md`](环境配置教程.md)。

```bash
git clone https://github.com/receiveaman/PY32_49led.git
```

- **`.pack/`（Puya DFP）和 `tools/openocd` 未入库**（体积原因），首次打开工程时 EIDE 会提示自动下载；离线可用教程里的地址手动装。
- VSCode 打开工程 → 左侧 EIDE 图标 → 构建（Build）→ 烧录（Upload，默认 pyOCD，也可切 OpenOCD/JLink）。

## 测试

- **PC 单元测试**：`tools/proto_test/test_main.c`，覆盖 CRC、收帧状态机、各功能码分支。
- **整机回归**：连好 485 转接器后执行 `tools/rs485_regression.ps1`，自动跑 7 条指令并校验应答。
- 硬件联调注意：PC 侧串口 COMx / 115200，回显自测宏 `APP_ECHO_SELFTEST` 默认关闭（见 `app/main.c`）。

## 文档索引

| 文档 | 内容 |
|---|---|
| [`RS485_Protocol_Spec_V1.0.md`](RS485_Protocol_Spec_V1.0.md) | 协议规范：帧格式、CRC、功能码、位图、时序、实现 TODO 与交接事项 |
| [`RS485_Command_Reference.md`](RS485_Command_Reference.md) | 指令速查：逐条可抄的收发帧 + 错误码对照 |
| [`PORTING.md`](PORTING.md) | 移植指南：换 MCU 只写 `port/` 下 4 对文件 |
| [`环境配置教程.md`](环境配置教程.md) | VSCode + EIDE + GCC + 烧录工具环境搭建 |
| `protocol_main&module_RS485_V1.8.9.pdf-*/` | 协议基线文档（huayiDF 主控↔子模块协议 PDF 转换稿） |
| `n49-查理复用IO映射标.xlsx` | 灯位 ↔ GPIO 映射表（`led_map.c` 与之一致） |
