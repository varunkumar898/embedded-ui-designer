#ifndef EW_TREE_H
#define EW_TREE_H

#include "ew_types.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    ew_node_t nodes[EW_MAX_NODES];
    uint16_t count;
    int16_t root_idx;
} ew_tree_t;

/* Compile-time verification of tree capacity and structural size invariants */
#if defined(__cplusplus)
  static_assert(EW_MAX_TREE_NODES == 128, "EW_MAX_TREE_NODES must be exactly 128 nodes");
#elif defined(__STDC_VERSION__) && __STDC_VERSION__ >= 201112L
  _Static_assert(EW_MAX_TREE_NODES == 128, "EW_MAX_TREE_NODES must be exactly 128 nodes");
#elif defined(__GNUC__)
  _Static_assert(EW_MAX_TREE_NODES == 128, "EW_MAX_TREE_NODES must be exactly 128 nodes");
#endif

#if (defined(__SIZEOF_POINTER__) && __SIZEOF_POINTER__ == 4) || (!defined(__SIZEOF_POINTER__) && (defined(__arm__) || defined(__XTENSA__) || defined(__riscv)))
  #if defined(__cplusplus)
    static_assert(sizeof(ew_node_t) == 92, "ew_node_t must be 92 bytes on 32-bit embedded targets");
    static_assert(sizeof(ew_tree_t) == 11780, "ew_tree_t must be exactly 11,780 bytes (128 * 92 + 4)");
  #elif defined(__STDC_VERSION__) && __STDC_VERSION__ >= 201112L
    _Static_assert(sizeof(ew_node_t) == 92, "ew_node_t must be 92 bytes on 32-bit embedded targets");
    _Static_assert(sizeof(ew_tree_t) == 11780, "ew_tree_t must be exactly 11,780 bytes (128 * 92 + 4)");
  #elif defined(__GNUC__)
    _Static_assert(sizeof(ew_node_t) == 92, "ew_node_t must be 92 bytes on 32-bit embedded targets");
    _Static_assert(sizeof(ew_tree_t) == 11780, "ew_tree_t must be exactly 11,780 bytes (128 * 92 + 4)");
  #endif
#endif

/**
 * @brief Initialize a tree, clearing all node slots.
 */
void ew_tree_init(ew_tree_t* tree);

/**
 * @brief Allocate a node slot in the static pool.
 * @return Index of allocated node, or -1 if pool is full.
 */
int16_t ew_tree_alloc_node(ew_tree_t* tree, ew_widget_type_t type);

/**
 * @brief Attach child_idx as the last child of parent_idx.
 * @return true if attached successfully, false on error.
 */
bool ew_tree_add_child(ew_tree_t* tree, int16_t parent_idx, int16_t child_idx);

/**
 * @brief Retrieve a pointer to the node at index.
 * @return Pointer to node, or NULL if invalid index.
 */
ew_node_t* ew_tree_get_node(ew_tree_t* tree, int16_t idx);

/**
 * @brief Retrieve a const pointer to the node at index.
 * @return Const pointer to node, or NULL if invalid index.
 */
const ew_node_t* ew_tree_get_node_const(const ew_tree_t* tree, int16_t idx);

/**
 * @brief Safe property setter for progress bar. Clamps value to [0, 100].
 * @return true if property was updated, false on error or if unchanged.
 */
bool ew_node_set_progress(ew_tree_t* tree, int16_t idx, int16_t value);

/**
 * @brief Safe text setter for labels/buttons. Safely copies up to EW_MAX_TEXT_LEN-1 bytes.
 * @return true if text was updated, false on error.
 */
bool ew_node_set_text(ew_tree_t* tree, int16_t idx, const char* text);

/**
 * @brief Safe visibility setter.
 * @return true if visibility changed, false on error or if unchanged.
 */
bool ew_node_set_visible(ew_tree_t* tree, int16_t idx, bool visible);

/**
 * @brief Safe touchable setter.
 * @return true if updated, false on error.
 */
bool ew_node_set_touchable(ew_tree_t* tree, int16_t idx, bool touchable);

/**
 * @brief Safe event callback registration.
 * @return true if registered, false on error.
 */
bool ew_node_set_event_cb(ew_tree_t* tree, int16_t idx, ew_node_event_cb cb, void* user_data);

#ifdef __cplusplus
}
#endif

#endif /* EW_TREE_H */
