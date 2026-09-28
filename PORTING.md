# 49LED 固件移植指南（换 MCU / 换 RS485 方案）

> 目标：把这套固件搬到另一颗 MCU（同样两线 RS485 从站 + 49 灯点阵）上，
> **只重写 `port/` 下的 4 个文件，`core/` 一个字都不用改。**

---

## 1. 目录结构与分层

```
49LED/
├── core/                 ← 与 MCU 无关，整包拷走即可（不要改）
│   ├── proto.c/.h            L5  协议从站：收帧状态机、CRC16、0x01/0x03/0x04/0x06 分发、
│   │                          异常帧、看门狗、配置校验
│   ├── rs485.c/.h            L2  485 收发原语：阻塞发送（自动拉 DE）、非阻塞接收、配置
│   ├── led_matrix.c/.h       L3  49 灯显存 + 时分扫描 + 亮度占空比
│   ├── led_map.c/.h          L3  线性序号 ↔ 查理复用行列映射
│   └── led_effect.c/.h       L4  演示效果动画（受控模式下停摆）
│
├── port/                 ← 换 MCU 只动这里（4 个 .c 重写，4 个 .h 保留）
│   ├── port_flash.c/.h       配置区掉电保存（站号/亮度/看门狗）
│   ├── rs485_port.c/.h       UART + 收发方向脚 + 接收中断
│   ├── led_port.c/.h         8 根查理复用 GPIO 的电平/方向
│   └── sys_time.c/.h         1ms 时基（SysTick）
│
├── app/                  ← 本板专属：main、中断向量转发、系统时钟（照着重写）
├── Startup/  Ld/  .pack/ ← 启动文件、链接脚本、厂商 DFP（新工程自带）
└── tools/
    ├── proto_test/           PC 上跑的协议单元测试（57 个断言）
    └── rs485_regression.ps1  上板 7 条指令回归脚本
```

依赖方向是单向的：`app → core → port`。`core/` 里**不含任何厂商头文件**，
唯一例外是 `core/rs485.h` 的配置宏（纯数值，可留可改）。

---

## 2. 拷贝清单

**拷到新工程**：`core/` 全部 + `port/` 的 4 个 `.h` + `port/` 的 4 个 `.c`（拷来当模板改）+ `tools/`。
**不要拷**：`app/`、`Startup/`、`Ld/`、`.pack/`、`build/`（都是本板的）。

新工程 include 路径加 3 条：`core`、`port`、你自己的应用目录。

---

## 3. 必须重写的 4 个文件

### 3.1 `port/sys_time.c` —— 1ms 时基（约 20 行）

```c
void     sys_time_init(void);        /* 开 SysTick，1ms 一次 */
void     sys_time_on_tick(void);     /* SysTick 中断里调用（中断计数） */
uint32_t sys_time_ms(void);          /* 开机毫秒数，允许回绕，必须是 1ms 单调递增 */
void     sys_time_delay_ms(uint32_t ms);   /* 忙等，供 485 层收尾用 */
```
> 要求：`sys_time_ms()` 精度 1ms 即可；协议层用它做 3ms 判头、5ms 字节间断、看门狗超时。

### 3.2 `port/rs485_port.c` —— UART + 方向脚（约 200 行，工作量最大）

```c
rs485_err_t rs485_port_init(const rs485_config_t *cfg);  /* UART 115200 8N1 + DE 脚 + 开 RX 中断 */
rs485_err_t rs485_port_deinit(void);
rs485_err_t rs485_port_set_tx(bool tx_enable);           /* DE/=RE 高=发送 低=接收，含 100us 建立时间 */
rs485_err_t rs485_port_lock(uint32_t timeout_ms);        /* 自旋锁，单主环境可做成 no-op */
rs485_err_t rs485_port_unlock(void);
int         rs485_port_write(const uint8_t *data, size_t len);   /* 字节写入 TX，返回已写字节数 */
rs485_err_t rs485_port_wait_tx_done(uint32_t timeout_ms);        /* 等 TC/发送移位寄存器空 */
int         rs485_port_read(uint8_t *data, size_t len, uint32_t timeout_ms); /* 从 RX 环形缓冲取 */
uint32_t    rs485_port_rx_pending(void);                          /* 缓冲里还有多少字节 */
void        rs485_port_delay_ms(uint32_t ms);
void        rs485_port_delay_us(uint32_t us);
void        rs485_port_irq_handler(void);                /* UART RX 中断里调用：读 DR → 压环形缓冲 */
```
要点：
- **接收必须走中断 + 环形缓冲**（本板缓冲 `RS485_RX_BUF_SIZE = 1024` 字节，占用 RAM 的大头）。
- 本板引脚：PB4=USART1_TX→MAX3485 DI，PB5=USART1_RX←RO，PB3=/RE+DE 短接（高=发）。换板照抄这个逻辑换引脚即可。
- `rs485_port_read(..., timeout_ms=0)` 必须**不阻塞**（`core/proto.c` 每次只喂 1 字节、timeout=0，为的是不卡 LED 扫描）。

