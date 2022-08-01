#pragma once

#include <stdio.h>

#include <stdint.h>
#include <assert.h>
#include "xmalloc.h"

#include <initializer_list>
#include <new>

#include "move.h"
#include "slice.h"
#include "defines.h"
#include "str.h"

template<typename T>
struct vec {
    vec() = default;
    vec(usize initial_cap) {
        grow(initial_cap);
    }
    vec(std::initializer_list<T> list) {
        reserve(list.size());
        for(auto &&l : list) {
            append(l);
        }
    }
    vec(const vec &other) {
        *this = other;
    }
    vec(vec &&other) {
        *this = move(other);
    }

    ~vec() {
        free(buf);
        buf = nullptr;
        len = 0;
        cap = 0;
    }

    static vec with_capacity(usize initial_cap) {
        vec out;
        out.grow(initial_cap);
        return out;
    }

    void reserve(usize items) {
        grow(len + items);
    }

    void resize(usize newsize, const T &value = {}) {
        if (newsize == len) return;

        if (newsize > cap) {
            usize newcap = cap ? cap * 2 : newsize;
            while(newcap < newsize) newcap *= 2;
            grow(newcap);
        }

        if (newsize < len) {
            for (usize i = newsize; i < len; ++i) {
                buf[i].~T();
            }
        }
        else if (newsize > len) {
            for(usize i = len; i < newsize; ++i) {
                buf[i] = value;
            }
        }

        len = newsize;
    }

    void grow(usize newcap) {
        if(newcap <= cap) return;
        buf = (T *)realloc(buf, sizeof(T) * newcap);
        cap = newcap;
    }

    void shrink(usize newcap) {
        if(newcap >= cap) return;
        for (usize i = newcap; i < len; ++i) {
            buf[i].~T();
        }
        len = newcap;
        buf = (T *)realloc(buf, sizeof(T) * newcap);
        cap = newcap;
    }

    void shrink_to_fit() {
        shrink(len);
    }

    void append(T &&value) {
        if ((len + 1) > cap) reserve(len + 1);
        buf[len++] = move(value);
    }
    
    void append(const T &value) {
        if ((len + 1) > cap) reserve(len + 1);
        buf[len++] = value;
    }

    void append_slice(slice<T> arr) {
        reserve(arr.len);
        for(const T &v : arr) {
            append(v);
        }
    }

    T &pop() {
        T last = move(back());
        back().~T();
        --len;
        return last;
    }

    void remove(usize index, bool swap_back = true) {
        if (index >= len) return;

        --len;
        
        if (swap_back) {
            buf[index] = move(buf[len]);
        }
        else {
            for(usize i = index; i < len; ++i) {
                buf[i] = move(buf[i+1]);
            }
        }
    }

    bool empty() {
        return len > 0;
    }

    T &operator[](usize i) {
        assert(buf);
        return buf[i];
    }

    const T &operator[](usize i) const {
        assert(buf);
        return buf[i];
    }

    void clear() {
        resize(0);
    }

    operator slice<T>() const {
        return to_slice();
    }

    slice<T> to_slice(usize from = 0, usize to = -1) const {
        assert(from <= to && from <= len);
        if (to > len) to = len;
        return { buf + from, to - from };
    }

    T &front() {
        assert(buf);
        return buf[0];
    }

    T &back() {
        assert(buf);
        return buf[len-1];
    }

    const T &front() const {
        assert(buf);
        return buf[0];
    }

    const T &back() const {
        assert(buf);
        return buf[len-1];
    }

    T *begin() {
        return buf;
    }

    T *end() {
        return buf ? buf + len : nullptr;
    }

    const T *begin() const {
        return buf;
    }

    const T *end() const {
        return buf ? buf + len : nullptr;
    }

    T *rbegin() {
        return buf ? buf + len - 1 : nullptr;
    }

    T *rend() {
        return buf ? buf - 1 : nullptr;
    }

    const T *rbegin() const {
        return buf ? buf + len - 1 : nullptr;
    }

    const T *rend() const {
        return buf ? buf - 1 : nullptr;
    }

    vec &operator=(const vec &other) {
        if (this == &other)
            return *this;
        clear();
        reserve(other.len);
        append_slice(other.to_slice());
        return *this;
    }

    vec &operator=(vec &&other) {
        if (this == &other)
            return *this;
        free(buf);
        buf = other.buf;
        len = other.len;
        cap = other.cap;

        other.buf = nullptr;
        other.len = 0;
        other.cap = 0;
        return *this;
    }

    str to_str() {
        assert(sizeof(T) == sizeof(char));
        return { (char *)buf, len };
    }

    T *buf = nullptr;
    usize len = 0;
    usize cap = 0;
};