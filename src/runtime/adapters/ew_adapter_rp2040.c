#include "ew_adapter_rp2040.h"
#include <string.h>

ew_hal_status_t ew_adapter_rp2040_init(void* ctx, const ew_adapter_config_t* config) {
    ew_rp2040_context_t* pico = (ew_rp2040_context_t*)ctx;
    if (!pico || !config) return EW_HAL_ERR_INIT;

    pico->touch_type = config->touch_type;
    pico->dma_busy = false;
    pico->flush_count = 0;
    pico->tick_counter = 0;

    return EW_HAL_OK;
}

ew_hal_status_t ew_adapter_rp2040_flush_pixels(void* ctx, int16_t x, int16_t y, int16_t w, int16_t h, const uint16_t* pixels) {
    ew_rp2040_context_t* pico = (ew_rp2040_context_t*)ctx;
    if (!pico || !pixels || w <= 0 || h <= 0) return EW_HAL_ERR_BUS;

    pico->flush_count++;
    pico->dma_busy = false;
    (void)x; (void)y;
    return EW_HAL_OK;
}

bool ew_adapter_rp2040_is_busy(void* ctx) {
    ew_rp2040_context_t* pico = (ew_rp2040_context_t*)ctx;
    return pico ? pico->dma_busy : false;
}

void ew_adapter_rp2040_wait_done(void* ctx) {
    ew_rp2040_context_t* pico = (ew_rp2040_context_t*)ctx;
    if (pico) pico->dma_busy = false;
}

bool ew_adapter_rp2040_poll_touch(void* ctx, int16_t* out_x, int16_t* out_y, bool* out_pressed) {
    ew_rp2040_context_t* pico = (ew_rp2040_context_t*)ctx;
    if (!pico || pico->touch_type == EW_HAL_TOUCH_NONE) {
        if (out_pressed) *out_pressed = false;
        return false;
    }

    if (out_x) *out_x = 0;
    if (out_y) *out_y = 0;
    if (out_pressed) *out_pressed = false;
    return false;
}

uint32_t ew_adapter_rp2040_get_tick(void* ctx) {
    ew_rp2040_context_t* pico = (ew_rp2040_context_t*)ctx;
    if (!pico) return 0;
    return ++pico->tick_counter;
}

void ew_adapter_rp2040_cleanup(void* ctx) {
    ew_rp2040_context_t* pico = (ew_rp2040_context_t*)ctx;
    if (pico) pico->dma_busy = false;
}

ew_target_adapter_t ew_adapter_rp2040_create(ew_rp2040_context_t* ctx) {
    ew_target_adapter_t adapt;
    memset(&adapt, 0, sizeof(adapt));
    adapt.target_name = "RP2040";
    adapt.user_ctx = (void*)ctx;
    adapt.init_platform = ew_adapter_rp2040_init;
    adapt.flush_pixels = ew_adapter_rp2040_flush_pixels;
    adapt.is_transfer_busy = ew_adapter_rp2040_is_busy;
    adapt.wait_transfer_done = ew_adapter_rp2040_wait_done;
    adapt.poll_touch = ew_adapter_rp2040_poll_touch;
    adapt.get_tick_ms = ew_adapter_rp2040_get_tick;
    adapt.cleanup = ew_adapter_rp2040_cleanup;
    return adapt;
}
