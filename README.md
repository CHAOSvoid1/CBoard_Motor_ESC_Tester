# RoboMaster C 板单电机 / 驱动器测试工程

## 1. 工程用途

本工程使用 **RoboMaster 开发板 C 型（STM32F407）**，一次对一个 RoboMaster 动力单元进行快速筛查，现支持：

- **M2006 + C610 外置电调**
- **M3508 + C620 外置电调**
- **GM6020 一体化电机**

GM6020 的驱动器已经集成在电机内部，测试时 **不需要、也不能再外接 C610/C620 电调**。GM6020 只需连接 24V 电源和 CAN 总线。

工程通过 CAN 发送控制指令并读取反馈，再由 USART1 按照 VOFA+ FireWater 文本格式持续输出：

```text
motor_test:angle,speed,current_raw,current_A,temp,command_raw,command_display,...
```

它适合检查：

- CAN 接线、CAN 接口和设备 ID 是否正常；
- 是否持续返回机械角度、转速、反馈电流和温度；
- 电机能否正反转；
- 正反转方向反馈是否相反；
- CAN 是否掉线、发送失败或进入错误状态；
- M3508/C620、GM6020 是否出现过温；
- 对 M2006/C610、M3508/C620 进行电机和电调交叉替换排查；
- 对 GM6020 整机及其内部驱动器进行整体功能筛查。

> 本工程属于低负载台架快速筛查，不能代替额定负载、绝缘、绕组、电流纹波、功率器件和长时间温升等专业检测。GM6020 为电机与驱动器一体化结构，本工程只能判断整个 GM6020 动力单元是否基本正常，不能仅凭测试结果精确区分内部电机本体和内部驱动器谁损坏。

---

## 2. 当前默认配置

更新后的工程默认配置为：

```c
#define TEST_MOTOR_TYPE      TEST_MOTOR_GM6020
#define GM6020_CONTROL_MODE  GM6020_CONTROL_VOLTAGE
#define TEST_CAN_BUS         1u
#define TEST_MOTOR_ID        1u
```

也就是：

- 测试 GM6020；
- 使用兼容性更好的 **电压控制模式**；
- 使用 C 板 CAN1；
- GM6020 ID = 1；
- VOFA 串口为 USART1，115200-8-N-1；
- 上电后控制指令始终为 0，必须从串口发送命令才会主动转动。

全部常用设置集中在：

```text
Hardware/Test_Config.h
```

---

## 3. 三种动力系统的切换方法

打开：

```text
Hardware/Test_Config.h
```

### 3.1 测试 M2006 + C610

```c
#define TEST_MOTOR_TYPE      TEST_MOTOR_M2006
#define TEST_CAN_BUS         1u
#define TEST_MOTOR_ID        1u
```

M2006 必须搭配外置 C610 电调，控制方式固定为电流指令。

### 3.2 测试 M3508 + C620

```c
#define TEST_MOTOR_TYPE      TEST_MOTOR_M3508
#define TEST_CAN_BUS         1u
#define TEST_MOTOR_ID        1u
```

M3508 必须搭配外置 C620 电调，控制方式固定为电流指令。

### 3.3 测试 GM6020

```c
#define TEST_MOTOR_TYPE      TEST_MOTOR_GM6020
#define TEST_CAN_BUS         1u
#define TEST_MOTOR_ID        1u
```

GM6020 内部已经集成驱动器，不外接电调。

#### GM6020 电压控制模式，推荐默认使用

```c
#define GM6020_CONTROL_MODE  GM6020_CONTROL_VOLTAGE
```

特点：

- 兼容性最好；
- 不需要先在 RoboMaster Assistant 中打开电流环；
- 指令原始范围为 `-25000～25000`；
- VOFA 的 CH6 显示为满量程百分比 `%`。

#### GM6020 电流控制模式

```c
#define GM6020_CONTROL_MODE  GM6020_CONTROL_CURRENT
```

