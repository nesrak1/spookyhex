#ifndef WTK_ASSETS_H
#define WTK_ASSETS_H

#if wtk_win32
#define WTK_RSRC_APPICON 100
#define WTK_RSRC_FILE 101
#define WTK_RSRC_FOLDER 102
#define WTK_RSRC_SAVE 103
#endif // wtk_win32
#if wtk_gtk
#define WTK_RSRC_APPICON "/images/icon.png"
#define WTK_RSRC_FILE "/images/file_poly.png"
#define WTK_RSRC_FOLDER "/images/folder-open_poly.png"
#define WTK_RSRC_SAVE "/images/save_poly.png"
#endif // wtk_gtk

#endif // WTK_ASSETS_H
