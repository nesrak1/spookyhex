#ifndef WTK_WINDOWS_H
#define WTK_WINDOWS_H

#include "wtk_common.h"

#if !wtk_win_os
#error "Must be using wtk_win_os to use this header."
#endif

#define UNICODE
#define _UNICODE
#define COBJMACROS
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

#endif // WTK_WINDOWS_H