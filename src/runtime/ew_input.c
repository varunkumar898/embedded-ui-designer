#include "ew_input.h"
#include <string.h>

void ew_input_state_init(ew_input_state_t* state) {
    if (!state) return;
    state->is_down = false;
    state->last_x = 0;
    state->last_y = 0;
    state->pressed_node_idx = -1;
}

void ew_input_state_reset(ew_input_state_t* state) {
    if (!state) return;
    state->is_down = false;
    state->last_x = 0;
    state->last_y = 0;
    state->pressed_node_idx = -1;
}

static inline bool ew_rect_contains(ew_rect_t rect, int16_t x, int16_t y) {
    return (x >= rect.x && x < rect.x + rect.w &&
            y >= rect.y && y < rect.y + rect.h);
}

/**
 * Recursive hit test for a single node subtree in reverse z-order.
 * Children drawn later are visually on top, so check children in reverse order.
 */
static int16_t hit_test_node(const ew_tree_t* tree, int16_t node_idx, int16_t x, int16_t y) {
    if (!tree || node_idx < 0 || node_idx >= tree->count) return -1;
    
    const ew_node_t* node = &tree->nodes[node_idx];
    if (!node->visible) return -1;
    
    /* Collect immediate children to traverse in reverse order */
    int16_t child_indices[EW_MAX_NODES];
    int child_count = 0;
    int16_t curr = node->first_child_idx;
    while (curr >= 0 && curr < tree->count && child_count < EW_MAX_NODES) {
        child_indices[child_count++] = curr;
        curr = tree->nodes[curr].next_sibling_idx;
    }
    
    /* Check children from last to first (reverse z-order) */
    for (int i = child_count - 1; i >= 0; i--) {
        int16_t hit = hit_test_node(tree, child_indices[i], x, y);
        if (hit >= 0) {
            return hit;
        }
    }
    
    /* If no child hit, check if this node itself is touchable and contains (x, y) */
    if (node->touchable && ew_rect_contains(node->rect, x, y)) {
        return node_idx;
    }
    
    return -1;
}

int16_t ew_input_hit_test(const ew_tree_t* tree, int16_t x, int16_t y) {
    if (!tree || tree->count == 0 || tree->root_idx < 0 || tree->root_idx >= tree->count) {
        return -1;
    }
    return hit_test_node(tree, tree->root_idx, x, y);
}

void ew_input_dispatch(ew_tree_t* tree, ew_input_state_t* state, const ew_input_event_t* event) {
    if (!tree || !state || !event) return;
    
    switch (event->type) {
        case EW_EVENT_TOUCH_DOWN: {
            int16_t hit = ew_input_hit_test(tree, event->x, event->y);
            state->is_down = true;
            state->last_x = event->x;
            state->last_y = event->y;
            state->pressed_node_idx = hit;
            
            if (hit >= 0 && hit < tree->count) {
                ew_node_t* node = &tree->nodes[hit];
                node->state = 1; /* Pressed */
                if (node->on_event) {
                    node->on_event(node, EW_EVENT_TOUCH_DOWN, node->user_data);
                }
            }
            break;
        }
        
        case EW_EVENT_TOUCH_MOVE: {
            if (state->is_down) {
                state->last_x = event->x;
                state->last_y = event->y;
                if (state->pressed_node_idx >= 0 && state->pressed_node_idx < tree->count) {
                    ew_node_t* node = &tree->nodes[state->pressed_node_idx];
                    if (node->on_event) {
                        node->on_event(node, EW_EVENT_TOUCH_MOVE, node->user_data);
                    }
                }
            }
            break;
        }
        
        case EW_EVENT_TOUCH_UP: {
            if (state->is_down) {
                state->is_down = false;
                int16_t pressed_idx = state->pressed_node_idx;
                state->pressed_node_idx = -1;
                
                if (pressed_idx >= 0 && pressed_idx < tree->count) {
                    ew_node_t* node = &tree->nodes[pressed_idx];
                    node->state = 0; /* Released */
                    
                    if (node->on_event) {
                        node->on_event(node, EW_EVENT_TOUCH_UP, node->user_data);
                    }
                    
                    /* Click detection: pointer must be released within node's bounding box */
                    if (ew_rect_contains(node->rect, event->x, event->y)) {
                        if (node->on_event) {
                            node->on_event(node, EW_EVENT_CLICK, node->user_data);
                        }
                    }
                }
            }
            break;
        }
        
        case EW_EVENT_NONE:
        default:
            /* Clean up state on cancel or empty event */
            ew_input_state_reset(state);
            break;
    }
}
