#include "util_number.h"

#include <inttypes.h>
#include <stdio.h>

bool util_int64_to_str_dec(int64_t value, char* dst, size_t dst_size)
{
    if (dst == NULL || dst_size == 0)
        return false;

    int n = snprintf(dst, dst_size, "%" PRIi64, value);
    return n >= 0 && (size_t)n < dst_size;
}

bool util_int64_to_str_hex(int64_t value, char* dst, size_t dst_size)
{
    if (dst == NULL || dst_size == 0)
        return false;

    int n = snprintf(dst, dst_size, "%" PRIx64, value);
    return n >= 0 && (size_t)n < dst_size;
}