使用前必须满足：

- GM6020 固件版本不低于 `1.0.11.2`；
- 使用 RoboMaster Assistant 打开 GM6020 的电流环；
- 指令原始范围为 `-16384～16384`，对应约 `-3A～3A`；
- VOFA 的 CH6 显示单位为 A。

如果没有确认固件和电流环设置，保持默认电压模式。

### 3.4 使用 CAN2

```c
#define TEST_CAN_BUS  2u
```

代码对应引脚：

| 接口 | STM32 引脚 |
|---|---|
| CAN1 RX / TX | PD0 / PD1 |
| CAN2 RX / TX | PB5 / PB6 |
| USART1 TX / RX | PA9 / PB7 |

### 3.5 修改设备 ID

```c
#define TEST_MOTOR_ID  1u
```

ID 范围：

| 动力系统 | 可用 ID |
|---|---:|
| M2006 + C610 | 1～8 |
| M3508 + C620 | 1～8 |
| GM6020 | 1～7 |

同一条 CAN 总线上不得出现重复 ID。本测试建议只连接一个待测动力单元。

程序会根据电机类型、控制模式和 ID 自动选择 CAN 报文：

| 动力系统 / 模式 | ID 1～4 控制帧 | ID 5～7/8 控制帧 | 反馈帧 |
|---|---:|---:|---:|
| M2006 + C610 | `0x200` | `0x1FF` | `0x200 + ID` |
| M3508 + C620 | `0x200` | `0x1FF` | `0x200 + ID` |
| GM6020 电压模式 | `0x1FF` | `0x2FF` | `0x204 + ID` |
| GM6020 电流模式 | `0x1FE` | `0x2FE` | `0x204 + ID` |

例如 GM6020 的 ID 为 1 时，反馈帧为 `0x205`。

---

## 4. 硬件连接

### 4.1 M2006 + C610

1. M2006 三相动力线连接 C610 三相接口；
2. M2006 的 4-Pin 位置传感器线连接 C610；
3. C610 接 24V 电源；
4. C610 的 CAN_H、CAN_L 连接 C 板所选 CAN 接口；
5. 确认电调 ID 与 `TEST_MOTOR_ID` 一致。

初次使用、替换 M2006 或替换 C610 后，应按 C610 手册完成电机校准。校准时电机会转动，必须空载并固定好电机。

### 4.2 M3508 + C620

1. M3508 三相动力线连接 C620，颜色和接口正确对应；
2. M3508 7-Pin 数据线连接 C620；
3. C620 接 24V 电源；
4. C620 的 CAN_H、CAN_L 连接 C 板所选 CAN 接口；
5. 确认 C620 输入模式为 CAN，状态灯没有故障提示；
6. 确认电调 ID 与 `TEST_MOTOR_ID` 一致。

更换 M3508 或 C620 后，为获得更好的适配参数，建议按 C620 手册在空载状态进行校准。

### 4.3 GM6020

GM6020 不使用外置电调，直接连接：

1. GM6020 XT30 电源接口接额定 24V 电源；
2. GM6020 CAN 信号线接 C 板所选 CAN 接口；
3. 红色线为 CAN_H，黑色线为 CAN_L；
4. 通过 GM6020 底部拨码开关设置 ID；
5. 第 4 位拨码用于接入或断开内部 CAN 终端电阻；
6. 正常 CAN 工作时，绿灯每秒闪烁 N 次，其中 N 对应当前 ID。

不要把 GM6020 的 PWM/串口调参端口当成 CAN 接口。进行 CAN 测试时，只使用其 CAN 信号端口。

### 4.4 CAN 注意事项

