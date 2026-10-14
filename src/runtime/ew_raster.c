#include "ew_raster.h"
#include <string.h>

/* Embedded 8x8 ASCII font bitmap for printable characters 32..126 (760 bytes const flash) */
static const uint8_t font_8x8[95][8] = {
    {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00}, /* ' ' */
    {0x18,0x3C,0x3C,0x18,0x18,0x00,0x18,0x00}, /* '!' */
    {0x66,0x66,0x24,0x00,0x00,0x00,0x00,0x00}, /* '"' */
    {0x6C,0x6C,0xFE,0x6C,0xFE,0x6C,0x6C,0x00}, /* '#' */
    {0x18,0x3E,0x60,0x3C,0x06,0x7C,0x18,0x00}, /* '$' */
    {0x00,0x66,0x66,0x0C,0x18,0x33,0x33,0x00}, /* '%' */
    {0x38,0x6C,0x38,0x76,0xDC,0xCC,0x76,0x00}, /* '&' */
    {0x18,0x18,0x30,0x00,0x00,0x00,0x00,0x00}, /* ''' */
    {0x0C,0x18,0x30,0x30,0x30,0x18,0x0C,0x00}, /* '(' */
    {0x30,0x18,0x0C,0x0C,0x0C,0x18,0x30,0x00}, /* ')' */
    {0x00,0x66,0x3C,0xFF,0x3C,0x66,0x00,0x00}, /* '*' */
    {0x00,0x18,0x18,0x7E,0x18,0x18,0x00,0x00}, /* '+' */
    {0x00,0x00,0x00,0x00,0x00,0x18,0x18,0x30}, /* ',' */
    {0x00,0x00,0x00,0x7E,0x00,0x00,0x00,0x00}, /* '-' */
    {0x00,0x00,0x00,0x00,0x00,0x18,0x18,0x00}, /* '.' */
    {0x06,0x0C,0x18,0x30,0x60,0xC0,0x80,0x00}, /* '/' */
    {0x7C,0xC6,0xCE,0xD6,0xE6,0xC6,0x7C,0x00}, /* '0' */
    {0x18,0x38,0x18,0x18,0x18,0x18,0x7E,0x00}, /* '1' */
    {0x7C,0xC6,0x06,0x1C,0x30,0x60,0xFE,0x00}, /* '2' */
    {0x7C,0xC6,0x06,0x3C,0x06,0xC6,0x7C,0x00}, /* '3' */
    {0x1C,0x3C,0x6C,0xCC,0xFE,0x0C,0x1E,0x00}, /* '4' */
    {0xFE,0xC0,0xFC,0x06,0x06,0xC6,0x7C,0x00}, /* '5' */
    {0x7C,0xC6,0xC0,0xFC,0xC6,0xC6,0x7C,0x00}, /* '6' */
    {0xFE,0xC6,0x0C,0x18,0x30,0x30,0x30,0x00}, /* '7' */
    {0x7C,0xC6,0xC6,0x7C,0xC6,0xC6,0x7C,0x00}, /* '8' */
    {0x7C,0xC6,0xC6,0x7E,0x06,0xC6,0x7C,0x00}, /* '9' */
    {0x00,0x18,0x18,0x00,0x00,0x18,0x18,0x00}, /* ':' */
    {0x00,0x18,0x18,0x00,0x00,0x18,0x18,0x30}, /* ';' */
    {0x0C,0x18,0x30,0x60,0x30,0x18,0x0C,0x00}, /* '<' */
    {0x00,0x00,0x7E,0x00,0x7E,0x00,0x00,0x00}, /* '=' */
    {0x30,0x18,0x0C,0x06,0x0C,0x18,0x30,0x00}, /* '>' */
    {0x7C,0xC6,0x0C,0x18,0x18,0x00,0x18,0x00}, /* '?' */
    {0x7C,0xC6,0xDE,0xDE,0xDC,0xC0,0x7C,0x00}, /* '@' */
    {0x38,0x6C,0xC6,0xFE,0xC6,0xC6,0xC6,0x00}, /* 'A' */
    {0xFC,0x66,0x66,0x7C,0x66,0x66,0xFC,0x00}, /* 'B' */
    {0x3C,0x66,0xC0,0xC0,0xC0,0x66,0x3C,0x00}, /* 'C' */
    {0xF8,0x6C,0x66,0x66,0x66,0x6C,0xF8,0x00}, /* 'D' */
    {0xFE,0x62,0x68,0x78,0x68,0x62,0xFE,0x00}, /* 'E' */
    {0xFE,0x62,0x68,0x78,0x68,0x60,0xF0,0x00}, /* 'F' */
    {0x3C,0x66,0xC0,0xC0,0xCE,0x66,0x3A,0x00}, /* 'G' */
    {0xC6,0xC6,0xC6,0xFE,0xC6,0xC6,0xC6,0x00}, /* 'H' */
    {0x3C,0x18,0x18,0x18,0x18,0x18,0x3C,0x00}, /* 'I' */
    {0x1E,0x0C,0x0C,0x0C,0xCC,0xCC,0x78,0x00}, /* 'J' */
    {0xE6,0x66,0x6C,0x78,0x6C,0x66,0xE6,0x00}, /* 'K' */
    {0xF0,0x60,0x60,0x60,0x62,0x66,0xFE,0x00}, /* 'L' */
    {0xC6,0xEE,0xFE,0xFE,0xD6,0xC6,0xC6,0x00}, /* 'M' */
    {0xC6,0xE6,0xF6,0xDE,0xCE,0xC6,0xC6,0x00}, /* 'N' */
    {0x7C,0xC6,0xC6,0xC6,0xC6,0xC6,0x7C,0x00}, /* 'O' */
    {0xFC,0x66,0x66,0x7C,0x60,0x60,0xF0,0x00}, /* 'P' */
    {0x7C,0xC6,0xC6,0xC6,0xD6,0xDE,0x7C,0x06}, /* 'Q' */
    {0xFC,0x66,0x66,0x7C,0x6C,0x66,0xE6,0x00}, /* 'R' */
    {0x7C,0xC6,0x60,0x38,0x0C,0xC6,0x7C,0x00}, /* 'S' */
    {0x7E,0x18,0x18,0x18,0x18,0x18,0x18,0x00}, /* 'T' */
    {0xC6,0xC6,0xC6,0xC6,0xC6,0xC6,0x7C,0x00}, /* 'U' */
    {0xC6,0xC6,0xC6,0xC6,0x6C,0x38,0x10,0x00}, /* 'V' */
    {0xC6,0xC6,0xD6,0xFE,0xFE,0xEE,0xC6,0x00}, /* 'W' */
    {0xC6,0xC6,0x6C,0x38,0x6C,0xC6,0xC6,0x00}, /* 'X' */
    {0x66,0x66,0x66,0x3C,0x18,0x18,0x3C,0x00}, /* 'Y' */
    {0xFE,0xC6,0x0C,0x18,0x30,0x63,0xFE,0x00}, /* 'Z' */
    {0x3C,0x30,0x30,0x30,0x30,0x30,0x3C,0x00}, /* '[' */
    {0xC0,0x60,0x30,0x18,0x0C,0x06,0x02,0x00}, /* '\' */
    {0x3C,0x0C,0x0C,0x0C,0x0C,0x0C,0x3C,0x00}, /* ']' */
    {0x10,0x38,0x6C,0xC6,0x00,0x00,0x00,0x00}, /* '^' */
    {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0xFF}, /* '_' */
    {0x30,0x18,0x0C,0x00,0x00,0x00,0x00,0x00}, /* '`' */
    {0x00,0x00,0x78,0x0C,0x7C,0xCC,0x76,0x00}, /* 'a' */
    {0xE0,0x60,0x7C,0x66,0x66,0x66,0xDC,0x00}, /* 'b' */
    {0x00,0x00,0x78,0xCC,0xC0,0xCC,0x78,0x00}, /* 'c' */
    {0x1C,0x0C,0x7C,0xCC,0xCC,0xCC,0x76,0x00}, /* 'd' */
    {0x00,0x00,0x78,0xCC,0xFC,0xC0,0x78,0x00}, /* 'e' */
    {0x38,0x6C,0x60,0xF0,0x60,0x60,0xF0,0x00}, /* 'f' */
    {0x00,0x00,0x76,0xCC,0xCC,0x7C,0x0C,0xF8}, /* 'g' */
    {0xE0,0x60,0x6C,0x76,0x66,0x66,0xE6,0x00}, /* 'h' */
    {0x18,0x00,0x38,0x18,0x18,0x18,0x3C,0x00}, /* 'i' */
    {0x06,0x00,0x06,0x06,0x06,0x66,0x66,0x3C}, /* 'j' */
    {0xE0,0x60,0x66,0x6C,0x78,0x6C,0xE6,0x00}, /* 'k' */
    {0x38,0x18,0x18,0x18,0x18,0x18,0x3C,0x00}, /* 'l' */
    {0x00,0x00,0xE6,0xFF,0xDB,0xDB,0xC3,0x00}, /* 'm' */
    {0x00,0x00,0xDC,0x66,0x66,0x66,0x66,0x00}, /* 'n' */
    {0x00,0x00,0x7C,0xC6,0xC6,0xC6,0x7C,0x00}, /* 'o' */
    {0x00,0x00,0xDC,0x66,0x66,0x7C,0x60,0xF0}, /* 'p' */
    {0x00,0x00,0x76,0xCC,0xCC,0x7C,0x0C,0x1E}, /* 'q' */
    {0x00,0x00,0xDC,0x76,0x60,0x60,0xF0,0x00}, /* 'r' */
    {0x00,0x00,0x7E,0xC0,0x7C,0x06,0xFC,0x00}, /* 's' */
    {0x30,0x30,0xFC,0x30,0x30,0x34,0x18,0x00}, /* 't' */
    {0x00,0x00,0xCC,0xCC,0xCC,0xCC,0x76,0x00}, /* 'u' */
    {0x00,0x00,0xC6,0xC6,0x6C,0x38,0x10,0x00}, /* 'v' */
    {0x00,0x00,0xC6,0xD6,0xFE,0xFE,0x6C,0x00}, /* 'w' */
    {0x00,0x00,0xC6,0x6C,0x38,0x6C,0xC6,0x00}, /* 'x' */
    {0x00,0x00,0xC6,0xC6,0xCE,0x76,0x06,0x7C}, /* 'y' */
    {0x00,0x00,0xFC,0x18,0x30,0x60,0xFC,0x00}, /* 'z' */
    {0x0E,0x18,0x18,0x70,0x18,0x18,0x0E,0x00}, /* '{' */
    {0x18,0x18,0x18,0x00,0x18,0x18,0x18,0x00}, /* '|' */
    {0x70,0x18,0x18,0x0E,0x18,0x18,0x70,0x00}, /* '}' */
    {0x76,0xDC,0x00,0x00,0x00,0x00,0x00,0x00}  /* '~' */
};

