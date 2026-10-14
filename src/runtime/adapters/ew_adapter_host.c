#include "ew_adapter_host.h"
#include <string.h>

ew_hal_status_t ew_adapter_host_init(void* ctx, const ew_adapter_config_t* config) {
    ew_host_context_t* host = (ew_host_context_t*)ctx;
    if (!host || !config) return EW_HAL_ERR_INIT;

    host->fb_width = config->width;
    host->fb_height = config->height;
    host->flush_count = 0;
    host->tick_counter = 0;
    host->simulated_pressed = false;

    return EW_HAL_OK;
}

ew_hal_status_t ew_adapter_host_flush_pixels(void* ctx, int16_t x, int16_t y, int16_t w, int16_t h, const uint16_t* pixels) {
    ew_host_context_t* host = (ew_host_context_t*)ctx;
    if (!host || !pixels || w <= 0 || h <= 0) return EW_HAL_ERR_BUS;

    host->flush_count++;

    /* If framebuffer allocated on host, copy pixels */
    if (host->framebuffer && host->fb_width > 0 && host->fb_height > 0) {
        for (int line = 0; line < h; line++) {
            int dst_y = y + line;
            if (dst_y < 0 || dst_y >= host->fb_height) continue;
            int dst_offset = dst_y * host->fb_width + x;
            int src_offset = line * w;
            memcpy(&host->framebuffer[dst_offset], &pixels[src_offset], w * sizeof(uint16_t));
        }
    }

    return EW_HAL_OK;
}

bool ew_adapter_host_is_busy(void* ctx) {
    (void)ctx;
    return false;
}

void ew_adapter_host_wait_done(void* ctx) {
    (void)ctx;
}

bool ew_adapter_host_poll_touch(void* ctx, int16_t* out_x, int16_t* out_y, bool* out_pressed) {
    ew_host_context_t* host = (ew_host_context_t*)ctx;
    if (!host) {
        if (out_pressed) *out_pressed = false;
        return false;
    }

    if (out_x) *out_x = host->simulated_touch_x;
    if (out_y) *out_y = host->simulated_touch_y;
    if (out_pressed) *out_pressed = host->simulated_pressed;
    return host->simulated_pressed;
}

uint32_t ew_adapter_host_get_tick(void* ctx) {
    ew_host_context_t* host = (ew_host_context_t*)ctx;
    if (!host) return 0;
    return ++host->tick_counter;
}

void ew_adapter_host_cleanup(void* ctx) {
    (void)ctx;
}

ew_target_adapter_t ew_adapter_host_create(ew_host_context_t* ctx) {
    ew_target_adapter_t adapt;
    memset(&adapt, 0, sizeof(adapt));
    adapt.target_name = "Host";
    adapt.user_ctx = (void*)ctx;
    adapt.init_platform = ew_adapter_host_init;
    adapt.flush_pixels = ew_adapter_host_flush_pixels;
    adapt.is_transfer_busy = ew_adapter_host_is_busy;
    adapt.wait_transfer_done = ew_adapter_host_wait_done;
    adapt.poll_touch = ew_adapter_host_poll_touch;
    adapt.get_tick_ms = ew_adapter_host_get_tick;
    adapt.cleanup = ew_adapter_host_cleanup;
    return adapt;
}
