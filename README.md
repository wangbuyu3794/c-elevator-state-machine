# C Elevator State Machine

[![CI](https://github.com/wangbuyu3794/c-elevator-state-machine/actions/workflows/ci.yml/badge.svg)](https://github.com/wangbuyu3794/c-elevator-state-machine/actions/workflows/ci.yml)

一个使用 C 语言实现的单电梯状态机模拟系统，支持多类型楼层请求、方向调度、门控流程、安全保护、停电恢复、备用电源救援、运行统计和 Web 可视化界面。

项目以单台电梯为控制对象，将电梯运行过程拆分为状态机、请求调度、门控、安全、电源恢复、状态输出和界面展示等模块。C 语言负责核心控制逻辑，Python Web 界面负责可视化展示和交互操作。

## 功能概览

- 支持楼层范围 `-1` 到 `34`，其中 `-1` 表示地下 1 层，不存在 `0` 楼；
- 支持单台电梯运行模拟；
- 支持电梯内部楼层按钮；
- 支持楼层外部上行、下行按钮；
- 支持同类请求去重；
- 支持顺路优先、方向兼容、最近目标选择的调度策略；
- 支持基础开门、停留、关门和移动耗时模拟；
- 支持轿厢门、层门、层门锁和楼层对齐状态；
- 支持超重、门阻挡、故障、管理员暂停、紧急呼叫等安全状态；
- 支持主电源关闭、突然停电、恢复供电和备用电源救援；
- 支持空闲后返回 1 楼候机；
- 支持请求等待时间、完成请求数、最长等待时间和平均等待时间统计；
- 支持命令行交互；
- 支持终端可视化调试面板；
- 支持 Python Web 图形界面；
- 支持手动验证场景和一键场景运行脚本。

## 系统模型

### 楼层规则

```text
-1：地下 1 层
1-34：地上楼层
0：不存在，用作“无目标楼层”的内部标记
```

合法楼层包括：

```text
-1, 1, 2, ..., 34
```

非法楼层包括：

```text
0, 小于 -1, 大于 34
```

### 请求类型

系统将请求拆分为三类：

- 外部上行请求：楼层外部上行按钮；
- 外部下行请求：楼层外部下行按钮；
- 内部楼层请求：电梯轿厢内目标楼层按钮。

边界楼层按钮规则：

- `-1` 层只有上行按钮；
- `34` 层只有下行按钮；
- `1` 到 `33` 层同时具有上行和下行按钮。

### 调度策略

电梯调度遵循以下原则：

1. 电梯正在上行时，优先处理上方的内部请求和上行外部请求；
2. 电梯正在下行时，优先处理下方的内部请求和下行外部请求；
3. 当前方向没有可服务请求时，再考虑反向请求；
4. 电梯没有方向时，优先选择最近的内部请求；
5. 没有内部请求时，选择与前往方向兼容的外部请求；
6. 最后使用最近请求作为兜底策略。

这个策略避免了电梯因为一个距离更近但方向不匹配的外部请求而频繁改变运行方向。

### 安全优先级

系统按照以下优先级处理运行状态：

```text
1. 主电源关闭 / 突然停电
2. 备用电源救援 / 安全恢复
3. 管理员暂停
4. 故障状态
5. 紧急呼叫
6. 超重 / 门阻挡 / 开门按钮保持
7. 门锁、层门、轿厢门、楼层对齐检查
8. 内部楼层请求
9. 同方向外部请求
10. 反方向外部请求
11. 空闲回 1 楼候机
```

只要安全条件不满足，普通调度和移动流程会被暂停。

## Web 图形界面

项目提供了一个基于 Python 内置 HTTP 服务的本地 Web 界面。界面通过 `ctypes` 调用 C 语言核心逻辑，所有操作都经过公开事件 API，界面层不会直接修改电梯内部状态。

运行：

```cmd
gui.bat
```

脚本会完成以下工作：

1. 将 C 核心模块编译为 Windows DLL；
2. 启动 Python 本地 Web 服务；
3. 自动打开浏览器访问：

```text
http://127.0.0.1:8765
```

Web 界面包含：

- 楼层外部按钮；
- 电梯内部楼层按钮；
- 电梯井状态显示；
- 当前楼层、目标楼层、方向和状态；
- 轿厢门、层门和门锁状态；
- 电源、安全、故障和紧急呼叫状态；
- 单步运行和自动运行；
- 安全控制按钮；
- 操作日志。

## 命令行运行

编译：

```cmd
build.bat
```

运行：

```cmd
run.bat
```

`build.bat` 会将编译产物输出到 Windows 临时目录，并记录最近一次成功生成的程序路径。`run.bat` 会运行最近一次成功编译的程序。

也可以手动编译：

```cmd
gcc -Wall -Wextra -Iinclude src/main.c src/elevator.c src/elevator_request.c src/elevator_door.c src/elevator_power.c src/elevator_safety.c src/elevator_status.c src/elevator_names.c src/elevator_debug.c -o %TEMP%\c_elevator_state_machine.exe
```

## 场景验证

项目内置验证场景位于 `scenarios/`。

运行全部场景：

```cmd
run_scenarios.bat
```

运行单个场景：

```cmd
run.bat < scenarios\01_single_car_request.txt
```

当前包含的场景：

- `01_single_car_request.txt`：内部楼层请求；
- `02_multiple_direction_requests.txt`：多方向请求调度；
- `03_overload_blocks_run.txt`：超重阻止运行；
- `04_door_blocked_reopen.txt`：门阻挡重新开门；
- `05_backup_power_rescue.txt`：突然停电和备用电源救援；
- `06_emergency_call_blocks_run.txt`：紧急呼叫阻止普通运行。

场景运行日志会输出到 Windows 临时目录，不会写入项目仓库。

## 项目结构

```text
c-elevator-state-machine/
├── include/
│   ├── elevator.h
│   └── elevator_internal.h
├── src/
│   ├── main.c
│   ├── elevator.c
│   ├── elevator_request.c
│   ├── elevator_door.c
│   ├── elevator_power.c
│   ├── elevator_safety.c
│   ├── elevator_status.c
│   ├── elevator_names.c
│   └── elevator_debug.c
├── scenarios/
│   ├── README.md
│   ├── 01_single_car_request.txt
│   ├── 02_multiple_direction_requests.txt
│   ├── 03_overload_blocks_run.txt
│   ├── 04_door_blocked_reopen.txt
│   ├── 05_backup_power_rescue.txt
│   └── 06_emergency_call_blocks_run.txt
├── ui/
│   └── elevator_web.py
├── build.bat
├── gui.bat
├── run.bat
├── run_scenarios.bat
├── README.md
└── .gitignore
```

## 模块说明

| 文件 | 职责 |
| --- | --- |
| `include/elevator.h` | 公开数据结构、枚举、事件入口和快照接口 |
| `include/elevator_internal.h` | 内部模块协作接口 |
| `src/elevator.c` | 初始化、快照、楼层转换、主状态机运行步骤 |
| `src/elevator_request.c` | 请求表、按钮事件、目标选择、请求统计 |
| `src/elevator_door.c` | 轿厢门、层门、门锁和门按钮控制 |
| `src/elevator_power.c` | 主电源、突然停电、备用电源救援和恢复流程 |
| `src/elevator_safety.c` | 载重、故障、管理员暂停、紧急呼叫和安全判断 |
| `src/elevator_status.c` | 状态输出、请求输出、统计输出和终端面板 |
| `src/elevator_names.c` | 状态、方向、门、故障和事件结果名称转换 |
| `src/elevator_debug.c` | 终端可视化调试运行辅助 |
| `src/main.c` | 命令行菜单入口 |
| `ui/elevator_web.py` | Python Web 图形界面 |

## 主要菜单

命令行程序支持以下操作：

- 内部楼层按钮；
- 外部上行按钮；
- 外部下行按钮；
- 运行一步；
- 一直运行到空闲；
- 查看状态、请求和统计；
- 设置载重；
- 设置或解除门阻挡；
- 设置或清除故障；
- 管理员暂停和恢复；
- 关闭或恢复主电源；
- 模拟突然停电；
- 设置备用电源；
- 执行备用电源救援；
- 模拟电梯停在两层之间；
- 执行安全恢复；
- 按住或松开开门按钮；
- 按下关门按钮；
- 触发或清除紧急呼叫；
- 查看终端可视化面板。

## 当前状态

当前版本已经完成单电梯状态机模拟、请求调度、安全机制、电源恢复、运行统计、命令行交互、终端调试面板和 Python Web 可视化界面。

项目适用于 C 语言工程结构、状态机设计、事件驱动接口、安全优先级控制和跨语言界面调用的综合示例。
