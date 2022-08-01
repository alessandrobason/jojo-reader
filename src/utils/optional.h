#pragma once

#include <stdlib.h>
#include "move.h"

template<typename T, typename Q>
struct optional {
    optional() = default;
    optional(const T &res) : result(res), success(true) {}
    optional(T &&res) : result(move(res)), success(true) {}
    optional(const Q &err) : error(err), success(false) {}
    optional(Q &&err) : error(move(err)), success(false) {}

    T &&unpack() {
        if (bad()) {
            abort();
        }
        return move(result);
    }

    bool good() { return success; }
    bool bad() { return !success; }

    bool success;
    T result;
    Q error;
};