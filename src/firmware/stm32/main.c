#include "stm32f0xx_hal.h"

// Forward declarations
int main(void);
void Reset_Handler(void);
void Default_Handler(void);
void SysTick_Handler(void);

// Linker symbols
extern uint32_t _estack;
extern uint32_t _sidata;
extern uint32_t _sdata;
extern uint32_t _edata;
extern uint32_t _sbss;
extern uint32_t _ebss;

// Interrupt Vector Table for STM32F030R8 (Cortex-M0)
__attribute__ ((section(".isr_vector")))
void (* const g_pfnVectors[])(void) = {
    (void (*)(void))((uint32_t)&_estack), // Initial Stack Pointer
    Reset_Handler,                         // Reset Handler
    Default_Handler,                       // NMI Handler
    Default_Handler,                       // Hard Fault Handler
    0, 0, 0, 0, 0, 0, 0,                  // Reserved
    Default_Handler,                       // SVCall Handler
    0, 0,                                  // Reserved
    Default_Handler,                       // PendSV Handler
    SysTick_Handler,                       // SysTick Handler
};

// Reset Handler initializes data/bss and jumps to main
void Reset_Handler(void) {
    uint32_t *src = &_sidata;
    uint32_t *dst = &_sdata;

    // Copy initialized data from Flash to RAM
    while (dst < &_edata) {
        *(dst++) = *(src++);
    }

    // Zero out BSS section in RAM
    dst = &_sbss;
    while (dst < &_ebss) {
        *(dst++) = 0;
    }

    // Call main entry point
    main();

    // Hang if main ever exits
    while (1) {
        asm("wfi");
    }
}

void Default_Handler(void) {
    while (1) {
        asm("nop");
    }
}

// UART/Serial communication buffer (for future expansion)
volatile uint8_t command_buffer[16];
volatile uint8_t command_index = 0;

// PA4 LED state
volatile uint8_t led_state = 0;  // 0 = off, 1 = on

void SystemClock_Config(void) {
    // Enable HSI (8 MHz internal oscillator)
    RCC->CR |= RCC_CR_HSION;
    while (!(RCC->CR & RCC_CR_HSIRDY));
    
    // Select HSI as system clock
    RCC->CFGR = (RCC->CFGR & ~RCC_CFGR_SW) | RCC_CFGR_SW_HSI;
    while ((RCC->CFGR & RCC_CFGR_SWS) != RCC_CFGR_SWS_HSI);
}

void GPIO_Config(void) {
    // Enable GPIOA clock
    RCC->AHBENR |= RCC_AHBENR_GPIOAEN;
    
    // Configure PA4 as GPIO output
    GPIOA->MODER = (GPIOA->MODER & ~(0x3 << (4*2))) | (0x1 << (4*2));  // Output mode
    GPIOA->OTYPER &= ~(1 << 4);                                          // Push-pull
    GPIOA->OSPEEDR = (GPIOA->OSPEEDR & ~(0x3 << (4*2))) | (0x1 << (4*2)); // Medium speed
    GPIOA->PUPDR &= ~(0x3 << (4*2));                                      // No pull

    // Configure PA5 also (standard Nucleo LD2 green LED) so both PA4 and PA5 respond
    GPIOA->MODER = (GPIOA->MODER & ~(0x3 << (5*2))) | (0x1 << (5*2));  // Output mode
    GPIOA->OTYPER &= ~(1 << 5);                                          // Push-pull
    GPIOA->OSPEEDR = (GPIOA->OSPEEDR & ~(0x3 << (5*2))) | (0x1 << (5*2)); // Medium speed
    GPIOA->PUPDR &= ~(0x3 << (5*2));                                      // No pull
    
    // Initial state: LED off
    GPIOA->ODR &= ~(1 << 4);
    GPIOA->ODR &= ~(1 << 5);
    led_state = 0;
}

void SysTick_Config_Local(void) {
    // Configure SysTick for 1ms interrupt (8 MHz / 8000 = 1 kHz)
    SysTick->LOAD = (8000000 / 8000) - 1;
    SysTick->VAL = 0;
    SysTick->CTRL = SysTick_CTRL_CLKSOURCE_Msk | SysTick_CTRL_TICKINT_Msk | SysTick_CTRL_ENABLE_Msk;
}

void SysTick_Handler(void) {
    // Called every 1ms
    // (Optional: add periodic tasks here)
}

// GDB/OpenOCD command handling
// Commands sent via OpenOCD telnet/GDB interface:
// - "LED_ON" -> set PA4 high
// - "LED_OFF" -> set PA4 low
// - "LED_TOGGLE" -> toggle PA4
// - "LED_STATUS" -> read PA4 state

void handle_command(const char* cmd) {
    if (strcmp(cmd, "LED_ON") == 0) {
        GPIOA->BSRR = (1 << 4) | (1 << 5);  // Set PA4 & PA5
        led_state = 1;
    } else if (strcmp(cmd, "LED_OFF") == 0) {
        GPIOA->BRR = (1 << 4) | (1 << 5);   // Reset PA4 & PA5
        led_state = 0;
    } else if (strcmp(cmd, "LED_TOGGLE") == 0) {
        GPIOA->ODR ^= (1 << 4) | (1 << 5);  // Toggle PA4 & PA5
        led_state = (GPIOA->ODR >> 4) & 1;
    }
}

int main(void) {
    SystemClock_Config();
    GPIO_Config();
    SysTick_Config_Local();
    
    // Blink LED 3 times to indicate boot
    for (int i = 0; i < 3; i++) {
        GPIOA->BSRR = (1 << 4) | (1 << 5);
        for (volatile int j = 0; j < 100000; j++) asm("nop");
        GPIOA->BRR = (1 << 4) | (1 << 5);
        for (volatile int j = 0; j < 100000; j++) asm("nop");
    }
    
    // Main loop
    while (1) {
        // Firmware ready; await commands from OpenOCD/debugger
        asm("wfi");  // Wait for interrupt
    }
    
    return 0;
}
