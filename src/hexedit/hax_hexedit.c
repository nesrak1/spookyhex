
#include "hax_hexedit.h"

#include <stdio.h>
#include <inttypes.h>

#include "wtk_container.h"
#include "wtk_wgtk.h"
#include "bp_defines.h"

wtk_register_custom(HexEditControl, hexedit)

// constants

const int SCROLL_ROWS_PER_NOTCH = 3;
const int SCROLL_PAGE_SIZE = 10;

// logic

static int hexedit_addr_digits(bp_len_t size)
{
    int digits = 8; // todo: customizable?
    if (size <= 0)
        return digits;

    bp_len_t max_addr = size - 1;
    while (digits < 16 && (max_addr >> (digits * 4)) != 0) {
        digits++;
    }

    return digits;
}

void hexedit_draw_recalc(HexEditControl* self, WtkDrawingContext* ctx)
{
    int cw, ch;
    wtk_draw_measure_text(ctx, "0", &cw, &ch);

    // shouldn't happen
    if (cw == 0)
        cw = HEXEDIT_DEFAULT_CHAR_WIDTH;
    if (ch == 0)
        ch = HEXEDIT_DEFAULT_CHAR_HEIGHT;

    self->char_width = cw;
    self->char_height = ch;

    self->needs_draw_recalc = false;
}

static size_t hexedit_row_count(HexEditControl* self)
{
    if (self->active_doc == NULL)
        return 1;

    int bpr = self->active_doc->bytes_per_row;
    if (bpr <= 0)
        return 1;
    
    size_t total = bp_doc_length(self->active_doc->tree_doc);
    size_t rows = (total + bpr - 1) / bpr;
    return rows > 0 ? rows : 1;
}

// events

#define MAX_ROW_BYTES 256

