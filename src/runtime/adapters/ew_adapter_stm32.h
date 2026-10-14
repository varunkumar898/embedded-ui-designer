#ifndef EW_ADAPTER_STM32_H
#define EW_ADAPTER_STM32_H

#include "ew_hal.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief STM32 Hardware Platform Configuration & Bus Context
 */
typedef struct {
    /* Bus interface */
    uint8_t bus_type;             /* EW_HAL_BUS_SPI or EW_HAL_BUS_PARALLEL_8080 */
    uint8_t spi_instance;         /* 1 for SPI1, 2 for SPI2, etc. */
    uint32_t spi_baud_prescaler;  /* e.g. 2, 4, 8 */

    /* Control Pin Assignments */
    int16_t pin_cs;
    int16_t pin_dc;
    int16_t pin_rst;
    int16_t pin_backlight;

    /* Touch Interface */
    uint8_t touch_type;           /* EW_HAL_TOUCH_NONE, EW_HAL_TOUCH_RESISTIVE_SPI, EW_HAL_TOUCH_CAPACITIVE_I2C */
    uint8_t touch_i2c_addr;       /* 0x38 for FT6206, 0x15 for CST816S */
    int16_t pin_touch_irq;

    /* DMA state */
    volatile bool dma_busy;
    uint32_t last_flush_pixels;
    uint32_t tick_counter;
} ew_stm32_context_t;

/**
 * @brief Create and initialize STM32 target adapter instance.
 */
ew_target_adapter_t ew_adapter_stm32_create(ew_stm32_context_t* ctx);

/* Direct Adapter Hook Functions */
ew_hal_status_t ew_adapter_stm32_init(void* ctx, const ew_adapter_config_t* config);
ew_hal_status_t ew_adapter_stm32_flush_pixels(void* ctx, int16_t x, int16_t y, int16_t w, int16_t h, const uint16_t* pixels);
bool ew_adapter_stm32_is_busy(void* ctx);
void ew_adapter_stm32_wait_done(void* ctx);
bool ew_adapter_stm32_poll_touch(void* ctx, int16_t* out_x, int16_t* out_y, bool* out_pressed);
uint32_t ew_adapter_stm32_get_tick(void* ctx);
void ew_adapter_stm32_cleanup(void* ctx);

#ifdef __cplusplus
}
#endif

#endif /* EW_ADAPTER_STM32_H */
