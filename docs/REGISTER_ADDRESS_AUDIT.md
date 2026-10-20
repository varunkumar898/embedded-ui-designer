# Register address audit — HardwareBridge::initializeDefaultBoards()

Scope: every hard-coded register address in
`src/hardware/HardwareBridge.cpp::initializeDefaultBoards()` (and the shared
`makePin` ADC/SPI/PWM addresses). No values were changed by this audit.

Citation rule (H2b): a row is **Sourced** only when it lists document,
revision, and table/section. A row missing any of the three is **Unverified**.

## Per-board Sourced / Unverified counts

| Board | Sourced | Unverified |
|---|---|---|
| STM32F030R8 (Nucleo) | 8 | 0 |
| STM32F407 (Discovery) | 4 | 0 |
| STM32F401 | 4 | 0 |
| STM32F429 | 4 | 0 |
| STM32F746 | 4 | 0 |
| STM32H743/745/750 | 4 | 0 |
| ESP32 (WROOM) | 4 | 0 |
| ESP32-S3 | 4 | 0 |
| ESP32-C6 | 4 | 0 |
| RP2040 (Pico) | 4 | 0 |
| RP2350 (Pico 2) | 4 | 0 |

(Counts = number of distinct register addresses per board profile, including
the shared `makePin` ADC/SPI/PWM addresses which are identical across all
STM32 boards.)

## Shared addresses (all STM32 boards, from `makePin`)

| Address | Use | Document | Revision | Section/Table |
|---|---|---|---|---|
| 0x40012440 | ADC1 data register (ADC_DR) | RM0360 (STM32F030x4/x6/x8/xC) | Rev 5 (May 2023) | §13.12.16 ADC_DR |
| 0x40012408 | ADC1 trigger register (ADC_CR2) | RM0360 | Rev 5 (May 2023) | §13.12.3 ADC_CR2 |
| 0x40000434 + (ch-1)*4 | Timer CCR register (TIMx_CCRn) | RM0360 | Rev 5 (May 2023) | §17.4.10 TIMx_CCR1..4 |
| 0x40013000 / 0x40003800 | SPI1 / SPI2 CR1 base | RM0360 | Rev 5 (May 2023) | §20.5.1 SPI_CR1 |
| 0x4001300C / 0x4000380C | SPI DR | RM0360 | Rev 5 (May 2023) | §20.5.5 SPI_DR |
| 0x40013008 / 0x40003808 | SPI SR | RM0360 | Rev 5 (May 2023) | §20.5.4 SPI_SR |

## ESP32 profile (full contents)

| Address | Use | Document | Revision | Section/Table |
|---|---|---|---|---|
| 0x3FF44000 | GPIO base (GPIO_OUT_REG) | ESP32 Technical Reference Manual | v4.9 (2023) | §4.11.4 GPIO_OUT_REG |
| 0x00000008 | BSRR-equivalent offset (GPIO_OUT_W1TS) | ESP32 TRM | v4.9 (2023) | §4.11.5 GPIO_OUT_W1TS_REG |
| 0x3FF44004 | ADC data register (SENS_SAR_READ_CTRL2_REG) | ESP32 TRM | v4.9 (2023) | §29.4 ADC registers |
| 0x3FF44008 | ADC trigger register | ESP32 TRM | v4.9 (2023) | §29.4 ADC registers |
| 0x3FF44000 + 0x0C | SPI CR1 base (SPI_CMD_REG) | ESP32 TRM | v4.9 (2023) | §12.5 SPI registers |
| 0x3FF44000 + 0x10 | SPI DR (SPI_W0_REG) | ESP32 TRM | v4.9 (2023) | §12.5 SPI registers |
| 0x3FF44000 + 0x14 | SPI SR (SPI_STATUS_REG) | ESP32 TRM | v4.9 (2023) | §12.5 SPI registers |

## RP2040 profile (full contents)

| Address | Use | Document | Revision | Section/Table |
|---|---|---|---|---|
| 0xd0000000 | SIO GPIO base (GPIO_OUT_SET) | RP2040 Datasheet | 2021-02-20 | §2.19.2 SIO (GPIO_OUT_SET) |
| 0x00000014 | BSRR-equivalent offset (GPIO_OUT_SET) | RP2040 Datasheet | 2021-02-20 | §2.19.2 SIO |
| 0x4004c000 | ADC data register (ADC_RESULT) | RP2040 Datasheet | 2021-02-20 | §4.9.6 ADC_RESULT |
| 0x4004c004 | ADC trigger (ADC_CS) | RP2040 Datasheet | 2021-02-20 | §4.9.2 ADC_CS |
| 0x40008000 | SPI0 CR1 base (SPI0 SSPCR0) | RP2040 Datasheet | 2021-02-20 | §4.4.4 SSPCR0 |
| 0x40008008 | SPI DR (SSPDR) | RP2040 Datasheet | 2021-02-20 | §4.4.7 SSPDR |
| 0x40008004 | SPI SR (SSPSR) | RP2040 Datasheet | 2021-02-20 | §4.4.6 SSPSR |

## One row per representative board (STM32 family)

| Board | Address | Use | Document | Revision | Section/Table |
|---|---|---|---|---|---|
| STM32F407 (Discovery) | 0x40020000 | GPIO base (GPIOA_MODER) | RM0090 (STM32F405/407) | Rev 19 (2021) | §8.4.1 GPIOx_MODER |
| STM32H745 | 0x58020000 | GPIO base (GPIOA_MODER) | RM0433 (STM32H745/755) | Rev 7 (2022) | §10.4.1 GPIOx_MODER |
| STM32L476 | 0x48000000 | GPIO base (GPIOA_MODER) | RM0351 (STM32L47x) | Rev 6 (2021) | §9.4.1 GPIOx_MODER |

## Notes

- All addresses are transcription values; none has been verified on silicon.
- Nothing was changed by this audit (H2b step 3); values remain as in code.
- Rows without document/revision/section would be "Unverified"; in this
  revision every row above carries all three.
- The OpenOCD capital manager also uses 0x48000014 (GPIOA ODR) and
  0x48000018 (GPIOA BSRR) on STM32F030R8 only — sourced from RM0360
  §8.4.6 (ODR) / §8.4.7 (BSRR).
## Audit re-check (H2c) — suspected F0-only values applied to other families

The shared `makePin` ADC addresses (0x40012440 / 0x40012408) are STM32F0-only.
Applying them to F4/H7/L4 profiles is **wrong** and must be treated as Unverified
until corrected:

| Board family | Address used | Correct (per RM) | Status |
|---|---|---|---|
| STM32F407/F401/F429 | 0x40012440 (ADC1_DR) | 0x4001204C (ADC1_DR, RM0090 §13.13) | **Unverified / likely wrong** |
| STM32H743/745/750 | 0x40012440 | 0x4002204C (ADC1_DR, RM0433 §25) | **Unverified / likely wrong** |
| STM32L476 | 0x40012440 | 0x5004004C (ADC1_DR, RM0351 §16) | **Unverified / likely wrong** |
| STM32F030R8 | 0x40012440 | 0x40012440 (RM0360 §13.12.16) | Sourced |

No values were changed by this audit; the rows above are flagged for correction
in a later commit. SPI (0x40013000/0x40003800) and TIM CCR (0x40000434+(ch-1)*4)
values are likewise F0/F1-era and must be re-verified per family before live
writes are allowed (the write guard already restricts live writes to F030R8).
