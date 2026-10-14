#ifndef EW_ADAPTER_RP2040_H
#define EW_ADAPTER_RP2040_H

#include "ew_hal.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Raspberry Pi Pico (RP2040 / RP2350) Target Adapter Context
 */
typedef struct {
    uint8_t spi_port;             /* 0 for spi0, 1 for spi1 */
    int16_t pin_sck;
    int16_t pin_tx;               /* MOSI */
    int16_t pin_cs;
    int16_t pin_dc;
    int16_t pin_rst;
    int16_t pin_bl;

    uint8_t touch_type;
    int16_t pin_touch_sda;
    int16_t pin_touch_scl;

    volatile bool dma_busy;
    uint32_t flush_count;
    uint32_t tick_counter;
} ew_rp2040_context_t;

/**
 * @brief Create RP2040 / RP2350 target adapter instance.
 */
ew_target_adapter_t ew_adapter_rp2040_create(ew_rp2040_context_t* ctx);

ew_hal_status_t ew_adapter_rp2040_init(void* ctx, const ew_adapter_config_t* config);
ew_hal_status_t ew_adapter_rp2040_flush_pixels(void* ctx, int16_t x, int16_t y, int16_t w, int16_t h, const uint16_t* pixels);
bool ew_adapter_rp2040_is_busy(void* ctx);
void ew_adapter_rp2040_wait_done(void* ctx);
bool ew_adapter_rp2040_poll_touch(void* ctx, int16_t* out_x, int16_t* out_y, bool* out_pressed);
uint32_t ew_adapter_rp2040_get_tick(void* ctx);
void ew_adapter_rp2040_cleanup(void* ctx);

#ifdef __cplusplus
}
#endif

#endif /* EW_ADAPTER_RP2040_H */
