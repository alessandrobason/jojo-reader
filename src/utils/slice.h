#pragma once

#include <assert.h>
#include <string.h>
#include "defines.h"

template<typename T>
struct slice {
    slice() = default;
    slice(T *buf, usize len) : buf(buf), len(len) {}
    slice(T *buf) = delete;

    T *begin()         { return buf; }
    T *end()           { return buf ? buf + len : nullptr; }
    T *rbegin()        { return buf ? buf + len - 1 : nullptr; }
    T *rend()          { return buf ? buf - 1 : nullptr; }
    const T *begin()  const { return buf; }
    const T *end()    const { return buf ? buf + len : nullptr; }
    const T *rbegin() const { return buf ? buf + len - 1 : nullptr; }
    const T *rend()   const { return buf ? buf - 1 : nullptr; }

    T &front() { assert(buf); return buf[0]; }
    T &back()  { assert(buf); return buf[len-1]; }
    const T &front() const { assert(buf); return buf[0]; }
    const T &back()  const { assert(buf); return buf[len-1]; }

    bool empty() const { return len == 0; }

    T &operator[](usize index) { assert(len > index); return buf[index]; }
    const T &operator[](usize index) const { assert(len > index); return buf[index]; }

    bool operator==(slice other) const {
        if (len != other.len) return false;
        for (usize i = 0; i < len; ++i) {
            if (buf[i] != other[i]) return false;
        }
        return true;
    }

    bool operator!=(slice other) const {
        return !(*this == other);
    }

    slice sub(usize from = 0, usize to = -1) const {
        assert(from <= to && from <= len);
        if (to > len) to = len;
        return { buf + from, to - from };
    }

    T *buf = nullptr;
    usize len = 0;
};

template<>
inline slice<const char>::slice(const char *buf)
    : buf(buf), len(strlen(buf)) {}


template<typename T>
using cslice = slice<const T>;

using str_view = slice<const char>;