- CAN 波特率固定为 **1Mbps**；
- CAN_H 接 CAN_H，CAN_L 接 CAN_L，不能接反；
- 线尽量短，建议使用双绞线；
- 按总线结构正确配置终端电阻；
- 断电测量 CAN_H 与 CAN_L，完整双终端总线通常约为 60Ω；
- C620 不要同时连接 PWM 调参线和 CAN 线，切换输入方式前先断电；
- C610 不要同时连接串口调参端口和 CAN 通信端口；
- GM6020 的同一条 CAN 总线上不要存在重复 ID；
- 修改接线、ID、终端电阻或控制模式前先断电。

### 4.5 VOFA 串口

本工程使用 C 板 **UART1 4-pin 接口**：

- C 板 UART1_TX → USB-TTL RX；
- C 板 UART1_RX → USB-TTL TX；
- C 板 GND → USB-TTL GND；
- C 板由自身电源供电，通常不要把 USB-TTL 的 5V 接入 C 板。

只看曲线时可只连接 TX 和 GND；需要从 VOFA 发送控制命令时，还必须连接 USB-TTL TX 到 C 板 RX。

---

## 5. 编译与下载

1. 将增量替换文件按原目录覆盖到工程；
2. 使用 Keil MDK5 打开：

```text
Project/CBoard.uvprojx
```

3. 修改 `Hardware/Test_Config.h`；
4. 点击 Build；
5. 使用 ST-Link 或 J-Link 通过 SWD 下载；
6. 固定电机并让输出部分无负载，确认周围无人接触旋转部件后再上电。

工程原配置：

```text
ARM Compiler 5.06 update 6
STM32F407IG
外部晶振 12MHz
系统时钟 168MHz
```

本工程采用 STM32 标准外设库，不依赖 FreeRTOS。

---

## 6. VOFA+ 设置

1. 新建设备，选择对应 USB-TTL 串口；
2. 串口参数：

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

数据默认每 50ms 发送一次，即约 20Hz：

```c
#define VOFA_SEND_PERIOD_MS  50u
```

---

## 7. 串口控制命令

| 命令 | 功能 |
|---|---|
| `A` | 启动自动正反转测试 |
| `S` | 启动手动运行 |
| `X` 或 `0` | 紧急停止，输出立即置 0 |
| `+` | 增大手动指令幅值 |
| `-` | 减小手动指令幅值 |
| `R` | 当前手动指令反向 |
| `F` | 设置为正方向 |
| `B` | 设置为反方向 |
| `H` 或 `?` | 打印帮助信息 |

建议先观察 CAN 反馈正常，再发送 `A`。只有需要进一步观察时才使用手动模式。

### 手动模式保护

- 30 秒没有收到新的手动控制命令，自动停机；
- CAN 反馈丢失后立即停机，恢复通信后不会自动重新输出；
- M3508/C620 或 GM6020 温度达到 85℃时立即停机；
- M2006/C610 没有温度反馈，CH4 固定显示 `-1`；
- 所有输出都由 `MOTOR_SAFE_MAX_RAW` 限幅。

---

## 8. 自动测试流程

发送 `A` 后执行：

1. 1.0 秒零输出准备；
2. 2.5 秒正向指令；
3. 1.0 秒零输出；
4. 2.5 秒反向指令；
5. 0.5 秒零输出并判定结果。

默认判定条件：

- CAN 反馈在线；
- 正向峰值转速绝对值不小于 30rpm；
- 反向峰值转速绝对值不小于 30rpm；
- 两个方向的转速符号相反；
- 未触发过温保护。

对应配置：

```c
#define AUTO_MIN_SPEED_RPM  30
#define MOTOR_AUTO_COMMAND_RAW  ...
```

注意：

- M2006、M3508 和 GM6020 电流模式的自动指令代表电流原始值；
- GM6020 默认电压模式的自动指令代表电压给定原始值；
- 自动测试只是空载筛查，不要一开始就提高到满量程；
- GM6020 的最大空载转速明显低于 M2006/M3508，但默认 30rpm 阈值仍适合基本正反转筛查。

---

## 9. VOFA 通道定义

每帧格式：

```text
motor_test:CH0,CH1,...,CH15
```

