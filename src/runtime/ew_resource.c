#include "ew_resource.h"
#include "ew_tree.h"
#include "ew_hal.h"

size_t ew_resource_calc_tree_ram(uint16_t node_count) {
    return (size_t)node_count * sizeof(ew_node_t) + sizeof(uint16_t) + sizeof(int16_t);
}

size_t ew_resource_calc_slice_ram(int16_t disp_w, int16_t slice_lines, uint8_t buffer_count) {
    if (disp_w <= 0 || slice_lines <= 0 || buffer_count == 0) return 0;
    return (size_t)disp_w * (size_t)slice_lines * sizeof(uint16_t) * (size_t)buffer_count;
}

size_t ew_resource_calc_font_rom(uint16_t glyph_count, uint8_t glyph_w, uint8_t glyph_h) {
    /* 1 bit per pixel: (w * h + 7) / 8 bytes per glyph */
    size_t bytes_per_glyph = ((size_t)glyph_w * (size_t)glyph_h + 7) / 8;
    return (size_t)glyph_count * bytes_per_glyph;
}

size_t ew_resource_calc_image_rom(uint16_t img_w, uint16_t img_h, ew_image_format_t format) {
    if (format == EW_IMAGE_FORMAT_UNSUPPORTED) return 0;
    /* RGB565 and RGB565_CHROMA use 2 bytes per pixel */
    return (size_t)img_w * (size_t)img_h * sizeof(uint16_t);
}

ew_memory_summary_t ew_resource_summarize(int16_t disp_w, int16_t slice_lines, uint8_t buffer_count, uint16_t node_count, size_t image_rom_bytes) {
    ew_memory_summary_t summary;
    summary.slice_buffer_ram = ew_resource_calc_slice_ram(disp_w, slice_lines, buffer_count);
    summary.tree_pool_ram = ew_resource_calc_tree_ram(node_count);
    summary.engine_ram = sizeof(ew_engine_t);
    summary.total_static_ram = summary.slice_buffer_ram + summary.tree_pool_ram + summary.engine_ram;
    summary.font_rom = ew_resource_calc_font_rom(95, 8, 8); /* Default 8x8 ASCII font */
    summary.image_assets_rom = image_rom_bytes;
    summary.exceeds_64k_sram = (summary.total_static_ram > 65536);
    return summary;
}
