# Changelog

All notable changes to this project are documented here.
Format based on [Keep a Changelog](https://keepachangelog.com/).

## [1.1.0] - 2026-11-07

### Fixed
- **Build:** added the missing HAL CAN driver (v1.1.10), enabled `HAL_CAN_MODULE_ENABLED`,
  moved CAN code into `Core/` (the old `CAN/` folder was not compiled) and fixed
  conflicting declarations in `main.c` / `main.h`.
- **Debugging:** Serial Wire Debug was disabled (`SWJ_DISABLE`), so ST-LINK could not
  connect after flashing. SWD is now enabled.
- **FreeRTOS heap:** 3 KB was too small for 7 tasks (tasks silently failed to start).
  Increased to 12 KB; task creation is now checked.
- **Alert path:** `alertQueue` had no consumer. Added `AlertTask` driving the PA5 LED.
- **Logging:** removed `printf` from multiple tasks and from the CAN ISR. A dedicated
  `LoggerTask` now owns the UART (115200 baud).
- **CAN bit timing:** was 125 kbit/s, now 500 kbit/s (87.5 % sample point).
- **Startup order:** HAL init that depends on `HAL_GetTick()` now runs before the first
  RTOS object is created.

### Added
- CAN TX/RX tasks with interrupt-driven reception and message codec (IDs 0x100, 0x200, 0x300).
- Silent-loopback CAN mode for testing without a transceiver.
- `HealthMonitorTask`: task heartbeats, stack/heap watermarks, queue overflows,
  CAN error state, status frame 0x300.
- Risk levels SAFE / LOW / MEDIUM / HIGH / CRITICAL with speed-dependent thresholds.
- Fail-safe distance filter (reacts immediately when an obstacle gets closer).
- Stack-overflow and malloc-failed hooks; debugger breakpoint in fault handlers.
- `g_adas_debug` structure for STM32CubeIDE Live Expressions.
- SIL test suite sharing `adas_logic.c` / `adas_can.c` with the firmware:
  25 scenarios + unit tests (219 checks).
- Streamlit dashboard, updated plots and PC simulation of the firmware pipeline.

### Changed
- `.ioc` synchronised with the code so CubeMX code generation keeps all changes.
- README rewritten to describe only implemented features.

### Removed
- Fake execution-time busy loop in the old test runner.
- Outdated `README_CAN_INTEGRATION.md` (content merged into README).