| 通道 | 名称 | 含义 |
|---:|---|---|
| CH0 | `angle_raw` | 机械角度原始值，范围通常为 0～8191 |
| CH1 | `speed_rpm` | 反馈转速，单位 rpm |
| CH2 | `feedback_current_raw` | 反馈转矩电流原始值 |
| CH3 | `feedback_current_A` | 按当前动力系统量程换算的反馈电流，单位 A |
| CH4 | `temperature_C` | M3508/C620、GM6020 的温度；M2006/C610 固定为 -1 |
| CH5 | `command_raw` | 当前发送的控制指令原始值 |
| CH6 | `command_display` | M2006/M3508/GM6020 电流模式为 A；GM6020 电压模式为 % |
| CH7 | `online` | 1=反馈在线，0=掉线 |
| CH8 | `rx_frequency_Hz` | 反馈接收频率，正常一般接近 1000Hz |
| CH9 | `tx_fail_count` | CAN 发送失败累计次数 |
| CH10 | `CAN_ESR` | STM32 CAN 错误状态寄存器原始值 |
| CH11 | `mode` | 测试模式代码 |
| CH12 | `result` | 测试结果代码 |
| CH13 | `auto_phase` | 自动测试阶段 |
| CH14 | `forward_peak_rpm` | 自动测试正向阶段峰值转速 |
| CH15 | `reverse_peak_rpm` | 自动测试反向阶段峰值转速 |

### CH3 反馈电流换算

| 动力系统 | 原始满量程 | 显示满量程 |
|---|---:|---:|
| M2006 + C610 | 10000 | 10A |
| M3508 + C620 | 16384 | 20A |
| GM6020 | 16384 | 3A |

### CH6 指令显示

| 模式 | CH6 单位 | 示例 |
|---|---|---|
| M2006 + C610 | A | `1.500` 表示约 1.5A 指令 |
| M3508 + C620 | A | `3.052` 表示约 3.052A 指令 |
| GM6020 电流模式 | A | `0.330` 表示约 0.330A 指令 |
| GM6020 电压模式 | % | `10.000` 表示约 10% 满量程电压给定 |

GM6020 电压模式下，CH6 不是电流，也不是实际母线电压值。

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
| 8 | ABORTED，人工停止、超时或手动运行中反馈丢失 |

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

## 10. 如何判断通信是否正常

正常连接后，一般应看到：

```text
CH7 online            = 1
CH8 rx_frequency_Hz   ≈ 1000
CH9 tx_fail_count     不再持续增加
CH10 CAN_ESR          正常时通常为 0
```

手动拨动电机时：

- CH0 机械角度应随转动变化；
- CH1 转速应出现正值或负值；
- 停止拨动后 CH1 回到接近 0；
- CH2、CH3 可能出现小幅正负变化。

如果 CAN_H、CAN_L 接反，常见表现为：

- `online=0`；
- `rx_frequency_Hz=0`；
- `tx_fail_count` 持续增加；
- CAN_ESR 出现错误状态。

修正接线并重新上电后，旧的 `tx_fail_count` 累计值才会清零。

---

## 11. 如何判断电机、外置电调或 GM6020 的好坏

### 11.1 `online=0`，`rx_frequency_Hz=0`

优先检查：

1. 设备是否接入 24V 并正常上电；
2. CAN_H、CAN_L 是否接反；
3. 实际连接口是否与 `TEST_CAN_BUS` 一致；
4. 实际 ID 是否与 `TEST_MOTOR_ID` 一致；
5. CAN 波特率是否为 1Mbps；
6. 终端电阻和线束是否正确；
7. 是否存在重复 ID；
8. M2006/C610、M3508/C620 的外置电调是否处于 CAN 模式；
9. GM6020 是否连接了正确的 CAN 端口，而不是 PWM/串口端口。

此时只能说明通信链路异常，不能直接认定电机损坏。

### 11.2 `online=1`，手拨角度和转速反馈正常，但给指令后不转

