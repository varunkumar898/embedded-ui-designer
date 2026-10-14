#include "ew_hal.h"

void ew_engine_init(ew_engine_t* engine, ew_tree_t* tree, ew_display_driver_t* disp, ew_input_driver_t* input, uint16_t* slice_buf, int16_t slice_lines) {
    if (!engine) return;
    engine->tree = tree;
    engine->display = disp;
    engine->input = input;
    engine->slice_buffer = slice_buf;
    engine->slice_height = slice_lines;
    engine->clear_color = EW_COLOR_BLACK;
    ew_input_state_init(&engine->input_state);
}

void ew_engine_render_dirty_region(ew_engine_t* engine, ew_rect_t dirty_rect) {
    if (!engine || !engine->display || !engine->display->flush_cb || !engine->slice_buffer) return;
    if (engine->slice_height <= 0 || engine->display->width <= 0 || engine->display->height <= 0) return;
    
    int16_t disp_w = engine->display->width;
    int16_t disp_h = engine->display->height;
    
    /* Clamp dirty_rect to display bounds */
    int16_t x1 = (dirty_rect.x > 0) ? dirty_rect.x : 0;
    int16_t y1 = (dirty_rect.y > 0) ? dirty_rect.y : 0;
    int16_t x2 = (dirty_rect.x + dirty_rect.w < disp_w) ? (int16_t)(dirty_rect.x + dirty_rect.w) : disp_w;
    int16_t y2 = (dirty_rect.y + dirty_rect.h < disp_h) ? (int16_t)(dirty_rect.y + dirty_rect.h) : disp_h;
    
    if (x2 <= x1 || y2 <= y1) return;
    
    ew_rect_t clamped_dirty = { x1, y1, (int16_t)(x2 - x1), (int16_t)(y2 - y1) };
    int16_t slice_lines = engine->slice_height;
    
    /* Calculate slice indices that intersect [y1, y2) */
    int16_t first_slice_y = (y1 / slice_lines) * slice_lines;
    int16_t last_slice_y = ((y2 - 1) / slice_lines) * slice_lines;
    
    ew_raster_slice_t slice;
    slice.buffer = engine->slice_buffer;
    slice.width = disp_w;
    
    for (int16_t y = first_slice_y; y <= last_slice_y; y += slice_lines) {
        int16_t cur_h = slice_lines;
        if (y + cur_h > disp_h) {
            cur_h = disp_h - y;
        }
        
        slice.height = cur_h;
        slice.slice_y = y;
        
        /* 1. Clear slice buffer (clipped to dirty rect) */
        ew_raster_clear(&slice, engine->clear_color);
        
        /* 2. Render tree into slice clipped to clamped dirty rect */
        if (engine->tree && engine->tree->root_idx >= 0) {
            ew_raster_render_tree_clipped(&slice, engine->tree, engine->tree->root_idx, clamped_dirty);
        }
        
        /* 3. Flush slice to display */
        engine->display->flush_cb(engine->display, 0, y, disp_w, cur_h, slice.buffer);
    }
}

void ew_engine_render_frame(ew_engine_t* engine) {
    if (!engine || !engine->display) return;
    ew_rect_t full_frame = { 0, 0, engine->display->width, engine->display->height };
    ew_engine_render_dirty_region(engine, full_frame);
}

void ew_engine_invalidate_node(ew_engine_t* engine, int16_t node_idx) {
    if (!engine || !engine->tree) return;
    const ew_node_t* node = ew_tree_get_node_const(engine->tree, node_idx);
    if (!node || !node->visible) return;
    ew_engine_render_dirty_region(engine, node->rect);
}

void ew_engine_process_input(ew_engine_t* engine) {
    if (!engine || !engine->input || !engine->input->poll_cb || !engine->tree) return;
    
    ew_input_event_t evt;
    while (engine->input->poll_cb(engine->input, &evt)) {
        if (evt.type == EW_EVENT_NONE) break;
        ew_input_dispatch(engine->tree, &engine->input_state, &evt);
    }
}

static void adapter_flush_proxy(ew_display_driver_t* disp, int16_t x, int16_t y, int16_t w, int16_t h, const uint16_t* pixels) {
    ew_target_adapter_t* adapt = (ew_target_adapter_t*)disp->user_data;
    if (adapt && adapt->flush_pixels) {
        adapt->flush_pixels(adapt->user_ctx, x, y, w, h, pixels);
    }
}

static bool adapter_poll_input_proxy(ew_input_driver_t* drv, ew_input_event_t* event) {
    ew_target_adapter_t* adapt = (ew_target_adapter_t*)drv->user_data;
    if (!adapt || !adapt->poll_touch) return false;
    
    int16_t tx = 0, ty = 0;
    bool pressed = false;
    if (!adapt->poll_touch(adapt->user_ctx, &tx, &ty, &pressed)) {
        return false;
    }
    
    event->x = tx;
    event->y = ty;
    event->type = pressed ? EW_EVENT_TOUCH_DOWN : EW_EVENT_TOUCH_UP;
    return true;
}

void ew_hal_bind_adapter(ew_display_driver_t* out_disp, ew_input_driver_t* out_input, ew_target_adapter_t* adapter, int16_t width, int16_t height) {
    if (out_disp) {
        out_disp->width = width;
        out_disp->height = height;
        out_disp->flush_cb = adapter_flush_proxy;
        out_disp->user_data = (void*)adapter;
    }
    if (out_input) {
        out_input->poll_cb = adapter_poll_input_proxy;
        out_input->user_data = (void*)adapter;
    }
}

