#ifndef BP_TREE_H
#define BP_TREE_H

#include "bp_defines.h"
#include "bp_node.h"

// 64 height ought to be enough for anybody
#define BP_MAX_PATH_NODES 64

typedef struct {
    BpNode* nodes[BP_MAX_PATH_NODES];
    int node_indices[BP_MAX_PATH_NODES];
    int node_count;
} BpPath;

typedef struct {
    BpNode* root;
    int height;
} BpTree;

// path

void bp_path_fix(BpPath* self);
void bp_path_fix_with_delta(BpPath* self, int64_t delta);

BpNode* bp_path_next_leaf(BpPath* self);

void bp_path_prune_empty(BpPath* self, BpTree* tree);

// tree

BpTree* bp_tree_new_empty(void);
BpTree* bp_tree_new_with_root(BpNode* node, int height);
BpTree* bp_tree_new_with_piece(BpPiece node);
void bp_tree_free(BpTree* self);

BpTree* bp_tree_snapshot(const BpTree* self);

BpNode* bp_tree_lock_root(BpTree* self);

bp_len_t bp_tree_length(const BpTree* self);

void bp_tree_find(const BpTree* self, bp_len_t off, BpPath* out_path, bp_len_t* out_leaf_off);
void bp_tree_find_for_write(BpTree* self, bp_len_t off, BpPath* out_path, bp_len_t* out_leaf_off);

BpTree* bp_tree_join(BpTree* left, BpTree* right);
void bp_tree_split(BpTree* self, bp_len_t off, BpTree** out_left, BpTree** out_right);

void bp_tree_collapse_root(BpTree* self);
void bp_tree_seam_coalesce(BpTree* self, bp_len_t off);

#endif // BP_TREE_H
