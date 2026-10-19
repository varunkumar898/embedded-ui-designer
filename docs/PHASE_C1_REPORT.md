# Phase C1 report — plan-driven STM32F030R8 generation from the pin/peripheral plan

Status: **completed and accepted** (accepted by the project owner on 2026-10-19).
Base branch: `feat/pin-plan-codegen`; commits:

- `901b39e` — Phase C1 implementation, capability data additions, and tests.
- `01a4973` — Docs: regenerate the STM32F030R8 value table (199 rows: 87 alternate-function rows + 112 other rows; I2C TIMINGR entries and max-clock entries with sources).

Everything below reflects the post-C1 state on that branch. Re-verification of the
key facts was done on 2026-10-19 (test logs and output hashes were re-checked).

## What C1 delivers

- The generation gate: the plan-driven path runs **only for STM32F030R8** and **only when
  the project's pin/peripheral plan actually has configured pins or peripherals**
  (`target.isDefaultF030 && !PeripheralPlan::configIsEmpty(hw)`). Every other device
  still uses the previous (template) generation path unchanged.
- Generation from `PeripheralPlan` for **USART, SPI, I2C, and GPIO**:
  pin mode configuration, peripheral clock enable, configuration registers, and
  enable/start bits — derived only from pin capability data that carries sources.
- Clock resolution (`resolveBusClock`): bus-specific clock value (apb1/apb2/hclk)
  when present; otherwise the system clock is assumed to drive the bus (documented
  assumption, Info diagnostic); otherwise the **8 MHz HSI reset default** is used and
  stated in the generated comment plus an Info diagnostic (documented:
  RM0360 Rev 5, section 7.2.6).
- I2C timing register values are taken **only** from the transcribed RM0360
  Tables 75/76/77 examples stored in the data file; they are never computed.
  I2C1 supports up to 1 MHz, I2C2 up to 400 kHz (Table 68); unsupported
  (clock, speed) combinations skip the whole peripheral with an Info diagnostic.
- SPI baud rate divisor: the highest legal division value (`fPCLK / 2^(BR+1)`)
  that does not exceed the requested speed; when the request is quantized down,
  an "adjusted" Info diagnostic is emitted (asserted in tests).
- Rate parameter reading (`readRateParameter`) tries, in order:
  UART: `baudRate`, `speedOrFreq`; SPI: `clockSpeedHz`, `speedOrFreq`;
  I2C: `speedHz`, `clockSpeedHz`, `speedOrFreq` (the hardware wizard UI writes
  `speedOrFreq`; descriptor defaults and peripheral JSON use the others).
- Missing configuration fields produce **Info diagnostics deduplicated per
  peripheral**; pin conflicts produce **Error** diagnostics; conflicting pins are
  removed from the pin plans before generation (Error diagnostics dominate
  Warnings which dominate Info in severity aggregation).
- Nothing is invented: no alternate-function numbers, register addresses, IRQ
  numbers, or clock bits are guessed. CMSIS device header names are used; no
  vendor code is vendored. A missing capability always means omission plus an
  Info diagnostic.
- Diagnostics surface via `TargetHalGenerator::planDiagnostics()` (static,
  single-slot storage; set by `generateHalSource`, cleared at its top) and are
  appended to the export result's warnings by `FirmwareExportManager` right after
  the `target_hal.c` generation call.

## Guarantees (re-verified 2026-10-19)

- **Golden compatibility**: exporting an empty plan produces byte-identical
  output to the pre-C1 template path. Asserted in tests by exact SHA-256:
  `target_hal.c` = `289cfeb2cfae39d05ac00edd7e9842711e9417cd58bda05c314e9e7b074a3219`,
  `target_hal.h` = `5bc23bdda8d8671112f26b3f48f75955a67b14f4c4ec3482eb624105666219c0`.
  (Probe outputs made before and after C1 — `outA`, `outA2`, `outB2` — all hashed
  to the same `289cfeb2...` value, which directly demonstrates the template path
  was untouched.)
- **Plan-driven output differs and is correct**: the plan-driven probe outputs
  `outA3` and `outB3` have `target_hal.c` hashes
  `11f6d15db8b7e70d045cf33d0b67f00c1c8b034b6396b758e4ffef3f6e094b6c` and
  `13092dbf991b02cc9a7a8fd6e5135b14b0050956caf565fdc4fa29a50c3b1c0a` respectively,
  both different from the template path and from each other. Probe A exercised
  USART1 on PA9/PA10 plus a missing-clock configuration that exercises the
  documented 8 MHz HSI diagnostic; probe B exercised SPI1/I2C1 pins with the
  conflicting/associated pin checks.
- **Test runs**: full in-repo post-C1 ctest: **24/24 test suites passed**
  (114.64 s; the pre-C1 baseline run was also 24/24 in 114.17 s). The AllCases
  suite passed (its previously recorded assertion count was 142/142).
  This includes `CustomPacksAcceptanceTests` and the 8 new C1 export tests.
- **Compile check**: `arm-none-eabi-gcc` (16.2.0) compiled the generated HAL
  against the ST CMSIS device headers (`-mcpu=cortex-m0 -mthumb -std=c11
  -Wall -Wextra`), 0 skipped. This is a compile check only — no hardware run.

## Known limitations and scope exclusions (intentional)

- No NVIC, EXTI, SYSCFG, ADC, Timer, PWM, or DMA code generation in C1
  (left for later phases).
- The plan path is STM32F030R8-only; the ESP adapter work had not started.
- Known data-pack inaccuracies were deliberately left unfixed because fixing
  them requires new sourced facts:
  TIM2 is offered although STM32F030x8 does not have it; PA9/PA10 I2C1 exists
  only on xC and x4/x6 variants (not x8, so PB10/PB11 carry I2C2, which is
  validated); PA2 lacks a real TIM15_CH1; PC13's RTC_OUT/RTC_TAMP1 functions
  are not AFR-selectable.
- Verification status: code is unit-tested and compile-checked. Nothing in this
  phase has been verified on real hardware, and no statement in this report
  claims otherwise.
