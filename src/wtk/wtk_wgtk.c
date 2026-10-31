#include "wtk_wgtk.h"
#include "wtk_str.h"
#include "wtk_container.h"

// platform specific constants
#if wtk_win32
const int FIRST_MENU_ID = 100;
const unsigned int DEFAULT_WINDOW_LOCATION = 0x80000000;
const int DEFAULT_TOOLBAR_ICON_SIZE = 16;
const int DEFAULT_SCROLL_LINE_JUMP = 1; // todo: should be configurable

struct Win32Inf win32_inf;

// yucky having a static like this, but it's not like
// we can have more than one selected control at a
// time given there's only one mouse...
// todo: let's fix this later, at least move into win32_inf
static HWND wtk_captured_hwnd = NULL;
static WtkMouseButton wtk_captured_button = WTK_MOUSE_NONE;
#endif // wtk_win32

// todo: null check all public function's pointer arguments?

// forward declarations ////////////////////////////////////////////////////////
void wtk_event_control_trigger(WtkEventable* eventable, WtkEventType event_type, void* param);
// ///////////////////////////////////////////////// end forward declarations //

// wtk controls ////////////////////////////////////////////////////////////////
static WtkWindow* wtk_create_blank_window()
{
    WtkWindow* ctrl = wtk_malloc(WtkWindow*, sizeof(WtkWindow));
    ctrl->b.b.events = NULL;
    ctrl->b.b.user_data = NULL;
    ctrl->b.con_type = WTK_CONTROL_WINDOW;
    ctrl->b.parent = NULL;
    ctrl->b.children = NULL;
    ctrl->b.anchor = (WtkAnchorType)0;
    ctrl->b.anchor_info.left = 0;
    ctrl->b.anchor_info.top = 0;
    ctrl->b.anchor_info.right = 0;
    ctrl->b.anchor_info.bottom = 0;
    ctrl->b.scroll_flags = (WtkScrollFlags)0;
#if wtk_win32
    ctrl->b.char_press_high_surrogate = 0;
#endif // wtk_win32
    ctrl->large_icon = NULL;
    ctrl->small_icon = NULL;
    ctrl->menu_bar = NULL;
    ctrl->toolbar = NULL;
    ctrl->id_to_eventable = NULL;
    wtkvec_new(ctrl->b.b.events, 4, NULL); // todo: non-null destructor
    wtkvec_new(ctrl->b.children, 4, NULL); // todo: non-null destructor
    wtkvec_new(ctrl->id_to_eventable, 4, NULL); // todo: non-null destructor
    wtk_idpool_new(&ctrl->id_pool, FIRST_MENU_ID);
    return ctrl;
}

static WtkControl* wtk_create_blank_control_sized(WtkControlType con_type, int malloc_size)
{
    WtkControl* ctrl = wtk_malloc(WtkControl*, malloc_size);
    ctrl->b.events = NULL;
    ctrl->b.user_data = NULL;
    ctrl->con_type = con_type;
    ctrl->parent = NULL;
    ctrl->children = NULL;
    ctrl->anchor = (WtkAnchorType)(WTK_ANCHOR_TOP | WTK_ANCHOR_LEFT);
    ctrl->anchor_info.left = 0;
    ctrl->anchor_info.top = 0;
    ctrl->anchor_info.right = 0;
    ctrl->anchor_info.bottom = 0;
    ctrl->scroll_flags = (WtkScrollFlags)0;
#if wtk_win32
    ctrl->char_press_high_surrogate = 0;
#endif // wtk_win32
    wtkvec_new(ctrl->b.events, 4, NULL); // todo: non-null destructor
    wtkvec_new(ctrl->children, 4, NULL); // todo: non-null destructor
    return ctrl;
}

static WtkControl* wtk_create_blank_control(WtkControlType con_type)
{
    return wtk_create_blank_control_sized(con_type, sizeof(WtkControl));
}

static void wtk_recalculate_anchor(WtkControl* control)
{
    if (control == NULL || control->parent == NULL) {
        return;
    }

    control->anchor_info.left = control->x;
    control->anchor_info.right = control->parent->w - (control->x + control->w);
    control->anchor_info.top = control->y;
    control->anchor_info.bottom = control->parent->h - (control->y + control->h);
}

#if wtk_win32
static WtkKeyCode wtk_input_key_win32_to_wtk(unsigned int win32_key)
{
    switch (win32_key) {
        case VK_UP: return WTK_KEY_UP;
        case VK_DOWN: return WTK_KEY_DOWN;
        case VK_LEFT: return WTK_KEY_LEFT;
        case VK_RIGHT: return WTK_KEY_RIGHT;
        case VK_HOME: return WTK_KEY_HOME;
        case VK_END: return WTK_KEY_END;
        case VK_PRIOR: return WTK_KEY_PAGE_UP;
        case VK_NEXT: return WTK_KEY_PAGE_DOWN;
        case VK_INSERT: return WTK_KEY_INSERT;
        case VK_DELETE: return WTK_KEY_DELETE;
        case VK_BACK: return WTK_KEY_BACKSPACE;
        case VK_RETURN: return WTK_KEY_ENTER;
        case VK_TAB: return WTK_KEY_TAB;
        case VK_ESCAPE: return WTK_KEY_ESC;
        case 'A': return WTK_KEY_A;
        case 'B': return WTK_KEY_B;
        case 'C': return WTK_KEY_C;
        case 'D': return WTK_KEY_D;
        case 'E': return WTK_KEY_E;
        case 'F': return WTK_KEY_F;
        case 'G': return WTK_KEY_G;
        case 'H': return WTK_KEY_H;
        case 'I': return WTK_KEY_I;
        case 'J': return WTK_KEY_J;
        case 'K': return WTK_KEY_K;
        case 'L': return WTK_KEY_L;
        case 'M': return WTK_KEY_M;
        case 'N': return WTK_KEY_N;
        case 'O': return WTK_KEY_O;
        case 'P': return WTK_KEY_P;
        case 'Q': return WTK_KEY_Q;
        case 'R': return WTK_KEY_R;
        case 'S': return WTK_KEY_S;
        case 'T': return WTK_KEY_T;
        case 'U': return WTK_KEY_U;
        case 'V': return WTK_KEY_V;
        case 'W': return WTK_KEY_W;
        case 'X': return WTK_KEY_X;
        case 'Y': return WTK_KEY_Y;
        case 'Z': return WTK_KEY_Z;
    }
    return WTK_KEY_UNKNOWN;
}

static WtkMouseButton wtk_input_mouse_btn_win32_to_wtk(unsigned int win32_mouse_btn)
{
    if ((win32_mouse_btn & MK_LBUTTON) != 0) return WTK_MOUSE_LEFT;
    if ((win32_mouse_btn & MK_RBUTTON) != 0) return WTK_MOUSE_RIGHT;
    if ((win32_mouse_btn & MK_MBUTTON) != 0) return WTK_MOUSE_MIDDLE;
    return WTK_MOUSE_NONE;
}

