#include <stdio.h>

#include "wtk_container.h"
#include "wtk_wgtk.h"
#include "wtk_assets.h"

#include "hax_hexedit.h"

WtkWindow* main_win;
HexEditControl* hex_editor;

void menu_item_new(void* control, void* param)
{
    HexEditDocument* doc = hexedit_doc_new_empty();
    hexedit_set_doc(hex_editor, doc);
    wtk_control_focus(hex_editor);
}

void menu_item_open(void* control, void* param)
{
    char* file_path = wtk_open_file_dialog_show("Open file");
    if (file_path == NULL)
        return;

    HexEditDocument* doc = hexedit_doc_load_file(file_path);
    if (doc == NULL)
        goto done;

    hexedit_set_doc(hex_editor, doc);
    wtk_control_focus(hex_editor);

done:
    wtk_free(file_path);
}

void menu_item_exit(void* control, void* param)
{
    if (hex_editor->active_doc != NULL) {
        WtkMbResponse resp = wtk_messagebox_show(
            "Exit Spooky Hex?", "Are you sure you want quit?",
            WTK_MBICON_QUESTION, WTK_MBBUTTONS_YES_NO);
        
        if (resp == WTK_MBRESPONSE_NO)
            return;
    }

    wtk_delete_control(&main_win->b);
}

void menu_item_about(void* control, void* param)
{
    wtk_simple_messagebox_show("About Spooky Hex", "A simple hex editor by nes");
}

void menu_item_undo(void* control, void* param)
{
    hexedit_undo(hex_editor);
}

void menu_item_redo(void* control, void* param)
{
    hexedit_redo(hex_editor);
}

void app_main()
{
    main_win = wtk_create_window(700, 600, "Spooky Hex");
    WtkControl* main_win_ctrl = (WtkControl*)main_win;
    
    WtkImage* win_icon_large = wtk_image_load_resource_icon(WTK_RSRC_APPICON, 64);
    WtkImage* win_icon_small = wtk_image_load_resource_icon(WTK_RSRC_APPICON, 16);
    wtk_window_set_icon(main_win, win_icon_large, win_icon_small);

    // WtkControl* btn1 = wtk_create_button(main_win_ctrl, 100, 100, main_win_ctrl->w - 100 - 100, 40, "Click this");
    // WtkControl* btn2 = wtk_create_button(main_win_ctrl, 100, 150, main_win_ctrl->w - 100 - 100, 40, "Or this");
    // wtk_set_anchor(btn1, WTK_ANCHOR_TOP | WTK_ANCHOR_LEFT | WTK_ANCHOR_RIGHT);
    // wtk_set_anchor(btn2, WTK_ANCHOR_TOP | WTK_ANCHOR_LEFT | WTK_ANCHOR_RIGHT);
    // wtk_event_control_add(btn1, WTK_EVENT_CLICK, btn_click_callback);
    // wtk_event_control_add(btn2, WTK_EVENT_CLICK, btn_click_callback);

    WtkMenuBar* menu_bar = wtk_create_menu_bar(main_win);

    WtkMenu* file_menu = wtk_create_menu(menu_bar, "File");
    WtkMenuItem* new_mi = wtk_create_menu_item(file_menu, WTK_MENU_TEXT, "New");
    WtkMenuItem* open_mi = wtk_create_menu_item(file_menu, WTK_MENU_TEXT, "Open");
    wtk_create_menu_item(file_menu, WTK_MENU_SEPARATOR, "");
    WtkMenuItem* exit_mi = wtk_create_menu_item(file_menu, WTK_MENU_TEXT, "Exit");
    //
    wtk_event_menu_item_add(new_mi, WTK_EVENT_CLICK, menu_item_new);
    wtk_event_menu_item_add(open_mi, WTK_EVENT_CLICK, menu_item_open);
    wtk_event_menu_item_add(exit_mi, WTK_EVENT_CLICK, menu_item_exit);

    WtkMenu* edit_menu = wtk_create_menu(menu_bar, "Edit");
    WtkMenuItem* undo_mi = wtk_create_menu_item(edit_menu, WTK_MENU_TEXT, "Undo");
    WtkMenuItem* redo_mi = wtk_create_menu_item(edit_menu, WTK_MENU_TEXT, "Redo");
    wtk_create_menu_item(edit_menu, WTK_MENU_SEPARATOR, "");
    WtkMenuItem* cut_mi = wtk_create_menu_item(edit_menu, WTK_MENU_TEXT, "Cut");
    WtkMenuItem* copy_mi = wtk_create_menu_item(edit_menu, WTK_MENU_TEXT, "Copy");
    WtkMenuItem* paste_mi = wtk_create_menu_item(edit_menu, WTK_MENU_TEXT, "Paste");
    WtkMenuItem* delete_mi = wtk_create_menu_item(edit_menu, WTK_MENU_TEXT, "Delete");
    //
    wtk_event_menu_item_add(undo_mi, WTK_EVENT_CLICK, menu_item_undo);
    wtk_event_menu_item_add(redo_mi, WTK_EVENT_CLICK, menu_item_redo);

    WtkMenu* help_menu = wtk_create_menu(menu_bar, "Help");
    WtkMenuItem* about_mi = wtk_create_menu_item(help_menu, WTK_MENU_TEXT, "About");
    //
    wtk_event_menu_item_add(about_mi, WTK_EVENT_CLICK, menu_item_about);

    WtkImage* file_icon = wtk_image_load_resource_icon(WTK_RSRC_FILE, 32);
    WtkImage* folder_icon = wtk_image_load_resource_icon(WTK_RSRC_FOLDER, 32);
    WtkImage* save_icon = wtk_image_load_resource_icon(WTK_RSRC_SAVE, 32);

    WtkToolbar* toolbar = wtk_create_toolbar(main_win, 16);
    WtkToolbarItem* file_tbi = wtk_create_toolbar_item(toolbar, "New", file_icon);
    WtkToolbarItem* folder_tbi = wtk_create_toolbar_item(toolbar, "Open...", folder_icon);
    WtkToolbarItem* save_tbi = wtk_create_toolbar_item(toolbar, "Save", save_icon);
    //
    wtk_event_toolbar_item_add(file_tbi, WTK_EVENT_CLICK, menu_item_new);
    wtk_event_toolbar_item_add(folder_tbi, WTK_EVENT_CLICK, menu_item_open);

    // todo: handle control placement more correctly
    const int TOOLBAR_HEIGHT = 26;

    hex_editor = hexedit_new(main_win_ctrl, 0, TOOLBAR_HEIGHT, main_win_ctrl->w, main_win_ctrl->h - TOOLBAR_HEIGHT);
    wtk_set_anchor((WtkControl*)hex_editor, WTK_ANCHOR_TOP | WTK_ANCHOR_LEFT | WTK_ANCHOR_BOTTOM | WTK_ANCHOR_RIGHT);
}

wtk_init(app_main);