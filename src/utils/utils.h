#pragma once

#include "str.h"
#include "vec.h"
#include "slice.h"

#undef min
#undef max

const char *format(const char *fmt, ...);
str read_whole_file(const char *fname);
vec<str_view> split_lines(const str &string);

inline int min(int a, int b) { return a < b ? a : b; }
inline int max(int a, int b) { return a > b ? a : b; }
inline int clamp(int val, int low, int high) {
    return max(low, min(high, val));
}

