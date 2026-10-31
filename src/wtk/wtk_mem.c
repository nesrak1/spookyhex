#include "wtk_mem.h"
#include "wtk_error.h"

void* wtk_malloc_impl(size_t size)
{
    void* result = malloc(size == 0 ? 1 : size);
    if (result == NULL)
        wtk_throw("out of memory");

    return result;
}

void* wtk_realloc_impl(void* block, size_t size)
{
    void* result = realloc(block, size == 0 ? 1 : size);
    if (result == NULL)
        wtk_throw("out of memory");

    return result;
}