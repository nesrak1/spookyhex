#include "hax_document.h"
#include "wtk_file.h"

HexEditDocument* hexedit_doc_new(BpDoc* bp_doc)
{
    HexEditDocument* hex_doc = wtk_malloc(HexEditDocument*, sizeof(HexEditDocument));
    hex_doc->file_path = "";
    hex_doc->file_name = "";
    hex_doc->tree_doc = bp_doc;
    hex_doc->cursor.start_cur = HEXEDIT_CURSOR_LEFT;
    hex_doc->cursor.end_cur = HEXEDIT_CURSOR_LEFT;
    hex_doc->cursor.start_off = 0;
    hex_doc->cursor.end_off = 0;
    hex_doc->cursor.active_pane = HEXEDIT_PANE_HEX;
    hex_doc->cursor.selecting = false;
    hex_doc->cursor.dragging = false;
    hex_doc->scroll_x = 0;
    hex_doc->scroll_y = 0;
    hex_doc->scroll_x_frac = 0;
    hex_doc->scroll_y_frac = 0;
    hex_doc->offset_base = HEXEDIT_OFFBASE_HEX;
    hex_doc->bytes_per_row = HEXEDIT_DEFAULT_BYTES_PER_ROW;
    hex_doc->group_size = HEXEDIT_DEFAULT_GROUP_SIZE;
    return hex_doc;
}

HexEditDocument* hexedit_doc_new_empty()
{
    BpDoc* bp_doc = bp_doc_new_empty(HEXEDIT_DEFAULT_HISTORY_SIZE);

    HexEditDocument* hex_doc = hexedit_doc_new(bp_doc);
    return hex_doc;
}

HexEditDocument* hexedit_doc_load_file(const char* path)
{
    wtk_assert(path != NULL);

    // todo: eventually support READWRITE sharing but with file watching so we know when to bail
    WtkFile* file = wtk_file_open(path, WTK_FILE_MODE_OPEN, WTK_FILE_ACCESS_READ, WTK_FILE_SHARE_READ);
    if (file == NULL)
        return NULL;

    BpDoc* bp_doc = bp_doc_new_from_file(file, HEXEDIT_DEFAULT_HISTORY_SIZE);

    HexEditDocument* hex_doc = hexedit_doc_new(bp_doc);
    hex_doc->file_path = wtk_strdup(path);
    hex_doc->file_name = wtk_file_path_filename(path);
    return hex_doc;
}
