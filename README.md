# 🚗 STM32 Mini ADAS Subsystem (FreeRTOS)

A real-time **Advanced Driver Assistance System (ADAS)** prototype built on STM32 with **FreeRTOS (CMSIS-RTOS v2)**.
It reads sensor data (lane position, obstacle distance, speed), evaluates the risk level and warns the driver with **LEDs** and **UART/SWV messages**.

The project was developed simulation-first (C simulation + Software-in-the-Loop tests) and is now **running and tested on real hardware: an STM32F407G-DISC1 Discovery board**.

![Block diagram](MINI%20ADAS%20BLOCK%20DIAGRAM.png)

---

## 📌 Project Status

| Area | Status |
|------|--------|
| RTOS architecture (6 tasks, queues, event flags, mutex) | ✅ Done |
| Dynamic risk assessment (SAFE → CRITICAL) | ✅ Done |
| Software-in-the-Loop validation (C, CSV, Python, Streamlit) | ✅ Done |
| **Port to STM32F407 Discovery and test on hardware** | ✅ Done (Nov 2026) |
| LED alerts on the board | ✅ Done |
| CAN bus documentation | ✅ Done ([README_CAN_INTEGRATION.md](README_CAN_INTEGRATION.md)) |
| Real sensors, CAN on hardware, on-target unit tests | 🔜 Planned |

> Sensor values are currently **simulated** (pseudo-random) inside `SensorTask`.

---

## 📁 Repository Structure

```
STM32-Mini-ADAS-Subsystem/
├── Mini_ADAS_F407/          # ✅ Main firmware – STM32F407G-DISC1 (tested on hardware)
├── Mini_ADAS_RTOS/          # Original firmware – STM32F103C8 (Blue Pill), legacy
├── simulation/              # Windows C simulation of the decision logic
├── ADAS_Testing/            # SIL test pipeline (C test engine, CSV, plots, dashboard)
├── README_CAN_INTEGRATION.md
├── STM32ADAS.pdf
└── MINI ADAS BLOCK DIAGRAM.png
```

---

## 🛠 Hardware & Tools

| Item | Details |
|------|---------|
| Board | **STM32F407G-DISC1** – STM32F407VGT6, ARM Cortex-M4, 1 MB Flash, 192 KB RAM, on-board ST-LINK |
| RTOS | FreeRTOS 10.3.1, CMSIS-RTOS v2 API |
| IDE | STM32CubeIDE 1.19 |
| Debug output | UART1 (PA9 TX, 115200 8N1) and SWV/ITM console (no adapter needed) |
| Language | Embedded C |
| Testing | C simulation, CSV test cases, Python (matplotlib), Streamlit |

---

## 🧠 System Architecture

```
SensorTask --sensorQueue--> FilterTask --filterQueue--> DecisionTask
                                                            |
                                          alertQueue + alertFlags (event flags)
                                                            v
                                   LEDs + printf  <--   AlertTask

defaultTask : green LED heartbeat          LoggerTask : status + free heap every 2 s
```

### Tasks

| Task | Priority | Responsibility |
|------|----------|----------------|
| SensorTask | Above Normal | Every 1 s generates lane (0/1), distance (0–99), speed (30–129) → `sensorQueue` |
| FilterTask | Normal | Simulates sensor noise filtering (±2 on distance) → `filterQueue` |
| DecisionTask | **High** | Applies risk rules, sends result to `alertQueue`, sets LANE / OBSTACLE event flags |
| AlertTask | Low | Waits on event flags (1.5 s timeout), uses the newest alert, drives LEDs, prints alerts |
| LoggerTask | Low | Prints system status and free heap every 2 s |
| defaultTask | Normal | Green LED heartbeat (proves the scheduler is alive) |

**Why these priorities?** The safety decision (DecisionTask) must never be delayed by slow output. Printing tasks are lowest because they are slow and not safety-critical.

