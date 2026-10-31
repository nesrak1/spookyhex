#ifndef BP_PIECE_H
#define BP_PIECE_H

#include "bp_refcount.h"
#include "bp_source.h"

typedef struct {
    BpSource* source;
    bp_len_t offset;
    bp_len_t length;
} BpPiece;

BpPiece bp_piece_new(BpSource* source, bp_len_t offset, bp_len_t length);
void bp_piece_dispose(BpPiece* self);

BpPiece bp_piece_copy(const BpPiece* self);

BpPiece bp_piece_split(BpPiece* self, bp_len_t offset);
bool bp_piece_can_merge(const BpPiece* self, const BpPiece* other);
void bp_piece_merge(BpPiece* self, BpPiece* other);

#define bp_piece_ref(self) bp_ref_ref((self), BpPiece)
#define bp_piece_unref(self) bp_ref_unref((self), NULL)

#endif // BP_PIECE_H
