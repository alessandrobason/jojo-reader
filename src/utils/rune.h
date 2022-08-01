#pragma once

struct rune {
    rune() = default;
    constexpr rune(u32 ch) : codepoint(ch) {}

    operator u32() { return codepoint; }

    template<typename T> rune operator+(const T &value) { return codepoint + value; }
    template<typename T> rune operator-(const T &value) { return codepoint - value; }
    template<typename T> rune operator*(const T &value) { return codepoint * value; }
    template<typename T> rune operator/(const T &value) { return codepoint / value; }
    template<typename T> rune operator%(const T &value) { return codepoint % value; }
    template<typename T> rune operator^(const T &value) { return codepoint ^ value; }
    template<typename T> rune operator&(const T &value) { return codepoint & value; }
    template<typename T> rune operator|(const T &value) { return codepoint | value; }
    template<typename T> rune operator~() { return ~codepoint; }

    template<typename T> rune &operator+=(const T &value) { codepoint += value; return *this; }
    template<typename T> rune &operator-=(const T &value) { codepoint -= value; return *this; }
    template<typename T> rune &operator*=(const T &value) { codepoint *= value; return *this; }
    template<typename T> rune &operator/=(const T &value) { codepoint /= value; return *this; }
    template<typename T> rune &operator%=(const T &value) { codepoint %= value; return *this; }
    template<typename T> rune &operator^=(const T &value) { codepoint ^= value; return *this; }
    template<typename T> rune &operator&=(const T &value) { codepoint &= value; return *this; }
    template<typename T> rune &operator|=(const T &value) { codepoint |= value; return *this; }

    bool operator!() { return !codepoint; }
    template<typename T> bool operator==(const T &value) { return codepoint == value; }
    bool operator==(const rune &value) { return codepoint == value.codepoint; }
    template<typename T> bool operator!=(const T &value) { return codepoint != value; }
    bool operator!=(const rune &value) { return codepoint != value.codepoint; }

    operator bool() { return (bool)codepoint; }

    rune &operator++() { ++codepoint; return *this; }
    rune &operator--() { --codepoint; return *this; }

    template<typename T> bool operator<(const T &value) { return codepoint < value; }
    template<typename T> bool operator>(const T &value) { return codepoint > value; }
    template<typename T> bool operator<=(const T &value) { return codepoint <= value; }
    template<typename T> bool operator>=(const T &value) { return codepoint >= value; }

    rune operator<<(int amount) { return codepoint << amount; }
    rune operator>>(int amount) { return codepoint >> amount; }
    rune &operator<<=(int amount) { codepoint <<= amount; return *this; }
    rune &operator>>=(int amount) { codepoint >>= amount; return *this; }

    u32 codepoint;
};