void hexedit_paint(void* control, void* param)
{
    HexEditControl* self = (HexEditControl*)control;
    WtkDrawingContext* ctx = (WtkDrawingContext*)param;

    wtk_draw_set_font(ctx, "Courier New", 12, 0);
    wtk_draw_set_sys_color(ctx, WTK_SYS_COLOR_WINDOW_BACKGROUND);
    wtk_draw_rectangle(ctx, 0, 0, self->b.w, self->b.h);

    if (self->active_doc == NULL)
        return;

    if (self->needs_draw_recalc)
        hexedit_draw_recalc(self, ctx);

    BpDoc* bp_doc = self->active_doc->tree_doc;

    // how many bytes per row?
    int bytes_per_row = self->active_doc->bytes_per_row;

    // how many bytes in this document?
    bp_len_t total_length = bp_doc_length(bp_doc);

    wtk_set_scrollbar_range(&self->b, WTK_SCROLLDIR_VERTICAL, 0, hexedit_row_count(self) - 1, 1);

    // a single character width/height
    // we're assuming all characters are the same width/height (based on the font's "0")
    int char_width = self->char_width;
    int char_height = self->char_height;

    // how tall is a single row of bytes?
    int row_height = char_height + (self->char_height / 4);
    // how many rows fit on the screen
    int vis_row_count = (self->b.h / row_height) + 1;
    // the start offset into the file based on the screen scroll position
    bp_len_t row_byte_off = self->active_doc->scroll_y;

    // how many digits for the address string?
    int address_digits = hexedit_addr_digits(total_length);

    // x offsets for each column
    int address_xoff = 2 * char_width;
    int hex_xoff = address_xoff + (address_digits + 2) * char_width;
    int text_xoff = hex_xoff + (bytes_per_row * 3 + 1) * char_width;

    // starting y offset
    int header_yoff = char_height / 2;
    int row_start_yoff = 2 * char_height;

    // buffer holding row bytes
    // todo: this should be dynamically allocated when bytes_per_row changes
    uint8_t row_bytes[MAX_ROW_BYTES];

    char addr_chars[20 + 1];
    char hex_chars[MAX_ROW_BYTES * 3 + 1];
    char text_chars[MAX_ROW_BYTES + 1];

    wtk_draw_set_sys_color(ctx, WTK_SYS_COLOR_TEXT_COLOR);

    // draw the header
    // todo: precalculate this
    int header_pos = 0;
    for (int i = 0; i < bytes_per_row; i++) {
        header_pos += snprintf(hex_chars + header_pos, sizeof(hex_chars) - header_pos, "%02X ", i);
    }
    wtk_draw_text(ctx, "Address", address_xoff, header_yoff);
    wtk_draw_text(ctx, hex_chars, hex_xoff, header_yoff);

    // a ghost row is the last row if it has no bytes on it yet
    // if length is 0, we are immediately building the ghost row
    bool building_ghost_row = total_length == 0 && row_byte_off == 0;
    int row_yoff = row_start_yoff;
    for (int i = 0; i < vis_row_count; i++) {
        // bp_doc_read is guaranteed to either return bytes_per_row or the remaining
        // bytes in the doc. we can use this to tell if we're the last row to draw.
        bp_len_t read_bytes = bp_doc_read(bp_doc, row_byte_off, bytes_per_row, row_bytes);
        if (read_bytes <= 0 && !building_ghost_row)
            break;
        
        // either this is the last visible row, we read less than bytes_per_row, or this is the last byte
        // todo: we could probably use this to return early, instead of read_bytes <= 0
        bp_len_t new_byte_off = row_byte_off + read_bytes;
        //
        bool last_vis_row = i == vis_row_count - 1;
        bool last_bytes_read = new_byte_off == total_length;
        bool full_row = read_bytes == bytes_per_row;
        bool will_build_ghost_row = last_bytes_read && full_row;
        bool last_row = last_vis_row || building_ghost_row || (last_bytes_read && !will_build_ghost_row);

        // fill addr_chars
        snprintf(addr_chars, sizeof(addr_chars), "%0*" PRIX64, address_digits, row_byte_off);

        // fill hex_chars & text_chars
        if (!building_ghost_row) {
            int hex_pos = 0;
            for (int j = 0; j < read_bytes; j++) {
                // todo: more encodings than just 32 <= x < 127
                hex_pos += snprintf(hex_chars + hex_pos, sizeof(hex_chars) - hex_pos, "%02X ", row_bytes[j]);
                text_chars[j] = (row_bytes[j] >= 32 && row_bytes[j] < 127) ? (char)row_bytes[j] : '.';
            }

            hex_chars[hex_pos] = '\0';
            text_chars[read_bytes] = '\0';
        }

        // draw
        if (building_ghost_row) {
            wtk_draw_set_color(ctx, 127, 127, 127);
        }
        wtk_draw_text(ctx, addr_chars, address_xoff, row_yoff);
        if (!building_ghost_row) {
            wtk_draw_text(ctx, hex_chars, hex_xoff, row_yoff);
            wtk_draw_text(ctx, text_chars, text_xoff, row_yoff);
        }

        HexEditCursorInfo* cursor_inf = &self->active_doc->cursor;
        HexEditCursorPosition cursor_pos = cursor_inf->end_cur;
        bp_len_t cursor_off = cursor_inf->end_off;
        bp_len_t cursor_check_off = cursor_pos == HEXEDIT_CURSOR_RIGHT_PREV
            ? (cursor_off - 1) : cursor_off;

        bool cursor_on_this_row = (row_byte_off <= cursor_check_off
            && cursor_check_off < row_byte_off + read_bytes)
            || (last_row && cursor_check_off == total_length);

        if (cursor_on_this_row) {
            int sub_xoff = 0;
            int pane_xoff = 0;
            int byte_width = 1;
            int cell_xoff = (int)(cursor_off - row_byte_off);
            if (cursor_inf->active_pane == HEXEDIT_PANE_HEX) {
                pane_xoff = hex_xoff;
                byte_width = 3;
                switch (cursor_pos) {
                    case HEXEDIT_CURSOR_LEFT:
                        sub_xoff = 0;
                        break;
                    case HEXEDIT_CURSOR_NIBBLE:
                        sub_xoff = 1;
                        break;
                    case HEXEDIT_CURSOR_RIGHT:
                        sub_xoff = 2;
                        break;
                    case HEXEDIT_CURSOR_RIGHT_PREV:
                        sub_xoff = 2;
                        cell_xoff = bytes_per_row - 1;
                        break;
                }
            } else if (cursor_inf->active_pane == HEXEDIT_PANE_CHARS) {
                pane_xoff = text_xoff;
                byte_width = 1;
                switch (cursor_pos) {
                    case HEXEDIT_CURSOR_LEFT:
                        sub_xoff = 0;
                        break;
                    case HEXEDIT_CURSOR_RIGHT:
                        sub_xoff = 1;
                        break;
                    case HEXEDIT_CURSOR_RIGHT_PREV:
                        sub_xoff = 1;
                        cell_xoff = bytes_per_row - 1;
                        break;
                }
            }

            int cursor_xoff = pane_xoff + (cell_xoff * byte_width + sub_xoff) * char_width;
            wtk_draw_set_color(ctx, 0, 0, 0);
            wtk_draw_rectangle(ctx, cursor_xoff, row_yoff, 2, char_height);
            wtk_draw_set_sys_color(ctx, WTK_SYS_COLOR_TEXT_COLOR);
        }

        if (will_build_ghost_row) {
            building_ghost_row = true;
        } else if (building_ghost_row) {
            break;
        }

        row_yoff += row_height;
        row_byte_off += read_bytes;
    }
}

