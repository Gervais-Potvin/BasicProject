# Copilot instructions — BorneAmicale

Short, focused guidance to help AI coding agents be productive in this repo.

## Purpose
- This project is an Arduino/PlatformIO firmware for a charging station (Mega2560).
- Primary runtime is embedded; most behaviour lives in `src/` and cross-module globals are defined in `include/`.

## Big picture architecture
- Entrypoint: `src/main.cpp` — holds global state, utility functions and many hardware integrations.
- State machine: `src/state_machine.cpp` with the `State` enum declared in `include/config.h`. Transitions use `handleTransitions()` and actions use `runStateActions()`.
- Modules: logical modules are split between `src/*.cpp` and `include/*.h` (e.g., `rfid`, `sdcard`, `rtc`, `lcd`, `sct013`, `admin`, `hardware`). Inspect these files for module-specific APIs.

## Project-specific patterns to preserve
- Global state pattern: `State state` plus boolean `entry_*` flags implement one-shot entry actions — changing this requires updating many call sites.
- UID handling: UID is a colon-separated hex string; validation is implemented in `isValidRFID()` inside `src/main.cpp`.
- SD I/O pattern: use `selectSD()` / `sdDeselectAll()` (see `src/sdcard.cpp`) before/after `SD.begin()` and file ops to avoid SPI conflicts with the RFID module.
- LCD rest mode: `lcdRestMode` gates display updates — avoid forcing LCD writes when rest mode is active.
- EEPROM: small, manual layout for storing a resume UID (`EEPROM_UID_ADDR` in `src/main.cpp`). Keep offsets consistent if modifying.

## Build / flash / debug workflows
- Build: `pio run` (or `platformio run`).
- Build + upload to the Mega2560 environment: `pio run -e mega2560 -t upload`.
- Serial monitor: `pio device monitor -p <COMx> -b 115200` (board configured at 115200 in `platformio.ini`).
- Quick check of available serial ports: `pio device list`.
- The project already sets `build_flags = -DDEBUG=1` in `platformio.ini`; toggle this or override to enable/disable `DBG()` logging.

## Key files to inspect (start here)
- `src/main.cpp` — globals, hardware init, utility functions, RTC/SD helpers.
- `src/state_machine.cpp` & `include/state_machine.h` — core runtime flow and per-state logic (`fctEtape1..8`).
- `src/sdcard.cpp` & `include/sdcard.h` — card roster, CSV format (`CardYYYY.csv`) and `LogYYYY.csv` conventions.
- `include/config.h` — `State` enum and key externs.
- `include/hardware.h` — pin map (RFID_SS, SD_CS, PIN_SSR_A/B, etc.).

## External dependencies / integration points
- PlatformIO env: `[env:mega2560]` (see `platformio.ini`).
- Libraries declared: `RTClib`, `MFRC522`, `LiquidCrystal_I2C`, `SD` (managed by PlatformIO).
- Hardware: MFRC522 (SPI), DS3231 RTC (I2C), LCD over I2C, SD over SPI, SCT013 current sensor on analog input.

## Safe modification guidelines
- Avoid refactoring global variables into local scope without updating all callers (they are widely referenced across modules).
- When changing SD/RFID SPI usage, retain `selectSD()`/`sdDeselectAll()` semantics to prevent bus contention.
- Keep log/file naming format (`LogYYYY.csv`, `CardYYYY.csv`) for compatibility with existing data on SD cards.
- If moving functions out of `main.cpp`, ensure `getTimestamp()`, `readRFID()` and logging utilities remain accessible.

## Common quick tasks (examples)
- Add a new admin serial command: update `handleSerialCommands()` in `src/main.cpp` and add the implementation in a matching module (e.g., `admin.*`).
- Add a new LCD screen: create `lcdX()` in `src/main.cpp` and call it from an appropriate `fctEtapeN()` in `src/state_machine.cpp`.

## What not to change lightly
- The `entry_*` flag convention and the global `state` machine wiring.
- The SD/RFID selection/deselection logic.
- ADC pin usage: `A0` is used for current sensing (SCT013) — watch for conflicts with other A0 usages.

If anything above is unclear or you want me to expand a section (examples, file links, or automated checks), tell me which part to iterate on.
