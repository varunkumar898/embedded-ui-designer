#ifndef EW_HAL_H
#define EW_HAL_H

#include "ew_types.h"
#include "ew_tree.h"
#include "ew_raster.h"
#include "ew_input.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Forward declarations */
struct ew_display_driver;
struct ew_input_driver;

/* Display interface */
typedef void (*ew_flush_fn)(struct ew_display_driver* disp, int16_t x, int16_t y, int16_t w, int16_t h, const uint16_t* pixels);

typedef struct ew_display_driver {
    int16_t width;
    int16_t height;
    ew_flush_fn flush_cb;
    void* user_data;
} ew_display_driver_t;

typedef bool (*ew_poll_input_fn)(struct ew_input_driver* drv, ew_input_event_t* event);

typedef struct ew_input_driver {
    ew_poll_input_fn poll_cb;
    void* user_data;
} ew_input_driver_t;

/* EmbedWeb Engine (Isolated runtime controller) */
typedef struct {
    ew_tree_t* tree;
    ew_display_driver_t* display;
    ew_input_driver_t* input;
    ew_input_state_t input_state; /* Embedded single-pointer input state */
    uint16_t* slice_buffer;
    int16_t slice_height;     /* Number of lines per slice (e.g. 16 or 20) */
    ew_color_t clear_color;
} ew_engine_t;

/**
 * @brief Initialize EmbedWeb engine with preallocated buffers and drivers.
 */
void ew_engine_init(ew_engine_t* engine, ew_tree_t* tree, ew_display_driver_t* disp, ew_input_driver_t* input, uint16_t* slice_buf, int16_t slice_lines);

/**
 * @brief Render full frame in slices via display flush callback.
 * Guaranteed zero dynamic allocation during rendering loop.
 */
void ew_engine_render_frame(ew_engine_t* engine);

/**
 * @brief Render only slices intersecting the dirty region, clipped to dirty_rect.
 * Guaranteed zero dynamic allocation during rendering loop.
 */
void ew_engine_render_dirty_region(ew_engine_t* engine, ew_rect_t dirty_rect);

/**
 * @brief Invalidate a specific node's bounds and trigger dirty-region render.
 */
void ew_engine_invalidate_node(ew_engine_t* engine, int16_t node_idx);

/**
 * @brief Poll and process input events.
 */
void ew_engine_process_input(ew_engine_t* engine);

/* ============================================================================
 * Target Hardware Adapter Contract (Stage 4.5 Multi-MCU Portable HAL)
 * ============================================================================ */

typedef enum {
    EW_HAL_BUS_SPI = 0,
    EW_HAL_BUS_PARALLEL_8080 = 1,
    EW_HAL_BUS_RGB_DOT_CLK = 2,
    EW_HAL_BUS_MIPI_DSI = 3
} ew_hal_bus_type_t;

typedef enum {
    EW_HAL_TOUCH_NONE = 0,
    EW_HAL_TOUCH_RESISTIVE_SPI = 1,   /* e.g. XPT2046 */
    EW_HAL_TOUCH_CAPACITIVE_I2C = 2   /* e.g. FT6206, CST816S, GT911 */
} ew_hal_touch_type_t;

typedef enum {
    EW_HAL_OK = 0,
    EW_HAL_ERR_INIT = -1,
    EW_HAL_ERR_BUS = -2,
    EW_HAL_ERR_TIMEOUT = -3,
    EW_HAL_ERR_DMA = -4,
    EW_HAL_ERR_UNSUPPORTED = -5
} ew_hal_status_t;

typedef struct {
    int16_t width;
    int16_t height;
    uint8_t bus_type;                 /* ew_hal_bus_type_t */
    uint8_t touch_type;               /* ew_hal_touch_type_t */
    uint8_t dma_enabled;              /* 1 if DMA transfer is active */
    uint8_t dma_needs_internal_sram;  /* 1 if DMA buffer must reside in internal SRAM */
    uint32_t bus_clock_hz;            /* SPI / DotClock frequency */
    const char* display_controller;   /* e.g. "ST7789", "ILI9341", "ST7796" */
    const char* touch_controller;     /* e.g. "CST816S", "FT6206", "XPT2046", "None" */
} ew_adapter_config_t;

/* Portable Hardware Adapter Operations */
typedef struct ew_target_adapter {
    const char* target_name;          /* "STM32", "RP2040", "ESP32", "NXP", "Host" */
    void* user_ctx;
    ew_hal_status_t (*init_platform)(void* ctx, const ew_adapter_config_t* config);
    ew_hal_status_t (*flush_pixels)(void* ctx, int16_t x, int16_t y, int16_t w, int16_t h, const uint16_t* pixels);
    bool (*is_transfer_busy)(void* ctx);
    void (*wait_transfer_done)(void* ctx);
    bool (*poll_touch)(void* ctx, int16_t* out_x, int16_t* out_y, bool* out_pressed);
    uint32_t (*get_tick_ms)(void* ctx);
    void (*cleanup)(void* ctx);
} ew_target_adapter_t;

/**
 * @brief Bind a target adapter to standard EmbedWeb display & input drivers.
 */
void ew_hal_bind_adapter(ew_display_driver_t* out_disp, ew_input_driver_t* out_input, ew_target_adapter_t* adapter, int16_t width, int16_t height);

#ifdef __cplusplus
}
#endif

#endif /* EW_HAL_H */