void hexedit_scroll(void* control, void* param)
{
    HexEditControl* self = (HexEditControl*)control;
    HexEditDocument* doc = self->active_doc;
    if (doc == NULL)
        return;

    WtkControl* self_ctrl = (WtkControl*)self;
    WtkScrollInfo* scroll_inf = (WtkScrollInfo*)param;
    if (scroll_inf->direction == WTK_SCROLLDIR_VERTICAL) {
        doc->scroll_y = scroll_inf->value * doc->bytes_per_row;
    }

    wtk_draw_redraw(self_ctrl);
}

void hexedit_mouse_scroll(void* control, void* param)
{
    HexEditControl* self = (HexEditControl*)control;
    HexEditDocument* doc = self->active_doc;
    if (doc == NULL)
        return;

    WtkControl* self_ctrl = (WtkControl*)self;
    WtkScrollInfo* scroll_inf = (WtkScrollInfo*)param;
    if (scroll_inf->direction != WTK_SCROLLDIR_VERTICAL)
        return;

    // the extra scroll math is so touchpad scrolling is more smooth
    doc->scroll_y_frac -= scroll_inf->value * SCROLL_ROWS_PER_NOTCH;

    bp_len_t new_scroll_y = doc->scroll_y;
    if (abs(doc->scroll_y_frac) >= WTK_SCROLL_DELTA) {
        int scroll_y_delta = doc->scroll_y_frac / WTK_SCROLL_DELTA;
        new_scroll_y += scroll_y_delta * doc->bytes_per_row;
        doc->scroll_y_frac -= WTK_SCROLL_DELTA * scroll_y_delta;
    }

    // clamp new scroll offset to addresses visible in the document
    bp_len_t max_snap_addr = bp_doc_length(self->active_doc->tree_doc) / doc->bytes_per_row * doc->bytes_per_row;
    new_scroll_y = wtk_max(0, wtk_min(new_scroll_y, max_snap_addr));

    doc->scroll_y = new_scroll_y;
    wtk_set_scrollbar_pos((WtkControl*)self, WTK_SCROLLDIR_VERTICAL, (int)(new_scroll_y / doc->bytes_per_row));
    wtk_draw_redraw(self_ctrl);
}