static void wtk_process_common_input(WtkControl* control, HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    if (control == NULL) {
        return;
    }

    // todo: deduplicate ctrl/shift/alt lines

    WtkEventable* eventable = &control->b;

    switch (message) {
        case WM_KEYDOWN:
        case WM_KEYUP: {
            WtkKeyInfo key_info;
            key_info.key_code = wtk_input_key_win32_to_wtk(wParam);
            key_info.ctrl_held = (GetKeyState(VK_CONTROL) & 0x8000) != 0;
            key_info.shift_held = (GetKeyState(VK_SHIFT) & 0x8000) != 0;
            key_info.alt_held = (GetKeyState(VK_MENU) & 0x8000) != 0;

            wtk_event_control_trigger(eventable, message == WM_KEYDOWN ? WTK_EVENT_KEY_DOWN : WTK_EVENT_KEY_UP, &key_info);
            return;
        }
        case WM_CHAR: {
            // because things have to be complicated, we don't get
            // everything at once. if we get a high surrogate, wait
            // until we get the low surrogate in the next event.

            WtkCharInfo char_info;
            if (IS_HIGH_SURROGATE(wParam)) {
                control->char_press_high_surrogate = (uint16_t)wParam;
                return;
            } else if (IS_LOW_SURROGATE(wParam)) {
                if (control->char_press_high_surrogate == 0)
                    return; // shouldn't happen
                
                uint16_t high = control->char_press_high_surrogate;
                uint16_t low = (uint16_t)wParam;
                char_info.codepoint = utf16_to_utf32_codepoint(high, low);
                control->char_press_high_surrogate = 0;
            } else {
                char_info.codepoint = (uint32_t)wParam;
                control->char_press_high_surrogate = 0;
            }

            wtk_event_control_trigger(eventable, WTK_EVENT_CHAR_PRESS, &char_info);
            return;
        }
        case WM_LBUTTONDOWN:
        case WM_LBUTTONUP:
        case WM_RBUTTONDOWN:
        case WM_RBUTTONUP:
        case WM_MBUTTONDOWN:
        case WM_MBUTTONUP:
        case WM_XBUTTONDOWN:
        case WM_XBUTTONUP:
        case WM_MOUSEMOVE: {
            bool is_down = message == WM_LBUTTONDOWN || message == WM_RBUTTONDOWN
                || message == WM_MBUTTONDOWN || message == WM_XBUTTONDOWN;
            if (is_down) {
                SetFocus(hWnd);
            }

            WtkMouseButton wtk_mouse_btn;
            switch (message) {
                case WM_LBUTTONDOWN:
                case WM_LBUTTONUP:
                    wtk_mouse_btn = WTK_MOUSE_LEFT;
                    break;
                case WM_RBUTTONDOWN:
                case WM_RBUTTONUP:
                    wtk_mouse_btn = WTK_MOUSE_RIGHT;
                    break;
                case WM_MBUTTONDOWN:
                case WM_MBUTTONUP:
                    wtk_mouse_btn = WTK_MOUSE_MIDDLE;
                    break;
                case WM_MOUSEMOVE:
                    // which mouse button is down is not in the event, so this is the best we can do
                    wtk_mouse_btn = wtk_input_mouse_btn_win32_to_wtk((unsigned int)wParam);
                    break;
                default:
                    // todo: don't support xbuttons yet
                    wtk_mouse_btn = WTK_MOUSE_LEFT;
                    break;
            }

            WtkEventType wtk_event_type;
            switch (message) {
                case WM_LBUTTONDOWN:
                case WM_RBUTTONDOWN:
                case WM_MBUTTONDOWN:
                case WM_XBUTTONDOWN:
                    wtk_event_type = WTK_EVENT_MOUSE_DOWN;
                    break;
                case WM_LBUTTONUP:
                case WM_RBUTTONUP:
                case WM_MBUTTONUP:
                case WM_XBUTTONUP:
                    wtk_event_type = WTK_EVENT_MOUSE_UP;
                    break;
                default:
                    wtk_event_type = WTK_EVENT_MOUSE_MOVE;
                    break;
            }

            WtkMouseInfo mouse_info;
            mouse_info.button = wtk_mouse_btn;
            mouse_info.x = LOWORD(lParam);
            mouse_info.y = HIWORD(lParam);
            mouse_info.ctrl_held = (GetKeyState(VK_CONTROL) & 0x8000) != 0;
            mouse_info.shift_held = (GetKeyState(VK_SHIFT) & 0x8000) != 0;
            mouse_info.alt_held = (GetKeyState(VK_MENU) & 0x8000) != 0;

            if (wtk_event_type == WTK_EVENT_MOUSE_DOWN) {
                wtk_captured_hwnd = hWnd;
                wtk_captured_button = wtk_mouse_btn;
                SetCapture(hWnd);
            }

            wtk_event_control_trigger(eventable, wtk_event_type, &mouse_info);

            if (wtk_event_type == WTK_EVENT_MOUSE_UP && wtk_captured_hwnd == hWnd && wtk_captured_button == wtk_mouse_btn) {
                wtk_captured_hwnd = NULL;
                wtk_captured_button = WTK_MOUSE_NONE;
                ReleaseCapture();
            }
            return;
        }
        case WM_CAPTURECHANGED: {
            // todo: this is so bad. need to tear this out and redo it eventually.
            if (wtk_captured_hwnd != hWnd) {
                return;
            }

            WtkMouseButton stolen_button = wtk_captured_button;
            wtk_captured_hwnd = NULL;
            wtk_captured_button = WTK_MOUSE_NONE;

            POINT pt;
            GetCursorPos(&pt);
            ScreenToClient(hWnd, &pt);

            WtkMouseInfo mouse_info;
            mouse_info.button = stolen_button;
            mouse_info.x = pt.x;
            mouse_info.y = pt.y;
            mouse_info.ctrl_held = (GetKeyState(VK_CONTROL) & 0x8000) != 0;
            mouse_info.shift_held = (GetKeyState(VK_SHIFT) & 0x8000) != 0;
            mouse_info.alt_held = (GetKeyState(VK_MENU) & 0x8000) != 0;
            wtk_event_control_trigger(eventable, WTK_EVENT_MOUSE_UP, &mouse_info);
            return;
        }
        case WM_MOUSEWHEEL:
        case WM_MOUSEHWHEEL: {
            WtkScrollInfo scroll_info;
            scroll_info.value = (int)(short)HIWORD(wParam);
            scroll_info.direction = message == WM_MOUSEWHEEL
                ? WTK_SCROLLDIR_VERTICAL
                : WTK_SCROLLDIR_HORIZONTAL;

            wtk_event_control_trigger(eventable, WTK_EVENT_MOUSE_SCROLL, &scroll_info);
            return;
        }
    }
}
#endif // wtk_win32

