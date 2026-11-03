// for debug only, please remove
#ifndef wtk_win32
#define wtk_win32 1
#endif // wtk_win32

#ifndef WTK_WGTK_H
#define WTK_WGTK_H

#include "wtk_common.h"
#include "wtk_container.h"

#if wtk_win32
#include "wtk_windows.h"
#include <initguid.h>
#include <uxtheme.h>
#include <shobjidl.h>
#endif

// todo: cleanup function names. some are struct_action and some are action_struct.

// forward declarations ////////////////////////////////////////////////////////
typedef struct WtkControl WtkControl;
typedef struct WtkWindow WtkWindow;
typedef struct WtkScrollInfo WtkScrollInfo;
typedef struct WtkMouseInfo WtkMouseInfo;
typedef struct WtkKeyInfo WtkKeyInfo;
typedef struct WtkCharInfo WtkCharInfo;
typedef struct WtkMenuBar WtkMenuBar;
typedef struct WtkMenu WtkMenu;
typedef struct WtkMenuItem WtkMenuItem;
typedef struct WtkToolbar WtkToolbar;
typedef struct WtkToolbarItem WtkToolbarItem;
typedef struct WtkDrawingContext WtkDrawingContext;
typedef struct WtkTimer WtkTimer;
// ///////////////////////////////////////////////// end forward declarations //

// wtk events //////////////////////////////////////////////////////////////////
typedef enum {
    WTK_EVENT_CLICK,
    WTK_EVENT_MOUSE_DOWN,
    WTK_EVENT_MOUSE_UP,
    WTK_EVENT_MOUSE_MOVE,
    WTK_EVENT_MOUSE_SCROLL,
    WTK_EVENT_KEY_DOWN,
    WTK_EVENT_KEY_UP,
    WTK_EVENT_CHAR_PRESS,
    WTK_EVENT_FOCUS,
    WTK_EVENT_FOCUS_LOST,
    WTK_EVENT_TEXT_CHANGED,
    WTK_EVENT_RESIZED,
    WTK_EVENT_CLOSED,
    WTK_EVENT_PAINT,
    WTK_EVENT_SCROLL,
    WTK_EVENT_UITIMER
} WtkEventType;

typedef void (*WtkCallback)(void*, void*);

typedef struct {
    WtkEventType event_type;
    WtkCallback callback;
} WtkEvent;

typedef struct {
    WtkVec(WtkEvent) events;
    void* user_data;
} WtkEventable;

#define wtk_event_vec_insert(id_to_vec, cmd_id, obj)   \
    while (wtkvec_elem_count(id_to_vec) <= (cmd_id)) { \
        wtkvec_push(id_to_vec, NULL);                  \
    }                                                  \
    id_to_vec[(cmd_id)] = (obj);

#define wtk_event_vec_trigger(id_to_vec, cmd_id, event)                               \
    if ((cmd_id) < wtkvec_elem_count(id_to_vec)) {                                    \
        wtk_event_control_trigger((WtkEventable*)id_to_vec[(cmd_id)], (event), NULL); \
    }

bool wtk_event_control_add(WtkControl* control, WtkEventType event_type, WtkCallback callback);
bool wtk_event_menu_item_add(WtkMenuItem* control, WtkEventType event_type, WtkCallback callback);
bool wtk_event_toolbar_item_add(WtkToolbarItem* control, WtkEventType event_type, WtkCallback callback);
bool wtk_event_timer_add(WtkTimer* timer, WtkEventType event_type, WtkCallback callback);
// /////////////////////////////////////////////////////////// end wtk events //

// wtk assets //////////////////////////////////////////////////////////////////
#if wtk_win32
typedef WORD WtkResourceId;
#else // wtk_win32
typedef const char* WtkResourceId;
#endif // !wtk_win32

typedef enum {
    WTK_IMAGESRC_RESOURCE_ICON,
    WTK_IMAGESRC_MEMORY
} WtkImageSource;

typedef struct {
    WtkImageSource source;
#if wtk_win32
    union {
        HBITMAP bitmap;
        HICON icon;
    };
#endif

    int width;
    int height;
} WtkImage;

WtkImage* wtk_image_load_resource_icon(WtkResourceId resource_id, int icon_dims);
void wtk_image_unload(WtkImage* image);
// /////////////////////////////////////////////////////////// end wtk assets //

// wtk controls ////////////////////////////////////////////////////////////////
typedef enum {
    WTK_CONTROL_WINDOW,
    WTK_CONTROL_BUTTON,
    WTK_CONTROL_LABEL,
    WTK_CONTROL_TEXTBOX,
    WTK_CONTROL_TOOLBAR,
    WTK_CONTROL_CUSTOM
} WtkControlType;

