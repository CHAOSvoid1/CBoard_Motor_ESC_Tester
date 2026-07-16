# RoboMaster C 板单电机 / 电调测试工程

## 1. 工程用途

本工程用于使用 **RoboMaster 开发板 C 型（STM32F407）**，对下列任意一套动力系统进行快速筛查：

- **M2006 + C610**
- **M3508 + C620**

工程一次只测试 **1 个电机和 1 个电调**，通过 CAN 发送电流指令并读取电调反馈，再由 USART1 按照 VOFA+ FireWater 文本格式持续输出：

```text
motor_test:angle,speed,current_raw,current_A,temp,command_raw,command_A,...
```

它适合检查：

- CAN 通信和电调 ID 是否正常；
- 电调是否持续返回角度、转速、电流和温度数据；
- 电机能否正反转；
- 正反转方向反馈是否相反；
- 电调是否存在发送失败、掉线或过温现象；
- 通过“已知正常电机 / 已知正常电调”交叉替换，进一步判断故障更可能来自电机还是电调。

> 本工程属于台架快速筛查，不能代替额定负载、绝缘、绕组、电流纹波、功率器件和长时间温升等专业检测。

---

## 2. 默认配置

工程默认配置为：

```c
#define TEST_MOTOR_TYPE  TEST_MOTOR_M3508
#define TEST_CAN_BUS     1u
#define TEST_ESC_ID      1u
```

也就是：

- M3508 + C620；
- C 板 CAN1；
- 电调 ID = 1；
- VOFA 串口为 USART1，115200-8-N-1；
- 上电后输出始终为 0，必须从串口发送命令才会转动。

全部常用设置集中在：

```text
Hardware/Test_Config.h
```

---

## 3. M2006 与 M3508 的切换方法

### 测试 M2006 + C610

打开 `Hardware/Test_Config.h`，修改为：

```c
#define TEST_MOTOR_TYPE  TEST_MOTOR_M2006
#define TEST_CAN_BUS     1u
#define TEST_ESC_ID      1u
```

### 测试 M3508 + C620

```c
#define TEST_MOTOR_TYPE  TEST_MOTOR_M3508
#define TEST_CAN_BUS     1u
#define TEST_ESC_ID      1u
```

### 使用 CAN2

```c
#define TEST_CAN_BUS     2u
```

代码对应引脚：

| 接口 | STM32 引脚 |
|---|---|
| CAN1 RX / TX | PD0 / PD1 |
| CAN2 RX / TX | PB5 / PB6 |
| USART1 TX / RX | PA9 / PB7 |

### 修改电调 ID

```c
#define TEST_ESC_ID      1u
```

允许范围为 1～8。工程会自动选择：

- ID 1～4：控制帧 `0x200`；
- ID 5～8：控制帧 `0x1FF`；
- 反馈帧：`0x200 + ID`，例如 ID=1 时反馈 ID 为 `0x201`。

同一条 CAN 总线上不要出现重复 ID。本测试建议只连接一个待测电调。

---

## 4. 硬件连接

### 4.1 电机与电调

#### M2006 + C610

1. M2006 三相动力线连接 C610 三相接口；
2. M2006 的 4-Pin 位置传感器线连接 C610；
3. C610 接 24V 电源；
4. C610 CANH、CANL 连接 C 板所选 CAN 接口。

初次使用、替换 M2006 或替换 C610 后，应按 C610 手册完成电机校准。校准时电机会转动，必须空载并固定好电机。

#### M3508 + C620

1. M3508 三相动力线连接 C620，颜色和接口必须正确对应；
2. M3508 7-Pin 数据线连接 C620；
3. C620 接 24V 电源；
4. C620 CANH、CANL 连接 C 板所选 CAN 接口；
5. 检查 C620 的输入模式为 CAN，并确认状态灯无故障提示；
6. 更换 M3508 或 C620 后，为获得更好的适配参数，建议按 C620 手册在空载状态运行一次电机校准。

### 4.2 CAN 注意事项

- CAN 波特率固定为 **1 Mbps**；
- CANH 接 CANH，CANL 接 CANL，不能接反；
- 线尽量短，并采用双绞线；
- 按 CAN 总线规范配置终端电阻；断电测量 CANH 与 CANL，完整双终端总线通常约为 60Ω；
- C620 不要同时连接 PWM 调参线和 CAN 线；切换输入方式前先断电；
- C610 不要同时连接串口调参端口和 CAN 通信端口。

### 4.3 VOFA 串口

本工程使用 C 板 **UART1 4-pin 接口**：

- C 板 UART1_TX → USB-TTL 的 RX；
- C 板 UART1_RX → USB-TTL 的 TX；
- C 板 GND → USB-TTL 的 GND；
- C 板由自身电源供电，通常不要再把 USB-TTL 的 5V 接入 C 板。

只看曲线时可只连接 TX 和 GND；需要从 VOFA 发送控制命令时，还必须连接 USB-TTL TX 到 C 板 RX。

---

## 5. 编译与下载

1. 使用 Keil MDK5 打开：

```text
Project/CBoard.uvprojx
```

2. 工程原配置使用：

