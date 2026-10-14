#ifndef EW_ADAPTER_ESP32_H
#define EW_ADAPTER_ESP32_H

#include "ew_hal.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief ESP32 / ESP32-S3 Hardware Platform Context
 */
typedef struct {
    uint8_t spi_host;             /* 1 for SPI2_HOST (FSPI), 2 for SPI3_HOST (HSPI) */
    int16_t pin_miso;
    int16_t pin_mosi;
    int16_t pin_sclk;
    int16_t pin_cs;
    int16_t pin_dc;
    int16_t pin_rst;
    int16_t pin_backlight;

    /* Touch Interface */
    uint8_t touch_type;           /* EW_HAL_TOUCH_CAPACITIVE_I2C */
    int16_t pin_touch_sda;
    int16_t pin_touch_scl;
    int16_t pin_touch_int;

    /* DMA & Memory Isolation */
    bool dma_enabled;
    bool require_internal_sram;   /* Enforces buffer in MALLOC_CAP_INTERNAL | MALLOC_CAP_DMA */
    volatile bool dma_busy;
    uint32_t flush_count;
    uint32_t tick_counter;
} ew_esp32_context_t;

/**
 * @brief Create ESP32 / ESP32-S3 target adapter instance.
 */
ew_target_adapter_t ew_adapter_esp32_create(ew_esp32_context_t* ctx);

ew_hal_status_t ew_adapter_esp32_init(void* ctx, const ew_adapter_config_t* config);
ew_hal_status_t ew_adapter_esp32_flush_pixels(void* ctx, int16_t x, int16_t y, int16_t w, int16_t h, const uint16_t* pixels);
bool ew_adapter_esp32_is_busy(void* ctx);
void ew_adapter_esp32_wait_done(void* ctx);
bool ew_adapter_esp32_poll_touch(void* ctx, int16_t* out_x, int16_t* out_y, bool* out_pressed);
uint32_t ew_adapter_esp32_get_tick(void* ctx);
void ew_adapter_esp32_cleanup(void* ctx);

#ifdef __cplusplus
}
#endif

#endif /* EW_ADAPTER_ESP32_H */