typedef enum {
    WTK_ANCHOR_TOP = 1,
    WTK_ANCHOR_BOTTOM = 2,
    WTK_ANCHOR_LEFT = 4,
    WTK_ANCHOR_RIGHT = 8
} WtkAnchorType;

typedef enum {
    WTK_SCROLLDIR_VERTICAL = 1,
    WTK_SCROLLDIR_HORIZONTAL = 2
} WtkScrollDir;

typedef enum {
    WTK_SCROLL_ENABLED = 1,
    WTK_SCROLL_VERTICAL_MANAGED = 2,
    WTK_SCROLL_HORIZONTAL_MANAGED = 4
} WtkScrollFlags;

typedef enum {
    WTK_MOUSE_NONE,
    WTK_MOUSE_LEFT,
    WTK_MOUSE_RIGHT,
    WTK_MOUSE_MIDDLE
} WtkMouseButton;

// todo: make a whole list of keycodes that apply to all platforms
typedef enum {
    WTK_KEY_UNKNOWN,
    WTK_KEY_UP,
    WTK_KEY_DOWN,
    WTK_KEY_LEFT,
    WTK_KEY_RIGHT,
    WTK_KEY_HOME,
    WTK_KEY_END,
    WTK_KEY_PAGE_UP,
    WTK_KEY_PAGE_DOWN,
    WTK_KEY_INSERT,
    WTK_KEY_DELETE,
    WTK_KEY_BACKSPACE,
    WTK_KEY_ENTER,
    WTK_KEY_TAB,
    WTK_KEY_ESC,
    WTK_KEY_A,
    WTK_KEY_B,
    WTK_KEY_C,
    WTK_KEY_D,
    WTK_KEY_E,
    WTK_KEY_F,
    WTK_KEY_G,
    WTK_KEY_H,
    WTK_KEY_I,
    WTK_KEY_J,
    WTK_KEY_K,
    WTK_KEY_L,
    WTK_KEY_M,
    WTK_KEY_N,
    WTK_KEY_O,
    WTK_KEY_P,
    WTK_KEY_Q,
    WTK_KEY_R,
    WTK_KEY_S,
    WTK_KEY_T,
    WTK_KEY_U,
    WTK_KEY_V,
    WTK_KEY_W,
    WTK_KEY_X,
    WTK_KEY_Y,
    WTK_KEY_Z
} WtkKeyCode;

typedef struct {
    int left, top, right, bottom;
} WtkAnchorInfo;

struct WtkControl {
    WtkEventable b;

#if wtk_win32
    HWND con_hnd;
#endif
    WtkControlType con_type;

    WtkControl* parent;

    WtkVec(WtkControl*) children;

    int x, y, w, h;
    WtkAnchorType anchor;
    WtkAnchorInfo anchor_info;
    WtkScrollFlags scroll_flags;

#if wtk_win32
    // see WM_CHAR handler
    uint16_t char_press_high_surrogate;
#endif
};

struct WtkWindow {
    WtkControl b;

    WtkImage* large_icon;
    WtkImage* small_icon;

    WtkMenuBar* menu_bar;
    WtkToolbar* toolbar;

    WtkIdpool id_pool;
    WtkVec(WtkEventable*) id_to_eventable;
};

struct WtkScrollInfo {
    WtkScrollDir direction;
    int value;
};

struct WtkMouseInfo {
    WtkMouseButton button;
    int x, y;
    bool shift_held;
    bool ctrl_held;
    bool alt_held;
};

struct WtkKeyInfo {
    WtkKeyCode key_code;
    bool shift_held;
    bool ctrl_held;
    bool alt_held;
};

struct WtkCharInfo {
    uint32_t codepoint;
};

#ifdef wtk_win32
    #define WTK_SCROLL_DELTA WHEEL_DELTA
#else
    // todo
    #define WTK_SCROLL_DELTA 100
#endif

WtkWindow* wtk_create_window(int width, int height, const char* title);

WtkControl* wtk_create_button(WtkControl* parent, int x, int y, int width, int height, const char* text);
WtkControl* wtk_create_label(WtkControl* parent, int x, int y, int width, int height, const char* text);
WtkControl* wtk_create_textbox(WtkControl* parent, int x, int y, int width, int height, const char* text);

bool wtk_delete_control(WtkControl* control);

void wtk_window_set_title(WtkWindow* window, const char* title);
void wtk_window_set_icon(WtkWindow* window, WtkImage* large_icon, WtkImage* small_icon);

bool wtk_control_focus(WtkControl* control);

void wtk_set_anchor(WtkControl* control, WtkAnchorType anchor);

void wtk_set_scrollbar(WtkControl* control, WtkScrollDir scroll_dir);
void wtk_set_scrollbar_range(WtkControl* control, WtkScrollDir scroll_dir, int min, int max, int page);
void wtk_set_scrollbar_pos(WtkControl* control, WtkScrollDir scroll_dir, int pos);
// ///////////////////////////////////////////////////////// end wtk controls //