```text
ARM Compiler 5.06 update 6
STM32F407IG
外部晶振 12 MHz
系统时钟 168 MHz
```

3. 先修改 `Hardware/Test_Config.h`；
4. 点击 Build；
5. 使用 ST-Link 或 J-Link 通过 SWD 下载；
6. 电机架空并固定，确认周围无人接触旋转部件后再上电。

本工程采用标准外设库，不依赖 FreeRTOS。

---

## 6. VOFA+ 设置

1. 新建设备，选择对应 USB-TTL 串口；
2. 串口参数设置为：

```text
115200 baud
8 data bits
1 stop bit
No parity
No flow control
```

3. 协议引擎选择 **FireWater**；
4. 打开串口后，会持续收到以 `motor_test:` 开头的数据；
5. 将需要观察的通道拖入波形窗口；
6. 在命令发送区使用 ASCII 文本发送单字符命令。

数据默认每 50ms 发送一次，即约 20Hz。可在 `Test_Config.h` 修改：

```c
#define VOFA_SEND_PERIOD_MS 50u
```

---

## 7. 串口控制命令

| 命令 | 功能 |
|---|---|
| `A` | 启动自动正反转测试 |
| `S` | 启动手动运行 |
| `X` 或 `0` | 紧急停止，输出立即置 0 |
| `+` | 增大手动电流幅值 |
| `-` | 减小手动电流幅值 |
| `R` | 当前手动电流反向 |
| `F` | 设置为正方向 |
| `B` | 设置为反方向 |
| `H` 或 `?` | 打印帮助信息 |

建议先发送 `A` 做自动测试。只有需要进一步观察时才使用手动模式。

### 手动模式保护

- 30 秒没有收到新的手动控制命令，会自动停机；
- CAN 反馈丢失后立即停机，恢复通信后不会自动重新输出；
- M3508 + C620 温度达到 85℃时立即停机；
- 所有输出都被 `MOTOR_SAFE_MAX_RAW` 限幅。

---

## 8. 自动测试流程

发送 `A` 后执行：

1. 1.0 秒零输出准备；
2. 2.5 秒正向电流；
3. 1.0 秒零输出；
4. 2.5 秒反向电流；
5. 0.5 秒零输出并判定结果。

默认判定条件：

- CAN 反馈在线；
- 正向峰值转速绝对值不小于 30rpm；
- 反向峰值转速绝对值不小于 30rpm；
- 两个方向的转速符号相反；
- 未触发过温保护。

自动测试是空载筛查。如果机械负载较大，可适当修改：

```c
#define MOTOR_AUTO_CURRENT_RAW  ...
#define AUTO_MIN_SPEED_RPM      ...
```

不要一开始就提高到电调满量程。

---

## 9. VOFA 通道定义

每帧格式：

```text
motor_test:CH0,CH1,...,CH15
```

| 通道 | 名称 | 含义 |
|---:|---|---|
| CH0 | `angle_raw` | 转子机械角度原始值，通常 0～8191 |
| CH1 | `speed_rpm` | 电调返回转速，rpm |
| CH2 | `feedback_current_raw` | 电调反馈的转矩电流原始值 |
| CH3 | `feedback_current_A` | 按电调量程换算的反馈电流，A |
| CH4 | `temperature_C` | C620 温度；M2006+C610 模式固定为 -1 |
| CH5 | `command_raw` | 当前发送给电调的电流指令原始值 |
| CH6 | `command_A` | 电流指令换算值，A |
| CH7 | `online` | 1=反馈在线，0=掉线 |
| CH8 | `rx_frequency_Hz` | 电调反馈接收频率，正常一般接近 1000Hz |
| CH9 | `tx_fail_count` | CAN 发送失败累计次数 |
| CH10 | `CAN_ESR` | STM32 CAN 错误状态寄存器原始值 |
| CH11 | `mode` | 测试模式代码 |
| CH12 | `result` | 测试结果代码 |
| CH13 | `auto_phase` | 自动测试阶段 |
| CH14 | `forward_peak_rpm` | 自动测试正向阶段峰值转速 |
| CH15 | `reverse_peak_rpm` | 自动测试反向阶段峰值转速 |

### mode 代码

| 数值 | 状态 |
|---:|---|
| 0 | STOPPED，已停止 |
| 1 | MANUAL，手动运行 |
| 2 | AUTO，自动测试中 |
| 3 | DONE，测试完成 |
| 4 | FAULT，保护停机 |

### result 代码

| 数值 | 结果 |
|---:|---|
| 0 | NOT_RUN，尚未测试 |
| 1 | RUNNING，测试中 |
| 2 | PASS，自动筛查通过 |
| 3 | FAIL_OFFLINE，无 CAN 反馈或运行中掉线 |
| 4 | FAIL_FORWARD，正向转速不足 |
| 5 | FAIL_REVERSE，反向转速不足 |
| 6 | FAIL_DIRECTION，正反向转速符号未反转 |
| 7 | FAIL_OVERTEMP，温度超限 |
| 8 | ABORTED，人工停止、超时或反馈丢失停机 |

### auto_phase 代码

