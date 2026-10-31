#include "bp_node.h"

#include <string.h>

#include "wtk_mem.h"

BpNode* bp_node_new(BpNodeKind kind)
{
    wtk_assert(kind == BP_NODE_INTERIOR || kind == BP_NODE_LEAF);

    BpNode* node = wtk_malloc(BpNode*, sizeof(BpNode));
    bp_ref_init(node);
    node->child_count = 0;
    node->kind = kind;

    // not really necessary but good for debugging
    if (node->kind == BP_NODE_INTERIOR) {
        for (int i = 0; i < BP_NODE_ORDER + 1; i++) {
            node->interior.child_nodes[i] = NULL;
            node->interior.child_sizes[i] = 0;
        }
    } else if (node->kind == BP_NODE_LEAF) {
        for (int i = 0; i < BP_NODE_ORDER + 1; i++) {
            BpPiece* piece = &node->leaf.pieces[i];
            piece->source = NULL;
            piece->offset = 0;
            piece->length = 0;
        }
    }

    return node;
}

BpNode* bp_node_clone(const BpNode* self)
{
    BpNode* node = wtk_malloc(BpNode*, sizeof(BpNode));
    bp_ref_init(node);
    node->child_count = self->child_count;
    node->kind = self->kind;

    if (self->kind == BP_NODE_INTERIOR) {
        for (int i = 0; i < self->child_count; i++) {
            BpNode* child_node = self->interior.child_nodes[i];
            node->interior.child_nodes[i] = bp_node_ref(child_node);
            node->interior.child_sizes[i] = self->interior.child_sizes[i];
        }
    } else if (self->kind == BP_NODE_LEAF) {
        for (int i = 0; i < self->child_count; i++) {
            node->leaf.pieces[i] = bp_piece_copy(&self->leaf.pieces[i]);
        }
    }

    return node;
}

void bp_node_destroy(void* object)
{
    BpNode* self = (BpNode*)object;

    if (self->kind == BP_NODE_INTERIOR) {
        for (int i = 0; i < self->child_count; i++) {
            bp_node_unref(self->interior.child_nodes[i]);
        }
    } else if (self->kind == BP_NODE_LEAF) {
        for (int i = 0; i < self->child_count; i++) {
            bp_piece_dispose(&self->leaf.pieces[i]);
        }
    }

    free(self);
}

void bp_node_unref(BpNode* self)
{
    if (self != NULL) {
        bp_ref_unref(self, bp_node_destroy);
    }
}

BpNode* bp_node_own(BpNode* self)
{
    if (self->rc.refcount == 1) {
        return self;
    }

    BpNode* clone = bp_node_clone(self);
    bp_node_unref(self);

    return clone;
}

BpNode* bp_node_take_child(BpNode* self, int idx)
{
    wtk_assert(self->rc.refcount == 1 && self->kind == BP_NODE_INTERIOR);

    BpNode* child = self->interior.child_nodes[idx];
    self->interior.child_nodes[idx] = NULL;
    self->interior.child_sizes[idx] = 0;

    return child;
}

BpNode* bp_node_lock_child(BpNode* self, int idx)
{
    wtk_assert(self->rc.refcount == 1 && self->kind == BP_NODE_INTERIOR);

    BpNode* locked_child = bp_node_own(self->interior.child_nodes[idx]);
    self->interior.child_nodes[idx] = locked_child;

    return locked_child;
}

void bp_node_move_entries(BpNode* dst, int dst_at, const BpNode* src, int src_at, int n)
{
    wtk_assert(n >= 0);
    wtk_assert(dst->kind == src->kind);
    wtk_assert(dst_at >= 0 && src_at >= 0);
    wtk_assert(dst_at + n <= BP_NODE_ORDER + 1);
    wtk_assert(src_at + n <= BP_NODE_ORDER + 1);

    if (n == 0)
        return;

    if (dst->kind == BP_NODE_INTERIOR) {
        memmove(&dst->interior.child_nodes[dst_at], &src->interior.child_nodes[src_at], n * sizeof(BpNode*));
        memmove(&dst->interior.child_sizes[dst_at], &src->interior.child_sizes[src_at], n * sizeof(bp_len_t));
    } else {
        memmove(&dst->leaf.pieces[dst_at], &src->leaf.pieces[src_at], n * sizeof(BpPiece));
    }
}

// todo: this dispose flag is kinda yucky.
// should the caller dispose or delete_piece always dispose?
void bp_node_delete_piece(BpNode* self, int idx, bool dispose)
{
    wtk_assert(self->kind == BP_NODE_LEAF);
    wtk_assert(idx >= 0 && idx < self->child_count);

    if (dispose)
        bp_piece_dispose(&self->leaf.pieces[idx]);

    for (int i = idx; i < self->child_count - 1; i++) {
        self->leaf.pieces[i] = self->leaf.pieces[i + 1];
    }

    self->child_count--;
}

