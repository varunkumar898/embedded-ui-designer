#include "ew_adapter_esp32.h"
#include <string.h>

ew_hal_status_t ew_adapter_esp32_init(void* ctx, const ew_adapter_config_t* config) {
    ew_esp32_context_t* esp = (ew_esp32_context_t*)ctx;
    if (!esp || !config) return EW_HAL_ERR_INIT;

    esp->dma_enabled = (config->dma_enabled != 0);
    esp->require_internal_sram = (config->dma_needs_internal_sram != 0);
    esp->dma_busy = false;
    esp->flush_count = 0;
    esp->tick_counter = 0;

    return EW_HAL_OK;
}

ew_hal_status_t ew_adapter_esp32_flush_pixels(void* ctx, int16_t x, int16_t y, int16_t w, int16_t h, const uint16_t* pixels) {
    ew_esp32_context_t* esp = (ew_esp32_context_t*)ctx;
    if (!esp || !pixels || w <= 0 || h <= 0) return EW_HAL_ERR_BUS;

    /* On ESP32-S3, verify that DMA buffers reside in internal SRAM.
     * SPI DMA transfers directly from external PSRAM will fail with a DMA error
     * or cause bus lockup on ESP32 silicon unless staged via internal SRAM. */
    if (esp->dma_enabled && esp->require_internal_sram) {
        /* Address verification guard: in real ESP-IDF, esp_ptr_dma_capable(pixels) is checked */
        uintptr_t addr = (uintptr_t)pixels;
        if (addr == 0) return EW_HAL_ERR_DMA;
    }

    esp->flush_count++;
    esp->dma_busy = false;
    (void)x; (void)y;
    return EW_HAL_OK;
}

bool ew_adapter_esp32_is_busy(void* ctx) {
    ew_esp32_context_t* esp = (ew_esp32_context_t*)ctx;
    return esp ? esp->dma_busy : false;
}

void ew_adapter_esp32_wait_done(void* ctx) {
    ew_esp32_context_t* esp = (ew_esp32_context_t*)ctx;
    if (esp) esp->dma_busy = false;
}

bool ew_adapter_esp32_poll_touch(void* ctx, int16_t* out_x, int16_t* out_y, bool* out_pressed) {
    ew_esp32_context_t* esp = (ew_esp32_context_t*)ctx;
    if (!esp || esp->touch_type == EW_HAL_TOUCH_NONE) {
        if (out_pressed) *out_pressed = false;
        return false;
    }

    /* In hardware: read I2C CST816S registers */
    if (out_x) *out_x = 0;
    if (out_y) *out_y = 0;
    if (out_pressed) *out_pressed = false;
    return false;
}

uint32_t ew_adapter_esp32_get_tick(void* ctx) {
    ew_esp32_context_t* esp = (ew_esp32_context_t*)ctx;
    if (!esp) return 0;
    return ++esp->tick_counter;
}

void ew_adapter_esp32_cleanup(void* ctx) {
    ew_esp32_context_t* esp = (ew_esp32_context_t*)ctx;
    if (esp) esp->dma_busy = false;
}

ew_target_adapter_t ew_adapter_esp32_create(ew_esp32_context_t* ctx) {
    ew_target_adapter_t adapt;
    memset(&adapt, 0, sizeof(adapt));
    adapt.target_name = "ESP32";
    adapt.user_ctx = (void*)ctx;
    adapt.init_platform = ew_adapter_esp32_init;
    adapt.flush_pixels = ew_adapter_esp32_flush_pixels;
    adapt.is_transfer_busy = ew_adapter_esp32_is_busy;
    adapt.wait_transfer_done = ew_adapter_esp32_wait_done;
    adapt.poll_touch = ew_adapter_esp32_poll_touch;
    adapt.get_tick_ms = ew_adapter_esp32_get_tick;
    adapt.cleanup = ew_adapter_esp32_cleanup;
    return adapt;
}
