#ifndef EW_RESOURCE_H
#define EW_RESOURCE_H

#include "ew_types.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    size_t slice_buffer_ram;
    size_t tree_pool_ram;
    size_t engine_ram;
    size_t total_static_ram;
    size_t font_rom;
    size_t image_assets_rom;
    bool exceeds_64k_sram;
} ew_memory_summary_t;

/**
 * @brief Calculate RAM required for a node tree pool of given capacity.
 */
size_t ew_resource_calc_tree_ram(uint16_t node_count);

/**
 * @brief Calculate RAM required for slice framebuffer (e.g. 1 or 2 ping-pong buffers).
 */
size_t ew_resource_calc_slice_ram(int16_t disp_w, int16_t slice_lines, uint8_t buffer_count);

/**
 * @brief Calculate Flash ROM footprint for bitmap font table.
 */
size_t ew_resource_calc_font_rom(uint16_t glyph_count, uint8_t glyph_w, uint8_t glyph_h);

/**
 * @brief Calculate Flash ROM footprint for an RGB565 raw image asset.
 */
size_t ew_resource_calc_image_rom(uint16_t img_w, uint16_t img_h, ew_image_format_t format);

/**
 * @brief Comprehensive memory summary for target configuration.
 */
ew_memory_summary_t ew_resource_summarize(int16_t disp_w, int16_t slice_lines, uint8_t buffer_count, uint16_t node_count, size_t image_rom_bytes);

#ifdef __cplusplus
}
#endif

#endif /* EW_RESOURCE_H */
