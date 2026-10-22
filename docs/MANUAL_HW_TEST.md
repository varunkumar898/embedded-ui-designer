# Manual hardware test procedure — STM32F030R8 (Nucleo-F030R8)

Scope: live control over OpenOCD/SWD. Only STM32F030R8 is live-write
allowed by the write guard. Requires an ST-Link (on-board on the Nucleo).

## Prerequisites

- STM32 Nucleo-F030R8 connected via USB.
- OpenOCD 0.12+ installed (`/usr/bin/openocd`).
- App built from `feat/hardware-link`.

## Steps

1. Start OpenOCD in a terminal:
   `openocd -f interface/stlink.cfg -f target/stm32f0x.cfg`
   Verify it listens on 127.0.0.1:4444 (`ss -ltnp | grep 4444`).
2. Launch the app, open a project with 3 configured pins (e.g. PA5 as
   digital out LED, PA0 as ADC, PA6 as PWM).
3. Open the pin binding dialog; bind a switch component to the digital-out
   signal, a gauge to the ADC signal, a slider to the PWM signal.
4. Click "Connect" in the dialog; verify the badge shows "Connected" only
   after identify() succeeds (chip identity read from DBGMCU_IDCODE
   0x40015800).
5. Toggle the switch in the UI; verify the on-board LED (LD2 on PA5)
   toggles and the live value updates.
6. Move the slider; verify PWM duty changes on PA6.
7. Read the gauge; verify it tracks the potentiometer on PA0.
8. Disconnect the probe; verify the state becomes Error/NoDevice and the UI
   shows "Simulated" only if simulated mode is explicitly enabled (never
   a real board name).

## Expected vs. simulated

- Simulated mode must show "Simulated device" and never a real board name.
- No hardware run in CI; this procedure is manual only.

## Logging

Every live write is logged (time, address, value, result) in the Device
Monitor panel. Out-of-table addresses and non-F030R8 boards are refused
by default.