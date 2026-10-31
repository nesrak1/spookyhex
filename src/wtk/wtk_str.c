#include "wtk_str.h"

#ifdef wtk_win_os

char* wtk_lpcwstr_to_utf8(LPCWSTR wstr)
{
    if (wstr == NULL)
        return NULL;

    int size_needed = WideCharToMultiByte(CP_UTF8, 0, wstr, -1, NULL, 0, NULL, NULL);
    if (size_needed <= 0)
        return NULL;

    char* utf8_str = wtk_malloc(char*, size_needed);
    if (!utf8_str)
        return NULL;

    WideCharToMultiByte(CP_UTF8, 0, wstr, -1, utf8_str, size_needed, NULL, NULL);
    return utf8_str;
}

LPWSTR wtk_utf8_to_lpwstr(const char* utf8_str)
{
    if (!utf8_str)
        return NULL;

    int size_needed = MultiByteToWideChar(CP_UTF8, 0, utf8_str, -1, NULL, 0);
    if (size_needed <= 0)
        return NULL;

    LPWSTR wstr = wtk_malloc(LPWSTR, size_needed * sizeof(WCHAR));
    if (!wstr)
        return NULL;

    MultiByteToWideChar(CP_UTF8, 0, utf8_str, -1, wstr, size_needed);
    return wstr;
}

#endif // wtk_win_os

uint32_t utf16_to_utf32_codepoint(uint16_t high, uint16_t low)
{
    uint32_t high_off = (uint32_t)high - 0xD800;
    uint32_t low_off = (uint32_t)low - 0xDC00;
    return (high_off << 10) + low_off + 0x10000;
}