WtkWindow* wtk_create_window(int width, int height, const char* title)
{
    WtkWindow* win_ctrl = wtk_create_blank_window();
    WtkControl* ctrl = &win_ctrl->b;
    ctrl->x = 0;
    ctrl->y = 0;
    ctrl->w = width;
    ctrl->h = height;

#if wtk_win32
    RECT rt = { 0, 0, width, height };
    AdjustWindowRect(&rt, WS_OVERLAPPEDWINDOW, FALSE);
    int aw = rt.right - rt.left;
    int ah = rt.bottom - rt.top;

    int x = DEFAULT_WINDOW_LOCATION;
    int y = DEFAULT_WINDOW_LOCATION;

    LPWSTR title_wstr = wtk_utf8_to_lpwstr(title);
    // todo: clip children???
    ctrl->con_hnd = CreateWindow(win32_inf.wc.lpszClassName, title_wstr,
        WS_OVERLAPPEDWINDOW | WS_VISIBLE | WS_CLIPCHILDREN, x, y, aw, 1, NULL, NULL, win32_inf.h_instance, NULL);

    wtk_free(title_wstr);

    SetWindowLongPtr(ctrl->con_hnd, GWLP_USERDATA, (LONG_PTR)ctrl);
    SetWindowPos(ctrl->con_hnd, HWND_TOP, x, y, aw, ah, SWP_NOMOVE);
    SendMessage(ctrl->con_hnd, WM_SETFONT, (WPARAM)win32_inf.stock_font, 1);

    win32_inf.windows_open++;
#endif // wtk_win32

    return win_ctrl;
}

WtkControl* wtk_create_button(WtkControl* parent, int x, int y, int width, int height, const char* text)
{
    if (parent == NULL) {
        return NULL;
    }

    WtkControl* ctrl = wtk_create_blank_control(WTK_CONTROL_BUTTON);
    ctrl->x = x;
    ctrl->y = y;
    ctrl->w = width;
    ctrl->h = height;
    ctrl->parent = parent;

#if wtk_win32
    LPWSTR text_wstr = wtk_utf8_to_lpwstr(text);
    ctrl->con_hnd = CreateWindow(WC_BUTTON, text_wstr,
        WS_TABSTOP | WS_VISIBLE | WS_CHILD, x, y, width, height, parent->con_hnd, NULL, win32_inf.h_instance, NULL);

    wtk_free(text_wstr);

    SetWindowLongPtr(ctrl->con_hnd, GWLP_USERDATA, (LONG_PTR)ctrl);
    SendMessage(ctrl->con_hnd, WM_SETFONT, (WPARAM)win32_inf.stock_font, 1);
#endif // wtk_win32

    wtkvec_push(ctrl->parent->children, ctrl);
    wtk_recalculate_anchor(ctrl);

    return ctrl;
}

WtkControl* wtk_create_label(WtkControl* parent, int x, int y, int width, int height, const char* text)
{
    return NULL;
}

WtkControl* wtk_create_textbox(WtkControl* parent, int x, int y, int width, int height, const char* text)
{
    return NULL;
}

void wtk_window_set_title(WtkWindow* window, const char* title)
{
#if wtk_win32
    LPWSTR title_wstr = wtk_utf8_to_lpwstr(title);
    SetWindowText(window->b.con_hnd, title_wstr);
    wtk_free(title_wstr);
#endif // wtk_win32
}

// this api is very win32-ish. should we make WtkImage also
// handle icons with multiple sizes (load with LoadIcon?)
void wtk_window_set_icon(WtkWindow* window, WtkImage* large_icon, WtkImage* small_icon)
{
    if (window->large_icon != NULL) {
        wtk_image_unload(window->large_icon);
    }
    if (window->small_icon != NULL) {
        wtk_image_unload(window->small_icon);
    }
    window->large_icon = large_icon;
    window->small_icon = small_icon;

#if wtk_win32
    SendMessage(window->b.con_hnd, WM_SETICON, ICON_BIG, (LPARAM)large_icon->icon);
    SendMessage(window->b.con_hnd, WM_SETICON, ICON_SMALL, (LPARAM)small_icon->icon);
#endif // wtk_win32
}

bool wtk_control_focus(WtkControl* control)
{
#if wtk_win32
    if (control->con_hnd != NULL) {
        return SetFocus(control->con_hnd) != NULL;
    }

    return false;
#endif // wtk_win32
}

void wtk_set_anchor(WtkControl* control, WtkAnchorType anchor)
{
    if (control == NULL) {
        return;
    }

    control->anchor = anchor;
    wtk_recalculate_anchor(control);
}

void wtk_set_scrollbar(WtkControl* control, WtkScrollDir scroll_dir)
{
    if (control == NULL) {
        return;
    }

    bool is_vertical = (scroll_dir & WTK_SCROLLDIR_VERTICAL) != 0;
    bool is_horizontal = (scroll_dir & WTK_SCROLLDIR_HORIZONTAL) != 0;

    bool scroll_enabled = is_vertical || is_horizontal;
    control->scroll_flags = scroll_enabled
        ? (control->scroll_flags | WTK_SCROLL_ENABLED)
        : (control->scroll_flags & ~WTK_SCROLL_ENABLED);

#if wtk_win32
    ShowScrollBar(control->con_hnd, SB_VERT, is_vertical);
    ShowScrollBar(control->con_hnd, SB_HORZ, is_horizontal);
#endif // wtk_win32
}

void wtk_set_scrollbar_range(WtkControl* control, WtkScrollDir scroll_dir, int min, int max, int page)
{
    if (control == NULL) {
        return;
    }

    bool is_vertical = false;
    if ((scroll_dir & WTK_SCROLLDIR_VERTICAL) != 0) {
        is_vertical = true;
    } else if ((scroll_dir & WTK_SCROLLDIR_HORIZONTAL) == 0) {
        // invalid direction
        return;
    }

#if wtk_win32
    SCROLLINFO si = { sizeof(SCROLLINFO) };
    si.fMask = SIF_RANGE | SIF_PAGE;
    si.nMin = min;
    si.nMax = max;
    si.nPage = page;
    SetScrollInfo(control->con_hnd, is_vertical ? SB_VERT : SB_HORZ, &si, TRUE);
#endif // wtk_win32
}

void wtk_set_scrollbar_pos(WtkControl* control, WtkScrollDir scroll_dir, int pos)
{
    if (control == NULL) {
        return;
    }

    bool is_vertical = false;
    if ((scroll_dir & WTK_SCROLLDIR_VERTICAL) != 0) {
        is_vertical = true;
    } else if ((scroll_dir & WTK_SCROLLDIR_HORIZONTAL) == 0) {
        // invalid direction
        return;
    }

#if wtk_win32
    SCROLLINFO si = { sizeof(SCROLLINFO) };
    si.fMask = SIF_POS;
    si.nPos = pos;
    SetScrollInfo(control->con_hnd, is_vertical ? SB_VERT : SB_HORZ, &si, TRUE);
#endif // wtk_win32
}

// ///////////////////////////////////////////////////////// end wtk controls //

