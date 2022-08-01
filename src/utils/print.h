#pragma once

#include "defines.h"
#include "slice.h"

#include <stdio.h>
#include <assert.h>

template<typename T, typename ...Targs>
void print_val(const char *fmt, const T &value, const Targs &...args);

template<typename T>
void print_val(const char *fmt, const T &value);

template<typename T>
void print_val(const T &value);

template<typename ...Targs>
void print(const char *fmt, const Targs &...args);

template<typename ...Targs>
void println(const char *fmt, const Targs &...args);

template<typename T>
void print(const T &value) = delete;

template<typename T, typename ...Targs>
void print_val(const char *fmt, const T &value, const Targs &...args) {
    print(value);
    print(fmt, args...);
}

template<typename T>
void print_val(const char *fmt, const T &value) {
    print(value);
    print(fmt);
}

template<typename T>
void print_val(const T &value) {
    print(value);
}

template<typename ...Targs>
void print(const char *fmt, const Targs &...args) {
    for(; *fmt; ++fmt) {
        if (*fmt != '%') {
            if(*fmt == '\\' && *(fmt+1) == '%') {
                ++fmt;
            }
            putchar(*fmt);
        }
        else {
            print_val(++fmt, args...);
            return;
        }
    }
}

template<typename ...Targs>
void println(const char *fmt, const Targs &...args) {
    print(fmt, args...);
    puts("");
}

struct str;
struct rune;

template<> void print(const char &c);
template<> void print(const char * const&s);
template<> void print(const u8 &u);
template<> void print(const u16 &u);
template<> void print(const u32 &u);
template<> void print(const u64 &u);
template<> void print(const i16 &i);
template<> void print(const i32 &i);
template<> void print(const i64 &i);
template<> void print(const f32 &f);
template<> void print(const f64 &f);
template<> void print(const void * const&ptr);
template<> void print(const str &s);

#ifndef STR_RUNE_U32
template<> void print(const rune &r);
#endif

template<typename T>
void print(const slice<T> &arr) {
    printf("[ ");
    for(const auto &n : arr) print("% ", n);
    printf("]");
}