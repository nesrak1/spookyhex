#ifndef WTK_COMMON_H
#define WTK_COMMON_H

#include <stdlib.h>
#include <stdio.h>
#include <assert.h>

// stdint/stdbool with compat for crappy ms compilers
#if defined(_MSC_VER) && (_MSC_VER < 1600)
    #include "stdint.h"
#else
    #include <stdint.h>
#endif
//
#if defined(_MSC_VER) && (_MSC_VER < 1800)
    #include "stdbool.h"
#else
    #include <stdbool.h>
#endif

// platform defines

#if defined(_WIN32)
    #define wtk_win_os 1
#else
    #define wtk_win_os 0
#endif

#if defined(__linux__)
    #define wtk_lin_os 1

    #define _FILE_OFFSET_BITS 64
#else
    #define wtk_lin_os 0
#endif

// no return compat

#if defined(__STDC_VERSION__) && __STDC_VERSION__ >= 202311L
    #define WTK_NORET [[noreturn]]
#elif defined(_MSC_VER)
    #define WTK_NORET __declspec(noreturn)
#elif defined(__GNUC__) || defined(__clang__)
    #define WTK_NORET __attribute__((noreturn))
#else
    #define WTK_NORET
#endif

// no-op to clearly indicate an opaque struct that is never defined
#define WtkOpaque(v) v v

// math

#define wtk_min(a, b) (((a) < (b)) ? (a) : (b))
#define wtk_max(a, b) (((a) > (b)) ? (a) : (b))
#define wtk_clamp(a, v, b) wtk_max(a, wtk_min(v, b))

// assertions

#define wtk_assert assert

#endif // WTK_COMMON_H