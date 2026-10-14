#ifndef EW_ADAPTER_HOST_H
#define EW_ADAPTER_HOST_H

#include "ew_hal.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    uint16_t* framebuffer;        /* Host memory buffer */
    int16_t fb_width;
    int16_t fb_height;
    int16_t simulated_touch_x;
    int16_t simulated_touch_y;
    bool simulated_pressed;
    uint32_t flush_count;
    uint32_t tick_counter;
} ew_host_context_t;

/**
 * @brief Create Host / Desktop PC simulator target adapter instance.
 */
ew_target_adapter_t ew_adapter_host_create(ew_host_context_t* ctx);

ew_hal_status_t ew_adapter_host_init(void* ctx, const ew_adapter_config_t* config);
ew_hal_status_t ew_adapter_host_flush_pixels(void* ctx, int16_t x, int16_t y, int16_t w, int16_t h, const uint16_t* pixels);
bool ew_adapter_host_is_busy(void* ctx);
void ew_adapter_host_wait_done(void* ctx);
bool ew_adapter_host_poll_touch(void* ctx, int16_t* out_x, int16_t* out_y, bool* out_pressed);
uint32_t ew_adapter_host_get_tick(void* ctx);
void ew_adapter_host_cleanup(void* ctx);

#ifdef __cplusplus
}
#endif

#endif /* EW_ADAPTER_HOST_H */