| 数值 | 阶段 |
|---:|---|
| 0 | 准备阶段，零输出 |
| 1 | 正向测试 |
| 2 | 中间零输出 |
| 3 | 反向测试 |
| 4 | 结束缓冲，零输出 |
| 5 | 判定完成 |

---

## 10. 如何判断电机或电调的好坏

### 情况 1：`online=0`，`rx_frequency_Hz=0`

优先检查：

1. 电调是否上电、状态灯是否正常；
2. CANH/CANL 是否接反；
3. CAN 口选择是否与 `TEST_CAN_BUS` 一致；
4. 电调 ID 是否与 `TEST_ESC_ID` 一致；
5. CAN 波特率是否为 1Mbps；
6. 终端电阻和接线是否正确；
7. 电调是否处于 CAN 输入模式；
8. 电调 CAN 收发器或 C 板 CAN 口是否损坏。

此时只能说明通信链路异常，不能直接认定电机损坏。

### 情况 2：`online=1`，角度会变化，但加电流后转速始终接近 0

可能原因：

- 电机机械堵转或减速箱损坏；
- 三相线接触不良；
- 电调功率输出级损坏；
- 位置传感器或数据线异常；
- C610 与 M2006 未正确校准；
- 测试电流太小或负载太大。

### 情况 3：`command_raw` 非 0，反馈电流和转速都接近 0

更需要怀疑：

- 电调未真正进入 CAN 控制模式；
- 电调输出级故障；
- 电机动力线断路；
- CAN 控制帧 ID 与电调 ID 不匹配。

### 情况 4：反馈频率正常，但 `tx_fail_count` 持续增加

检查：

- CAN 总线负载或接线质量；
- CAN 是否进入 error passive / bus-off；
- 终端电阻；
- 是否有重复 ID；
- 总线上是否连接了过多设备或存在错误波特率节点。

### 情况 5：自动测试 PASS，但电机有异响、抖动或明显发热

仍不能判定完全正常。需进一步检查：

- 轴承、减速箱和转子；
- 三相线接触；
- 空载电流是否异常；
- 电机温升；
- 电调功率管温升；
- 带载能力和长时间运行稳定性。

### 最可靠的交叉测试方法

准备一个已知正常的同型号电机和一个已知正常的同型号电调：

1. 待测电机 + 正常电调；
2. 正常电机 + 待测电调。

判断原则：

- 待测电机配正常电调仍失败，而正常电机配待测电调通过：更可能是电机故障；
- 正常电机配待测电调失败，而待测电机配正常电调通过：更可能是电调故障；
- 两种组合都失败：先排查接线、ID、校准、电源和 CAN 链路；
- 两种组合都通过：原故障可能来自接插件、线束、负载或间歇性问题。

---

## 11. 默认电流限制

### M2006 + C610

```c
MOTOR_COMMAND_FULL_RAW    = 10000   // 约对应 10A 满量程
MOTOR_SAFE_MAX_RAW        = 3000
MOTOR_DEFAULT_CURRENT_RAW = 1200
MOTOR_AUTO_CURRENT_RAW    = 1500
```

### M3508 + C620

```c
MOTOR_COMMAND_FULL_RAW    = 16384   // 约对应 20A 满量程
MOTOR_SAFE_MAX_RAW        = 4000
MOTOR_DEFAULT_CURRENT_RAW = 2000
MOTOR_AUTO_CURRENT_RAW    = 2500
```

默认值用于空载低风险筛查，不代表所有负载都适用。提高电流前必须观察温度、声音、机械负载和供电电流。

---

## 12. 工程文件说明

```text
Hardware/Test_Config.h   用户配置：电机类型、CAN、ID、电流和保护参数
Hardware/MyCAN.c/.h      CAN1/CAN2 初始化、发送电流、解析反馈
Hardware/Motor.c/.h      自动测试、手动控制、故障判定和保护
Hardware/Motor_Timer.c/.h 1ms 时间基准
Hardware/Usart.c/.h      USART1 收发与命令接收
Hardware/Vofa.c/.h       VOFA FireWater 数据输出
User/main.c              主循环和调度
Project/CBoard.uvprojx   Keil 工程文件
VOFA_Channels.csv        VOFA 通道速查表
```

---

## 13. 安全要求

- 第一次测试必须让输出轴悬空；
- 电机必须固定，不能手持运行；
- 先限流供电，确认无短路后再逐步提高电源限流；
- 不要触碰正在转动的输出轴；
- 不要长时间堵转；
- 出现异响、冒烟、焦味、快速升温或电流异常时立即断电；
- 更换线缆、切换 CAN/PWM/调参接口前必须先断电；
- 自动测试前确认 `TEST_MOTOR_TYPE`、`TEST_CAN_BUS` 和 `TEST_ESC_ID` 配置正确。

---

## 14. 已完成的软件检查

- 已更新 Keil 工程文件并加入全部新增源文件；
- 已检查工程 XML 可正常解析；
- 已对新增和修改的 C 文件完成 ARM Cortex-M4 目标语法检查；
- 未在真实 C 板、电机和电调上进行硬件上电验证，首次使用应保持低电流、空载和可随时断电。
