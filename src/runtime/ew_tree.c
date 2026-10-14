#include "ew_tree.h"
#include <string.h>

void ew_tree_init(ew_tree_t* tree) {
    if (!tree) return;
    memset(tree->nodes, 0, sizeof(tree->nodes));
    tree->count = 0;
    tree->root_idx = -1;
}

int16_t ew_tree_alloc_node(ew_tree_t* tree, ew_widget_type_t type) {
    if (!tree || tree->count >= EW_MAX_NODES) {
        return -1;
    }
    
    int16_t idx = (int16_t)tree->count;
    tree->count++;
    
    ew_node_t* node = &tree->nodes[idx];
    memset(node, 0, sizeof(ew_node_t));
    node->id = (uint16_t)idx;
    node->type = (uint8_t)type;
    node->visible = 1;
    node->req_w = -1;
    node->req_h = -1;
    node->parent_idx = -1;
    node->first_child_idx = -1;
    node->next_sibling_idx = -1;
    node->style.opacity = 255;
    if (type == EW_WIDGET_BUTTON) {
        node->touchable = 1;
    }
    
    if (tree->root_idx == -1) {
        tree->root_idx = idx;
    }
    
    return idx;
}

bool ew_tree_add_child(ew_tree_t* tree, int16_t parent_idx, int16_t child_idx) {
    if (!tree) return false;
    if (parent_idx < 0 || parent_idx >= tree->count) return false;
    if (child_idx < 0 || child_idx >= tree->count) return false;
    if (parent_idx == child_idx) return false;
    
    /* Cycle detection: ensure child_idx is not an ancestor of parent_idx */
    int16_t ancestor = parent_idx;
    while (ancestor != -1) {
        if (ancestor == child_idx) return false;
        ancestor = tree->nodes[ancestor].parent_idx;
    }
    
    ew_node_t* parent = &tree->nodes[parent_idx];
    ew_node_t* child = &tree->nodes[child_idx];
    
    /* Check if child is already attached to parent */
    int16_t existing = parent->first_child_idx;
    while (existing != -1) {
        if (existing == child_idx) return false;
        existing = tree->nodes[existing].next_sibling_idx;
    }
    
    child->parent_idx = parent_idx;
    child->next_sibling_idx = -1;
    
    if (parent->first_child_idx == -1) {
        parent->first_child_idx = child_idx;
    } else {
        /* Append to end of sibling chain */
        int16_t curr_idx = parent->first_child_idx;
        while (tree->nodes[curr_idx].next_sibling_idx != -1) {
            curr_idx = tree->nodes[curr_idx].next_sibling_idx;
        }
        tree->nodes[curr_idx].next_sibling_idx = child_idx;
    }
    
    return true;
}

ew_node_t* ew_tree_get_node(ew_tree_t* tree, int16_t idx) {
    if (!tree || idx < 0 || idx >= tree->count) return NULL;
    return &tree->nodes[idx];
}

const ew_node_t* ew_tree_get_node_const(const ew_tree_t* tree, int16_t idx) {
    if (!tree || idx < 0 || idx >= tree->count) return NULL;
    return &tree->nodes[idx];
}

bool ew_node_set_progress(ew_tree_t* tree, int16_t idx, int16_t value) {
    ew_node_t* node = ew_tree_get_node(tree, idx);
    if (!node || node->type != EW_WIDGET_PROGRESS_BAR) return false;
    
    /* Clamp value to [0, 100] */
    if (value < 0) value = 0;
    if (value > 100) value = 100;
    
    if (node->progress_value == value) return false;
    node->progress_value = value;
    return true;
}

bool ew_node_set_text(ew_tree_t* tree, int16_t idx, const char* text) {
    ew_node_t* node = ew_tree_get_node(tree, idx);
    if (!node || !text) return false;
    
    /* Compare existing text */
    if (strncmp(node->text, text, EW_MAX_TEXT_LEN) == 0) return false;
    
    strncpy(node->text, text, EW_MAX_TEXT_LEN - 1);
    node->text[EW_MAX_TEXT_LEN - 1] = '\0';
    return true;
}

bool ew_node_set_visible(ew_tree_t* tree, int16_t idx, bool visible) {
    ew_node_t* node = ew_tree_get_node(tree, idx);
    if (!node) return false;
    
    uint8_t v = visible ? 1 : 0;
    if (node->visible == v) return false;
    node->visible = v;
    return true;
}

bool ew_node_set_touchable(ew_tree_t* tree, int16_t idx, bool touchable) {
    ew_node_t* node = ew_tree_get_node(tree, idx);
    if (!node) return false;
    
    uint8_t t = touchable ? 1 : 0;
    if (node->touchable == t) return false;
    node->touchable = t;
    return true;
}

bool ew_node_set_event_cb(ew_tree_t* tree, int16_t idx, ew_node_event_cb cb, void* user_data) {
    ew_node_t* node = ew_tree_get_node(tree, idx);
    if (!node) return false;
    
    node->on_event = cb;
    node->user_data = user_data;
    return true;
}
