# STM32F030R8 pin capability data - full per-value provenance

Source of truth: `data/hardware_packs/stmicroelectronics/stm32f030r8_pin_data.json`.
Every row below is derived from the `sources` / `license` / `verified` fields of that file;
the table is an audit view of the pin capability data (Phase B), kept in the repository so
each generated value can be traced to a document, revision and table/section.

License codes:  
- **L1** = Vendor-document fact, transcribed with citation (no vendor content redistributed)  
- **L2** = Apache-2.0 (CMSIS stm32f030x8.h) + vendor-document fact, transcribed with citation  
- **L3** = Apache-2.0 (stm32f030x8.h, STM32Cube_FW_F0_V1.11.6 per License.md)  

## Alternate functions (87 entries, DS9773 Rev 5 Tables 12/13, x8 filter applied)

| Value | Document | Rev | Table / page | License |
|---|---|---|---|---|
| PA0: USART2_CTS = AF1 | DS9773 | Rev 5 (Nov 2021) | Table 12 (GPIOA_AFR alternate functions), pin PA0 | L1 |
| PA1: EVENTOUT = AF0 | DS9773 | Rev 5 (Nov 2021) | Table 12 (GPIOA_AFR alternate functions), pin PA1 | L1 |
| PA1: USART2_RTS = AF1 | DS9773 | Rev 5 (Nov 2021) | Table 12 (GPIOA_AFR alternate functions), pin PA1 | L1 |
| PA2: TIM15_CH1 = AF0 | DS9773 | Rev 5 (Nov 2021) | Table 12 (GPIOA_AFR alternate functions), pin PA2 | L1 |
| PA2: USART2_TX = AF1 | DS9773 | Rev 5 (Nov 2021) | Table 12 (GPIOA_AFR alternate functions), pin PA2 | L1 |
| PA3: TIM15_CH2 = AF0 | DS9773 | Rev 5 (Nov 2021) | Table 12 (GPIOA_AFR alternate functions), pin PA3 | L1 |
| PA3: USART2_RX = AF1 | DS9773 | Rev 5 (Nov 2021) | Table 12 (GPIOA_AFR alternate functions), pin PA3 | L1 |
| PA4: SPI1_NSS = AF0 | DS9773 | Rev 5 (Nov 2021) | Table 12 (GPIOA_AFR alternate functions), pin PA4 | L1 |
| PA4: USART2_CK = AF1 | DS9773 | Rev 5 (Nov 2021) | Table 12 (GPIOA_AFR alternate functions), pin PA4 | L1 |
| PA4: TIM14_CH1 = AF3 | DS9773 | Rev 5 (Nov 2021) | Table 12 (GPIOA_AFR alternate functions), pin PA4 | L1 |
| PA5: SPI1_SCK = AF0 | DS9773 | Rev 5 (Nov 2021) | Table 12 (GPIOA_AFR alternate functions), pin PA5 | L1 |
| PA6: SPI1_MISO = AF0 | DS9773 | Rev 5 (Nov 2021) | Table 12 (GPIOA_AFR alternate functions), pin PA6 | L1 |
| PA6: TIM3_CH1 = AF1 | DS9773 | Rev 5 (Nov 2021) | Table 12 (GPIOA_AFR alternate functions), pin PA6 | L1 |
| PA6: TIM1_BKIN = AF2 | DS9773 | Rev 5 (Nov 2021) | Table 12 (GPIOA_AFR alternate functions), pin PA6 | L1 |
| PA6: TIM16_CH1 = AF5 | DS9773 | Rev 5 (Nov 2021) | Table 12 (GPIOA_AFR alternate functions), pin PA6 | L1 |
| PA6: EVENTOUT = AF6 | DS9773 | Rev 5 (Nov 2021) | Table 12 (GPIOA_AFR alternate functions), pin PA6 | L1 |
| PA7: SPI1_MOSI = AF0 | DS9773 | Rev 5 (Nov 2021) | Table 12 (GPIOA_AFR alternate functions), pin PA7 | L1 |
| PA7: TIM3_CH2 = AF1 | DS9773 | Rev 5 (Nov 2021) | Table 12 (GPIOA_AFR alternate functions), pin PA7 | L1 |
| PA7: TIM1_CH1N = AF2 | DS9773 | Rev 5 (Nov 2021) | Table 12 (GPIOA_AFR alternate functions), pin PA7 | L1 |
| PA7: TIM14_CH1 = AF4 | DS9773 | Rev 5 (Nov 2021) | Table 12 (GPIOA_AFR alternate functions), pin PA7 | L1 |
| PA7: TIM17_CH1 = AF5 | DS9773 | Rev 5 (Nov 2021) | Table 12 (GPIOA_AFR alternate functions), pin PA7 | L1 |
| PA7: EVENTOUT = AF6 | DS9773 | Rev 5 (Nov 2021) | Table 12 (GPIOA_AFR alternate functions), pin PA7 | L1 |
| PA8: MCO = AF0 | DS9773 | Rev 5 (Nov 2021) | Table 12 (GPIOA_AFR alternate functions), pin PA8 | L1 |
| PA8: USART1_CK = AF1 | DS9773 | Rev 5 (Nov 2021) | Table 12 (GPIOA_AFR alternate functions), pin PA8 | L1 |
| PA8: TIM1_CH1 = AF2 | DS9773 | Rev 5 (Nov 2021) | Table 12 (GPIOA_AFR alternate functions), pin PA8 | L1 |
| PA8: EVENTOUT = AF3 | DS9773 | Rev 5 (Nov 2021) | Table 12 (GPIOA_AFR alternate functions), pin PA8 | L1 |
| PA9: TIM15_BKIN = AF0 | DS9773 | Rev 5 (Nov 2021) | Table 12 (GPIOA_AFR alternate functions), pin PA9 | L1 |
| PA9: USART1_TX = AF1 | DS9773 | Rev 5 (Nov 2021) | Table 12 (GPIOA_AFR alternate functions), pin PA9 | L1 |
| PA9: TIM1_CH2 = AF2 | DS9773 | Rev 5 (Nov 2021) | Table 12 (GPIOA_AFR alternate functions), pin PA9 | L1 |
| PA10: TIM17_BKIN = AF0 | DS9773 | Rev 5 (Nov 2021) | Table 12 (GPIOA_AFR alternate functions), pin PA10 | L1 |
| PA10: USART1_RX = AF1 | DS9773 | Rev 5 (Nov 2021) | Table 12 (GPIOA_AFR alternate functions), pin PA10 | L1 |
| PA10: TIM1_CH3 = AF2 | DS9773 | Rev 5 (Nov 2021) | Table 12 (GPIOA_AFR alternate functions), pin PA10 | L1 |
| PA11: EVENTOUT = AF0 | DS9773 | Rev 5 (Nov 2021) | Table 12 (GPIOA_AFR alternate functions), pin PA11 | L1 |
| PA11: USART1_CTS = AF1 | DS9773 | Rev 5 (Nov 2021) | Table 12 (GPIOA_AFR alternate functions), pin PA11 | L1 |
| PA11: TIM1_CH4 = AF2 | DS9773 | Rev 5 (Nov 2021) | Table 12 (GPIOA_AFR alternate functions), pin PA11 | L1 |
| PA12: EVENTOUT = AF0 | DS9773 | Rev 5 (Nov 2021) | Table 12 (GPIOA_AFR alternate functions), pin PA12 | L1 |
| PA12: USART1_RTS = AF1 | DS9773 | Rev 5 (Nov 2021) | Table 12 (GPIOA_AFR alternate functions), pin PA12 | L1 |
| PA12: TIM1_ETR = AF2 | DS9773 | Rev 5 (Nov 2021) | Table 12 (GPIOA_AFR alternate functions), pin PA12 | L1 |
| PA13: SWDIO = AF0 | DS9773 | Rev 5 (Nov 2021) | Table 12 (GPIOA_AFR alternate functions), pin PA13 | L1 |
| PA13: IR_OUT = AF1 | DS9773 | Rev 5 (Nov 2021) | Table 12 (GPIOA_AFR alternate functions), pin PA13 | L1 |
| PA14: SWCLK = AF0 | DS9773 | Rev 5 (Nov 2021) | Table 12 (GPIOA_AFR alternate functions), pin PA14 | L1 |
| PA14: USART2_TX = AF1 | DS9773 | Rev 5 (Nov 2021) | Table 12 (GPIOA_AFR alternate functions), pin PA14 | L1 |
| PA15: SPI1_NSS = AF0 | DS9773 | Rev 5 (Nov 2021) | Table 12 (GPIOA_AFR alternate functions), pin PA15 | L1 |
| PA15: USART2_RX = AF1 | DS9773 | Rev 5 (Nov 2021) | Table 12 (GPIOA_AFR alternate functions), pin PA15 | L1 |
| PA15: EVENTOUT = AF3 | DS9773 | Rev 5 (Nov 2021) | Table 12 (GPIOA_AFR alternate functions), pin PA15 | L1 |
| PB0: EVENTOUT = AF0 | DS9773 | Rev 5 (Nov 2021) | Table 13 (GPIOB_AFR alternate functions), pin PB0 | L1 |
| PB0: TIM3_CH3 = AF1 | DS9773 | Rev 5 (Nov 2021) | Table 13 (GPIOB_AFR alternate functions), pin PB0 | L1 |
| PB0: TIM1_CH2N = AF2 | DS9773 | Rev 5 (Nov 2021) | Table 13 (GPIOB_AFR alternate functions), pin PB0 | L1 |
| PB1: TIM14_CH1 = AF0 | DS9773 | Rev 5 (Nov 2021) | Table 13 (GPIOB_AFR alternate functions), pin PB1 | L1 |
| PB1: TIM3_CH4 = AF1 | DS9773 | Rev 5 (Nov 2021) | Table 13 (GPIOB_AFR alternate functions), pin PB1 | L1 |
| PB1: TIM1_CH3N = AF2 | DS9773 | Rev 5 (Nov 2021) | Table 13 (GPIOB_AFR alternate functions), pin PB1 | L1 |
| PB3: SPI1_SCK = AF0 | DS9773 | Rev 5 (Nov 2021) | Table 13 (GPIOB_AFR alternate functions), pin PB3 | L1 |
| PB3: EVENTOUT = AF1 | DS9773 | Rev 5 (Nov 2021) | Table 13 (GPIOB_AFR alternate functions), pin PB3 | L1 |
| PB4: SPI1_MISO = AF0 | DS9773 | Rev 5 (Nov 2021) | Table 13 (GPIOB_AFR alternate functions), pin PB4 | L1 |
| PB4: TIM3_CH1 = AF1 | DS9773 | Rev 5 (Nov 2021) | Table 13 (GPIOB_AFR alternate functions), pin PB4 | L1 |
| PB4: EVENTOUT = AF2 | DS9773 | Rev 5 (Nov 2021) | Table 13 (GPIOB_AFR alternate functions), pin PB4 | L1 |
| PB5: SPI1_MOSI = AF0 | DS9773 | Rev 5 (Nov 2021) | Table 13 (GPIOB_AFR alternate functions), pin PB5 | L1 |
| PB5: TIM3_CH2 = AF1 | DS9773 | Rev 5 (Nov 2021) | Table 13 (GPIOB_AFR alternate functions), pin PB5 | L1 |
| PB5: TIM16_BKIN = AF2 | DS9773 | Rev 5 (Nov 2021) | Table 13 (GPIOB_AFR alternate functions), pin PB5 | L1 |
| PB5: I2C1_SMBA = AF3 | DS9773 | Rev 5 (Nov 2021) | Table 13 (GPIOB_AFR alternate functions), pin PB5 | L1 |
| PB6: USART1_TX = AF0 | DS9773 | Rev 5 (Nov 2021) | Table 13 (GPIOB_AFR alternate functions), pin PB6 | L1 |
| PB6: I2C1_SCL = AF1 | DS9773 | Rev 5 (Nov 2021) | Table 13 (GPIOB_AFR alternate functions), pin PB6 | L1 |
| PB6: TIM16_CH1N = AF2 | DS9773 | Rev 5 (Nov 2021) | Table 13 (GPIOB_AFR alternate functions), pin PB6 | L1 |
| PB7: USART1_RX = AF0 | DS9773 | Rev 5 (Nov 2021) | Table 13 (GPIOB_AFR alternate functions), pin PB7 | L1 |
| PB7: I2C1_SDA = AF1 | DS9773 | Rev 5 (Nov 2021) | Table 13 (GPIOB_AFR alternate functions), pin PB7 | L1 |
| PB7: TIM17_CH1N = AF2 | DS9773 | Rev 5 (Nov 2021) | Table 13 (GPIOB_AFR alternate functions), pin PB7 | L1 |
| PB8: I2C1_SCL = AF1 | DS9773 | Rev 5 (Nov 2021) | Table 13 (GPIOB_AFR alternate functions), pin PB8 | L1 |
| PB8: TIM16_CH1 = AF2 | DS9773 | Rev 5 (Nov 2021) | Table 13 (GPIOB_AFR alternate functions), pin PB8 | L1 |
| PB9: IR_OUT = AF0 | DS9773 | Rev 5 (Nov 2021) | Table 13 (GPIOB_AFR alternate functions), pin PB9 | L1 |
| PB9: I2C1_SDA = AF1 | DS9773 | Rev 5 (Nov 2021) | Table 13 (GPIOB_AFR alternate functions), pin PB9 | L1 |
| PB9: TIM17_CH1 = AF2 | DS9773 | Rev 5 (Nov 2021) | Table 13 (GPIOB_AFR alternate functions), pin PB9 | L1 |
| PB9: EVENTOUT = AF3 | DS9773 | Rev 5 (Nov 2021) | Table 13 (GPIOB_AFR alternate functions), pin PB9 | L1 |
| PB10: I2C2_SCL = AF1 | DS9773 | Rev 5 (Nov 2021) | Table 13 (GPIOB_AFR alternate functions), pin PB10 | L1 |
| PB11: EVENTOUT = AF0 | DS9773 | Rev 5 (Nov 2021) | Table 13 (GPIOB_AFR alternate functions), pin PB11 | L1 |
| PB11: I2C2_SDA = AF1 | DS9773 | Rev 5 (Nov 2021) | Table 13 (GPIOB_AFR alternate functions), pin PB11 | L1 |
| PB12: SPI2_NSS = AF0 | DS9773 | Rev 5 (Nov 2021) | Table 13 (GPIOB_AFR alternate functions), pin PB12 | L1 |
| PB12: EVENTOUT = AF1 | DS9773 | Rev 5 (Nov 2021) | Table 13 (GPIOB_AFR alternate functions), pin PB12 | L1 |
| PB12: TIM1_BKIN = AF2 | DS9773 | Rev 5 (Nov 2021) | Table 13 (GPIOB_AFR alternate functions), pin PB12 | L1 |
| PB13: SPI2_SCK = AF0 | DS9773 | Rev 5 (Nov 2021) | Table 13 (GPIOB_AFR alternate functions), pin PB13 | L1 |
| PB13: TIM1_CH1N = AF2 | DS9773 | Rev 5 (Nov 2021) | Table 13 (GPIOB_AFR alternate functions), pin PB13 | L1 |
| PB14: SPI2_MISO = AF0 | DS9773 | Rev 5 (Nov 2021) | Table 13 (GPIOB_AFR alternate functions), pin PB14 | L1 |
| PB14: TIM15_CH1 = AF1 | DS9773 | Rev 5 (Nov 2021) | Table 13 (GPIOB_AFR alternate functions), pin PB14 | L1 |
| PB14: TIM1_CH2N = AF2 | DS9773 | Rev 5 (Nov 2021) | Table 13 (GPIOB_AFR alternate functions), pin PB14 | L1 |
| PB15: SPI2_MOSI = AF0 | DS9773 | Rev 5 (Nov 2021) | Table 13 (GPIOB_AFR alternate functions), pin PB15 | L1 |
| PB15: TIM15_CH2 = AF1 | DS9773 | Rev 5 (Nov 2021) | Table 13 (GPIOB_AFR alternate functions), pin PB15 | L1 |
| PB15: TIM1_CH3N = AF2 | DS9773 | Rev 5 (Nov 2021) | Table 13 (GPIOB_AFR alternate functions), pin PB15 | L1 |
| PB15: TIM15_CH1N = AF3 | DS9773 | Rev 5 (Nov 2021) | Table 13 (GPIOB_AFR alternate functions), pin PB15 | L1 |

