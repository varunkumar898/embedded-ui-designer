#ifndef EW_LAYOUT_H
#define EW_LAYOUT_H

#include "ew_tree.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Compute integer-based Flexbox layout for the component subtree starting at node_idx.
 * @param tree Pointer to EmbedWeb tree
 * @param node_idx Root of layout pass (usually tree->root_idx)
 * @param avail_w Available viewport or container width in pixels
 * @param avail_h Available viewport or container height in pixels
 */
void ew_layout_compute(ew_tree_t* tree, int16_t node_idx, int16_t avail_w, int16_t avail_h);

#ifdef __cplusplus
}
#endif

#endif /* EW_LAYOUT_H */
