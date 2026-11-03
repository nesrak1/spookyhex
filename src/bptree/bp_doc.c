#include "bp_doc.h"

#include <stdlib.h>

#include "wtk_mem.h"

// history

void bp_history_initialize(BpHistory* self, int capacity)
{
    self->undos = wtk_malloc(BpHistoryEntry*, sizeof(BpHistoryEntry) * capacity);
    self->redos = wtk_malloc(BpHistoryEntry*, sizeof(BpHistoryEntry) * capacity);
    self->undo_count = 0;
    self->redo_count = 0;
    self->capacity = capacity;
    self->edit_active = false;
    self->edit_pos = 0;
    self->edit_kind = BP_EDIT_KIND_INSERT;
}

void bp_history_set_capacity(BpHistory* self, int capacity)
{
    bp_history_clear(self);
    self->capacity = capacity;
    free(self->undos);
    free(self->redos);

    self->undos = wtk_malloc(BpHistoryEntry*, sizeof(BpHistoryEntry) * capacity);
    self->redos = wtk_malloc(BpHistoryEntry*, sizeof(BpHistoryEntry) * capacity);
    self->undo_count = 0;
    self->redo_count = 0;
}

void bp_history_clear(BpHistory* self)
{
    while (self->undo_count > 0) {
        bp_tree_free(self->undos[--self->undo_count].tree);
    }
    bp_history_clear_redos(self);
    self->edit_active = false;
}

void bp_history_clear_redos(BpHistory* self)
{
    while (self->redo_count > 0) {
        bp_tree_free(self->redos[--self->redo_count].tree);
    }
}

void bp_history_drop_undo(BpHistory* self)
{
    if (self->undo_count == 0)
        return;

    bp_tree_free(self->undos[0].tree);
    for (int i = 1; i < self->undo_count; i++) {
        self->undos[i - 1] = self->undos[i];
    }

    self->undo_count--;
}

// todo: history start/end_pos only setup for insert. backspace will be broken.
void bp_history_begin_edit(BpHistory* self, bp_len_t pos, BpEditKind kind, BpTree* working)
{
    // if we don't have history, don't bother keeping track of anything
    if (self->capacity == 0)
        return;
    
    // if we're continuing an edit, do nothing
    if (self->edit_active && self->edit_pos == pos && self->edit_kind == kind)
        return;

    bp_history_clear_redos(self);
    if (self->undo_count == self->capacity)
        bp_history_drop_undo(self);

    self->edit_active = true;
    self->edit_pos = pos;
    self->edit_kind = kind;

    BpHistoryEntry* hist_entry = &self->undos[self->undo_count++];
    hist_entry->tree = bp_tree_snapshot(working);
    hist_entry->start_pos = pos;
    hist_entry->end_pos = pos;
}

void bp_history_complete_edit(BpHistory* self, bp_len_t next_pos, BpEditKind kind)
{
    if (self->capacity == 0 || !self->edit_active)
        return;

    self->edit_pos = next_pos;
    self->edit_kind = kind;

    // we should always be > 0 if we run this after begin_edit, but just in case...
    if (self->undo_count > 0)
        self->undos[self->undo_count - 1].end_pos = next_pos;
}

void bp_history_end_edit(BpHistory* self)
{
    self->edit_active = false;
}

// cursor

bool bp_cursor_seek(BpCursor* self, const BpTree* tree, bp_len_t pos)
{
    // todo: check
    if (pos > bp_tree_length(tree))
        return false;

    bp_len_t leaf_off;
    bp_tree_find(tree, pos, &self->path, &leaf_off);

    BpNode* leaf = self->path.nodes[self->path.node_count - 1];
    bp_len_t piece_off = leaf_off;
    self->piece_idx = bp_node_find_piece(leaf, &piece_off);
    self->piece_off = piece_off;

    return true;
}

bp_len_t bp_cursor_read(BpCursor* self, bp_len_t length, uint8_t* out_data)
{
    // todo: check
    bp_len_t read_bytes = 0;
    while (read_bytes < length) {
        BpNode* leaf = self->path.nodes[self->path.node_count - 1];
        if (self->piece_idx >= leaf->child_count) {
            if (bp_path_next_leaf(&self->path) == NULL)
                break;

            self->piece_idx = 0;
            self->piece_off = 0;
            continue;
        }

        BpPiece piece = leaf->leaf.pieces[self->piece_idx];
        bp_len_t read_len = (bp_len_t)wtk_min(length - read_bytes, piece.length - self->piece_off);
        bp_len_t read_bytes_src = bp_source_read(piece.source, out_data, piece.offset + self->piece_off, read_len);
        if (read_bytes_src == -1)
            return read_bytes;
        
        // shouldn't happen (should be covered by self->piece_idx >= leaf->child_count) above
        if (read_bytes_src == 0)
            return read_bytes;

        read_bytes += read_bytes_src;
        out_data += read_bytes_src;
        self->piece_off += read_bytes_src;

        if (self->piece_off >= piece.length) {
            self->piece_idx++;
            self->piece_off = 0;
        }
    }

    return read_bytes;
}