### 3.3 `port/led_port.c` —— 8 根查理复用引脚（约 70 行）

```c
void led_port_init(void);                       /* 时钟 + 8 脚初始化为高阻（=全灭） */
void led_port_release(void);                    /* 8 脚全高阻 */
void led_port_level(led_pin_t pin, bool high);  /* 只写输出寄存器，不改方向 */
void led_port_dir(led_pin_t pin, bool output);  /* 推挽输出 / 高阻输入 */
void led_port_drive(led_pin_t anode, led_pin_t cathode); /* 点亮一对脚，其余 6 脚高阻 */
```
> 引脚编号 `led_pin_t` 在 `port/led_port.h`，与 `.c` 里的硬件表一一对应；
> 行/列 → 引脚的映射在 `core/led_map.c`（与 MCU 无关，不用改）。

### 3.4 `port/port_flash.c` —— 配置区保存（约 60 行）

```c
bool port_flash_load(void *buf, uint16_t len);   /* 读配置区，未编程/失败返回 false */
bool port_flash_save(const void *buf, uint16_t len);  /* 先擦后写，成功返回 true */
```
> 只要求"有一块独立区域 + 整页擦 + 写"。本板取 Flash 最后一页（`0x08001F80`，128 字节）。
> `core/proto.c` 会自己校验魔数 `0xA55A` 和字段范围，读到垃圾自动退回默认值，所以实现可以很朴素。

---

## 4. `app/` 要写什么

| 文件 | 作用 |
|---|---|
| `app/main.c` | 系统时钟 → `sys_time_init()` → `led_matrix_init()` → `led_effect_init()` → `rs485_init()` → `proto_init()`，然后死循环 |
| 中断转发 | `SysTick_Handler → sys_time_on_tick()`；`UARTx_IRQHandler → rs485_port_irq_handler()` |
| 启动文件/链接脚本 | 新工程自带，注意向量表里放对这两个 handler |

主循环必须是这个形状（顺序别改）：

```c
while (1) {
    led_effect_update();   /* 效果层：受控模式下自己什么都不做 */
    led_matrix_scan();     /* 扫描：调用越频繁灯越亮，必须持续调 */
    proto_poll();          /* 协议：非阻塞收帧，只在回帧时阻塞约 1.3ms */
}
```

---

## 5. 本板实测资源占用（移植时的预算参考）

| 资源 | 用量 | 说明 |
|---|---|---|
| FLASH | **6776 B** / 24448 B（27.7%） | 含全部 12 个演示效果 |
| RAM | **2392 B** / 3072 B（77.9%） | 其中 485 接收环形缓冲 1024 B 最大；改小 `RS485_RX_BUF_SIZE` 可省 |
| 单帧时延 | ≤1.3 ms | 14 字节 @115200，发送期间 `led_matrix_scan()` 停顿 |
| 配置区 | 1 页 128 B | 写参数时整页擦写，灯扫会有几 ms 停顿 |

新 MCU 若 RAM 紧张：`RS485_RX_BUF_SIZE` 可降到 32（一帧最多 14 字节，主站帧间隔 ≥3ms）。

---

## 6. 移植验收（两步，都必须绿）

**第 1 步：PC 上跑协议单测**（不接硬件，改完 `core/` 或 `port/` 接口先跑这个）

```powershell
cd tools\proto_test
python -m ziglang cc -std=c11 -Wall -I..\..\core -I..\..\port test_main.c ..\..\core\proto.c -o proto_test.exe
.\proto_test.exe          # 期望输出: 57 passed, 0 failed
```

**第 2 步：上板 7 条指令回归**

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File tools\rs485_regression.ps1 -Port <你的串口>
# 期望输出: 7/7 PASS
```

两步全绿 = 协议链路、CRC、模式矩阵、Flash 保存、位图读写全部正常。

---

## 7. 移植踩坑清单（按出现概率排序）

1. **CRC 顺序**：本协议 CRC **高字节在前**，不是 Modbus 标准的低字节在前。主从必须一致，否则全部静默丢弃。
2. **模式不落盘**：上电固定"本地演示"，必须先发 `01 06 00 03 00 00 01 22 CA` 才能写灯。
3. **看门狗默认 500ms**：不发帧就踢回演示模式；联调时先发 `01 06 00 03 02 00 00 22 AA` 关掉。
4. **3ms 判头**：总线静默 ≥3ms 后的第一个字节才被认作站号；主站发帧间隔别小于 3ms。
5. **DE 脚时序**：拉高后等 100us 再发第一个字节；`wait_tx_done` 后再保持 DE 1ms 才切回接收，否则最后一字节被截断。
6. **`led_matrix_scan()` 不能停**：`rs485_send()` 和 Flash 擦写期间会停顿几毫秒，属正常；但主循环里不能加别的长延时。
7. **回帧位图末字节是 `80` 不是 `FF`**：只有 49 灯，D6 只有 bit7，主站别拿 `FF` 当期望值。
8. **供电与共地**：485 的 A/B 不能接反，USB 转 485 与目标板要共地，否则能发不能收。