// wtk custom controls /////////////////////////////////////////////////////////
struct WtkDrawingContext {
#if wtk_win32
    HDC drawer;
    RECT client_rect;
    HBRUSH brush;
    HFONT font;
#endif
};

typedef enum {
    WTK_SYS_COLOR_TEXT_COLOR,
    WTK_SYS_COLOR_WINDOW_BACKGROUND
} WtkDrawingSysColor;

#if wtk_win32
typedef struct {
    LPCWSTR type_name;
    void (*register_fn)();
} WtkControlInfo;

#define wtk_register_custom_stringify(s) L"" #s
#define wtk_register_custom(class_name, class_prefix)                 \
    static bool class_prefix##_registered = 0;                        \
    static void class_prefix##_register_control(void) {               \
        if (class_prefix##_registered) {                              \
            return;                                                   \
        }                                                             \
        WNDCLASS wc = { 0 };                                          \
        wc.style = CS_GLOBALCLASS | CS_HREDRAW | CS_VREDRAW;          \
        wc.lpfnWndProc = (WNDPROC)wtk_custom_process_event;           \
        wc.hCursor = LoadCursor(NULL, IDC_ARROW);                     \
        wc.lpszClassName = wtk_register_custom_stringify(class_name); \
        RegisterClass(&wc);                                           \
        class_prefix##_registered = 1;                                \
    }                                                                 \
    static WtkControlInfo class_prefix##_ctrl_info = {                \
        wtk_register_custom_stringify(class_name),                    \
        class_prefix##_register_control                               \
    };
// static void class_prefix##_load_info(WtkControlInfo* control_info) {
//     control_info->type_name = wtk_register_custom_stringify(class_name);
//     control_info->register_fn = class_prefix##_register_control;
// }
#endif // wtk_win32