// doc

BpDoc* bp_doc_new_empty(int history_capacity)
{
    BpDoc* doc = wtk_malloc(BpDoc*, sizeof(BpDoc));
    doc->working = bp_tree_new_empty();
    doc->cur_chunk = bp_source_new_as_buffer(BP_CHUNK_SIZE);
    bp_history_initialize(&doc->history, history_capacity);

    return doc;
}

BpDoc* bp_doc_new_from_file(WtkFile* file, int history_capacity)
{
    BpSource* source = bp_source_new_as_file(file);
    bp_len_t size = wtk_file_length(file);

    BpDoc* doc = wtk_malloc(BpDoc*, sizeof(BpDoc));
    doc->working = size > 0
        ? bp_tree_new_with_piece(bp_piece_new(source, 0, size))
        : bp_tree_new_empty();

    bp_source_unref(source);

    doc->cur_chunk = bp_source_new_as_buffer(BP_CHUNK_SIZE);
    bp_history_initialize(&doc->history, history_capacity);

    return doc;
}

void bp_doc_close(BpDoc* self)
{
    bp_history_clear(&self->history);
    free(self->history.undos);
    free(self->history.redos);
    if (self->cur_chunk)
        bp_source_unref(self->cur_chunk);
    if (self->working)
        bp_tree_free(self->working);

    free(self);
}

bp_len_t bp_doc_undo(BpDoc* self)
{
    if (self->history.undo_count == 0)
        return -1;

    bp_history_end_edit(&self->history);

    BpHistoryEntry* redo = &self->history.redos[self->history.redo_count++];
    BpHistoryEntry* undo = &self->history.undos[--self->history.undo_count];
    
    redo->tree = self->working;
    redo->start_pos = undo->start_pos;
    redo->end_pos = undo->end_pos;

    self->working = undo->tree;
    // we do not allow the user to continue to edit we just undid to,
    // so no need to update self->history.edit_pos/edit_start_pos.
    return undo->start_pos;
}

bp_len_t bp_doc_redo(BpDoc* self)
{
    if (self->history.redo_count == 0)
        return -1;

    bp_history_end_edit(&self->history);

    BpHistoryEntry* redo = &self->history.redos[--self->history.redo_count];
    BpHistoryEntry* undo = &self->history.undos[self->history.undo_count++];

    undo->tree = self->working;
    undo->start_pos = redo->start_pos;
    undo->end_pos = redo->end_pos;

    self->working = redo->tree;
    // ditto
    return redo->end_pos;
}

BpPiece bp_doc_stage_bytes(BpDoc* self, const uint8_t* data, bp_len_t length)
{
    if (length > BP_CHUNK_SIZE) {
        // our data is larger than chunk (probably a large new paste)
        // paste data into dedicated chunk (we won't add to it)
        BpSource* chunk = bp_source_new_as_buffer(length);
        bp_len_t offset = bp_source_append(chunk, data, length);
        BpPiece piece = bp_piece_new(chunk, offset, length);
        bp_source_unref(chunk);

        return piece;
    }

    if (bp_source_length(self->cur_chunk) + length > bp_source_capacity(self->cur_chunk)) {
        // not enough space in this chunk, make a new one and put it in there
        bp_source_unref(self->cur_chunk);
        self->cur_chunk = bp_source_new_as_buffer(BP_CHUNK_SIZE);
    }

    bp_len_t offset = bp_source_append(self->cur_chunk, data, length);
    BpPiece piece = bp_piece_new(self->cur_chunk, offset, length);
    return piece;
}

bool bp_doc_fast_extend(BpDoc* self, bp_len_t pos, const uint8_t* data, bp_len_t length)
{
    wtk_assert(self->working != NULL && self->cur_chunk != NULL);

    if (pos == 0)
        return false;
    if (bp_source_length(self->cur_chunk) + length > bp_source_capacity(self->cur_chunk))
        return false;

    BpPath path;
    bp_len_t leaf_off;
    // read one byte to the left of pos because we're
    // checking for the last byte of the current chunk
    bp_tree_find(self->working, pos - 1, &path, &leaf_off);

    BpNode* leaf = path.nodes[path.node_count - 1];
    bp_len_t piece_off = leaf_off;
    int piece_idx = bp_node_find_piece(leaf, &piece_off);
    BpPiece* piece = &leaf->leaf.pieces[piece_idx];

    if (piece_off != piece->length - 1)
        return false;
    if (piece->source != self->cur_chunk || piece->offset + piece->length != bp_source_length(self->cur_chunk))
        return false;

    bp_tree_find_for_write(self->working, pos - 1, &path, &leaf_off);

    leaf = path.nodes[path.node_count - 1];
    piece = &leaf->leaf.pieces[piece_idx];
    bp_source_append(self->cur_chunk, data, length);

    piece->length += length;
    bp_path_fix_with_delta(&path, length);

    return true;
}