M2006/M3508 可能原因：

- 三相动力线断路或接触不良；
- 位置传感器/7-Pin 数据线异常；
- 外置电调功率输出级故障；
- 电机机械堵转或减速箱故障；
- 电机与电调未正确校准；
- 指令过小或机械负载过大。

GM6020 可能原因：

- 内部驱动器关闭输出或处于保护状态；
- 电机机械卡滞；
- 电压过高或温度过高；
- 使用电流模式但固件、电流环设置不满足要求；
- 指令太小或实际负载过大。

### 11.3 `command_raw` 非 0，但反馈电流和转速都接近 0

优先检查：

- 控制帧是否与当前电机类型、ID 和 GM6020 控制模式匹配；
- 外置电调是否真正处于 CAN 控制模式；
- M2006/M3508 三相动力线是否连接；
- GM6020 是否因为告警或异常关闭输出；
- GM6020 电流模式是否已在 Assistant 中开启电流环。

### 11.4 反馈频率正常，但 `tx_fail_count` 持续增加

检查：

- CAN 线质量和接触；
- 终端电阻；
- 是否进入 error passive 或 bus-off；
- 是否有重复 ID；
- 是否存在波特率错误的节点；
- 总线是否受到强干扰。

### 11.5 自动测试 PASS，但有异响、抖动或快速发热

仍不能判定完全正常，需要继续检查：

- 轴承、减速箱和转子；
- 三相线及传感器线接触；
- 空载电流是否明显异常；
- 电机和驱动器温升；
- 带载能力和长时间运行稳定性；
- GM6020 是否接近强磁性材料或存在机械安装干涉。

### 11.6 M2006/C610、M3508/C620 的交叉测试

准备已知正常的同型号电机和外置电调：

1. 待测电机 + 正常电调；
2. 正常电机 + 待测电调。

判断原则：

- 待测电机配正常电调仍失败，而正常电机配待测电调通过：更可能是电机故障；
- 正常电机配待测电调失败，而待测电机配正常电调通过：更可能是电调故障；
- 两种组合都失败：先排查接线、ID、校准、电源和 CAN；
- 两种组合都通过：原故障可能来自接插件、线束、负载或间歇性问题。

### 11.7 GM6020 的判断限制

GM6020 没有可单独替换的外置电调，因此不能按 M2006/M3508 的方法交叉交换电机和电调。

推荐交叉方法：

1. 使用同一套 C 板、CAN 线和电源测试一台已知正常的 GM6020；
2. 再保持线束不变，替换为待测 GM6020；
3. 对比在线状态、反馈频率、角度、转速、电流、温度和自动测试结果。

如果正常 GM6020 通过，而待测 GM6020 在同一线束下失败，更可能是待测 GM6020 整体故障。但仍不能仅凭本工程区分其内部电机本体还是内部驱动器故障。

---

## 12. 默认指令限制

### 12.1 M2006 + C610

```c
MOTOR_COMMAND_FULL_RAW      = 10000   // 约对应 10A 满量程
MOTOR_SAFE_MAX_RAW          = 3000
MOTOR_DEFAULT_COMMAND_RAW   = 1200
MOTOR_AUTO_COMMAND_RAW      = 1500
MOTOR_COMMAND_STEP_RAW      = 200
```

### 12.2 M3508 + C620

```c
MOTOR_COMMAND_FULL_RAW      = 16384   // 约对应 20A 满量程
MOTOR_SAFE_MAX_RAW          = 4000
MOTOR_DEFAULT_COMMAND_RAW   = 2000
MOTOR_AUTO_COMMAND_RAW      = 2500
MOTOR_COMMAND_STEP_RAW      = 400
```

### 12.3 GM6020 电压模式

