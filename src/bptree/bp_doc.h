#ifndef BP_DOC_H
#define BP_DOC_H

#include "bp_defines.h"
#include "bp_tree.h"
#include "bp_source.h"
#include "wtk_file.h"

// #define BP_CHUNK_SIZE 1024
#define BP_CHUNK_SIZE 256

typedef enum {
    // edited by typing keys while in insert mode
    BP_EDIT_KIND_INSERT,
    // edited by pressing the backspace key
    BP_EDIT_KIND_BACKSPACE,
    // edited by typing keys while in overwrite mode
    BP_EDIT_KIND_OVERWRITE,
    // edited by pressing the delete key
    //BP_EDIT_KIND_DELETE,
    // edited by removing bytes, either with backspace or
    // by selecting and pressing backspace/delete
    //BP_EDIT_KIND_REMOVE,
    // edited by selecting and cutting
    //BP_EDIT_KIND_CUT,
    // edited by pasting
    //BP_EDIT_KIND_PASTE,
    // todo: others
    BP_EDIT_KIND_OTHER
} BpEditKind;

typedef struct {
    BpTree* tree;
    bp_len_t start_pos;
    bp_len_t end_pos;
} BpHistoryEntry;

typedef struct {
    BpHistoryEntry* undos;
    BpHistoryEntry* redos;
    int undo_count;
    int redo_count;
    int capacity;

    bool edit_active;
    bp_len_t edit_pos;
    BpEditKind edit_kind;
} BpHistory;

typedef struct {
    BpPath path;
    int piece_idx;
    bp_len_t piece_off;
} BpCursor;

typedef struct {
    BpTree* working;
    BpSource* cur_chunk;
    BpHistory history;
} BpDoc;

// todo: should we reorder to length then data
// so data, when an out parameter, can move towards the end?

// edit kind

// todo: edit kind code

// history

void bp_history_initialize(BpHistory* self, int capacity);
void bp_history_set_capacity(BpHistory* self, int capacity);

void bp_history_clear(BpHistory* self);
void bp_history_clear_redos(BpHistory* self);
void bp_history_drop_undo(BpHistory* self);

void bp_history_begin_edit(BpHistory* self, bp_len_t pos, BpEditKind kind, BpTree* working);
void bp_history_complete_edit(BpHistory* self, bp_len_t next_pos, BpEditKind kind);
void bp_history_end_edit(BpHistory* self);

// cursor

bool bp_cursor_seek(BpCursor* self, const BpTree* tree, bp_len_t pos);
bp_len_t bp_cursor_read(BpCursor* self, bp_len_t length, uint8_t* out_data);

// doc

BpDoc* bp_doc_new_empty(int history_capacity);
BpDoc* bp_doc_new_from_file(WtkFile* file, int history_capacity);
void bp_doc_close(BpDoc* self);

bp_len_t bp_doc_undo(BpDoc* self);
bp_len_t bp_doc_redo(BpDoc* self);

BpPiece bp_doc_stage_bytes(BpDoc* self, const uint8_t* data, bp_len_t length);
bool bp_doc_fast_extend(BpDoc* self, bp_len_t pos, const uint8_t* data, bp_len_t length);
bool bp_doc_fast_shrink(BpDoc* self, bp_len_t pos, bp_len_t length);

bp_len_t bp_doc_length(BpDoc* self);
bp_len_t bp_doc_read(BpDoc* self, bp_len_t pos, bp_len_t length, uint8_t* out_data);

bool bp_doc_insert(BpDoc* self, bp_len_t pos, const uint8_t* data, bp_len_t length);
bool bp_doc_delete(BpDoc* self, bp_len_t pos, bp_len_t length);

#endif // BP_DOC_H