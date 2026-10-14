#include "ew_layout.h"

void ew_layout_compute(ew_tree_t* tree, int16_t node_idx, int16_t avail_w, int16_t avail_h) {
    if (!tree || node_idx < 0 || node_idx >= tree->count) return;
    
    ew_node_t* node = &tree->nodes[node_idx];
    if (!node->visible) return;
    
    /* Determine node's own width and height */
    if (node->req_w >= 0) {
        node->rect.w = node->req_w;
    } else {
        node->rect.w = avail_w;
    }
    
    if (node->req_h >= 0) {
        node->rect.h = node->req_h;
    } else {
        node->rect.h = avail_h;
    }
    
    /* If no children, nothing to layout inside */
    if (node->first_child_idx == -1) {
        return;
    }
    
    int16_t pad = (int16_t)node->style.padding;
    int16_t gap = (int16_t)node->style.gap;
    
    int16_t inner_w = node->rect.w - (2 * pad);
    if (inner_w < 0) inner_w = 0;
    
    int16_t inner_h = node->rect.h - (2 * pad);
    if (inner_h < 0) inner_h = 0;
    
    /* First pass: count visible children and flexible children */
    int16_t child_count = 0;
    int16_t flex_child_count = 0;
    int16_t fixed_main_sum = 0;
    
    int16_t curr = node->first_child_idx;
    while (curr != -1) {
        ew_node_t* c = &tree->nodes[curr];
        if (c->visible) {
            child_count++;
            if (node->flex_direction == EW_FLEX_DIR_ROW) {
                if (c->req_w >= 0) {
                    fixed_main_sum += c->req_w;
                } else {
                    flex_child_count++;
                }
            } else { /* EW_FLEX_DIR_COLUMN */
                if (c->req_h >= 0) {
                    fixed_main_sum += c->req_h;
                } else {
                    flex_child_count++;
                }
            }
        }
        curr = c->next_sibling_idx;
    }
    
    if (child_count == 0) return;
    
    int16_t total_gaps = (child_count > 1) ? (child_count - 1) * gap : 0;
    int16_t main_space = (node->flex_direction == EW_FLEX_DIR_ROW) ? inner_w : inner_h;
    int16_t free_space = main_space - fixed_main_sum - total_gaps;
    if (free_space < 0) free_space = 0;
    
    int16_t flex_item_size = 0;
    if (flex_child_count > 0) {
        flex_item_size = free_space / flex_child_count;
        free_space = 0; /* All consumed by flex children */
    }
    
    /* Main-axis start offset according to justify */
    int16_t main_offset = pad;
    int16_t between_space = 0;
    if (flex_child_count == 0 && free_space > 0) {
        switch (node->justify) {
            case EW_JUSTIFY_CENTER:
                main_offset += free_space / 2;
                break;
            case EW_JUSTIFY_END:
                main_offset += free_space;
                break;
            case EW_JUSTIFY_SPACE_BETWEEN:
                if (child_count > 1) {
                    between_space = free_space / (child_count - 1);
                }
                break;
            case EW_JUSTIFY_SPACE_AROUND:
                if (child_count > 0) {
                    between_space = free_space / child_count;
                    main_offset += between_space / 2;
                }
                break;
            case EW_JUSTIFY_START:
            default:
                break;
        }
    }
    
    /* Second pass: place and layout children */
    curr = node->first_child_idx;
    while (curr != -1) {
        ew_node_t* c = &tree->nodes[curr];
        if (c->visible) {
            int16_t c_w, c_h;
            int16_t c_x, c_y;
            
            if (node->flex_direction == EW_FLEX_DIR_ROW) {
                /* Width */
                if (c->req_w >= 0) {
                    c_w = c->req_w;
                } else {
                    c_w = flex_item_size;
                }
                
                /* Height & cross-axis alignment */
                if (node->align == EW_ALIGN_STRETCH && c->req_h < 0) {
                    c_h = inner_h;
                    c_y = node->rect.y + pad;
                } else {
                    c_h = (c->req_h >= 0) ? c->req_h : inner_h;
                    if (node->align == EW_ALIGN_CENTER) {
                        c_y = node->rect.y + pad + ((inner_h - c_h) / 2);
                    } else if (node->align == EW_ALIGN_END) {
                        c_y = node->rect.y + pad + (inner_h - c_h);
                    } else {
                        c_y = node->rect.y + pad;
                    }
                }
                
                c_x = node->rect.x + main_offset;
                main_offset += c_w + gap + between_space;
            } else { /* EW_FLEX_DIR_COLUMN */
                /* Height */
                if (c->req_h >= 0) {
                    c_h = c->req_h;
                } else {
                    c_h = flex_item_size;
                }
                
                /* Width & cross-axis alignment */
                if (node->align == EW_ALIGN_STRETCH && c->req_w < 0) {
                    c_w = inner_w;
                    c_x = node->rect.x + pad;
                } else {
                    c_w = (c->req_w >= 0) ? c->req_w : inner_w;
                    if (node->align == EW_ALIGN_CENTER) {
                        c_x = node->rect.x + pad + ((inner_w - c_w) / 2);
                    } else if (node->align == EW_ALIGN_END) {
                        c_x = node->rect.x + pad + (inner_w - c_w);
                    } else {
                        c_x = node->rect.x + pad;
                    }
                }
                
                c_y = node->rect.y + main_offset;
                main_offset += c_h + gap + between_space;
            }
            
            c->rect.x = c_x;
            c->rect.y = c_y;
            c->rect.w = c_w;
            c->rect.h = c_h;
            
            /* Recursively compute children's layouts */
            ew_layout_compute(tree, curr, c_w, c_h);
        }
        curr = c->next_sibling_idx;
    }
}