### Inter-task communication

- **Message queues** carry data between pipeline stages (thread-safe, decoupled).
- **Event flags** (`LANE_ALERT_FLAG`, `OBSTACLE_ALERT_FLAG`) wake AlertTask only when something happens – no polling.
- **Mutex** serialises `printf` output from multiple tasks.

---

## ⚠️ Risk Assessment Rules

Checked top to bottom – the first match wins:

| Risk Level | Condition | Action |
|------------|-----------|--------|
| CRITICAL | speed > 120 **or** distance < 5 | APPLY BRAKES! |
| HIGH | speed > 100 **or** distance < 10 | Slow Down Immediately |
| MEDIUM | speed > 80 **or** distance < 20 | Reduce Speed |
| LOW | speed > 50 **or** distance < 40 | Stay Alert |
| SAFE | none of the above | – |

- **Lane warning:** `lane_status == 1`
- **Obstacle warning:** `distance < 20`

---

## 💡 LED Alerts (STM32F407 Discovery)

| LED | Pin | Meaning |
|-----|-----|---------|
| 🟢 Green | PD12 | Heartbeat – blinks at 1 Hz. If it stops, the system has frozen |
| 🟠 Orange | PD13 | Lane departure warning |
| 🔴 Red | PD14 | Obstacle too close |
| 🔵 Blue | PD15 | Risk HIGH or CRITICAL |
| All 4 solid ON | – | Fault: stack overflow or RTOS object allocation failed (CPU halted on purpose) |

Warning LEDs turn off automatically if no new warning arrives for 1.5 s.

---

## 📊 Example Output (captured on hardware via SWV)

```
LANE=1, DIST=12, SPEED=102
ALERT: Lane Deviation!
ALERT: Obstacle Too Close!
RISK LEVEL: HIGH
ACTION: Slow Down Immediately
--------------------------
LANE=1, DIST=70, SPEED=46
ALERT: Lane Deviation!
RISK LEVEL: SAFE
--------------------------
LANE=0, DIST=4, SPEED=116
ALERT: Obstacle Too Close!
RISK LEVEL: CRITICAL
ACTION: APPLY BRAKES!
--------------------------
SYSTEM STATUS: Running... free heap=25440 B
```

---

## 🔁 Porting to STM32F407 & Hardware Testing (Nov 2026)

The original firmware targeted the STM32F103C8. It was ported to the STM32F407 Discovery board:

**Chip configuration (`Mini_ADAS_F407.ioc`)**
- SYS: Debug = *Trace Asynchronous Sw* (SWD + SWO for printf), HAL timebase = **TIM6** (FreeRTOS owns SysTick)
- USART1: asynchronous, 115200 baud, PA9/PA10
- FreeRTOS: CMSIS_V2, heap 32 KB, stack overflow check = option 2, newlib reentrant enabled
- Clock: HSI 16 MHz
- LEDs PD12–PD15 configured in the `USER CODE` section of `MX_GPIO_Init()`

### 🐞 Bugs found during hardware testing

Testing on real hardware exposed issues that were not visible in simulation:

| Bug | Effect | Fix |
|-----|--------|-----|
| FreeRTOS heap only 3 KB | The last tasks/objects silently failed to allocate | Heap increased to 32 KB; every RTOS object is checked after creation |
| DecisionTask never sent the alert to `alertQueue` | AlertTask printed a risk level from uninitialised memory | DecisionTask now sends the alert before setting the flags |
| Old alerts stayed in `alertQueue` | Wrong risk was reported (e.g. speed 98 shown as LOW instead of MEDIUM) | AlertTask drains the queue and uses only the newest alert |
| 128-word stacks with `printf` | Risk of stack overflow | 256-word stacks + overflow hook (all LEDs on, CPU halted) |
| `printf` from several tasks | Interleaved output lines | Mutex around the output function |