// todo: ditto
void bp_node_delete_child(BpNode* self, int idx, bool dispose)
{
    wtk_assert(self->kind == BP_NODE_INTERIOR);
    wtk_assert(idx >= 0 && idx < self->child_count);

    if (dispose)
        bp_node_unref(self->interior.child_nodes[idx]);

    for (int i = idx; i < self->child_count - 1; i++) {
        self->interior.child_nodes[i] = self->interior.child_nodes[i + 1];
        self->interior.child_sizes[i] = self->interior.child_sizes[i + 1];
    }

    self->child_count--;
}

void bp_node_replace_taken_slot(BpNode* self, int slot, BpJoin join)
{
    int count = join.count;
    wtk_assert(count == 1 || count == 2);
    wtk_assert(self->kind == BP_NODE_INTERIOR);
    wtk_assert(slot >= 0 && slot < self->child_count);

    if (count == 2) {
        for (int i = self->child_count - 1; i > slot; i--) {
            self->interior.child_nodes[i + 1] = self->interior.child_nodes[i];
            self->interior.child_sizes[i + 1] = self->interior.child_sizes[i];
        }

        self->child_count++;
    }

    for (int j = 0; j < count; j++) {
        self->interior.child_nodes[slot + j] = join.nodes[j];
        self->interior.child_sizes[slot + j] = bp_node_total_length(join.nodes[j]);
    }
}

bp_len_t bp_node_total_length(BpNode* self)
{
    bp_len_t total = 0;

    if (self->kind == BP_NODE_INTERIOR) {
        for (int i = 0; i < self->child_count; i++) {
            total += self->interior.child_sizes[i];
        }
    } else if (self->kind == BP_NODE_LEAF) {
        for (int i = 0; i < self->child_count; i++) {
            total += self->leaf.pieces[i].length;
        }
    }

    return total;
}

int bp_node_find_child(BpNode* self, bp_len_t* local_off)
{
    wtk_assert(self->kind == BP_NODE_INTERIOR);
    bp_len_t local_off_val = *local_off;
    bp_len_t* child_sizes = self->interior.child_sizes;

    int i;
    for (i = 0; i < self->child_count; i++) {
        if (local_off_val < child_sizes[i])
            break;

        if (i != self->child_count - 1)
            local_off_val -= child_sizes[i];
    }

    *local_off = local_off_val;
    return i;
}

int bp_node_find_piece(BpNode* self, bp_len_t* local_off)
{
    wtk_assert(self->kind == BP_NODE_LEAF);
    bp_len_t local_off_val = *local_off;
    BpPiece* pieces = self->leaf.pieces;

    int i;
    for (i = 0; i < self->child_count; i++) {
        if (local_off_val < pieces[i].length) {
            *local_off = local_off_val;
            return i;
        }

        if (i != self->child_count - 1)
            local_off_val -= pieces[i].length;
    }

    *local_off = 0;
    return self->child_count;
}

// splitting

void bp_leaf_split(BpNode* self, bp_len_t local_off, BpNode** out_left, BpNode** out_right)
{
    bp_len_t piece_off = local_off;
    int split_idx = bp_node_find_piece(self, &piece_off);

    BpNode* left = bp_node_new(BP_NODE_LEAF);
    BpNode* right = bp_node_new(BP_NODE_LEAF);

    int src_idx = 0;

    for (; src_idx < split_idx; src_idx++) {
        left->leaf.pieces[left->child_count++] = bp_piece_copy(&self->leaf.pieces[src_idx]);
    }

    if (piece_off > 0 && split_idx < self->child_count) {
        BpPiece* head = &left->leaf.pieces[left->child_count++];
        BpPiece* tail = &right->leaf.pieces[right->child_count++];
        *head = bp_piece_copy(&self->leaf.pieces[src_idx++]);
        *tail = bp_piece_split(head, piece_off);
    }

    for (; src_idx < self->child_count; src_idx++) {
        right->leaf.pieces[right->child_count++] = bp_piece_copy(&self->leaf.pieces[src_idx]);
    }

    *out_left = left;
    *out_right = right;
}

// joining

BpJoin bp_join_one_new(BpNode* a_node)
{
    BpJoin join;
    join.nodes[0] = a_node;
    join.nodes[1] = NULL;
    join.count = 1;
    return join;
}

BpJoin bp_join_two_new(BpNode* a_node, BpNode* b_node)
{
    BpJoin join;
    join.nodes[0] = a_node;
    join.nodes[1] = b_node;
    join.count = 2;
    return join;
}

