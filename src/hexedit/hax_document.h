#ifndef HAX_DOCUMENT_H
#define HAX_DOCUMENT_H

#include "wtk_container.h"
#include "wtk_str.h"

#include "bp_doc.h"

#define HEXEDIT_DEFAULT_BYTES_PER_ROW 16
#define HEXEDIT_DEFAULT_GROUP_SIZE 1
// #define HEXEDIT_DEFAULT_HISTORY_SIZE 64
#define HEXEDIT_DEFAULT_HISTORY_SIZE 2048

typedef enum {
    // |00 or |A
    HEXEDIT_CURSOR_LEFT,
    // 0|0 (not applicable to chars)
    HEXEDIT_CURSOR_NIBBLE,
    // 00| or A| (only when selecting)
    HEXEDIT_CURSOR_RIGHT,
    // .. .. .. 00| or A|
    // 00 <-- cursor is technically on this byte
    HEXEDIT_CURSOR_RIGHT_PREV
} HexEditCursorPosition;

typedef enum {
    HEXEDIT_PANE_HEX,
    HEXEDIT_PANE_CHARS
} HexEditPane;

typedef enum {
    HEXEDIT_OFFBASE_HEX,
    HEXEDIT_OFFBASE_DEC,
    HEXEDIT_OFFBASE_OCT
} HexEditOffsetBase;

typedef struct {
    HexEditCursorPosition start_cur;
    HexEditCursorPosition end_cur;
    int64_t start_off;
    int64_t end_off;

    HexEditPane active_pane;

    bool selecting;
    bool dragging;
} HexEditCursorInfo;

typedef struct {
    char* file_path;
    char* file_name;
    BpDoc* tree_doc;

    HexEditCursorInfo cursor;
    // the scroll byte offset (both x and y are single byte offsets)
    // the start address will always be scroll_x + scroll_y
    // todo: currently only scroll_y is supported
    bp_len_t scroll_x;
    bp_len_t scroll_y;

    int32_t scroll_x_frac;
    int32_t scroll_y_frac;

    HexEditOffsetBase offset_base;
    int bytes_per_row;
    int group_size;
} HexEditDocument;

HexEditDocument* hexedit_doc_new_empty();
HexEditDocument* hexedit_doc_load_file(const char* path);

#endif // HAX_DOCUMENT_H