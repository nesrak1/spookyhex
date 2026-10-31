#ifndef WTK_STR_H
#define WTK_STR_H

#include "wtk_common.h"

#if wtk_win_os
#include "wtk_windows.h"
#endif

#include "wtk_mem.h"

// internally a wtkvec
#define WtkStr char*

#if wtk_win_os

char* wtk_lpcwstr_to_utf8(LPCWSTR wstr);
LPWSTR wtk_utf8_to_lpwstr(const char* utf8_str);

#endif // wtk_win_os

uint32_t utf16_to_utf32_codepoint(uint16_t high, uint16_t low);

#endif // WTK_STR_H