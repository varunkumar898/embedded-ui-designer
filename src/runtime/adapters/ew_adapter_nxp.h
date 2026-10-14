#ifndef EW_ADAPTER_NXP_H
#define EW_ADAPTER_NXP_H

#include "ew_hal.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief NXP i.MX RT (Cortex-M7) Target Adapter Context
 */
typedef struct {
    uint8_t bus_type;             /* EW_HAL_BUS_SPI or EW_HAL_BUS_PARALLEL_8080 or EW_HAL_BUS_RGB_DOT_CLK */
    uint8_t lpspi_instance;       /* 1 for LPSPI1, etc. */
    int16_t pin_cs;
    int16_t pin_dc;
    int16_t pin_rst;
    int16_t pin_bl;

    uint8_t touch_type;
    volatile bool dma_busy;
    uint32_t flush_count;
    uint32_t tick_counter;
} ew_nxp_context_t;

/**
 * @brief Create NXP i.MX RT target adapter instance.
 */
ew_target_adapter_t ew_adapter_nxp_create(ew_nxp_context_t* ctx);

ew_hal_status_t ew_adapter_nxp_init(void* ctx, const ew_adapter_config_t* config);
ew_hal_status_t ew_adapter_nxp_flush_pixels(void* ctx, int16_t x, int16_t y, int16_t w, int16_t h, const uint16_t* pixels);
bool ew_adapter_nxp_is_busy(void* ctx);
void ew_adapter_nxp_wait_done(void* ctx);
bool ew_adapter_nxp_poll_touch(void* ctx, int16_t* out_x, int16_t* out_y, bool* out_pressed);
uint32_t ew_adapter_nxp_get_tick(void* ctx);
void ew_adapter_nxp_cleanup(void* ctx);

#ifdef __cplusplus
}
#endif

#endif /* EW_ADAPTER_NXP_H */