void hexedit_key_down(void* control, void* param)
{
    HexEditControl* self = (HexEditControl*)control;
    HexEditDocument* doc = self->active_doc;
    if (doc == NULL)
        return;

    WtkControl* self_ctrl = (WtkControl*)self;
    WtkKeyInfo* key_inf = (WtkKeyInfo*)param;

    // todo: when we handle rows that are not all bytes_per_row, this will need fixing
    HexEditCursorInfo* cursor_inf = &self->active_doc->cursor;
    int bytes_per_row = self->active_doc->bytes_per_row;
    bp_len_t total_length = bp_tree_length(self->active_doc->tree_doc->working);

    bool makes_redraw = false;

    // we need to be very careful! the code assumes RIGHT_PREV is set only when
    // the cursor off is set to the next line but needs to be displayed on the
    // previous line. do not use RIGHT_PREV if you want to display the cursor
    // to the right of the previous character on the same line, it won't work!
    switch (key_inf->key_code)
    {
        // redraw is set to true even when end_off doesn't change because
        // we want the cursor timer to reset which will potentially require
        // a redraw to unhide it.
        // we always add/subtract cursor offset, regardless of where we're at
        // and only clamp it at the end. this means pressing up on the top line
        // or down on the bottom line sends you to the very start or end. this
        // makes sense to me, so that's what I'm using, but no other hex editor
        // seems to do this. text editors have something similar, but they oddly
        // snap back to the column you were on once you go off of that line. we
        // don't do that because we don't have a concept of a so called "goal
        // column", but maybe we could support that in the future.
        case WTK_KEY_UP: {
            bool was_at_file_end = cursor_inf->end_off == total_length;
            cursor_inf->end_off -= bytes_per_row;
            cursor_inf->end_off = wtk_clamp(0, cursor_inf->end_off, total_length);

            if (cursor_inf->end_cur == HEXEDIT_CURSOR_RIGHT_PREV
                && cursor_inf->end_off > 0
                && (!was_at_file_end || (cursor_inf->end_off % bytes_per_row) == 0)
            ) {
                cursor_inf->end_cur = HEXEDIT_CURSOR_RIGHT_PREV;
            } else {
                cursor_inf->end_cur = HEXEDIT_CURSOR_LEFT;
            }

            makes_redraw = true;
            break;
        }
        case WTK_KEY_DOWN: {
            bool was_at_file_end = cursor_inf->end_off >= total_length;
            if (was_at_file_end && cursor_inf->end_cur == HEXEDIT_CURSOR_RIGHT_PREV) {
                // cursor doesn't move offset but does move from RIGHT_PREV to LEFT
                cursor_inf->end_cur = HEXEDIT_CURSOR_LEFT;
            } else {
                // currently, we don't have a cursor position for "left of previous char",
                // so we're just using left of what would be the next char. this doesn't
                // match up with what some other hex editors do where they RIGHT to the
                // previous character on the same line (not the previous line), so tbd
                // if it changes. 010 matches what we do, but 010 also doesn't support
                // RIGHT_PREV, so it's a bit of a meaningless comparison.
                cursor_inf->end_off += bytes_per_row;
                cursor_inf->end_off = wtk_clamp(0, cursor_inf->end_off, total_length);

                bool at_file_end = cursor_inf->end_off >= total_length;
                if (cursor_inf->end_cur == HEXEDIT_CURSOR_RIGHT_PREV
                    && (!at_file_end || (cursor_inf->end_off % bytes_per_row) == 0)
                ) {
                    cursor_inf->end_cur = HEXEDIT_CURSOR_RIGHT_PREV;
                } else {
                    cursor_inf->end_cur = HEXEDIT_CURSOR_LEFT;
                }
            }
            
            makes_redraw = true;
            break;
        }
        case WTK_KEY_LEFT: {
            cursor_inf->end_off--;
            cursor_inf->end_off = wtk_clamp(0, cursor_inf->end_off, total_length);

            cursor_inf->end_cur = HEXEDIT_CURSOR_LEFT;
            makes_redraw = true;
            break;
        }
        case WTK_KEY_RIGHT: {
            // if we're RIGHT_PREV at the end of a line and press right, we snap
            // to the second byte of the next line. this somewhat makes sense and
            // is what HxD does, so we'll do the same (by doing nothing special).
            cursor_inf->end_off++;
            cursor_inf->end_off = wtk_clamp(0, cursor_inf->end_off, total_length);

            cursor_inf->end_cur = HEXEDIT_CURSOR_LEFT;
            makes_redraw = true;
            break;
        }
        case WTK_KEY_HOME: {
            cursor_inf->end_off -= cursor_inf->end_off % bytes_per_row;
            if (cursor_inf->end_cur == HEXEDIT_CURSOR_RIGHT_PREV) {
                // we're actually on the previous line
                cursor_inf->end_off -= bytes_per_row;
            }
            cursor_inf->end_off = wtk_clamp(0, cursor_inf->end_off, total_length);

            cursor_inf->end_cur = HEXEDIT_CURSOR_LEFT;
            makes_redraw = true;
            break;
        }
        case WTK_KEY_END: {
            bool was_at_file_end = cursor_inf->end_off >= total_length;
            if (cursor_inf->end_cur != HEXEDIT_CURSOR_RIGHT_PREV && !was_at_file_end) {
                // we're not already at the right of this row
                // (this prevents spamming end from going down a line each time)
                cursor_inf->end_off += bytes_per_row - (cursor_inf->end_off % bytes_per_row);
                cursor_inf->end_off = wtk_clamp(0, cursor_inf->end_off, total_length);
                
                bool at_file_end = cursor_inf->end_off >= total_length;
                if (at_file_end && (cursor_inf->end_off % bytes_per_row) != 0) {
                    // see WTK_KEY_DOWN for more details, we need to use left here
                    cursor_inf->end_cur = HEXEDIT_CURSOR_LEFT;
                } else {
                    cursor_inf->end_cur = HEXEDIT_CURSOR_RIGHT_PREV;
                }
            }

            makes_redraw = true;
            break;
        }
        case WTK_KEY_TAB: {
            if (cursor_inf->active_pane == HEXEDIT_PANE_HEX)
                cursor_inf->active_pane = HEXEDIT_PANE_CHARS;
            else if (cursor_inf->active_pane == HEXEDIT_PANE_CHARS)
                cursor_inf->active_pane = HEXEDIT_PANE_HEX;
            
            makes_redraw = true;
            break;
        }
        case WTK_KEY_BACKSPACE: {
            if (cursor_inf->end_off > 0) {
                bp_doc_delete(self->active_doc->tree_doc, cursor_inf->end_off - 1, 1);
                cursor_inf->end_off--;
                cursor_inf->end_cur = HEXEDIT_CURSOR_LEFT;
                makes_redraw = true;
            }
            break;
        }
        // don't need to set redraw because undo/redo handle it
        case WTK_KEY_Z: {
            if (key_inf->ctrl_held) {
                if (key_inf->shift_held) {
                    hexedit_redo(self);
                } else {
                    hexedit_undo(self);
                }
            }
            break;
        }
        case WTK_KEY_Y: {
            if (key_inf->ctrl_held) {
                hexedit_redo(self);
            }
            break;
        }
    }

    if (makes_redraw)
        wtk_draw_redraw(self_ctrl);
}

