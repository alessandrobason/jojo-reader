#pragma once

#include "defines.h"

#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include <assert.h>
#include <ctype.h>

#include <iterator>

#include "print.h"
#include "slice.h"

#ifdef STR_RUNE_U32
using rune = u32;
#else
#include "rune.h"
#endif

constexpr int utf8_max_size = 4;
constexpr rune utf8_invalid = 0x80;

// grabs the next UTF-8 codepoint and advances string ptr
rune utf8_decode(const char **str);
// encodes a codepoint as UTF-8 and returns the length
usize utf8_encode(char *str, rune ch);
// returns the size of the next UTF-8 codepoint
int utf8_size(const char *str);
// returns the size of a UTF-8 codepoint
usize utf8_cpsize(rune ch);
// reads and returns the next codepoint from the file
rune utf8_get(FILE *f);

template<typename T>
struct vec;

struct str {
    str() = default;
    str(const char *cstr, usize clen = 0) {
        if (clen == 0) clen = strlen(cstr);
        len = clen;
        buf = (char *)malloc(len + 1);
        memcpy(buf, cstr, len);
        buf[len] = '\0';
    }
    str(slice<char> arr) : str(arr.buf, arr.len) {}
    str(slice<const char> arr) : str(arr.buf, arr.len) {}
    str(char *str) {
        len = strlen(str);
        buf = str;
    }
    str(const str &other) {
        len = other.len;
        buf = (char *)malloc(len + 1);
        memcpy(buf, other.buf, len);
        buf[len] = '\0';
    }

    char &operator[](usize index) { assert(index<len); return buf[index]; }
    const char &operator[](usize index) const { assert(index<len); return buf[index]; }

    slice<const char> to_slice(usize from = 0, usize to = -1) const {
        assert(from < len);
        if (to > len) to = len;
        return { (const char *)buf + from, to - from };
    }

    bool equals(slice<const char> other, bool case_sensitive = true) const {
        if (len != other.len) return false;
        if (case_sensitive) {
            return *this == other;
        }
        else {
            for(usize i = 0; i < len; ++i) {
                if (tolower(buf[i]) != tolower(other[i])) return false;
            }
            return true;
        }
    }

    bool operator==(slice<const char> other) const {
        if (len != other.len) return false;
        for(usize i = 0; i < len; ++i) {
            if (buf[i] != other[i]) return false;
        }
        return true;
    }

    bool operator!=(slice<const char> other) const {
        return !(*this == other);
    }

    bool empty() const {
        return len == 0;
    }

    vec<rune> runes();

    char *buf = nullptr;
    usize len = 0;
    // utf-8 iterator stuff 
    template<typename base>
    struct utf8_iter {
        using iterator_category = std::forward_iterator_tag;
        using difference_type   = ptrdiff_t;
        using value_type        = rune;
        using pointer           = value_type*;
        using reference         = value_type&;

        utf8_iter(base *ptr) {
            buf = ptr;
            codepoint = utf8_decode((const char **)&buf);
        }

        rune operator*() const { return codepoint; }
        rune *operator->() { return &codepoint; }
        utf8_iter &operator++() { codepoint = utf8_decode((const char **)&buf); return *this; }

        bool operator==(const utf8_iter &o) const { return buf == o.buf; }
        bool operator!=(const utf8_iter &o) const { return buf != o.buf; }

        base *buf = nullptr;
        rune codepoint = 0;
    };

    utf8_iter<char> begin() const { return { buf }; }
    utf8_iter<char> end() const { return { buf + len }; }

    const utf8_iter<const char> cbegin() const { return { buf }; }
    const utf8_iter<const char> cend() const { return { buf + len }; }

    template<typename base>
    struct ascii_iterator {
        ascii_iterator(base *buf, usize len)
            : buf(buf), len(len) {}

        base *begin() { return buf; }
        base *end() { return buf + len; }

        base *buf = nullptr;
        usize len = 0;
    };

    ascii_iterator<char> iter_ascii() { return { buf, len }; }
    ascii_iterator<const char> iter_ascii() const { return { buf, len }; }
};