// todo: can we skip hwnd and pull it straight from control?
void wtk_draw_callback_wrapper(WtkControl* control, WtkEvent* event, HWND hwnd);
LRESULT wtk_custom_process_event(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
WtkControl* wtk_create_custom(WtkControl* parent, int x, int y, int width, int height, int struct_size, WtkControlInfo* control_info);

void wtk_draw_redraw(WtkControl* control);

void wtk_draw_rectangle(WtkDrawingContext* ctx, int x, int y, int w, int h);
void wtk_draw_text(WtkDrawingContext* ctx, char* text, int x, int y);

void wtk_draw_set_color(WtkDrawingContext* ctx, int r, int g, int b);
void wtk_draw_set_sys_color(WtkDrawingContext* ctx, WtkDrawingSysColor color);

void wtk_draw_set_font(WtkDrawingContext* ctx, char* font_face, int font_size, bool bold);

void wtk_draw_measure_text(WtkDrawingContext* ctx, char* text, int* out_w, int* out_h);
// ///////////////////////////////////////////////////// end wtk custom controls 

// wtk menus ///////////////////////////////////////////////////////////////////
typedef enum {
    WTK_MENU_TEXT,
    WTK_MENU_SEPARATOR,
    // WTK_MENU_TEXTBOX,
    // WTK_MENU_DROPDOWN
} WtkMenuItemType;

struct WtkMenuBar {
#if wtk_win32
    HMENU bar_hnd;
#endif
    WtkWindow* window;
};

struct WtkMenu {
    // todo: menus with no children are clickable
    // we should make WtkMenu clickable
    // WtkEventable b;

#if wtk_win32
    HMENU child_hnd;
#endif
    WtkMenuBar* menu_bar;
    WtkVec(WtkMenuItem*) children;
};

struct WtkMenuItem {
    WtkEventable b;

#if wtk_win32
    HMENU item_hnd;
    int cmd_id;
#endif
    WtkMenu* menu;
    WtkMenuItemType item_type;
};

// ///

typedef enum {
    WTK_TOOLBAR_BUTTON,
    WTK_TOOLBAR_SEPARATOR,
    // WTK_TOOLBAR_TOGGLE
} WtkToolbarItemType;

struct WtkToolbar {
#if wtk_win32
    HWND bar_hnd;
    HIMAGELIST image_list;
#endif
    WtkWindow* window;
#if wtk_win32
    WtkIdpool image_id_pool;
#endif
};

struct WtkToolbarItem {
    WtkEventable b;

#if wtk_win32
    int cmd_id;
#endif
    WtkToolbar* toolbar;
    WtkToolbarItemType item_type;
    char* tooltip;
    WtkImage* icon;
};

WtkMenuBar* wtk_create_menu_bar(WtkWindow* window);
WtkMenu* wtk_create_menu(WtkMenuBar* menu_bar, const char* text);
WtkMenuItem* wtk_create_menu_item(WtkMenu* menu, WtkMenuItemType item_type, const char* text);

WtkToolbar* wtk_create_toolbar(WtkWindow* window, int icon_dims);
WtkToolbarItem* wtk_create_toolbar_item(WtkToolbar* toolbar, const char* tooltip, WtkImage* icon);
WtkToolbarItem* wtk_create_toolbar_separator(WtkToolbar* toolbar);
// //////////////////////////////////////////////////////////// end wtk menus //

// wtk popup boxes /////////////////////////////////////////////////////////////
typedef enum {
    WTK_MBICON_NONE,
    WTK_MBICON_INFO,
    WTK_MBICON_WARNING,
    WTK_MBICON_QUESTION,
    WTK_MBICON_ERROR
} WtkMbIcon;

typedef enum {
    WTK_MBBUTTONS_NONE,
    WTK_MBBUTTONS_OK,
    WTK_MBBUTTONS_OK_CANCEL,
    WTK_MBBUTTONS_YES_NO,
    WTK_MBBUTTONS_YES_NO_CANCEL
} WtkMbButtons;

typedef enum {
    WTK_MBRESPONSE_NONE,
    WTK_MBRESPONSE_OK,
    WTK_MBRESPONSE_CANCEL,
    WTK_MBRESPONSE_YES,
    WTK_MBRESPONSE_NO
} WtkMbResponse;

WtkMbResponse wtk_messagebox_show(const char* title, const char* text, WtkMbIcon icon, WtkMbButtons buttons);
void wtk_simple_messagebox_show(const char* title, const char* text);

char* wtk_open_file_dialog_show(const char* title);
// ////////////////////////////////////////////////////// end wtk popup boxes //

// wtk timers //////////////////////////////////////////////////////////////////
typedef enum {
    WTK_TIMER_REPEATING,
    WTK_TIMER_ONESHOT
} WtkTimerType;

struct WtkTimer {
    WtkEventable b;

    WtkTimerType timer_type;
    int delay_ms;

#if wtk_win32
    int timer_id;
#endif // wtk_win32
};

WtkTimer* wtk_create_timer(WtkTimerType timer_type, int delay_ms);
// /////////////////////////////////////////////////////////// end wtk timers //

// wtk theming /////////////////////////////////////////////////////////////////
typedef enum {
    WTK_THEME_LIGHT,
    WTK_THEME_DARK,
    WTK_THEME_AUTO
} WtkTheme;

#if wtk_win32
typedef struct {
    int Attrib;
    PVOID pvData;
    SIZE_T cbData;
} WINDOWCOMPOSITIONATTRIBDATA;
	
typedef BOOL (*TYPE_SetWindowCompositionAttribute)(HWND hWnd, WINDOWCOMPOSITIONATTRIBDATA*);
#endif // wtk_win32
///////////////////////////////////////////////////////////// end wtk theming //

// initial setup ///////////////////////////////////////////////////////////////
#if wtk_win32
struct Win32Inf {
    HINSTANCE h_instance;
    HINSTANCE h_prev_instance;
    LPSTR lp_cmd_line;
    int n_cmd_show;

    WNDCLASSW wc;
    int windows_open;
    HFONT stock_font;

    // timers are lazy loaded just in case they aren't needed for an app
    bool timers_setup;
    WNDCLASSW timer_wc;
    WtkIdpool timer_pool;
    WtkVec(WtkTimer*) timer_objs;
    HWND timer_win;
};

extern struct Win32Inf win32_inf;

void WndUpdateLoop();
LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);

#define wtk_init(callback) \
    int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) { \
        win32_inf.h_instance = hInstance;                                                             \
        win32_inf.h_prev_instance = hPrevInstance;                                                    \
        win32_inf.lp_cmd_line = lpCmdLine;                                                            \
        win32_inf.n_cmd_show = nCmdShow;                                                              \
        win32_inf.wc.lpfnWndProc = WndProc;                                                           \
        win32_inf.wc.hInstance = hInstance;                                                           \
        win32_inf.wc.hCursor = LoadCursor(NULL, IDC_ARROW);                                           \
        win32_inf.wc.hbrBackground = (HBRUSH)(COLOR_WINDOW);                                          \
        win32_inf.wc.lpszClassName = L"WTKWindow";                                                    \
        win32_inf.windows_open = 0;                                                                   \
        win32_inf.stock_font = (HFONT)GetStockObject(DEFAULT_GUI_FONT);                               \
        RegisterClass(&win32_inf.wc);                                                                 \
        win32_inf.timers_setup = false;                                                               \
        callback();                                                                                   \
        WndUpdateLoop();                                                                              \
    }
#endif // wtk_win32
// //////////////////////////////////////////////////////// end initial setup //

#endif // WTK_WGTK_H