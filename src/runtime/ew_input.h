#ifndef EW_INPUT_H
#define EW_INPUT_H

#include "ew_types.h"
#include "ew_tree.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Architectural Note on Touch Model:
 * EmbedWeb input engine currently implements a deterministic single-pointer model (EW_MAX_TOUCH_POINTS = 1).
 * Multi-touch gestures (pinch-to-zoom, dual swipe) are intentionally not supported on low-cost
 * resistive/single-point capacitive MCU touch controllers (XPT2046, FT6206, CST816S).
 */
#define EW_MAX_TOUCH_POINTS 1

typedef struct {
    bool is_down;
    int16_t last_x;
    int16_t last_y;
    int16_t pressed_node_idx; /* Node index pressed at TOUCH_DOWN, or -1 */
} ew_input_state_t;

/**
 * @brief Initialize input state tracker.
 */
void ew_input_state_init(ew_input_state_t* state);

/**
 * @brief Reset input state tracker (e.g. on pointer cancel or release).
 */
void ew_input_state_reset(ew_input_state_t* state);

/**
 * @brief Hit-test tree against coordinate (x, y) taking z-order into account.
 * Traverses children in reverse order so visually topmost elements receive input.
 * @return Node index of topmost visible, touchable widget, or -1 if none.
 */
int16_t ew_input_hit_test(const ew_tree_t* tree, int16_t x, int16_t y);

/**
 * @brief Dispatch a normalized input event into the tree.
 * Handles TOUCH_DOWN, TOUCH_MOVE, TOUCH_UP, and triggers EW_EVENT_CLICK
 * when touch is released inside the originating widget's bounds.
 */
void ew_input_dispatch(ew_tree_t* tree, ew_input_state_t* state, const ew_input_event_t* event);

#ifdef __cplusplus
}
#endif

#endif /* EW_INPUT_H */
