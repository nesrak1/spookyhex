mkdir build
cl /MD /D wtk_win32 /Fo.\obj\ /Fd.\build\ ^
    /I src/bptree ^
    /I src/hexedit ^
    /I src/wtk ^
    src/bptree/bp_doc.c ^
    src/bptree/bp_node.c ^
    src/bptree/bp_piece.c ^
    src/bptree/bp_refcount.c ^
    src/bptree/bp_source.c ^
    src/bptree/bp_tree.c ^
    src/hexedit/hax_document.c ^
    src/hexedit/hax_hexedit.c ^
    src/wtk/wtk_container.c ^
    src/wtk/wtk_error.c ^
    src/wtk/wtk_file.c ^
    src/wtk/wtk_mem.c ^
    src/wtk/wtk_str.c ^
    src/wtk/wtk_wgtk.c ^
    src/main.c ^
    rsrc/shex.res ^
    /link user32.lib Wtsapi32.lib gdi32.lib Comdlg32.lib Comctl32.lib Shell32.lib Ole32.lib ^
    /subsystem:windows,5.02 ^
    /out:.\build\shex.exe
