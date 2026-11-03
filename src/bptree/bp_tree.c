#include "bp_tree.h"

#include <string.h>

#include "wtk_mem.h"

// path

void bp_path_fix(BpPath* self)
{
    for (int i = self->node_count - 1; i >= 1; i--) {
        BpNode* node = self->nodes[i - 1];
        BpNode* child = self->nodes[i];
        int child_idx = self->node_indices[i];
        wtk_assert(node->kind == BP_NODE_INTERIOR);

        node->interior.child_sizes[child_idx] = bp_node_total_length(child);
    }
}

void bp_path_fix_with_delta(BpPath* self, int64_t delta)
{
    for (int i = self->node_count - 1; i >= 1; i--) {
        BpNode* node = self->nodes[i - 1];
        int child_idx = self->node_indices[i];
        wtk_assert(node->kind == BP_NODE_INTERIOR);

        node->interior.child_sizes[child_idx] += delta;
    }
}

BpNode* bp_path_next_leaf(BpPath* self)
{
    for (int i = self->node_count - 1; i >= 1; i--) {
        BpNode* node = self->nodes[i - 1];
        int child_idx = self->node_indices[i];
        wtk_assert(node->kind == BP_NODE_INTERIOR);

        int next_child_idx = child_idx + 1;
        if (next_child_idx < node->child_count) {
            self->nodes[i] = node->interior.child_nodes[next_child_idx];
            self->node_indices[i] = next_child_idx;

            for (int j = i + 1; j < self->node_count; j++) {
                self->nodes[j] = self->nodes[j - 1]->interior.child_nodes[0];
                self->node_indices[j] = 0;
            }

            return self->nodes[self->node_count - 1];
        }
    }

    return NULL;
}

void bp_path_prune_empty(BpPath* self, BpTree* tree) {
    int i;
    for (i = self->node_count - 1; i >= 1 && self->nodes[i]->child_count == 0; i--) {
        BpNode* node = self->nodes[i - 1];
        int child_idx = self->node_indices[i];
        wtk_assert(node->kind == BP_NODE_INTERIOR);

        bp_node_delete_child(node, child_idx, true);
    }

    self->node_count = i + 1;
    bp_path_fix(self);
    bp_tree_collapse_root(tree);
}

// tree

BpTree* bp_tree_new_empty(void)
{
    BpTree* tree = wtk_malloc(BpTree*, sizeof(BpTree));

    BpNode* root = bp_node_new(BP_NODE_LEAF);
    root->child_count = 0;

    tree->root = root;
    tree->height = 0;

    return tree;
}

BpTree* bp_tree_new_with_root(BpNode* node, int height)
{
    BpTree* tree = wtk_malloc(BpTree*, sizeof(BpTree));
    tree->root = node;
    tree->height = height;

    return tree;
}

BpTree* bp_tree_new_with_piece(BpPiece node)
{
    BpTree* tree = wtk_malloc(BpTree*, sizeof(BpTree));

    BpNode* root = bp_node_new(BP_NODE_LEAF);
    root->child_count = 1;
    root->leaf.pieces[0] = node;

    tree->root = root;
    tree->height = 0;

    return tree;
}

void bp_tree_free(BpTree* self)
{
    if (self == NULL)
        return;

    bp_node_unref(self->root);
    free(self);
}

BpTree* bp_tree_snapshot(const BpTree* self)
{
    BpTree* new_tree = wtk_malloc(BpTree*, sizeof(BpTree));
    new_tree->root = bp_node_ref(self->root);
    new_tree->height = self->height;

    return new_tree;
}

BpNode* bp_tree_lock_root(BpTree* self)
{
    wtk_assert(self->root != NULL);
    self->root = bp_node_own(self->root);
    return self->root;
}

bp_len_t bp_tree_length(const BpTree* self)
{
    if (self->root == NULL)
        return 0;

    return bp_node_total_length(self->root);
}

void bp_tree_find(const BpTree* self, bp_len_t off, BpPath* out_path, bp_len_t* out_leaf_off)
{
    wtk_assert(self->root != NULL);
    wtk_assert(off >= 0 && off <= bp_tree_length(self));

    bp_len_t cur_off = off;

    BpNode* cur_node = self->root;
    out_path->node_count = 0;
    out_path->node_indices[0] = 0;

    int i;
    for (i = 0; i <= self->height; i++) {
        out_path->nodes[i] = cur_node;
        if (cur_node->kind == BP_NODE_LEAF)
            break;

        int child_idx = bp_node_find_child(cur_node, &cur_off);
        out_path->node_indices[i + 1] = child_idx;
        cur_node = cur_node->interior.child_nodes[child_idx];
    }

    out_path->node_count = i + 1;
    *out_leaf_off = cur_off;
}

void bp_tree_find_for_write(BpTree* self, bp_len_t off, BpPath* out_path, bp_len_t* out_leaf_off)
{
    wtk_assert(self->root != NULL);
    wtk_assert(off >= 0 && off < bp_tree_length(self));

    bp_len_t cur_off = off;

    BpNode* cur_node = bp_tree_lock_root(self);
    out_path->node_count = 0;
    out_path->node_indices[0] = 0;

    int i;
    for (i = 0; i <= self->height; i++) {
        out_path->nodes[i] = cur_node;
        if (cur_node->kind == BP_NODE_LEAF)
            break;

        int child_idx = bp_node_find_child(cur_node, &cur_off);
        out_path->node_indices[i + 1] = child_idx;
        cur_node = bp_node_lock_child(cur_node, child_idx);
    }

    out_path->node_count = i + 1;
    *out_leaf_off = cur_off;
}

