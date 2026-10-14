#ifndef EW_TYPES_H
#define EW_TYPES_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Maximum bounded capacity */
#define EW_MAX_NODES 128
#define EW_MAX_TREE_NODES EW_MAX_NODES
#define EW_MAX_TEXT_LEN 32

/* Color definitions (RGB565 native embedded representation) */
typedef uint16_t ew_color_t;

static inline ew_color_t ew_color_rgb565(uint8_t r, uint8_t g, uint8_t b) {
    return (ew_color_t)(((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3));
}

#define EW_COLOR_BLACK   ((ew_color_t)0x0000)
#define EW_COLOR_WHITE   ((ew_color_t)0xFFFF)
#define EW_COLOR_RED     ((ew_color_t)0xF800)
#define EW_COLOR_GREEN   ((ew_color_t)0x07E0)
#define EW_COLOR_BLUE    ((ew_color_t)0x001F)
#define EW_COLOR_TRANSPARENT ((ew_color_t)0x0001) /* Reserved transparent key */

/* Geometry types */
typedef struct {
    int16_t x;
    int16_t y;
} ew_point_t;

typedef struct {
    int16_t x;
    int16_t y;
    int16_t w;
    int16_t h;
} ew_rect_t;

/* Input event definitions */
typedef enum {
    EW_EVENT_NONE = 0,
    EW_EVENT_TOUCH_DOWN,
    EW_EVENT_TOUCH_MOVE,
    EW_EVENT_TOUCH_UP,
    EW_EVENT_CLICK
} ew_event_type_t;

typedef struct {
    ew_event_type_t type;
    int16_t x;
    int16_t y;
} ew_input_event_t;

/* Flexbox Layout enums */
typedef enum {
    EW_FLEX_DIR_ROW = 0,
    EW_FLEX_DIR_COLUMN = 1
} ew_flex_direction_t;

typedef enum {
    EW_JUSTIFY_START = 0,
    EW_JUSTIFY_CENTER = 1,
    EW_JUSTIFY_END = 2,
    EW_JUSTIFY_SPACE_BETWEEN = 3,
    EW_JUSTIFY_SPACE_AROUND = 4
} ew_justify_t;

typedef enum {
    EW_ALIGN_START = 0,
    EW_ALIGN_CENTER = 1,
    EW_ALIGN_END = 2,
    EW_ALIGN_STRETCH = 3
} ew_align_t;

/* Widget types */
typedef enum {
    EW_WIDGET_CONTAINER = 0,
    EW_WIDGET_RECTANGLE = 1,
    EW_WIDGET_LABEL = 2,
    EW_WIDGET_BUTTON = 3,
    EW_WIDGET_PROGRESS_BAR = 4,
    EW_WIDGET_IMAGE = 5,
    EW_WIDGET_UNSUPPORTED = 255
} ew_widget_type_t;

/**
 * ARCHITECTURAL DECISION ON ARGB8888 vs RGB565:
 * EmbedWeb standardizes natively on RGB565 (16 bits per pixel).
 * Full ARGB8888 framebuffers are explicitly DEFERRED/NOT SUPPORTED on MCU targets because:
 *   1. 32bpp doubles framebuffer memory consumption (e.g. 480x16 slice requires 30.72 KB vs 15.36 KB),
 *      jeopardizing the <64 KB SRAM budget.
 *   2. Typical embedded SPI/QSPI LCD controllers (ILI9341, ST7789, ST7796) accept RGB565 natively;
 *      transmitting 32-bit ARGB requires on-chip format conversion or 2x bus traffic, halving frame rate.
 * For transparency, EmbedWeb uses RGB565 Chroma-Keying and fast 5-6-5 integer alpha blending.
 */
typedef enum {
    EW_IMAGE_FORMAT_RGB565 = 0,
    EW_IMAGE_FORMAT_RGB565_CHROMA = 1, /* Chroma key transparent pixel */
    EW_IMAGE_FORMAT_UNSUPPORTED = 255
} ew_image_format_t;

typedef struct {
    uint16_t width;
    uint16_t height;
    uint8_t format;            /* ew_image_format_t */
    ew_color_t chroma_key;     /* Transparent key if format == EW_IMAGE_FORMAT_RGB565_CHROMA */
    const uint16_t* pixels;    /* Stored in Flash ROM (.rodata) or preloaded RAM */
} ew_image_asset_t;

/**
 * @brief Fast integer-only alpha blending for two RGB565 pixels without float math.
 */
static inline ew_color_t ew_color_blend565(ew_color_t fg, ew_color_t bg, uint8_t alpha) {
    if (alpha == 0) return bg;
    if (alpha == 255) return fg;
    uint32_t fg_r = (fg >> 11) & 0x1F;
    uint32_t fg_g = (fg >> 5) & 0x3F;
    uint32_t fg_b = fg & 0x1F;
    uint32_t bg_r = (bg >> 11) & 0x1F;
    uint32_t bg_g = (bg >> 5) & 0x3F;
    uint32_t bg_b = bg & 0x1F;
    uint32_t a = alpha;
    uint32_t inv_a = 255 - alpha;
    uint32_t r = (fg_r * a + bg_r * inv_a) / 255;
    uint32_t g = (fg_g * a + bg_g * inv_a) / 255;
    uint32_t b = (fg_b * a + bg_b * inv_a) / 255;
    return (ew_color_t)((r << 11) | (g << 5) | b);
}

/* Widget Styling */
typedef struct {
    ew_color_t bg_color;
    ew_color_t border_color;
    ew_color_t text_color;
    uint8_t border_width;
    uint8_t border_radius;
    uint8_t padding;
    uint8_t gap;
    uint8_t opacity;        /* 0 to 255 */
    uint8_t has_bg;         /* 1 if bg_color is rendered */
    uint8_t has_border;     /* 1 if border is rendered */
} ew_style_t;

/* Node event callback signature */
struct ew_node;
typedef void (*ew_node_event_cb)(struct ew_node* node, uint8_t event_type, void* user_data);

/* Component Node structure (Bounded POD) */
typedef struct ew_node {
    uint16_t id;
    uint8_t type;            /* ew_widget_type_t */
    uint8_t visible;
    
    /* Layout properties */
    ew_rect_t rect;          /* Computed absolute/relative geometry */
    int16_t req_w;           /* Requested width (-1 for auto/flex) */
    int16_t req_h;           /* Requested height (-1 for auto/flex) */
    uint8_t flex_direction;  /* ew_flex_direction_t */
    uint8_t justify;         /* ew_justify_t */
    uint8_t align;           /* ew_align_t */
    
    /* Hierarchy link indices into tree node pool */
    int16_t parent_idx;      /* -1 if root */
    int16_t first_child_idx; /* -1 if no children */
    int16_t next_sibling_idx;/* -1 if no next sibling */
    
    /* Style */
    ew_style_t style;
    
    /* Specific control state payloads */
    int16_t progress_value;  /* 0 to 100 for EW_WIDGET_PROGRESS_BAR */
    const ew_image_asset_t* image_asset; /* For EW_WIDGET_IMAGE */
    
    /* Text payload (for labels/buttons) */
    char text[EW_MAX_TEXT_LEN];

    /* Interaction and Input Dispatch */
    uint8_t touchable;       /* 1 if hit-testable, 0 if transparent to clicks */
    uint8_t state;           /* 0 = default, 1 = pressed */
    ew_node_event_cb on_event; /* Optional event callback */
    void* user_data;         /* User context passed to callback */
} ew_node_t;

#ifdef __cplusplus
}
#endif

#endif /* EW_TYPES_H */