// wtk custom controls /////////////////////////////////////////////////////////
#if wtk_win32
void wtk_draw_callback_wrapper(WtkControl* control, WtkEvent* event, HWND hwnd)
{
    WtkDrawingContext ctx;

    RECT client_rect;
    PAINTSTRUCT ps;
    HDC hdc;
    HDC hdc_mem;
    HBITMAP hbm_mem;
    int width, height;

    GetClientRect(hwnd, &client_rect);
    width = client_rect.right - client_rect.left;
    height = client_rect.bottom - client_rect.top;

    hdc = BeginPaint(hwnd, &ps);
    hdc_mem = CreateCompatibleDC(hdc);
    hbm_mem = CreateCompatibleBitmap(hdc, width, height);

    HANDLE old_hnd = SelectObject(hdc_mem, hbm_mem);

    SetBkMode(hdc_mem, TRANSPARENT);

    ctx.drawer = hdc_mem;
    ctx.client_rect = client_rect;
    ctx.brush = NULL;
    ctx.font = NULL;
    event->callback(control, &ctx);

    BitBlt(hdc, 0, 0, width, height, hdc_mem, 0, 0, SRCCOPY);

    if (ctx.brush != NULL) {
        DeleteObject(ctx.brush);
    }
    if (ctx.font != NULL) {
        DeleteObject(ctx.font);
    }

    SelectObject(hdc_mem, old_hnd);

    DeleteObject(hbm_mem);
    DeleteDC(hdc_mem);
    EndPaint(hwnd, &ps);
}

LRESULT wtk_custom_process_event(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uMsg) {
        case WM_PAINT: {
            LONG_PTR user_data = GetWindowLongPtr(hwnd, GWLP_USERDATA);
            if (user_data == 0)
                return 0;

            WtkControl* control = (WtkControl*)user_data;
            WtkEventable* eventable = &control->b;
            for (wtkvec_each(eventable->events, WtkEvent, evt)) {
                if (evt->event_type == WTK_EVENT_PAINT && evt->callback != NULL) {
                    wtk_draw_callback_wrapper(control, evt, hwnd);
                    break;
                }
            }

            return 0;
        }
        case WM_VSCROLL:
        case WM_HSCROLL: {
            LONG_PTR user_data = GetWindowLongPtr(hwnd, GWLP_USERDATA);
            if (user_data == 0)
                break;

            bool is_vertical = uMsg == WM_VSCROLL;
            int w32_bar_dir = is_vertical ? SB_VERT : SB_HORZ;

            SCROLLINFO si = { sizeof(SCROLLINFO), SIF_ALL };
            GetScrollInfo(hwnd, w32_bar_dir, &si);

            int pos = si.nPos;
            switch (LOWORD(wParam)) {
                case SB_LINEUP:
                    pos -= DEFAULT_SCROLL_LINE_JUMP; break;
                case SB_LINEDOWN:
                    pos += DEFAULT_SCROLL_LINE_JUMP; break;
                case SB_PAGEUP:
                    pos -= si.nPage; break;
                case SB_PAGEDOWN:
                    pos += si.nPage; break;
                case SB_THUMBTRACK:
                    pos = si.nTrackPos; break;
            }
            pos = wtk_max(si.nMin, wtk_min(pos, si.nMax - (int)si.nPage + 1)); // todo: ???

            si.fMask = SIF_POS;
            si.nPos = pos;
            SetScrollInfo(hwnd, w32_bar_dir, &si, TRUE);

            WtkControl* control = (WtkControl*)user_data;
            WtkEventable* eventable = &control->b;

            WtkScrollInfo scroll_info;
            scroll_info.direction = is_vertical ? WTK_SCROLLDIR_VERTICAL : WTK_SCROLLDIR_HORIZONTAL;
            scroll_info.value = pos;
            wtk_event_control_trigger(eventable, WTK_EVENT_SCROLL, &scroll_info);

            // ScrollWindowEx(hwnd, 0, old_pos - pos, NULL, NULL, NULL, NULL, SW_INVALIDATE);
            break;
        }
        case WM_KEYDOWN:
        case WM_KEYUP:
        case WM_CHAR:
        case WM_LBUTTONDOWN:
        case WM_LBUTTONUP:
        case WM_RBUTTONDOWN:
        case WM_RBUTTONUP:
        case WM_MBUTTONDOWN:
        case WM_MBUTTONUP:
        case WM_XBUTTONDOWN:
        case WM_XBUTTONUP:
        case WM_MOUSEMOVE:
        case WM_CAPTURECHANGED:
        case WM_MOUSEWHEEL:
        case WM_MOUSEHWHEEL: {
            LONG_PTR user_data = GetWindowLongPtr(hwnd, GWLP_USERDATA);
            WtkControl* control = (WtkControl*)user_data;
            wtk_process_common_input(control, hwnd, uMsg, wParam, lParam);
            break;
        }
        case WM_ERASEBKGND: {
            return 1;
        }
    }
    return DefWindowProc(hwnd, uMsg, wParam, lParam);
}
#endif // wtk_win32

WtkControl* wtk_create_custom(WtkControl* parent, int x, int y, int width, int height, int struct_size, WtkControlInfo* control_info)
{
    if (parent == NULL || struct_size < sizeof(WtkControl)) {
        return NULL;
    }

    WtkControl* ctrl = wtk_create_blank_control_sized(WTK_CONTROL_CUSTOM, struct_size);
    ctrl->x = x;
    ctrl->y = y;
    ctrl->w = width;
    ctrl->h = height;
    ctrl->parent = parent;

#if wtk_win32
    control_info->register_fn();
    ctrl->con_hnd = CreateWindow(control_info->type_name, L"",
        WS_VISIBLE | WS_CHILD, x, y, width, height, parent->con_hnd, NULL, win32_inf.h_instance, NULL);

    SetWindowLongPtr(ctrl->con_hnd, GWLP_USERDATA, (LONG_PTR)ctrl);
    SendMessage(ctrl->con_hnd, WM_SETFONT, (WPARAM)win32_inf.stock_font, 1);
#endif // wtk_win32

    wtkvec_push(ctrl->parent->children, ctrl);
    wtk_recalculate_anchor(ctrl);

    return ctrl;
}

void wtk_draw_redraw(WtkControl* control)
{
#if wtk_win32
    InvalidateRect(control->con_hnd, NULL, 0);
    //UpdateWindow(control->con_hnd);
#endif // wtk_win32
}

void wtk_draw_rectangle(WtkDrawingContext* ctx, int x, int y, int w, int h)
{
    RECT rect;
    rect.left = x;
    rect.top = y;
    rect.right = x + w;
    rect.bottom = y + h;

    FillRect(ctx->drawer, &rect, ctx->brush);
}

void wtk_draw_text(WtkDrawingContext* ctx, char* text, int x, int y)
{
    RECT rect;
    rect.left = x;
    rect.top = y;
    rect.right = ctx->client_rect.right;
    rect.bottom = ctx->client_rect.bottom;

    HFONT old_font = (HFONT)SelectObject(ctx->drawer, ctx->font);

    LPWSTR text_wstr = wtk_utf8_to_lpwstr(text);
    DrawText(ctx->drawer, text_wstr, -1, &rect, DT_SINGLELINE | DT_NOCLIP | DT_NOPREFIX);
    wtk_free(text_wstr);

    SelectObject(ctx->drawer, old_font);
}

void wtk_draw_set_color(WtkDrawingContext* ctx, int r, int g, int b)
{
#if wtk_win32
    SetTextColor(ctx->drawer, RGB(r, g, b));
    // docs state no harm is done by calling DeleteObject on a system brush,
    // so I'm going to leave this as-is and not track custom/system brushes
    if (ctx->brush != NULL)
        DeleteObject(ctx->brush);

    ctx->brush = CreateSolidBrush(RGB(r, g, b));
#endif // wtk_win32
}