BpTree* bp_tree_join(BpTree* left, BpTree* right)
{
    if (bp_tree_length(left) == 0) {
        bp_tree_free(left);
        return right;
    } else if (bp_tree_length(right) == 0) {
        bp_tree_free(right);
        return left;
    }

    BpNode* left_root = left->root;
    BpNode* right_root = right->root;
    int left_height = left->height;
    int right_height = right->height;
    int result_height = wtk_max(left_height, right_height);
    left->root = NULL;
    right->root = NULL;
    bp_tree_free(left);
    bp_tree_free(right);

    BpJoin join;
    if (left_height == right_height) {
        join = bp_join_equal(left_root, right_root, left_height);
    } else if (left_height > right_height) {
        join = bp_attach_edge(left_root, right_root, left_height, right_height, BP_JOIN_RIGHT);
    } else /* if (left_height < right_height) */ {
        join = bp_attach_edge(right_root, left_root, right_height, left_height, BP_JOIN_LEFT);
    }

    if (join.count == 1) {
        return bp_tree_new_with_root(join.nodes[0], result_height);
    }

    BpNode* root = bp_node_new(BP_NODE_INTERIOR);
    root->interior.child_nodes[0] = join.nodes[0];
    root->interior.child_nodes[1] = join.nodes[1];
    root->interior.child_sizes[0] = bp_node_total_length(join.nodes[0]);
    root->interior.child_sizes[1] = bp_node_total_length(join.nodes[1]);
    root->child_count = 2;

    // + 1 because of our new root node
    return bp_tree_new_with_root(root, result_height + 1);
}

BpTree* bp_tree_group_children(BpNode* node, int from, int to, int height)
{
    int n = to - from + 1;
    if (n <= 0) {
        return bp_tree_new_empty();
    } else if (n == 1) {
        return bp_tree_new_with_root(bp_node_ref(node->interior.child_nodes[from]), height - 1);
    }

    BpNode* root = bp_node_new(BP_NODE_INTERIOR);
    for (int i = from; i <= to; i++) {
        int root_count = root->child_count;
        root->interior.child_nodes[root_count] = bp_node_ref(node->interior.child_nodes[i]);
        root->interior.child_sizes[root_count] = node->interior.child_sizes[i];
        root->child_count++;
    }

    return bp_tree_new_with_root(root, height);
}

void bp_tree_split(BpTree* self, bp_len_t off, BpTree** out_left, BpTree** out_right)
{
    wtk_assert(off >= 0 && off <= bp_tree_length(self));

    bp_len_t length = bp_tree_length(self);
    if (off == 0) {
        *out_left = bp_tree_new_empty();
        *out_right = self;
        return;
    } else if (off >= length) {
        *out_left = self;
        *out_right = bp_tree_new_empty();
        return;
    }

    BpPath path;
    bp_len_t leaf_off;
    bp_tree_find(self, off, &path, &leaf_off);

    BpTree* left_tree;
    BpTree* right_tree;
    BpNode* leaf = path.nodes[path.node_count - 1];
    if (leaf_off == 0) {
        left_tree = bp_tree_new_empty();
        right_tree = bp_tree_new_with_root(bp_node_ref(leaf), 0);
    } else {
        BpNode* left_leaf;
        BpNode* right_leaf;
        bp_leaf_split(leaf, leaf_off, &left_leaf, &right_leaf);

        left_tree = bp_tree_new_with_root(left_leaf, 0);
        right_tree = bp_tree_new_with_root(right_leaf, 0);
    }

    // - 2 because we're skipping the leaf
    for (int i = path.node_count - 2; i >= 0; i--) {
        BpNode* node = path.nodes[i];
        int child_idx = path.node_indices[i + 1];
        int height = path.node_count - 1 - i;
        left_tree = bp_tree_join(bp_tree_group_children(node, 0, child_idx - 1, height), left_tree);
        right_tree = bp_tree_join(right_tree, bp_tree_group_children(node, child_idx + 1, node->child_count - 1, height));
    }

    bp_tree_free(self);
    *out_left = left_tree;
    *out_right = right_tree;
}

void bp_tree_collapse_root(BpTree* self)
{
    while (self->root->kind == BP_NODE_INTERIOR && self->root->child_count == 1) {
        BpNode* child = bp_node_ref(self->root->interior.child_nodes[0]);
        bp_node_unref(self->root);
        self->root = child;
        self->height--;
    }

    if (self->root->kind == BP_NODE_INTERIOR && self->root->child_count == 0) {
        bp_node_unref(self->root);
        self->root = bp_node_new(BP_NODE_LEAF);
        self->height = 0;
    }
}

void bp_tree_seam_coalesce(BpTree* self, bp_len_t off)
{
    BpPath path;
    bp_len_t leaf_off;
    bp_tree_find_for_write(self, off, &path, &leaf_off);

    BpNode* leaf = path.nodes[path.node_count - 1];
    bp_len_t piece_off = leaf_off;
    int piece_idx = bp_node_find_piece(leaf, &piece_off);

    if (piece_off != 0 || piece_idx == 0)
        return;

    BpPiece* piece_a = &leaf->leaf.pieces[piece_idx - 1];
    BpPiece* piece_b = &leaf->leaf.pieces[piece_idx];
    if (bp_piece_can_merge(piece_a, piece_b)) {
        bp_piece_merge(piece_a, piece_b);
        bp_node_delete_piece(leaf, piece_idx, false); // piece[piece_idx] disposed by merge
    }
}
