#include "ew_adapter_stm32.h"
#include <string.h>

ew_hal_status_t ew_adapter_stm32_init(void* ctx, const ew_adapter_config_t* config) {
    ew_stm32_context_t* stm = (ew_stm32_context_t*)ctx;
    if (!stm || !config) return EW_HAL_ERR_INIT;

    stm->bus_type = config->bus_type;
    stm->touch_type = config->touch_type;
    stm->dma_busy = false;
    stm->last_flush_pixels = 0;
    stm->tick_counter = 0;

    /* In a full STM32 firmware build, this initializes SPI / FMC peripherals,
     * configures GPIOs, and sends LCD controller power-on sequences. */
    return EW_HAL_OK;
}

ew_hal_status_t ew_adapter_stm32_flush_pixels(void* ctx, int16_t x, int16_t y, int16_t w, int16_t h, const uint16_t* pixels) {
    ew_stm32_context_t* stm = (ew_stm32_context_t*)ctx;
    if (!stm || !pixels || w <= 0 || h <= 0) return EW_HAL_ERR_BUS;

    /* Wait for previous DMA burst if busy */
    if (stm->dma_busy) {
        ew_adapter_stm32_wait_done(ctx);
    }

    /* Record pixels flushed and simulate DMA trigger */
    stm->last_flush_pixels = (uint32_t)(w * h);
    stm->dma_busy = false; /* Synchronous/DMA complete */
    (void)x; (void)y;

    return EW_HAL_OK;
}

bool ew_adapter_stm32_is_busy(void* ctx) {
    ew_stm32_context_t* stm = (ew_stm32_context_t*)ctx;
    return stm ? stm->dma_busy : false;
}

void ew_adapter_stm32_wait_done(void* ctx) {
    ew_stm32_context_t* stm = (ew_stm32_context_t*)ctx;
    if (stm) {
        stm->dma_busy = false;
    }
}

bool ew_adapter_stm32_poll_touch(void* ctx, int16_t* out_x, int16_t* out_y, bool* out_pressed) {
    ew_stm32_context_t* stm = (ew_stm32_context_t*)ctx;
    if (!stm || stm->touch_type == EW_HAL_TOUCH_NONE) {
        if (out_pressed) *out_pressed = false;
        return false;
    }

    /* In hardware: read I2C registers from FT6206 / CST816S or SPI from XPT2046 */
    if (out_x) *out_x = 0;
    if (out_y) *out_y = 0;
    if (out_pressed) *out_pressed = false;
    return false;
}

uint32_t ew_adapter_stm32_get_tick(void* ctx) {
    ew_stm32_context_t* stm = (ew_stm32_context_t*)ctx;
    if (!stm) return 0;
    return ++stm->tick_counter;
}

void ew_adapter_stm32_cleanup(void* ctx) {
    ew_stm32_context_t* stm = (ew_stm32_context_t*)ctx;
    if (stm) {
        stm->dma_busy = false;
    }
}

ew_target_adapter_t ew_adapter_stm32_create(ew_stm32_context_t* ctx) {
    ew_target_adapter_t adapt;
    memset(&adapt, 0, sizeof(adapt));
    adapt.target_name = "STM32";
    adapt.user_ctx = (void*)ctx;
    adapt.init_platform = ew_adapter_stm32_init;
    adapt.flush_pixels = ew_adapter_stm32_flush_pixels;
    adapt.is_transfer_busy = ew_adapter_stm32_is_busy;
    adapt.wait_transfer_done = ew_adapter_stm32_wait_done;
    adapt.poll_touch = ew_adapter_stm32_poll_touch;
    adapt.get_tick_ms = ew_adapter_stm32_get_tick;
    adapt.cleanup = ew_adapter_stm32_cleanup;
    return adapt;
}
