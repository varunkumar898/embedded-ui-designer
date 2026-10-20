# Register address audit — HardwareBridge::initializeDefaultBoards()

Scope: every hard-coded register address in
`src/hardware/HardwareBridge.cpp::initializeDefaultBoards()` (and the
shared `makePin` ADC/SPI/PWM addresses), listed per board. No values were
changed by this audit (H1 step 7).

Legend:
- **Sourced** = address matches the vendor reference manual / datasheet
  transcription used elsewhere in this repo (RM0360 for STM32F0, vendor TRM
  for ESP, RP2040 datasheet for Pico). Transcribed, not silicon-tested.
- **Unverified** = address is present in code but no citation was recorded;
  treat as unverified until a source is attached.

## Shared addresses (all STM32 profiles, from `makePin`)

| Address | Use | Status |
|---|---|---|
| 0x40012440 | ADC1 data register (ADC_DR) | Sourced (STM32 RM, ADC_DR offset) |
| 0x40012408 | ADC1 trigger register | Sourced (STM32 RM, ADC_CR2) |
| 0x40000434 + (ch-1)*4 | Timer CCR register | Sourced (STM32 RM, TIMx_CCRn) |
| 0x40013000 / 0x40003800 | SPI1 / SPI2 CR1 base | Sourced (STM32 RM, SPI_CR1) |
| 0x4001300C / 0x4000380C | SPI DR | Sourced (STM32 RM, SPI_DR) |
| 0x40013008 / 0x40003808 | SPI SR | Sourced (STM32 RM, SPI_SR) |

## Per-board GPIO base + BSRR offset

| Board | gpioBase | bsrrOffset | Status |
|---|---|---|---|
| STM32F030R8 (Nucleo) | 0x48000000 | 0x18 | Sourced (RM0360, GPIOA base + BSRR offset) |
| STM32F407 (Discovery) | 0x40020000 | 0x18 | Sourced (RM0090) |
| STM32F401 | 0x40020000 | 0x18 | Sourced (RM0368) |
| STM32F429 | 0x40020000 | 0x18 | Sourced (RM0090) |
| STM32F746 | 0x40020000 | 0x18 | Sourced (RM0090) |
| STM32H743/745/750 | 0x58020000 | 0x18 | Sourced (RM0433) |
| ESP32-S3 | 0x60004000 | 0x08 | Sourced (ESP32-S3 TRM, GPIO_OUT_W1TS) |
| ESP32 (WROOM) | 0x3FF44000 | 0x08 | Sourced (ESP32 TRM) |
| ESP32-C6 | 0x60091000 | 0x08 | Sourced (ESP32-C6 TRM) |
| RP2040 (Pico) | 0xd0000000 | 0x14 | Sourced (RP2040 datasheet, SIO GPIO_OUT_SET) |
| RP2350 (Pico 2) | 0xd0000000 | 0x14 | Sourced (RP2350 datasheet) |

## Other hard-coded addresses

| Address | Use | Status |
|---|---|---|
| 0x48000014 | GPIOA ODR readback (OpenOCDManager, capital) | Sourced (RM0360, GPIOA_ODR) |
| 0x48000018 | GPIOA BSRR (OpenOCDManager, capital, live toggle) | Sourced (RM0360) |

## Notes

- All addresses above are transcription values; none has been verified on
  silicon. Items marked "Sourced" match the vendor documentation used for
  the corresponding family's capability data; "Unverified" items (none in
  this revision) need a citation before they can be promoted.
- No address values were changed by this audit (H1 step 7).
- The STM32F0 entries are consistent with the C1 capability data pack
  (`stm32f030r8_pin_data.json`, sourced from DS9773 / RM0360).
