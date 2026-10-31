#ifndef HAX_HEXEDIT_H
#define HAX_HEXEDIT_H

#include "wtk_wgtk.h"
#include "hax_document.h"

#define HEXEDIT_DEFAULT_CHAR_WIDTH 8
#define HEXEDIT_DEFAULT_CHAR_HEIGHT 12

typedef struct {
    WtkControl b;

    // state
    HexEditDocument* active_doc;

    // drawing
    int char_width;
    int char_height;
    bool needs_draw_recalc;
} HexEditControl;

HexEditControl* hexedit_new(WtkControl* parent, int x, int y, int w, int h);

void hexedit_set_doc(HexEditControl* self, HexEditDocument* doc);
bool hexedit_undo(HexEditControl* self);
bool hexedit_redo(HexEditControl* self);

#endif // HAX_HEXEDIT_H