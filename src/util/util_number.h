#ifndef UTIL_NUMBER_H
#define UTIL_NUMBER_H

#include "wtk_common.h"

bool util_int64_to_str_dec(int64_t value, char* dst, size_t dst_size);
bool util_int64_to_str_hex(int64_t value, char* dst, size_t dst_size);

#endif // UTIL_NUMBER_H