void hexedit_char_pressed(void* control, void* param)
{
    HexEditControl* self = (HexEditControl*)control;
    HexEditDocument* doc = self->active_doc;
    if (doc == NULL)
        return;

    WtkControl* self_ctrl = (WtkControl*)self;
    WtkCharInfo* char_inf = (WtkCharInfo*)param;
    uint32_t codepoint = char_inf->codepoint;

    HexEditCursorInfo* cursor_inf = &self->active_doc->cursor;

    bool moves_cursor = false;
    bool makes_redraw = false;

    if (codepoint >= 0x20 && codepoint < 0x7f) {
        uint8_t data = codepoint & 0xff;
        bp_doc_insert(self->active_doc->tree_doc, cursor_inf->end_off, &data, 1);
        cursor_inf->end_off++;
        cursor_inf->end_cur = HEXEDIT_CURSOR_LEFT;
        self->needs_draw_recalc = true;
        
        moves_cursor = true;
        makes_redraw = true;
    }

    // todo: deduplicate
    if (moves_cursor)
        cursor_inf->end_off = wtk_max(0, wtk_min(cursor_inf->end_off, bp_tree_length(self->active_doc->tree_doc->working)));
    if (makes_redraw)
        wtk_draw_redraw(self_ctrl);
}

HexEditControl* hexedit_new(WtkControl* parent, int x, int y, int w, int h)
{
    HexEditControl* self = (HexEditControl*)wtk_create_custom(
        parent, x, y, w, h, sizeof(HexEditControl), &hexedit_ctrl_info);

    self->active_doc = NULL;
    self->needs_draw_recalc = false;

    WtkControl* self_ctrl = (WtkControl*)self;
    wtk_set_scrollbar(self_ctrl, WTK_SCROLLDIR_VERTICAL);
    wtk_set_scrollbar_range(self_ctrl, WTK_SCROLLDIR_VERTICAL, 0, 0, SCROLL_PAGE_SIZE);
    wtk_event_control_add(self_ctrl, WTK_EVENT_PAINT, hexedit_paint);
    wtk_event_control_add(self_ctrl, WTK_EVENT_SCROLL, hexedit_scroll);
    wtk_event_control_add(self_ctrl, WTK_EVENT_MOUSE_SCROLL, hexedit_mouse_scroll);
    wtk_event_control_add(self_ctrl, WTK_EVENT_KEY_DOWN, hexedit_key_down);
    wtk_event_control_add(self_ctrl, WTK_EVENT_CHAR_PRESS, hexedit_char_pressed);
    return self;
}

void hexedit_set_doc(HexEditControl* self, HexEditDocument* doc)
{
    self->active_doc = doc;
    self->needs_draw_recalc = true;

    WtkControl* self_ctrl = (WtkControl*)self;
    wtk_draw_redraw(self_ctrl);
}

bool hexedit_undo(HexEditControl* self)
{
    WtkControl* self_ctrl = (WtkControl*)self;

    bp_len_t new_off = -1;
    if (self->active_doc != NULL)
        new_off = bp_doc_undo(self->active_doc->tree_doc);

    if (new_off != -1) {
        wtk_draw_redraw(self_ctrl);
        self->active_doc->cursor.end_off = new_off;
        self->active_doc->cursor.end_cur = HEXEDIT_CURSOR_LEFT;
        return true;
    }
    
    return false;
}

bool hexedit_redo(HexEditControl* self)
{
    WtkControl* self_ctrl = (WtkControl*)self;

    bp_len_t new_off = -1;
    if (self->active_doc != NULL)
        new_off = bp_doc_redo(self->active_doc->tree_doc);

    if (new_off != -1) {
        wtk_draw_redraw(self_ctrl);
        self->active_doc->cursor.end_off = new_off;
        self->active_doc->cursor.end_cur = HEXEDIT_CURSOR_LEFT;
        return true;
    }
    
    return false;
}
