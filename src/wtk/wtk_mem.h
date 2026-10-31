#ifndef WTK_MEM_H
#define WTK_MEM_H

#include <stdlib.h>

#include "wtk_error.h"

void* wtk_malloc_impl(size_t size);
void* wtk_realloc_impl(void* block, size_t size);

// yay I love this language
// https://stackoverflow.com/q/605845
#ifdef __cplusplus
    #define wtk_malloc(T, size) (T)wtk_malloc_impl(size)
    #define wtk_realloc(T, block, size) (T)wtk_realloc_impl(block, size)
#else
    #define wtk_malloc(T, size) wtk_malloc_impl(size)
    #define wtk_realloc(T, block, size) wtk_realloc_impl(block, size)
#endif

#define wtk_memmove memmove
#define wtk_free free
#define wtk_strdup strdup

#endif // WTK_MEM_H