void wtk_draw_set_sys_color(WtkDrawingContext* ctx, WtkDrawingSysColor color)
{
#if wtk_win32
    HBRUSH brush = NULL;
    switch (color) {
    case WTK_SYS_COLOR_TEXT_COLOR:
        brush = GetSysColorBrush(COLOR_WINDOWTEXT);
    case WTK_SYS_COLOR_WINDOW_BACKGROUND:
        brush = GetSysColorBrush(COLOR_WINDOW);
    }

    if (brush != NULL) {
        DeleteObject(ctx->brush);
        ctx->brush = brush;
    }
#endif // wtk_win32
}

void wtk_draw_set_font(WtkDrawingContext* ctx, char* font_face, int font_size, bool bold)
{
    if (ctx == NULL || font_face == NULL) {
        return;
    }

#if wtk_win32
    // todo: reuse existing font if it hasn't changed

    LPWSTR font_face_wstr = wtk_utf8_to_lpwstr(font_face);
    HFONT new_font = CreateFont(-font_size, 0, 0, 0,
        bold == 1 ? FW_BOLD : FW_DONTCARE, FALSE, FALSE, FALSE, DEFAULT_CHARSET,
        OUT_OUTLINE_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY, VARIABLE_PITCH, font_face_wstr);

    wtk_free(font_face_wstr);

    if (ctx->font != NULL) {
        DeleteObject(ctx->font);
    }
    ctx->font = new_font;
#endif // wtk_win32
}

void wtk_draw_measure_text(WtkDrawingContext* ctx, char* text, int* out_w, int* out_h)
{
    if (ctx == NULL || text == NULL || text[0] == '\0') {
        if (out_w != NULL)
            *out_w = 0;
        if (out_h != NULL)
            *out_h = 0;

        return;
    }

    bool one_char = text[1] == '\0';

#if wtk_win32
    SIZE size;
    HFONT old_font = (HFONT)SelectObject(ctx->drawer, ctx->font);

    if (one_char) {
        // opto path
        wchar_t text_wstr[2] = { (wchar_t)text[0], L'\0' };
        GetTextExtentPoint32W(ctx->drawer, text_wstr, 1, &size);
    } else {
        LPWSTR text_wstr = wtk_utf8_to_lpwstr(text);
        GetTextExtentPoint32W(ctx->drawer, text_wstr, (int)wcslen(text_wstr), &size);
        wtk_free(text_wstr);
    }

    SelectObject(ctx->drawer, old_font);

    if (out_w != NULL)
        *out_w = size.cx;
    if (out_h != NULL)
        *out_h = size.cy;
#endif // wtk_win32
}
// ///////////////////////////////////////////////////// end wtk custom controls 

// wtk menus ///////////////////////////////////////////////////////////////////
WtkMenuBar* wtk_create_menu_bar(WtkWindow* window)
{
    if (window == NULL) {
        return NULL;
    }

    WtkMenuBar* ctrl = wtk_malloc(WtkMenuBar*, sizeof(WtkMenuBar));
    ctrl->window = window;
    ctrl->bar_hnd = CreateMenu();
    SetMenu(window->b.con_hnd, ctrl->bar_hnd);
    return ctrl;
}

WtkMenu* wtk_create_menu(WtkMenuBar* menu_bar, const char* text)
{
    if (menu_bar == NULL) {
        return NULL;
    }

    WtkMenu* ctrl = wtk_malloc(WtkMenu*, sizeof(WtkMenu));
    ctrl->menu_bar = menu_bar;
    ctrl->children = NULL;
    wtkvec_new(ctrl->children, 4, NULL); // todo: non-null destructor

#if wtk_win32
    ctrl->child_hnd = CreateMenu();

    LPWSTR text_wstr = wtk_utf8_to_lpwstr(text);
    AppendMenu(menu_bar->bar_hnd, MF_POPUP, (UINT_PTR)ctrl->child_hnd, text_wstr);
    wtk_free(text_wstr);

    MENUINFO menu_info;
    menu_info.cbSize = sizeof(MENUINFO);
    // menu_info.dwStyle = MNS_NOTIFYBYPOS;
    menu_info.fMask = MIM_STYLE;
    SetMenuInfo(menu_bar->bar_hnd, &menu_info);

    menu_info.cbSize = sizeof(MENUINFO);
    menu_info.dwMenuData = (ULONG_PTR)ctrl;
    menu_info.fMask = MIM_MENUDATA;
    SetMenuInfo(ctrl->child_hnd, &menu_info);
#endif // wtk_win32

    return ctrl;
}

WtkMenuItem* wtk_create_menu_item(WtkMenu* menu, WtkMenuItemType item_type, const char* text)
{
    if (menu == NULL) {
        return NULL;
    }

    WtkMenuBar* menu_bar = menu->menu_bar;

    WtkMenuItem* menu_item = wtk_malloc(WtkMenuItem*, sizeof(WtkMenuItem));
    menu_item->menu = menu;
    menu_item->b.events = NULL;
    menu_item->b.user_data = NULL;
    menu_item->item_type = item_type;
    wtkvec_new(menu_item->b.events, 4, NULL); // todo: non-null destructor
    wtkvec_push(menu->children, menu_item);

#if wtk_win32
    LPWSTR text_wstr = wtk_utf8_to_lpwstr(text);
    switch (item_type) {
        case WTK_MENU_TEXT: {
            // need to grab a new id from the window
            WtkWindow* window = (WtkWindow*)menu->menu_bar->window;
            int next_id = wtk_idpool_get(&window->id_pool);

            // while (wtkvec_elem_count(window->id_to_eventable) <= next_id) {
            //     wtkvec_push(window->id_to_eventable, NULL);
            // }
            // window->id_to_eventable[next_id] = &menu_item->b;
            wtk_event_vec_insert(window->id_to_eventable, next_id, &menu_item->b);

            AppendMenu(menu->child_hnd, MF_STRING, (UINT)next_id, text_wstr);
            break;
        }
        case WTK_MENU_SEPARATOR: {
            AppendMenu(menu->child_hnd, MF_SEPARATOR, (UINT)0, text_wstr);
            break;
        }
    }
    wtk_free(text_wstr);

    menu_item->item_hnd = CreateMenu();
    menu_item->cmd_id = 0;

    SetMenu(menu_bar->window->b.con_hnd, menu_bar->bar_hnd);
#endif // wtk_win32

    return menu_item;
}

