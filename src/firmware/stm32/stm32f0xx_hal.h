#pragma once
#include <stdint.h>
#include <string.h>

/* ==============================================================================
 * Minimal Standalone Register & HAL Definitions for STM32F030R8 (Cortex-M0)
 * ============================================================================== */

#define __IO volatile

/* Base addresses */
#define PERIPH_BASE           (0x40000000UL)
#define AHBPERIPH_BASE        (PERIPH_BASE + 0x00020000UL)
#define AHB2PERIPH_BASE       (PERIPH_BASE + 0x08000000UL)

#define GPIOA_BASE            (AHB2PERIPH_BASE + 0x00000000UL)
#define RCC_BASE              (AHBPERIPH_BASE + 0x00001000UL)

/* SysTick Base */
#define SCS_BASE              (0xE000E000UL)
#define SysTick_BASE          (SCS_BASE + 0x0010UL)

/* GPIO Structure */
typedef struct {
    __IO uint32_t MODER;    /*!< GPIO port mode register,               Address offset: 0x00 */
    __IO uint32_t OTYPER;   /*!< GPIO port output type register,        Address offset: 0x04 */
    __IO uint32_t OSPEEDR;  /*!< GPIO port output speed register,       Address offset: 0x08 */
    __IO uint32_t PUPDR;    /*!< GPIO port pull-up/pull-down register,  Address offset: 0x0C */
    __IO uint32_t IDR;      /*!< GPIO port input data register,         Address offset: 0x10 */
    __IO uint32_t ODR;      /*!< GPIO port output data register,        Address offset: 0x14 */
    __IO uint32_t BSRR;     /*!< GPIO port bit set/reset register,      Address offset: 0x18 */
    __IO uint32_t LCKR;     /*!< GPIO port configuration lock register, Address offset: 0x1C */
    __IO uint32_t AFR[2];   /*!< GPIO alternate function registers,     Address offset: 0x20-0x24 */
    __IO uint32_t BRR;      /*!< GPIO port bit reset register,          Address offset: 0x28 */
} GPIO_TypeDef;

/* RCC Structure */
typedef struct {
    __IO uint32_t CR;       /*!< RCC clock control register,                Address offset: 0x00 */
    __IO uint32_t CFGR;     /*!< RCC clock configuration register,          Address offset: 0x04 */
    __IO uint32_t CIR;      /*!< RCC clock interrupt register,              Address offset: 0x08 */
    __IO uint32_t APB2RSTR; /*!< RCC APB2 peripheral reset register,        Address offset: 0x0C */
    __IO uint32_t APB1RSTR; /*!< RCC APB1 peripheral reset register,        Address offset: 0x10 */
    __IO uint32_t AHBENR;   /*!< RCC AHB peripheral clock register,         Address offset: 0x14 */
    __IO uint32_t APB2ENR;  /*!< RCC APB2 peripheral clock enable register, Address offset: 0x18 */
    __IO uint32_t APB1ENR;  /*!< RCC APB1 peripheral clock enable register, Address offset: 0x1C */
    __IO uint32_t BDCR;     /*!< RCC Backup domain control register,        Address offset: 0x20 */
    __IO uint32_t CSR;      /*!< RCC clock control & status register,       Address offset: 0x24 */
    __IO uint32_t AHBRSTR;  /*!< RCC AHB peripheral reset register,         Address offset: 0x28 */
    __IO uint32_t CFGR2;    /*!< RCC clock configuration register 2,        Address offset: 0x2C */
    __IO uint32_t CFGR3;    /*!< RCC clock configuration register 3,        Address offset: 0x30 */
    __IO uint32_t CR2;      /*!< RCC clock control register 2,              Address offset: 0x34 */
} RCC_TypeDef;

/* SysTick Structure */
typedef struct {
    __IO uint32_t CTRL;     /*!< SysTick Control and Status Register, Address offset: 0x00 */
    __IO uint32_t LOAD;     /*!< SysTick Reload Value Register,       Address offset: 0x04 */
    __IO uint32_t VAL;      /*!< SysTick Current Value Register,      Address offset: 0x08 */
    __IO uint32_t CALIB;    /*!< SysTick Calibration Register,         Address offset: 0x0C */
} SysTick_Type;

#define GPIOA                 ((GPIO_TypeDef *) GPIOA_BASE)
#define RCC                   ((RCC_TypeDef *) RCC_BASE)
#define SysTick               ((SysTick_Type *) SysTick_BASE)

/* Bit definitions */
#define RCC_CR_HSION          (1U << 0)
#define RCC_CR_HSIRDY         (1U << 1)
#define RCC_CFGR_SW           (3U << 0)
#define RCC_CFGR_SW_HSI       (0U << 0)
#define RCC_CFGR_SWS          (3U << 2)
#define RCC_CFGR_SWS_HSI      (0U << 2)
#define RCC_AHBENR_GPIOAEN    (1U << 17)

#define SysTick_CTRL_ENABLE_Pos    0U
#define SysTick_CTRL_ENABLE_Msk    (1U << SysTick_CTRL_ENABLE_Pos)
#define SysTick_CTRL_TICKINT_Pos   1U
#define SysTick_CTRL_TICKINT_Msk   (1U << SysTick_CTRL_TICKINT_Pos)
#define SysTick_CTRL_CLKSOURCE_Pos 2U
#define SysTick_CTRL_CLKSOURCE_Msk (1U << SysTick_CTRL_CLKSOURCE_Pos)
