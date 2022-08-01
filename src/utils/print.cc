#include "print.h"

#include "str.h"

template<>
void print(const char &c) {
    printf("%c", c);
}

template<>
void print(const char * const&s) {
    printf("%s", s);
}

template<>
void print(const u8 &u) {
    printf("%u", u);
}

template<>
void print(const u16 &u) {
    printf("%u", u);
}

template<>
void print(const u32 &u) {
    printf("%u", u);
}

template<>
void print(const u64 &u) {
    printf("%zu", u);
}

template<>
void print(const i16 &i) {
    printf("%i", i);
}

template<>
void print(const i32 &i) {
    printf("%i", i);
}

template<>
void print(const i64 &i) {
    printf("%zi", i);
}

template<>
void print(const f32 &f) {
    printf("%f", f);
}

template<>
void print(const f64 &f) {
    printf("%f", f);
}

template<>
void print(const void * const&ptr) {
    printf("%p", ptr);
}

template<>
void print(const str &string) {
    printf("%.*s", (int)string.len, string.buf);
}

template<> 
void print(const rune &r) {
    char buf[utf8_max_size];
    usize len = utf8_encode(buf, r);
    printf("%.*s", (int)len, buf);
}