void ew_raster_clear(ew_raster_slice_t* slice, ew_color_t color) {
    if (!slice || !slice->buffer || slice->width <= 0 || slice->height <= 0) return;
    int total_pixels = slice->width * slice->height;
    for (int i = 0; i < total_pixels; i++) {
        slice->buffer[i] = color;
    }
}

void ew_raster_draw_rect(ew_raster_slice_t* slice, ew_rect_t rect, ew_color_t color) {
    if (!slice || !slice->buffer || rect.w <= 0 || rect.h <= 0) return;
    
    int16_t slice_top = slice->slice_y;
    int16_t slice_bottom = slice->slice_y + slice->height;
    
    int16_t rect_top = rect.y;
    int16_t rect_bottom = rect.y + rect.h;
    
    /* Check vertical overlap */
    if (rect_bottom <= slice_top || rect_top >= slice_bottom) return;
    
    int16_t y_start = (rect_top > slice_top) ? rect_top : slice_top;
    int16_t y_end = (rect_bottom < slice_bottom) ? rect_bottom : slice_bottom;
    
    int16_t rect_left = rect.x;
    int16_t rect_right = rect.x + rect.w;
    
    if (rect_right <= 0 || rect_left >= slice->width) return;
    
    int16_t x_start = (rect_left > 0) ? rect_left : 0;
    int16_t x_end = (rect_right < slice->width) ? rect_right : slice->width;
    
    for (int16_t y = y_start; y < y_end; y++) {
        int16_t local_y = y - slice_top;
        uint16_t* row = &slice->buffer[local_y * slice->width];
        for (int16_t x = x_start; x < x_end; x++) {
            row[x] = color;
        }
    }
}