WtkToolbar* wtk_create_toolbar(WtkWindow* window, int icon_dims)
{
    if (window == NULL) {
        return NULL;
    }

    if (icon_dims <= 0) {
        icon_dims = DEFAULT_TOOLBAR_ICON_SIZE;
    }

    WtkToolbar* ctrl = wtk_malloc(WtkToolbar*, sizeof(WtkToolbar));
    ctrl->window = window;

#if wtk_win32
    wtk_idpool_new(&ctrl->image_id_pool, 0);

    ctrl->bar_hnd = CreateWindow(TOOLBARCLASSNAME, NULL,
        WS_CHILD | TBSTYLE_WRAPABLE | TBSTYLE_TOOLTIPS | TBSTYLE_FLAT,
        0, 0, 0, 0, window->b.con_hnd, NULL, win32_inf.h_instance, NULL);

    ctrl->image_list = ImageList_Create(icon_dims, icon_dims,
        ILC_COLOR32, 1, 8);

    SetWindowLongPtr(ctrl->bar_hnd, GWLP_USERDATA, (LONG_PTR)ctrl);
    SendMessage(ctrl->bar_hnd, TB_SETIMAGELIST, (WPARAM)0, (LPARAM)ctrl->image_list);
    SendMessage(ctrl->bar_hnd, WM_SETFONT, (WPARAM)win32_inf.stock_font, 1);
    SendMessage(ctrl->bar_hnd, TB_BUTTONSTRUCTSIZE, (WPARAM)sizeof(TBBUTTON), 0);
    SendMessage(ctrl->bar_hnd, TB_SETMAXTEXTROWS, 0, 0);
    ShowWindow(ctrl->bar_hnd, TRUE);
#endif // wtk_win32

    ctrl->window->toolbar = ctrl;
    return ctrl;
}

WtkToolbarItem* wtk_create_toolbar_item(WtkToolbar* toolbar, const char* tooltip, WtkImage* icon)
{
    if (toolbar == NULL) {
        return NULL;
    }

    WtkToolbarItem* item = wtk_malloc(WtkToolbarItem*, sizeof(WtkToolbarItem));
    item->b.events = NULL;
    item->b.user_data = NULL;
    item->toolbar = toolbar;
    item->item_type = WTK_TOOLBAR_BUTTON;
    item->tooltip = wtk_strdup(tooltip); // should we forget the tooltip value?
    item->icon = icon;
    wtkvec_new(item->b.events, 4, NULL); // todo: non-null destructor

#if wtk_win32
    WtkWindow* window = toolbar->window;
    int image_id = wtk_idpool_get(&toolbar->image_id_pool);
    int next_command_id = wtk_idpool_get(&toolbar->window->id_pool);

    // todo: make function
    // while (wtkvec_elem_count(window->id_to_eventable) <= next_command_id) {
    //     wtkvec_push(window->id_to_eventable, NULL);
    // }
    // window->id_to_eventable[next_command_id] = &item->b;
    wtk_event_vec_insert(window->id_to_eventable, next_command_id, &item->b);

    LPWSTR tooltip_wstr = wtk_utf8_to_lpwstr(tooltip);

    ImageList_AddIcon(toolbar->image_list, icon->icon);

    TBBUTTON btn = { 0 };
    btn.iBitmap = image_id;
    btn.idCommand = next_command_id;
    btn.fsState = TBSTATE_ENABLED;
    btn.fsStyle = BTNS_BUTTON;
    btn.iString = (INT_PTR)tooltip_wstr;

    SendMessage(toolbar->bar_hnd, TB_ADDBUTTONS, 1, (LPARAM)&btn);
    SendMessage(toolbar->bar_hnd, TB_AUTOSIZE, 0, 0);
#endif // wtk_win32

    return item;
}

WtkToolbarItem* wtk_create_toolbar_separator(WtkToolbar* toolbar)
{
    return NULL; // todo
}
// //////////////////////////////////////////////////////////// end wtk menus //

// wtk events //////////////////////////////////////////////////////////////////
static void wtk_event_control_trigger(WtkEventable* eventable, WtkEventType event_type, void* param)
{
    for (wtkvec_each(eventable->events, WtkEvent, evt)) {
        if (evt->event_type == event_type && evt->callback != NULL) {
            evt->callback(eventable, param);
            break;
        }
    }
}

// todo: should this be publicly exposed in header?
bool wtk_event_control_supports(WtkControlType control_type, WtkEventType event_type)
{
    switch (control_type) {
        case WTK_CONTROL_BUTTON:
            return event_type == WTK_EVENT_CLICK || event_type == WTK_EVENT_FOCUS
                || event_type == WTK_EVENT_FOCUS_LOST;
        case WTK_CONTROL_CUSTOM:
            return event_type == WTK_EVENT_PAINT || event_type == WTK_EVENT_SCROLL
                || event_type == WTK_EVENT_MOUSE_DOWN || event_type == WTK_EVENT_MOUSE_UP
                || event_type == WTK_EVENT_MOUSE_MOVE || event_type == WTK_EVENT_MOUSE_SCROLL
                || event_type == WTK_EVENT_KEY_DOWN || event_type == WTK_EVENT_KEY_UP
                || event_type == WTK_EVENT_CHAR_PRESS;
        case WTK_CONTROL_WINDOW:
            return event_type == WTK_EVENT_MOUSE_DOWN || event_type == WTK_EVENT_MOUSE_UP
                || event_type == WTK_EVENT_MOUSE_MOVE || event_type == WTK_EVENT_MOUSE_SCROLL
                || event_type == WTK_EVENT_KEY_DOWN || event_type == WTK_EVENT_KEY_UP
                || event_type == WTK_EVENT_CHAR_PRESS;
        default:
            return false;
    }
}

bool wtk_event_control_add(WtkControl* control, WtkEventType event_type, WtkCallback callback)
{
    if (!wtk_event_control_supports(control->con_type, event_type)) {
        return false;
    }

    WtkEvent event = { event_type, callback };
    wtkvec_push(control->b.events, event);
    return true;
}

bool wtk_event_menu_item_add(WtkMenuItem* control, WtkEventType event_type, WtkCallback callback)
{
    if (event_type != WTK_EVENT_CLICK) {
        return false;
    }

    WtkEvent event = { event_type, callback };
    wtkvec_push(control->b.events, event);
    return true;
}

bool wtk_event_toolbar_item_add(WtkToolbarItem* control, WtkEventType event_type, WtkCallback callback)
{
    if (event_type != WTK_EVENT_CLICK) {
        return false;
    }

    WtkEvent event = { event_type, callback };
    wtkvec_push(control->b.events, event);
    return true;
}

bool wtk_event_timer_add(WtkTimer* timer, WtkEventType event_type, WtkCallback callback)
{
    if (event_type != WTK_EVENT_UITIMER) {
        return false;
    }

    WtkEvent event = { event_type, callback };
    wtkvec_push(timer->b.events, event);
    return true;
}
// /////////////////////////////////////////////////////////// end wtk events //

// wtk assets //////////////////////////////////////////////////////////////////
WtkImage* wtk_image_load_resource_icon(WtkResourceId resource_id, int icon_dims)
{
#if wtk_win32
    WtkImage* image = wtk_malloc(WtkImage*, sizeof(WtkImage));
    HICON icon = (HICON)LoadImage(
        win32_inf.h_instance,
        MAKEINTRESOURCE(resource_id),
        IMAGE_ICON,
        icon_dims, icon_dims,
        LR_DEFAULTCOLOR | LR_LOADTRANSPARENT);

    image->source = WTK_IMAGESRC_RESOURCE_ICON;
    image->icon = icon;
    image->width = icon_dims;
    image->height = icon_dims;

    //HBITMAP hbitmap = (HBITMAP)LoadImage(
    //    win32_inf.h_instance, MAKEINTRESOURCE(resource_id),
    //    IMAGE_BITMAP, 0, 0, LR_CREATEDIBSECTION
    //);

    //BITMAP bitmap;
    //GetObject(hbitmap, (int)sizeof(bitmap), &bitmap);

    //int width = bitmap.bmWidth;
    //int height = bitmap.bmHeight;

    //image->source = WTK_IMAGESRC_RESOURCE_ICON;
    //image->bitmap = hbitmap;
    //image->width = width;
    //image->height = height;
#endif // wtk_win32

    return image;
}