BpJoin bp_node_rebalance(BpNode* a_node, BpNode* b_node)
{
    int a_count = a_node->child_count;
    int b_count = b_node->child_count;
    bool coalescible = a_node->kind == BP_NODE_LEAF
        && a_count > 0 && b_count > 0
        && bp_piece_can_merge(&a_node->leaf.pieces[a_count - 1], &b_node->leaf.pieces[0]);

    if (!coalescible && a_count >= BP_MIN_FILL && b_count >= BP_MIN_FILL) {
        return bp_join_two_new(a_node, b_node);
    }

    a_node = bp_node_own(a_node);
    b_node = bp_node_own(b_node);

    if (coalescible) {
        bp_piece_merge(&a_node->leaf.pieces[a_count - 1], &b_node->leaf.pieces[0]);
        for (int i = 1; i < b_count; i++) {
            b_node->leaf.pieces[i - 1] = b_node->leaf.pieces[i];
        }

        b_node->child_count--;
        b_count--;
    }

    if (b_count == 0) {
        bp_node_unref(b_node);
        return bp_join_one_new(a_node);
    }

    if (a_count + b_count <= BP_NODE_ORDER) {
        bp_node_move_entries(a_node, a_count, b_node, 0, b_count);
        a_node->child_count = a_count + b_count;
        b_node->child_count = 0;

        bp_node_unref(b_node);
        return bp_join_one_new(a_node);
    }

    if (a_count >= BP_MIN_FILL && b_count >= BP_MIN_FILL) {
        return bp_join_two_new(a_node, b_node);
    }

    int total = a_count + b_count;
    int new_a_count = total / 2;
    if (new_a_count > a_count) {
        int n = new_a_count - a_count;
        bp_node_move_entries(a_node, a_count, b_node, 0, n);
        bp_node_move_entries(b_node, 0, b_node, n, b_count - n);
    } else {
        int n = a_count - new_a_count;
        bp_node_move_entries(b_node, n, b_node, 0, b_count);
        bp_node_move_entries(b_node, 0, a_node, new_a_count, n);
    }

    a_node->child_count = new_a_count;
    b_node->child_count = total - new_a_count;

    return bp_join_two_new(a_node, b_node);
}

BpJoin bp_join_equal(BpNode* a_node, BpNode* b_node, int height)
{
    wtk_assert(a_node != NULL && b_node != NULL && height >= 0);

    if (height == 0) {
        return bp_node_rebalance(a_node, b_node);
    }

    if (a_node->child_count + b_node->child_count <= BP_NODE_ORDER) {
        a_node = bp_node_own(a_node);
        b_node = bp_node_own(b_node);

        int a_count = a_node->child_count;
        int b_count = b_node->child_count;

        if (a_node->kind == BP_NODE_INTERIOR) {
            wtk_assert(b_node->kind == BP_NODE_INTERIOR);

            for (int i = 0; i < b_count; i++) {
                a_node->interior.child_nodes[a_count + i] = b_node->interior.child_nodes[i];
                a_node->interior.child_sizes[a_count + i] = b_node->interior.child_sizes[i];
            }
        } else if (a_node->kind == BP_NODE_LEAF) {
            wtk_assert(b_node->kind == BP_NODE_LEAF);

            for (int i = 0; i < b_count; i++) {
                a_node->leaf.pieces[a_count + i] = b_node->leaf.pieces[i];
            }
        }

        a_node->child_count = a_count + b_count;

        b_node->child_count = 0;
        bp_node_unref(b_node);

        return bp_join_one_new(a_node);
    }

    return bp_join_two_new(a_node, b_node);
}

BpJoin split_if_overfull(BpNode* node)
{
    if (node->child_count <= BP_NODE_ORDER) {
        return bp_join_one_new(node);
    }

    BpNode* new_node = bp_node_new(node->kind);
    int old_count = node->child_count;
    int k = (old_count + 1) / 2;
    bp_node_move_entries(new_node, 0, node, k, old_count - k);
    new_node->child_count = old_count - k;
    node->child_count = k;

    return bp_join_two_new(node, new_node);
}

BpJoin bp_attach_edge(BpNode* tall_node, BpNode* short_node, int tall_height, int short_height, BpJoinEdge edge)
{
    tall_node = bp_node_own(tall_node);

    int slot = (edge == BP_JOIN_RIGHT) ? tall_node->child_count - 1 : 0;
    BpNode* edge_child = bp_node_take_child(tall_node, slot);

    BpJoin out_join;
    if (tall_height == short_height + 1) {
        out_join = (edge == BP_JOIN_RIGHT)
            ? bp_node_rebalance(edge_child, short_node)
            : bp_node_rebalance(short_node, edge_child);
    } else {
        out_join = bp_attach_edge(edge_child, short_node, tall_height - 1, short_height, edge);
    }

    bp_node_replace_taken_slot(tall_node, slot, out_join);
    return split_if_overfull(tall_node);
}

void bp_leaf_coalesce(BpNode* self)
{
    int i = 0;
    while (i + 1 < self->child_count) {
        BpPiece* piece_a = &self->leaf.pieces[i];
        BpPiece* piece_b = &self->leaf.pieces[i + 1];
        if (bp_piece_can_merge(piece_a, piece_b)) {
            bp_piece_merge(piece_a, piece_b);
            bp_node_delete_piece(self, i + 1, false); // piece[i + 1] disposed by merge
        } else {
            i++;
        }
    }
}