void ew_raster_draw_rect_border(ew_raster_slice_t* slice, ew_rect_t rect, ew_color_t color, uint8_t border_width) {
    if (border_width == 0) return;
    
    /* Top edge */
    ew_rect_t top = { rect.x, rect.y, rect.w, (int16_t)border_width };
    ew_raster_draw_rect(slice, top, color);
    
    /* Bottom edge */
    ew_rect_t bottom = { rect.x, (int16_t)(rect.y + rect.h - border_width), rect.w, (int16_t)border_width };
    ew_raster_draw_rect(slice, bottom, color);
    
    /* Left edge */
    ew_rect_t left = { rect.x, (int16_t)(rect.y + border_width), (int16_t)border_width, (int16_t)(rect.h - (2 * border_width)) };
    ew_raster_draw_rect(slice, left, color);
    
    /* Right edge */
    ew_rect_t right = { (int16_t)(rect.x + rect.w - border_width), (int16_t)(rect.y + border_width), (int16_t)border_width, (int16_t)(rect.h - (2 * border_width)) };
    ew_raster_draw_rect(slice, right, color);
}

void ew_raster_draw_char(ew_raster_slice_t* slice, int16_t x, int16_t y, char c, ew_color_t color) {
    if (!slice || !slice->buffer) return;
    if (c < 32 || c > 126) c = '?';
    
    const uint8_t* glyph = font_8x8[c - 32];
    int16_t slice_top = slice->slice_y;
    int16_t slice_bottom = slice->slice_y + slice->height;
    
    for (int row = 0; row < 8; row++) {
        int16_t py = y + row;
        if (py < slice_top || py >= slice_bottom) continue;
        int16_t local_y = py - slice_top;
        
        uint8_t line = glyph[row];
        for (int col = 0; col < 8; col++) {
            if (line & (0x80 >> col)) {
                int16_t px = x + col;
                if (px >= 0 && px < slice->width) {
                    slice->buffer[local_y * slice->width + px] = color;
                }
            }
        }
    }
}