## Ports, peripherals, system facts (98 entries)

| Value | Document | Rev | Table / page or register macro | License |
|---|---|---|---|---|
| GPIOA base | stm32f030x8.h (CMSIS) | Cube FW_F0 V1.11.6 | macro GPIOA_BASE (+ also: Table 17 (peripheral register boundary addresses)) | L2 |
| GPIOA clock enable | stm32f030x8.h (CMSIS) | Cube FW_F0 V1.11.6 | macro RCC_AHBENR_GPIOAEN_Pos | L3 |
| GPIOA AFR register present: True | DS9773 | Rev 5 (Nov 2021) | Table 12 (GPIOA_AFR registers for port A) | L1 |
| GPIOB base | stm32f030x8.h (CMSIS) | Cube FW_F0 V1.11.6 | macro GPIOB_BASE (+ also: Table 17 (peripheral register boundary addresses)) | L2 |
| GPIOB clock enable | stm32f030x8.h (CMSIS) | Cube FW_F0 V1.11.6 | macro RCC_AHBENR_GPIOBEN_Pos | L3 |
| GPIOB AFR register present: True | DS9773 | Rev 5 (Nov 2021) | Table 13 (GPIOB_AFR registers for port B) | L1 |
| GPIOC base | stm32f030x8.h (CMSIS) | Cube FW_F0 V1.11.6 | macro GPIOC_BASE (+ also: Table 17 (peripheral register boundary addresses)) | L2 |
| GPIOC clock enable | stm32f030x8.h (CMSIS) | Cube FW_F0 V1.11.6 | macro RCC_AHBENR_GPIOCEN_Pos | L3 |
| GPIOC AFR register present: False | DS9773 | Rev 5 (Nov 2021) | Table 14 (GPIOC), note 2: STM32F030x4/x6/x8 do not have the GPIOC_AFR registers | L1 |
| USART1 present on device | DS9773 | Rev 5 (Nov 2021) | Table 2 (device features and peripheral counts), STM32F030R8 column | L1 |
| USART1 base | stm32f030x8.h (CMSIS) | Cube FW_F0 V1.11.6 | macro USART1_BASE (+ also: macro APBPERIPH_BASE; Table 17 (peripheral register boundary addresses)) | L2 |
| USART1 clock bus: APB2 | DS9773 | Rev 5 (Nov 2021) | Table 2 (device features and peripheral counts), STM32F030R8 column | L1 |
| USART1 clock enable (RCC_APB2ENR bit 14) | stm32f030x8.h (CMSIS) | Cube FW_F0 V1.11.6 | macro RCC_APB2ENR_USART1EN_Pos | L3 |
| USART1 IRQ USART1 = 27 | RM0360 | Rev 5 (May 2023) | Table 31 (vector table) (+ also: macro IRQn_Type USART1_IRQn) | L1 |
| USART2 present on device | DS9773 | Rev 5 (Nov 2021) | Table 2 (device features and peripheral counts), STM32F030R8 column | L1 |
| USART2 base | stm32f030x8.h (CMSIS) | Cube FW_F0 V1.11.6 | macro USART2_BASE (+ also: macro APBPERIPH_BASE; Table 17 (peripheral register boundary addresses)) | L2 |
| USART2 clock bus: APB1 | DS9773 | Rev 5 (Nov 2021) | Table 2 (device features and peripheral counts), STM32F030R8 column | L1 |
| USART2 clock enable (RCC_APB1ENR bit 17) | stm32f030x8.h (CMSIS) | Cube FW_F0 V1.11.6 | macro RCC_APB1ENR_USART2EN_Pos | L3 |
| USART2 IRQ USART2 = 28 | RM0360 | Rev 5 (May 2023) | Table 31 (vector table) (+ also: macro IRQn_Type USART2_IRQn) | L1 |
| SPI1 present on device | DS9773 | Rev 5 (Nov 2021) | Table 2 (device features and peripheral counts), STM32F030R8 column | L1 |
| SPI1 base | stm32f030x8.h (CMSIS) | Cube FW_F0 V1.11.6 | macro SPI1_BASE (+ also: macro APBPERIPH_BASE; Table 17 (peripheral register boundary addresses)) | L2 |
| SPI1 clock bus: APB2 | DS9773 | Rev 5 (Nov 2021) | Table 2 (device features and peripheral counts), STM32F030R8 column | L1 |
| SPI1 clock enable (RCC_APB2ENR bit 12) | stm32f030x8.h (CMSIS) | Cube FW_F0 V1.11.6 | macro RCC_APB2ENR_SPI1EN_Pos | L3 |
| SPI1 IRQ SPI1 = 25 | RM0360 | Rev 5 (May 2023) | Table 31 (vector table) (+ also: macro IRQn_Type SPI1_IRQn) | L1 |
| SPI2 present on device | DS9773 | Rev 5 (Nov 2021) | Table 2 (device features and peripheral counts), STM32F030R8 column | L1 |
| SPI2 base | stm32f030x8.h (CMSIS) | Cube FW_F0 V1.11.6 | macro SPI2_BASE (+ also: macro APBPERIPH_BASE; Table 17 (peripheral register boundary addresses)) | L2 |
| SPI2 clock bus: APB1 | DS9773 | Rev 5 (Nov 2021) | Table 2 (device features and peripheral counts), STM32F030R8 column | L1 |
| SPI2 clock enable (RCC_APB1ENR bit 14) | stm32f030x8.h (CMSIS) | Cube FW_F0 V1.11.6 | macro RCC_APB1ENR_SPI2EN_Pos | L3 |
| SPI2 IRQ SPI2 = 26 | RM0360 | Rev 5 (May 2023) | Table 31 (vector table) (+ also: macro IRQn_Type SPI2_IRQn) | L1 |
| I2C1 present on device | DS9773 | Rev 5 (Nov 2021) | Table 2 (device features and peripheral counts), STM32F030R8 column | L1 |
| I2C1 base | stm32f030x8.h (CMSIS) | Cube FW_F0 V1.11.6 | macro I2C1_BASE (+ also: macro APBPERIPH_BASE; Table 17 (peripheral register boundary addresses)) | L2 |
| I2C1 clock bus: APB1 | DS9773 | Rev 5 (Nov 2021) | Table 2 (device features and peripheral counts), STM32F030R8 column | L1 |
| I2C1 clock enable (RCC_APB1ENR bit 21) | stm32f030x8.h (CMSIS) | Cube FW_F0 V1.11.6 | macro RCC_APB1ENR_I2C1EN_Pos | L3 |
| I2C1 IRQ I2C1 = 23 | RM0360 | Rev 5 (May 2023) | Table 31 (vector table) (+ also: macro IRQn_Type I2C1_IRQn) | L1 |
| I2C2 present on device | DS9773 | Rev 5 (Nov 2021) | Table 2 (device features and peripheral counts), STM32F030R8 column | L1 |
| I2C2 base | stm32f030x8.h (CMSIS) | Cube FW_F0 V1.11.6 | macro I2C2_BASE (+ also: macro APBPERIPH_BASE; Table 17 (peripheral register boundary addresses)) | L2 |
| I2C2 clock bus: APB1 | DS9773 | Rev 5 (Nov 2021) | Table 2 (device features and peripheral counts), STM32F030R8 column | L1 |
| I2C2 clock enable (RCC_APB1ENR bit 22) | stm32f030x8.h (CMSIS) | Cube FW_F0 V1.11.6 | macro RCC_APB1ENR_I2C2EN_Pos | L3 |
| I2C2 IRQ I2C2 = 24 | RM0360 | Rev 5 (May 2023) | Table 31 (vector table) (+ also: macro IRQn_Type I2C2_IRQn) | L1 |
| ADC1 present on device | DS9773 | Rev 5 (Nov 2021) | Table 2 (device features and peripheral counts), STM32F030R8 column | L1 |
| ADC1 base | stm32f030x8.h (CMSIS) | Cube FW_F0 V1.11.6 | macro ADC1_BASE (+ also: macro APBPERIPH_BASE; Table 17 (peripheral register boundary addresses)) | L2 |
| ADC1 clock bus: APB2 | DS9773 | Rev 5 (Nov 2021) | Table 2 (device features and peripheral counts), STM32F030R8 column | L1 |
| ADC1 clock enable (RCC_APB2ENR bit 9) | stm32f030x8.h (CMSIS) | Cube FW_F0 V1.11.6 | macro RCC_APB2ENR_ADCEN_Pos | L3 |
| ADC1 IRQ ADC1 = 12 | RM0360 | Rev 5 (May 2023) | Table 31 (vector table) (+ also: macro IRQn_Type ADC1_IRQn) | L1 |
| TIM1 present on device | DS9773 | Rev 5 (Nov 2021) | Table 2 (device features and peripheral counts), STM32F030R8 column | L1 |
| TIM1 base | stm32f030x8.h (CMSIS) | Cube FW_F0 V1.11.6 | macro TIM1_BASE (+ also: macro APBPERIPH_BASE; Table 17 (peripheral register boundary addresses)) | L2 |
| TIM1 clock bus: APB2 | DS9773 | Rev 5 (Nov 2021) | Table 2 (device features and peripheral counts), STM32F030R8 column | L1 |
| TIM1 clock enable (RCC_APB2ENR bit 11) | stm32f030x8.h (CMSIS) | Cube FW_F0 V1.11.6 | macro RCC_APB2ENR_TIM1EN_Pos | L3 |
| TIM1 IRQ TIM1_BRK_UP_TRG_COM = 13 | RM0360 | Rev 5 (May 2023) | Table 31 (vector table) (+ also: macro IRQn_Type TIM1_BRK_UP_TRG_COM_IRQn) | L1 |
| TIM1 IRQ TIM1_CC = 14 | RM0360 | Rev 5 (May 2023) | Table 31 (vector table) (+ also: macro IRQn_Type TIM1_CC_IRQn) | L1 |
| TIM3 present on device | DS9773 | Rev 5 (Nov 2021) | Table 2 (device features and peripheral counts), STM32F030R8 column | L1 |
| TIM3 base | stm32f030x8.h (CMSIS) | Cube FW_F0 V1.11.6 | macro TIM3_BASE (+ also: macro APBPERIPH_BASE; Table 17 (peripheral register boundary addresses)) | L2 |
| TIM3 clock bus: APB1 | DS9773 | Rev 5 (Nov 2021) | Table 2 (device features and peripheral counts), STM32F030R8 column | L1 |
| TIM3 clock enable (RCC_APB1ENR bit 1) | stm32f030x8.h (CMSIS) | Cube FW_F0 V1.11.6 | macro RCC_APB1ENR_TIM3EN_Pos | L3 |
| TIM3 IRQ TIM3 = 16 | RM0360 | Rev 5 (May 2023) | Table 31 (vector table) (+ also: macro IRQn_Type TIM3_IRQn) | L1 |
| TIM6 present on device | DS9773 | Rev 5 (Nov 2021) | Table 2 (device features and peripheral counts), STM32F030R8 column | L1 |
| TIM6 base | stm32f030x8.h (CMSIS) | Cube FW_F0 V1.11.6 | macro TIM6_BASE (+ also: macro APBPERIPH_BASE; Table 17 (peripheral register boundary addresses)) | L2 |
| TIM6 clock bus: APB1 | DS9773 | Rev 5 (Nov 2021) | Table 2 (device features and peripheral counts), STM32F030R8 column | L1 |
| TIM6 clock enable (RCC_APB1ENR bit 4) | stm32f030x8.h (CMSIS) | Cube FW_F0 V1.11.6 | macro RCC_APB1ENR_TIM6EN_Pos | L3 |
| TIM6 IRQ TIM6 = 17 | RM0360 | Rev 5 (May 2023) | Table 31 (vector table) (+ also: macro IRQn_Type TIM6_IRQn) | L1 |
| TIM14 present on device | DS9773 | Rev 5 (Nov 2021) | Table 2 (device features and peripheral counts), STM32F030R8 column | L1 |
| TIM14 base | stm32f030x8.h (CMSIS) | Cube FW_F0 V1.11.6 | macro TIM14_BASE (+ also: macro APBPERIPH_BASE; Table 17 (peripheral register boundary addresses)) | L2 |
| TIM14 clock bus: APB1 | DS9773 | Rev 5 (Nov 2021) | Table 2 (device features and peripheral counts), STM32F030R8 column | L1 |
| TIM14 clock enable (RCC_APB1ENR bit 8) | stm32f030x8.h (CMSIS) | Cube FW_F0 V1.11.6 | macro RCC_APB1ENR_TIM14EN_Pos | L3 |
| TIM14 IRQ TIM14 = 19 | RM0360 | Rev 5 (May 2023) | Table 31 (vector table) (+ also: macro IRQn_Type TIM14_IRQn) | L1 |
| TIM15 present on device | DS9773 | Rev 5 (Nov 2021) | Table 2 (device features and peripheral counts), STM32F030R8 column | L1 |
| TIM15 base | stm32f030x8.h (CMSIS) | Cube FW_F0 V1.11.6 | macro TIM15_BASE (+ also: macro APBPERIPH_BASE; Table 17 (peripheral register boundary addresses)) | L2 |
| TIM15 clock bus: APB2 | DS9773 | Rev 5 (Nov 2021) | Table 2 (device features and peripheral counts), STM32F030R8 column | L1 |
| TIM15 clock enable (RCC_APB2ENR bit 16) | stm32f030x8.h (CMSIS) | Cube FW_F0 V1.11.6 | macro RCC_APB2ENR_TIM15EN_Pos | L3 |
| TIM15 IRQ TIM15 = 20 | RM0360 | Rev 5 (May 2023) | Table 31 (vector table) (+ also: macro IRQn_Type TIM15_IRQn) | L1 |
| TIM16 present on device | DS9773 | Rev 5 (Nov 2021) | Table 2 (device features and peripheral counts), STM32F030R8 column | L1 |
| TIM16 base | stm32f030x8.h (CMSIS) | Cube FW_F0 V1.11.6 | macro TIM16_BASE (+ also: macro APBPERIPH_BASE; Table 17 (peripheral register boundary addresses)) | L2 |
| TIM16 clock bus: APB2 | DS9773 | Rev 5 (Nov 2021) | Table 2 (device features and peripheral counts), STM32F030R8 column | L1 |
| TIM16 clock enable (RCC_APB2ENR bit 17) | stm32f030x8.h (CMSIS) | Cube FW_F0 V1.11.6 | macro RCC_APB2ENR_TIM16EN_Pos | L3 |
| TIM16 IRQ TIM16 = 21 | RM0360 | Rev 5 (May 2023) | Table 31 (vector table) (+ also: macro IRQn_Type TIM16_IRQn) | L1 |
| TIM17 present on device | DS9773 | Rev 5 (Nov 2021) | Table 2 (device features and peripheral counts), STM32F030R8 column | L1 |
| TIM17 base | stm32f030x8.h (CMSIS) | Cube FW_F0 V1.11.6 | macro TIM17_BASE (+ also: macro APBPERIPH_BASE; Table 17 (peripheral register boundary addresses)) | L2 |
| TIM17 clock bus: APB2 | DS9773 | Rev 5 (Nov 2021) | Table 2 (device features and peripheral counts), STM32F030R8 column | L1 |
| TIM17 clock enable (RCC_APB2ENR bit 18) | stm32f030x8.h (CMSIS) | Cube FW_F0 V1.11.6 | macro RCC_APB2ENR_TIM17EN_Pos | L3 |
| TIM17 IRQ TIM17 = 22 | RM0360 | Rev 5 (May 2023) | Table 31 (vector table) (+ also: macro IRQn_Type TIM17_IRQn) | L1 |
| NVIC available | DS9773 | Rev 5 (Nov 2021) | features: 32 maskable interrupt channels (+ also: 11.1 Nested vectored interrupt controller (NVIC)) | L1 |
| SYSCFG base | stm32f030x8.h (CMSIS) | Cube FW_F0 V1.11.6 | macro SYSCFG_BASE (+ also: Table 17 (peripheral register boundary addresses)) | L2 |
| SYSCFG EXTI config registers | RM0360 | Rev 5 (May 2023) | 9.1.2-9.1.5 (SYSCFG_EXTICR1..4) | L1 |
| SYSCFG clock enable | stm32f030x8.h (CMSIS) | Cube FW_F0 V1.11.6 | macro RCC_APB2ENR_SYSCFGCOMPEN_Pos | L3 |
| EXTI available | RM0360 | Rev 5 (May 2023) | 11.2 Extended interrupts and events controller (EXTI) | L1 |
| EXTI base | stm32f030x8.h (CMSIS) | Cube FW_F0 V1.11.6 | macro EXTI_BASE (+ also: Table 17 (peripheral register boundary addresses)) | L2 |
| EXTI lines | RM0360 | Rev 5 (May 2023) | 11.2 (EXTI line assignment), 9.1.2-9.1.5 (EXTICR) | L1 |
| EXTI IRQ EXTI0_1 = 5 | RM0360 | Rev 5 (May 2023) | Table 31 (vector table) (+ also: macro IRQn_Type EXTI0_1_IRQn) | L1 |
| EXTI IRQ EXTI2_3 = 6 | RM0360 | Rev 5 (May 2023) | Table 31 (vector table) (+ also: macro IRQn_Type EXTI2_3_IRQn) | L1 |
| EXTI IRQ EXTI4_15 = 7 | RM0360 | Rev 5 (May 2023) | Table 31 (vector table) (+ also: macro IRQn_Type EXTI4_15_IRQn) | L1 |
| DMA available | DS9773 | Rev 5 (Nov 2021) | Table 2 (STM32F030R8: 5-channel DMA controller) | L1 |
| DMA controller | RM0360 | Rev 5 (May 2023) | 10 (Direct memory access controller: one instance, 5 channels) | L1 |
| DMA base | stm32f030x8.h (CMSIS) | Cube FW_F0 V1.11.6 | macro DMA1_BASE (+ also: Table 17 (peripheral register boundary addresses)) | L2 |
| DMA channels | RM0360 | Rev 5 (May 2023) | 10 (one instance of DMA with 5 channels) | L1 |
| DMA clock enable | stm32f030x8.h (CMSIS) | Cube FW_F0 V1.11.6 | macro RCC_AHBENR_DMAEN_Pos | L3 |
| DMA IRQ DMA1_Channel1 = 9 | RM0360 | Rev 5 (May 2023) | Table 31 (vector table) (+ also: macro IRQn_Type DMA1_Channel1_IRQn) | L1 |
| DMA IRQ DMA1_Channel2_3 = 10 | RM0360 | Rev 5 (May 2023) | Table 31 (vector table) (+ also: macro IRQn_Type DMA1_Channel2_3_IRQn) | L1 |
| DMA IRQ DMA1_Channel4_5 = 11 | RM0360 | Rev 5 (May 2023) | Table 31 (vector table) (+ also: macro IRQn_Type DMA1_Channel4_5_IRQn) | L1 |
