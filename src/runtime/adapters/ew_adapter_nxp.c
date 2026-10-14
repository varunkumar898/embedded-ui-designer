#include "ew_adapter_nxp.h"
#include <string.h>

ew_hal_status_t ew_adapter_nxp_init(void* ctx, const ew_adapter_config_t* config) {
    ew_nxp_context_t* nxp = (ew_nxp_context_t*)ctx;
    if (!nxp || !config) return EW_HAL_ERR_INIT;

    nxp->bus_type = config->bus_type;
    nxp->touch_type = config->touch_type;
    nxp->dma_busy = false;
    nxp->flush_count = 0;
    nxp->tick_counter = 0;

    return EW_HAL_OK;
}

ew_hal_status_t ew_adapter_nxp_flush_pixels(void* ctx, int16_t x, int16_t y, int16_t w, int16_t h, const uint16_t* pixels) {
    ew_nxp_context_t* nxp = (ew_nxp_context_t*)ctx;
    if (!nxp || !pixels || w <= 0 || h <= 0) return EW_HAL_ERR_BUS;

    nxp->flush_count++;
    nxp->dma_busy = false;
    (void)x; (void)y;
    return EW_HAL_OK;
}

bool ew_adapter_nxp_is_busy(void* ctx) {
    ew_nxp_context_t* nxp = (ew_nxp_context_t*)ctx;
    return nxp ? nxp->dma_busy : false;
}

void ew_adapter_nxp_wait_done(void* ctx) {
    ew_nxp_context_t* nxp = (ew_nxp_context_t*)ctx;
    if (nxp) nxp->dma_busy = false;
}

bool ew_adapter_nxp_poll_touch(void* ctx, int16_t* out_x, int16_t* out_y, bool* out_pressed) {
    ew_nxp_context_t* nxp = (ew_nxp_context_t*)ctx;
    if (!nxp || nxp->touch_type == EW_HAL_TOUCH_NONE) {
        if (out_pressed) *out_pressed = false;
        return false;
    }

    if (out_x) *out_x = 0;
    if (out_y) *out_y = 0;
    if (out_pressed) *out_pressed = false;
    return false;
}

uint32_t ew_adapter_nxp_get_tick(void* ctx) {
    ew_nxp_context_t* nxp = (ew_nxp_context_t*)ctx;
    if (!nxp) return 0;
    return ++nxp->tick_counter;
}

void ew_adapter_nxp_cleanup(void* ctx) {
    ew_nxp_context_t* nxp = (ew_nxp_context_t*)ctx;
    if (nxp) nxp->dma_busy = false;
}

ew_target_adapter_t ew_adapter_nxp_create(ew_nxp_context_t* ctx) {
    ew_target_adapter_t adapt;
    memset(&adapt, 0, sizeof(adapt));
    adapt.target_name = "NXP";
    adapt.user_ctx = (void*)ctx;
    adapt.init_platform = ew_adapter_nxp_init;
    adapt.flush_pixels = ew_adapter_nxp_flush_pixels;
    adapt.is_transfer_busy = ew_adapter_nxp_is_busy;
    adapt.wait_transfer_done = ew_adapter_nxp_wait_done;
    adapt.poll_touch = ew_adapter_nxp_poll_touch;
    adapt.get_tick_ms = ew_adapter_nxp_get_tick;
    adapt.cleanup = ew_adapter_nxp_cleanup;
    return adapt;
}