void ew_raster_draw_text(ew_raster_slice_t* slice, int16_t x, int16_t y, const char* text, ew_color_t color) {
    if (!slice || !text) return;
    int16_t cur_x = x;
    while (*text) {
        ew_raster_draw_char(slice, cur_x, y, *text, color);
        cur_x += 8;
        text++;
    }
}

void ew_raster_draw_rect_clipped(ew_raster_slice_t* slice, ew_rect_t rect, ew_rect_t clip, ew_color_t color) {
    if (!slice || !slice->buffer || rect.w <= 0 || rect.h <= 0 || clip.w <= 0 || clip.h <= 0) return;
    
    /* Intersect rect with clip */
    int16_t ix1 = (rect.x > clip.x) ? rect.x : clip.x;
    int16_t iy1 = (rect.y > clip.y) ? rect.y : clip.y;
    int16_t ix2 = (rect.x + rect.w < clip.x + clip.w) ? (int16_t)(rect.x + rect.w) : (int16_t)(clip.x + clip.w);
    int16_t iy2 = (rect.y + rect.h < clip.y + clip.h) ? (int16_t)(rect.y + rect.h) : (int16_t)(clip.y + clip.h);
    
    if (ix2 <= ix1 || iy2 <= iy1) return;
    
    ew_rect_t clipped_rect = { ix1, iy1, (int16_t)(ix2 - ix1), (int16_t)(iy2 - iy1) };
    ew_raster_draw_rect(slice, clipped_rect, color);
}

void ew_raster_draw_progress_bar(ew_raster_slice_t* slice, ew_rect_t rect, int16_t progress_value, ew_color_t track_color, ew_color_t fill_color, ew_color_t border_color, uint8_t border_width) {
    if (!slice || !slice->buffer || rect.w <= 0 || rect.h <= 0) return;
    
    /* Clamp progress */
    if (progress_value < 0) progress_value = 0;
    if (progress_value > 100) progress_value = 100;
    
    /* 1. Draw Track Background */
    ew_raster_draw_rect(slice, rect, track_color);
    
    /* 2. Draw Fill Bar */
    int16_t inner_x = (int16_t)(rect.x + border_width);
    int16_t inner_y = (int16_t)(rect.y + border_width);
    int16_t inner_w = (int16_t)(rect.w - (2 * border_width));
    int16_t inner_h = (int16_t)(rect.h - (2 * border_width));
    
    if (inner_w > 0 && inner_h > 0) {
        int16_t fill_w = (int16_t)((inner_w * progress_value) / 100);
        if (fill_w > 0) {
            ew_rect_t fill_rect = { inner_x, inner_y, fill_w, inner_h };
            ew_raster_draw_rect(slice, fill_rect, fill_color);
        }
    }
    
    /* 3. Draw Outer Border */
    if (border_width > 0) {
        ew_raster_draw_rect_border(slice, rect, border_color, border_width);
    }
}