void wtk_image_unload(WtkImage* image)
{
#if wtk_win32
    FreeResource(image->bitmap);
#endif // wtk_win32

    free(image);
}
// /////////////////////////////////////////////////////////// end wtk assets //

// wtk popup boxes /////////////////////////////////////////////////////////////
WtkMbResponse wtk_messagebox_show(const char* title, const char* text, WtkMbIcon icon, WtkMbButtons buttons)
{
#if wtk_win32
    int flags = 0;
    switch (icon) {
        case WTK_MBICON_INFO: flags |= MB_ICONINFORMATION; break;
        case WTK_MBICON_WARNING: flags |= MB_ICONWARNING; break;
        case WTK_MBICON_QUESTION: flags |= MB_ICONQUESTION; break;
        case WTK_MBICON_ERROR: flags |= MB_ICONERROR; break;
    }

    switch (buttons) {
        case WTK_MBBUTTONS_OK: flags |= MB_OK; break;
        case WTK_MBBUTTONS_OK_CANCEL: flags |= MB_OKCANCEL; break;
        case WTK_MBBUTTONS_YES_NO: flags |= MB_YESNO; break;
        case WTK_MBBUTTONS_YES_NO_CANCEL: flags |= MB_YESNOCANCEL; break;
    }

    LPWSTR title_wstr = wtk_utf8_to_lpwstr(title);
    LPWSTR text_wstr = wtk_utf8_to_lpwstr(text);
    int response = MessageBox(NULL, text_wstr, title_wstr, flags);
    wtk_free(title_wstr);
    wtk_free(text_wstr);

    switch (response) {
        case IDOK: return WTK_MBRESPONSE_OK;
        case IDCANCEL: return WTK_MBRESPONSE_CANCEL;
        case IDYES: return WTK_MBRESPONSE_YES;
        case IDNO: return WTK_MBRESPONSE_NO;
        default: return WTK_MBRESPONSE_NONE;
    }
#endif // wtk_win32
}

void wtk_simple_messagebox_show(const char* title, const char* text)
{
    wtk_messagebox_show(title, text, WTK_MBICON_NONE, WTK_MBBUTTONS_OK);
}

char* wtk_open_file_dialog_show(const char* title)
{
#if wtk_win32
    // todo: only supported on vista+
    // future me: this function used to use the old open file dialog
    // back in 2020 when this was written, but was replaced with
    // IFileOpenDialog shortly after. we should probably both:
    // - not use FileOpenDialog if it's not supported and
    // - use COBJMACROS so this works across C and C++
    HRESULT hr;

    hr = CoInitializeEx(NULL, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE);
    if (!SUCCEEDED(hr)) {
        return NULL;
    }

    char* file_path_utf8 = NULL;

    IFileOpenDialog* file_dlg;
    hr = CoCreateInstance(
        &CLSID_FileOpenDialog, NULL, CLSCTX_ALL,
        &IID_IFileOpenDialog, (void**)&file_dlg
    );
    if (SUCCEEDED(hr)) {
        LPWSTR title_wstr = wtk_utf8_to_lpwstr(title);
        hr = file_dlg->lpVtbl->SetTitle(file_dlg, title_wstr);
        wtk_free(title_wstr);
        if (SUCCEEDED(hr)) {
            hr = file_dlg->lpVtbl->Show(file_dlg, NULL);
            if (SUCCEEDED(hr)) {
                IShellItem* shell_item;
                hr = file_dlg->lpVtbl->GetResult(file_dlg, &shell_item);
                if (SUCCEEDED(hr)) {
                    PWSTR file_path;
                    hr = shell_item->lpVtbl->GetDisplayName(shell_item, SIGDN_FILESYSPATH, &file_path);
                    if (SUCCEEDED(hr)) {
                        file_path_utf8 = wtk_lpcwstr_to_utf8(file_path);
                        CoTaskMemFree(file_path);
                    }
                }
            }
            file_dlg->lpVtbl->Release(file_dlg);
        }
    }
    CoUninitialize();
    return file_path_utf8;
#endif // wtk_win32
}
// ////////////////////////////////////////////////////// end wtk popup boxes //

// wtk timers //////////////////////////////////////////////////////////////////
#if wtk_win32
LRESULT CALLBACK TimerWndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    if (message == WM_TIMER && win32_inf.timers_setup) {
        int command_id = LOWORD(wParam);
        // if (command_id < wtkvec_elem_count(win32_inf.timer_objs)) {
        //     WtkEventable* eventable = &win32_inf.timer_objs[command_id]->b;
        //     wtk_event_control_trigger(eventable, WTK_EVENT_UITIMER, NULL);
        // }
        wtk_event_vec_trigger(win32_inf.timer_objs, command_id, WTK_EVENT_UITIMER);
    }

    return DefWindowProc(hWnd, message, wParam, lParam);
}
#endif

WtkTimer* wtk_create_timer(WtkTimerType timer_type, int delay_ms)
{
    WtkTimer* timer = wtk_malloc(WtkTimer*, sizeof(WtkTimer));
    timer->b.events = NULL;
    timer->b.user_data = NULL;
    timer->timer_type = timer_type;
    timer->delay_ms = delay_ms;
    wtkvec_new(timer->b.events, 4, NULL); // todo: non-null destructor

#if wtk_win32
    if (!win32_inf.timers_setup) {
        win32_inf.timer_wc.lpfnWndProc = TimerWndProc;
        win32_inf.timer_wc.hInstance = win32_inf.h_instance;
        win32_inf.timer_wc.lpszClassName = L"WTKTimerWindow";

        if (!RegisterClassW(&win32_inf.timer_wc))
            return NULL; // shouldn't happen

        win32_inf.timer_win = CreateWindow(
            win32_inf.timer_wc.lpszClassName, NULL, 0,
            0, 0, 0, 0, HWND_MESSAGE, NULL, win32_inf.h_instance, NULL);
        
        wtk_idpool_new(&win32_inf.timer_pool, 1);
        win32_inf.timers_setup = true;
    }
#endif // wtk_win32

    int timer_id = wtk_idpool_get(&win32_inf.timer_pool);
    wtkvec_push(win32_inf.timer_objs, timer);

    return timer;
}
// /////////////////////////////////////////////////////////// end wtk timers //

