#ifndef EW_RASTER_H
#define EW_RASTER_H

#include "ew_types.h"
#include "ew_tree.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Slice rendering buffer descriptor (e.g. 320x16 or 480x16 RGB565)
 */
typedef struct {
    uint16_t* buffer;        /* Pointer to preallocated RGB565 pixel buffer */
    int16_t width;           /* Width of slice in pixels */
    int16_t height;          /* Height of slice in lines */
    int16_t slice_y;         /* Global Y offset of this slice in screen space */
} ew_raster_slice_t;

/**
 * @brief Clear slice buffer with a solid background color.
 */
void ew_raster_clear(ew_raster_slice_t* slice, ew_color_t color);

/**
 * @brief Draw a solid rectangle clipped to slice bounds.
 */
void ew_raster_draw_rect(ew_raster_slice_t* slice, ew_rect_t rect, ew_color_t color);

/**
 * @brief Draw a rectangle border clipped to slice bounds.
 */
void ew_raster_draw_rect_border(ew_raster_slice_t* slice, ew_rect_t rect, ew_color_t color, uint8_t border_width);

/**
 * @brief Draw a single character using the embedded 8x8 bitmap font.
 */
void ew_raster_draw_char(ew_raster_slice_t* slice, int16_t x, int16_t y, char c, ew_color_t color);

/**
 * @brief Draw text string clipped to slice bounds.
 */
void ew_raster_draw_text(ew_raster_slice_t* slice, int16_t x, int16_t y, const char* text, ew_color_t color);

/**
 * @brief Draw a solid rectangle clipped to both slice bounds and an arbitrary clipping rectangle.
 */
void ew_raster_draw_rect_clipped(ew_raster_slice_t* slice, ew_rect_t rect, ew_rect_t clip, ew_color_t color);

/**
 * @brief Draw a horizontal progress bar (track, fill bar, border) clipped to slice bounds.
 */
void ew_raster_draw_progress_bar(ew_raster_slice_t* slice, ew_rect_t rect, int16_t progress_value, ew_color_t track_color, ew_color_t fill_color, ew_color_t border_color, uint8_t border_width);

/**
 * @brief Draw a raw RGB565 bitmap image asset, clipped to slice bounds.
 * Supports opaque RGB565, chroma-key transparent pixels, and 8-bit opacity blending.
 */
void ew_raster_draw_image(ew_raster_slice_t* slice, int16_t x, int16_t y, const ew_image_asset_t* image, uint8_t opacity);

/**
 * @brief Render an entire component tree or subtree into the given slice.
 * Guaranteed zero dynamic heap allocations.
 */
void ew_raster_render_tree(ew_raster_slice_t* slice, const ew_tree_t* tree, int16_t node_idx);

/**
 * @brief Render component subtree into the slice with dirty-region clipping bounds.
 */
void ew_raster_render_tree_clipped(ew_raster_slice_t* slice, const ew_tree_t* tree, int16_t node_idx, ew_rect_t clip_rect);

#ifdef __cplusplus
}
#endif

#endif /* EW_RASTER_H */