bool bp_doc_fast_shrink(BpDoc* self, bp_len_t pos, bp_len_t length)
{
    wtk_assert(self->working != NULL);

    BpPath path;
    bp_len_t leaf_off;
    bp_tree_find(self->working, pos, &path, &leaf_off);

    BpNode* leaf = path.nodes[path.node_count - 1];
    bp_len_t piece_off = leaf_off;
    int piece_idx = bp_node_find_piece(leaf, &piece_off);
    BpPiece* piece = &leaf->leaf.pieces[piece_idx];

    if (piece_idx >= leaf->child_count)
        return false;
    if (piece_off + length > piece->length)
        return false;
    if (piece_off != 0 && piece_off + length != piece->length)
        return false;

    bp_tree_find_for_write(self->working, pos, &path, &leaf_off);

    leaf = path.nodes[path.node_count - 1];
    piece = &leaf->leaf.pieces[piece_idx];

    if (piece_off == 0)
        piece->offset += length;

    piece->length -= length;
    if (piece->length == 0) {
        bp_node_delete_piece(leaf, piece_idx, true);
        bp_path_prune_empty(&path, self->working);
    } else {
        bp_path_fix_with_delta(&path, -length);
    }

    return true;
}

bp_len_t bp_doc_length(BpDoc* self)
{
    BpTree* tree = self->working;
    if (tree == NULL)
        return 0;

    return bp_tree_length(tree);
}

bp_len_t bp_doc_read(BpDoc* self, bp_len_t pos, bp_len_t length, uint8_t* out_data)
{
    BpTree* tree = self->working;
    if (tree == NULL)
        return 0;

    BpCursor cursor;
    if (!bp_cursor_seek(&cursor, self->working, pos))
        return 0;

    return bp_cursor_read(&cursor, length, out_data);
}

void bp_doc_insert_impl(BpDoc* self, bp_len_t pos, const uint8_t* data, bp_len_t length)
{
    BpPiece piece = bp_doc_stage_bytes(self, data, length);

    BpTree* left;
    BpTree* middle = bp_tree_new_with_piece(piece);
    BpTree* right;
    bp_tree_split(self->working, pos, &left, &right);

    self->working = bp_tree_join(left, bp_tree_join(middle, right));
}

void bp_doc_delete_impl(BpDoc* self, bp_len_t pos, bp_len_t length)
{
    BpTree* left;
    BpTree* mid_right;
    BpTree* middle;
    BpTree* right;

    bp_tree_split(self->working, pos, &left, &mid_right);
    bp_tree_split(mid_right, length, &middle, &right);
    bp_tree_free(middle);
    // self->working, mid_right and middle are freed at this point
    self->working = bp_tree_join(left, right);
}

bool bp_doc_insert(BpDoc* self, bp_len_t pos, const uint8_t* data, bp_len_t length)
{
    if (length == 0)
        return true;
    if (pos < 0 || pos > bp_doc_length(self))
        return false;
    
    bp_history_begin_edit(&self->history, pos, BP_EDIT_KIND_INSERT, self->working);
    {
        // try fast insert
        if (!bp_doc_fast_extend(self, pos, data, length)) {
            // nope, fallback to slow
            bp_doc_insert_impl(self, pos, data, length);
        }
    }
    bp_history_complete_edit(&self->history, pos + length, BP_EDIT_KIND_INSERT);
    return true;
}

bool bp_doc_delete(BpDoc* self, bp_len_t pos, bp_len_t length)
{
    if (length == 0)
        return true;
    if (pos < 0 || pos > bp_doc_length(self))
        return false;
    
    bp_history_begin_edit(&self->history, pos + length, BP_EDIT_KIND_BACKSPACE, self->working);
    {
        // try fast shrink
        if (!bp_doc_fast_shrink(self, pos, length)) {
            // nope, fallback to slow
            bp_doc_delete_impl(self, pos, length);
        }
    }
    bp_history_complete_edit(&self->history, pos, BP_EDIT_KIND_BACKSPACE);
    return true;
}