```c
MOTOR_COMMAND_FULL_RAW      = 25000   // 100% 电压给定满量程
MOTOR_SAFE_MAX_RAW          = 6000    // 限制为约 24% 满量程
MOTOR_DEFAULT_COMMAND_RAW   = 2500    // 约 10%
MOTOR_AUTO_COMMAND_RAW      = 3500    // 约 14%
MOTOR_COMMAND_STEP_RAW      = 500     // 每次约 2%
```

### 12.4 GM6020 电流模式

```c
MOTOR_COMMAND_FULL_RAW      = 16384   // 约对应 3A 满量程
MOTOR_SAFE_MAX_RAW          = 3000    // 约 0.549A
MOTOR_DEFAULT_COMMAND_RAW   = 1200    // 约 0.220A
MOTOR_AUTO_COMMAND_RAW      = 1800    // 约 0.330A
MOTOR_COMMAND_STEP_RAW      = 200     // 每次约 0.037A
```

默认值用于空载低风险筛查，不代表所有负载都适用。提高指令前必须观察温度、声音、机械负载和电源电流。

---

## 13. GM6020 指示灯与 ID 快速判断

### 正常状态

- 绿灯每秒闪 N 次：CAN 正常，当前 ID 为 N；
- 绿灯慢闪：PWM 通信正常；
- 绿灯常亮：PWM 行程校准中。

### 常见警告

- 橙灯每秒闪 1 次：高温警告；
- 橙灯每秒闪 2 次：CAN 总线上存在相同 ID；
- 橙灯常亮：电流控制模式下收到电压指令，通常表示代码控制模式与 GM6020 当前设置不匹配。

### 异常状态

- 红灯每秒闪 1 次：供电电压过高；
- 红灯每秒闪 4 次：电机温度过高，驱动器会关闭输出。

---

## 14. 工程文件说明

完整工程主要文件：

```text
Hardware/Test_Config.h     用户配置：动力系统、GM6020模式、CAN、ID和保护参数
Hardware/MyCAN.c/.h        CAN1/CAN2初始化、控制帧发送和反馈解析
Hardware/Motor.c/.h        自动测试、手动控制、故障判定和保护
Hardware/Motor_Timer.c/.h  1ms时间基准
Hardware/Usart.c/.h        USART1收发和命令接收
Hardware/Vofa.c/.h         VOFA FireWater数据输出
User/main.c                主循环和调度
Project/CBoard.uvprojx     Keil工程文件
README.md                  本使用说明
VOFA_Channels.csv          VOFA通道速查表
```

本次增加 GM6020 时需要覆盖的文件：

```text
Hardware/Test_Config.h
Hardware/MyCAN.h
Hardware/MyCAN.c
Hardware/Motor.h
Hardware/Motor.c
Hardware/Vofa.c
User/main.c
README.md
```

---

## 15. 安全要求

- 第一次测试必须让电机输出部分无负载；
- 电机必须可靠固定，不能手持运行；
- GM6020 的定子和转子都需要避免与周围结构碰撞；
- 先限流供电，确认无短路后再逐步提高电源限流；
- 不要触碰正在转动的输出轴或转子；
- 不要长时间堵转；
- 出现异响、冒烟、焦味、快速升温或电流异常时立即断电；
- 更换线缆、切换 CAN/PWM/调参接口、修改 ID 或终端电阻前必须断电；
- 自动测试前确认 `TEST_MOTOR_TYPE`、`TEST_CAN_BUS`、`TEST_MOTOR_ID` 和 `GM6020_CONTROL_MODE` 配置正确。

---

## 16. 已完成的软件检查

- 已增加 M2006、M3508、GM6020 三种配置；
- 已增加 GM6020 电压模式和电流模式；
- 已根据不同设备自动选择控制帧和反馈帧；
- 已保持 16 路 VOFA FireWater 通道格式；
- 已保留 CAN 掉线停机、手动超时、过温和指令限幅保护；
- 已分别检查 M2006、M3508、GM6020 电压模式和 GM6020 电流模式的代码配置；
- 仍需在真实 C 板和 GM6020 上进行首次低指令、空载验证。