### Known limitations
- A CRITICAL risk caused **only by speed** raises no alert, because AlertTask wakes only on lane/obstacle flags. Planned fix: add a `RISK_ALERT_FLAG` for HIGH/CRITICAL.
- An occasional duplicated `SYSTEM STATUS` line has been seen in the SWV console; cause not yet confirmed.

---

## ▶️ Build, Flash & Run (STM32F407 Discovery)

1. In STM32CubeIDE: *File → New → STM32 Project from an Existing STM32CubeMX Configuration File* → select `Mini_ADAS_F407/Mini_ADAS_F407.ioc`.
   (Eclipse `.project`/`.cproject` files are not in the repo, so the project is recreated from the `.ioc`; all `USER CODE` sections are kept.)
2. Connect the board via the **mini-USB (ST-LINK)** port.
3. Build with **Ctrl+B**.
4. Debug with **F11** – the program stops at `main()`; press **F8** to run.
5. To see `printf` output without a USB-UART adapter:
   - Debug configuration → *Debugger* tab → enable **Serial Wire Viewer**, core clock **16 MHz**
   - *Window → Show View → SWV → SWV ITM Data Console* → settings → enable **port 0**
   - While halted, press **Start Trace**, then resume.
6. Optional: enable *RTOS Proxy* (driver FreeRTOS, port ARM_CM4F) to see all FreeRTOS threads in the Debug view.

> Only write code between `/* USER CODE BEGIN */` and `/* USER CODE END */` – CubeMX overwrites everything else when regenerating.

---

## 🧪 Software-in-the-Loop (SIL) Validation

Before hardware was available, the logic was validated on the PC:

- CSV-based test case execution with automated PASS/FAIL validation
- Execution time measurement and logging (`log.csv`)
- Graphs with Python (matplotlib) and an interactive Streamlit dashboard

**Workflow:** Test Cases → C Logic → CSV Log → Graphs → Dashboard

| File | Purpose |
|------|---------|
| `ADAS_Testing/test.c` | Test execution engine |
| `ADAS_Testing/test_cases.csv` | Input scenarios |
| `ADAS_Testing/log.csv` | Output log |
| `ADAS_Testing/plot.py` | Graph visualisation |
| `ADAS_Testing/dashboard.py` | Interactive dashboard |

### 💻 C Simulation (Windows)

`simulation/adas_simulation.c` is a console version of the decision logic for quick checks:

```
gcc simulation/adas_simulation.c -o adas
adas
```

---

## 📡 CAN Bus

CAN integration design (ECU-style messages, timing, EMC, analyzers) is documented in
[README_CAN_INTEGRATION.md](README_CAN_INTEGRATION.md). CAN on hardware is planned next (CAN1 loopback mode first).

---

## 🚀 Roadmap

- [ ] Alert for speed-only HIGH/CRITICAL risk (`RISK_ALERT_FLAG`)
- [ ] On-target unit tests and fixed test vectors (`ADAS_TEST_MODE`)
- [ ] User button (PA0) as driver acknowledge via EXTI interrupt + semaphore
- [ ] Watchdog (IWDG) with task health monitoring
- [ ] CAN1 loopback alert frames
- [ ] Real sensors: HC-SR04 ultrasonic (distance), ADC input (lane position)
- [ ] Shared decision-logic module used by both simulation and firmware
- [ ] Edge AI for lane/object detection

---

## 🎯 What This Project Demonstrates

- RTOS-based embedded design: task priorities, queues, event flags, mutexes
- Porting firmware between STM32 families (F1 → F4) with STM32CubeMX
- Debugging on real hardware: SWV/ITM tracing, FreeRTOS thread awareness, stack/heap analysis
- Finding and fixing concurrency and memory bugs
- Simulation-first development with SIL validation
- Automotive ECU-style architecture thinking

---

## 👨‍💻 Author

**Mohit Babu Pasam**
Email: mohitbabuuol@gmail.com
