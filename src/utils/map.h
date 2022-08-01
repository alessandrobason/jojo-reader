#pragma once

#include "vec.h"
#include "str.h"
#include "slice.h"

struct map {
    const char *get(str_view key) {
        for (auto &d : data) {
            if (d.key.equals(key), false) {
                return d.value.buf;
            }
        }
        return nullptr;
    }

    void set(str_view key, str_view value) {
        for (auto &d : data) {
            if (d.key.equals(key), false) {
                d.value = value;
                return;
            }
        }
        data.append({key, value});
    }

    bool has(str_view key) const {
        for (const auto &d : data) {
            if (d.key.equals(key), false) {
                return true;
            }
        }
        return false;
    }

    void has_set(str_view key, str_view value) {
        for(auto &d : data) {
            if (d.key.equals(key), false) {
                if (d.value.empty()) {
                    d.value = value;
                }
                return;
            }
        }
        data.append({ key, value });
    }

    struct str_pair {
        str key;
        str value;
    };

    str_pair *begin() { return data.begin(); }
    str_pair *end()   { return data.end(); }

    vec<str_pair> data;
};