void ew_raster_draw_image(ew_raster_slice_t* slice, int16_t x, int16_t y, const ew_image_asset_t* image, uint8_t opacity) {
    if (!slice || !slice->buffer || !image || !image->pixels) return;
    if (image->width == 0 || image->height == 0) return;
    if (image->format == EW_IMAGE_FORMAT_UNSUPPORTED) return;
    if (opacity == 0) return;
    
    int16_t slice_top = slice->slice_y;
    int16_t slice_bottom = slice->slice_y + slice->height;
    
    int16_t img_top = y;
    int16_t img_bottom = y + (int16_t)image->height;
    
    if (img_bottom <= slice_top || img_top >= slice_bottom) return;
    
    int16_t y_start = (img_top > slice_top) ? img_top : slice_top;
    int16_t y_end = (img_bottom < slice_bottom) ? img_bottom : slice_bottom;
    
    int16_t img_left = x;
    int16_t img_right = x + (int16_t)image->width;
    
    if (img_right <= 0 || img_left >= slice->width) return;
    
    int16_t x_start = (img_left > 0) ? img_left : 0;
    int16_t x_end = (img_right < slice->width) ? img_right : slice->width;
    
    for (int16_t py = y_start; py < y_end; py++) {
        int16_t local_y = py - slice_top;
        int16_t src_y = py - y;
        uint16_t* dst_row = &slice->buffer[local_y * slice->width];
        const uint16_t* src_row = &image->pixels[src_y * image->width];
        
        for (int16_t px = x_start; px < x_end; px++) {
            int16_t src_x = px - x;
            uint16_t src_pixel = src_row[src_x];
            
            if (image->format == EW_IMAGE_FORMAT_RGB565_CHROMA && src_pixel == image->chroma_key) {
                continue; /* Transparent pixel */
            }
            
            if (opacity == 255) {
                dst_row[px] = src_pixel;
            } else {
                dst_row[px] = ew_color_blend565(src_pixel, dst_row[px], opacity);
            }
        }
    }
}

void ew_raster_render_tree(ew_raster_slice_t* slice, const ew_tree_t* tree, int16_t node_idx) {
    if (!slice) return;
    ew_rect_t full_clip = { 0, 0, slice->width, (int16_t)(slice->slice_y + slice->height) };
    ew_raster_render_tree_clipped(slice, tree, node_idx, full_clip);
}

void ew_raster_render_tree_clipped(ew_raster_slice_t* slice, const ew_tree_t* tree, int16_t node_idx, ew_rect_t clip_rect) {
    if (!slice || !tree || node_idx < 0 || node_idx >= tree->count) return;
    
    const ew_node_t* node = &tree->nodes[node_idx];
    if (!node->visible) return;
    
    /* 1. Render Background if enabled */
    if (node->style.has_bg) {
        ew_raster_draw_rect_clipped(slice, node->rect, clip_rect, node->style.bg_color);
    }
    
    /* 2. Render Border if enabled */
    if (node->style.has_border && node->style.border_width > 0) {
        ew_raster_draw_rect_border(slice, node->rect, node->style.border_color, node->style.border_width);
    }
    
    /* 3. Render Component Specifics */
    if (node->type == EW_WIDGET_LABEL || node->type == EW_WIDGET_BUTTON) {
        if (node->text[0] != '\0') {
            int16_t text_len = (int16_t)strlen(node->text);
            int16_t text_w = text_len * 8;
            int16_t text_h = 8;
            
            /* Center text within node bounds */
            int16_t tx = node->rect.x + ((node->rect.w - text_w) / 2);
            int16_t ty = node->rect.y + ((node->rect.h - text_h) / 2);
            if (tx < node->rect.x) tx = node->rect.x;
            if (ty < node->rect.y) ty = node->rect.y;
            
            ew_raster_draw_text(slice, tx, ty, node->text, node->style.text_color);
        }
    } else if (node->type == EW_WIDGET_PROGRESS_BAR) {
        ew_raster_draw_progress_bar(slice, node->rect, node->progress_value,
                                    node->style.bg_color, node->style.text_color,
                                    node->style.border_color, node->style.border_width);
    } else if (node->type == EW_WIDGET_IMAGE) {
        if (node->image_asset) {
            ew_raster_draw_image(slice, node->rect.x, node->rect.y, node->image_asset, node->style.opacity);
        }
    }
    
    /* 4. Recursively Render Children in DFS order */
    int16_t child_idx = node->first_child_idx;
    while (child_idx != -1) {
        ew_raster_render_tree_clipped(slice, tree, child_idx, clip_rect);
        child_idx = tree->nodes[child_idx].next_sibling_idx;
    }
}
