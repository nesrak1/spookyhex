#ifndef BP_NODE_H
#define BP_NODE_H

#include "wtk_common.h"
#include "bp_refcount.h"
#include "bp_defines.h"
#include "bp_piece.h"

typedef enum {
    BP_NODE_INTERIOR,
    BP_NODE_LEAF,
} BpNodeKind;

// ifdef in case we want to change it elsewhere (e.g., tests)
#ifndef BP_NODE_ORDER
    #define BP_NODE_ORDER 10
#endif
#define BP_MIN_FILL BP_NODE_ORDER / 2

typedef struct _BpNode {
    BpRcBase rc;
    int child_count;
    BpNodeKind kind;
    union {
        // + 1 for the "slack slot" in case of overflow (which is temporary)
        struct {
            struct _BpNode* child_nodes[BP_NODE_ORDER + 1];
            bp_len_t child_sizes[BP_NODE_ORDER + 1];
        } interior;
        struct {
            BpPiece pieces[BP_NODE_ORDER + 1];
        } leaf;
    };
} BpNode;

typedef struct {
    BpNode* nodes[2];
    int count;
} BpJoin;

typedef enum {
    BP_JOIN_LEFT,
    BP_JOIN_RIGHT
} BpJoinEdge;

BpNode* bp_node_new(BpNodeKind kind);
BpNode* bp_node_clone(const BpNode* self);

#define bp_node_ref(self) bp_ref_ref((self), BpNode)
void bp_node_unref(BpNode* self);

BpNode* bp_node_own(BpNode* self);

BpNode* bp_node_take_child(BpNode* self, int idx);
BpNode* bp_node_lock_child(BpNode* self, int idx);

void bp_node_delete_piece(BpNode* self, int idx, bool dispose);
void bp_node_delete_child(BpNode* self, int idx, bool dispose);
void bp_node_replace_taken_slot(BpNode* self, int slot, BpJoin join);

bp_len_t bp_node_total_length(BpNode* self);

int bp_node_find_child(BpNode* self, bp_len_t* local_off);
int bp_node_find_piece(BpNode* self, bp_len_t* local_off);

void bp_leaf_split(BpNode* self, bp_len_t local_off, BpNode** out_left, BpNode** out_right);

// joining

BpJoin bp_join_one_new(BpNode* a_node);
BpJoin bp_join_two_new(BpNode* a_node, BpNode* b_node);

BpJoin bp_join_equal(BpNode* a_node, BpNode* b_node, int height);
BpJoin bp_attach_edge(BpNode* tall_node, BpNode* short_node, int tall_height, int short_height, BpJoinEdge edge);

void bp_leaf_coalesce(BpNode* self);

#endif // BP_NODE_H