// wtk control sizing //////////////////////////////////////////////////////////
static void wtk_compute_anchor_bounds(
    WtkControl* control, int parent_w, int parent_h,
    int* out_x, int* out_y, int* out_w, int* out_h)
{
    WtkAnchorType anchor = control->anchor;
    int left = control->anchor_info.left;
    int right = control->anchor_info.right;
    int top = control->anchor_info.top;
    int bottom = control->anchor_info.bottom;
    int cur_w = control->w;
    int cur_h = control->h;

    // x-axis
    if ((anchor & WTK_ANCHOR_LEFT) && (anchor & WTK_ANCHOR_RIGHT)) {
        *out_w = parent_w - left - right;
        *out_x = left;
    } else if (anchor & WTK_ANCHOR_RIGHT) {
        *out_w = cur_w;
        *out_x = parent_w - cur_w - right;
    } else if (anchor & WTK_ANCHOR_LEFT) {
        *out_w = cur_w;
        *out_x = left;
    } else {
        *out_w = cur_w;
        *out_x = (parent_w - cur_w) / 2;
    }

    // y-axis
    if ((anchor & WTK_ANCHOR_TOP) && (anchor & WTK_ANCHOR_BOTTOM)) {
        *out_h = parent_h - top - bottom;
        *out_y = top;
    } else if (anchor & WTK_ANCHOR_BOTTOM) {
        *out_h = cur_h;
        *out_y = parent_h - cur_h - bottom;
    } else if (anchor & WTK_ANCHOR_TOP) {
        *out_h = cur_h;
        *out_y = top;
    } else {
        *out_h = cur_h;
        *out_y = (parent_h - cur_h) / 2;
    }

#if wtk_win32
    const int MIN_WIDTH = 0;
    const int MIN_HEIGHT = 0;
#endif

    if (*out_w < MIN_WIDTH) {
        *out_w = MIN_WIDTH;
    }
    if (*out_h < MIN_HEIGHT) {
        *out_h = MIN_HEIGHT;
    }
}

#if wtk_win32
static void wtk_layout_controls(WtkControl* control, HDWP* hdwp)
#else
static void wtk_layout_controls(WtkControl* control)
#endif
{
    if (control == NULL) {
        return;
    }

    for (int i = 0; i < wtkvec_elem_count(control->children); i++) {
        WtkControl* child = control->children[i];
        if (child != NULL) {
            int new_x, new_y, new_w, new_h;
            wtk_compute_anchor_bounds(
                child, control->w, control->h,
                &new_x, &new_y, &new_w, &new_h
            );

            child->x = new_x;
            child->y = new_y;
            child->w = new_w;
            child->h = new_h;

#if wtk_win32
            *hdwp = DeferWindowPos(*hdwp, child->con_hnd, NULL, child->x, child->y, child->w, child->h,
                SWP_NOACTIVATE | SWP_NOOWNERZORDER | SWP_NOZORDER | SWP_NOREDRAW);

            wtk_layout_controls(child, hdwp);
#endif
        }
    }
}

// /////////////////////////////////////////////////// end wtk control sizing //

// event loop //////////////////////////////////////////////////////////////////

#if wtk_win32

void WndUpdateLoop()
{
    MSG msg = { 0 };
    while (GetMessage(&msg, NULL, 0, 0) > 0) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
}

LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message) {
        case WM_CLOSE: {
            DestroyWindow(hWnd);
            win32_inf.windows_open--;
            if (win32_inf.windows_open <= 0) {
                PostQuitMessage(0);
            }
            break;
        }
        case WM_SIZE: {
            WtkControl* control = (WtkControl*)GetWindowLongPtr(hWnd, GWLP_USERDATA);
            if (control == NULL || control->con_type != WTK_CONTROL_WINDOW) {
                break;
            }

            WtkWindow* window = (WtkWindow*)control;
            int width = LOWORD(lParam);
            int height = HIWORD(lParam);

            control->w = width;
            control->h = height;

            int defer_guess_size = 4;
            if (control->children != NULL) {
                // not accurate with recursive children, but all we need is a guess
                defer_guess_size = wtkvec_elem_count(control->children);
                // add a little extra space, then
                defer_guess_size = (defer_guess_size * 3) >> 1;
            }

            // resize all child controls
            HDWP hdwp = BeginDeferWindowPos(defer_guess_size);
            wtk_layout_controls(control, &hdwp);
            EndDeferWindowPos(hdwp);
            // InvalidateRect(control->con_hnd, NULL, 0);

            // toolbar's size does not resize automatically like the menubar
            if (window->toolbar != NULL) {
                SendMessage(window->toolbar->bar_hnd, TB_AUTOSIZE, 0, 0);
            }

            RedrawWindow(control->con_hnd, NULL, NULL, RDW_INVALIDATE | RDW_ALLCHILDREN);
            break;
        }
        case WM_COMMAND: {
            WtkControl* control = (WtkControl*)GetWindowLongPtr(hWnd, GWLP_USERDATA);
            if (control == NULL || control->con_type != WTK_CONTROL_WINDOW) {
                break;
            }

            WtkWindow* window = (WtkWindow*)control;
            HWND child_hwnd = (HWND)lParam;

            bool is_menu_bar = child_hwnd == NULL;
            bool is_toolbar = window->toolbar != NULL && window->toolbar->bar_hnd == child_hwnd;
            if (is_menu_bar || is_toolbar) {
                // menu items/toolbar items have no user data, so we use command ids
                WtkWindow* window = (WtkWindow*)GetWindowLongPtr(hWnd, GWLP_USERDATA);
                int command_id = LOWORD(wParam);
                // if (command_id < wtkvec_elem_count(window->id_to_eventable)) {
                //     WtkEventable* eventable = window->id_to_eventable[command_id];
                //     wtk_event_control_trigger(eventable, WTK_EVENT_CLICK, NULL);
                // }
                wtk_event_vec_trigger(window->id_to_eventable, command_id, WTK_EVENT_CLICK);
            } else {
                // controls (buttons, etc.) have user data
                WtkControl* control = (WtkControl*)GetWindowLongPtr(child_hwnd, GWLP_USERDATA);
                if (control == NULL) {
                    break;
                }

                if (control->b.events != NULL && control->con_type == WTK_CONTROL_BUTTON) {
                    wtk_event_control_trigger(&control->b, WTK_EVENT_CLICK, NULL);
                }
            }

            break;
        }
        case WM_KEYDOWN:
        case WM_KEYUP:
        case WM_CHAR:
        case WM_LBUTTONDOWN:
        case WM_LBUTTONUP:
        case WM_RBUTTONDOWN:
        case WM_RBUTTONUP:
        case WM_MBUTTONDOWN:
        case WM_MBUTTONUP:
        case WM_XBUTTONDOWN:
        case WM_XBUTTONUP:
        case WM_MOUSEMOVE:
        case WM_CAPTURECHANGED:
        case WM_MOUSEWHEEL:
        case WM_MOUSEHWHEEL: {
            LONG_PTR user_data = GetWindowLongPtr(hWnd, GWLP_USERDATA);
            WtkControl* control = (WtkControl*)user_data;
            if (control == NULL || control->con_type != WTK_CONTROL_WINDOW) {
                break;
            }
            wtk_process_common_input(control, hWnd, message, wParam, lParam);
            break;
        }
    }
    return DefWindowProc(hWnd, message, wParam, lParam);
}

#endif // wtk_win32

// /////////////////////////////////////////////////////////// end event loop //