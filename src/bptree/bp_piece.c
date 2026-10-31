#include "bp_piece.h"

BpPiece bp_piece_new(BpSource* source, bp_len_t offset, bp_len_t length)
{
    BpPiece piece;
    piece.source = bp_source_ref(source);
    piece.offset = offset;
    piece.length = length;

    return piece;
}

void bp_piece_dispose(BpPiece* self)
{
    if (self->source == NULL)
        return;

    bp_source_unref(self->source);
    self->source = NULL;
    self->offset = 0;
    self->length = 0;
}

BpPiece bp_piece_copy(const BpPiece* self)
{
    BpPiece new_piece;
    new_piece.source = bp_source_ref(self->source);
    new_piece.offset = self->offset;
    new_piece.length = self->length;

    return new_piece;
}

BpPiece bp_piece_split(BpPiece* self, bp_len_t offset)
{
    wtk_assert(offset > 0 && offset < self->length);

    BpPiece end_piece;
    end_piece.source = bp_source_ref(self->source);
    end_piece.offset = self->offset + offset;
    end_piece.length = self->length - offset;

    self->length = offset;

    return end_piece;
}

bool bp_piece_can_merge(const BpPiece* self, const BpPiece* other)
{
    return self->source == other->source
        && self->offset + self->length == other->offset;
}

void bp_piece_merge(BpPiece* self, BpPiece* other)
{
    self->length += other->length;
    bp_piece_dispose(other);
}
