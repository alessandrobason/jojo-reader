#include "utils.h"

#include <stdarg.h>
#include <stdio.h>

#include "defines.h"

const char *format(const char *fmt, ...) {
    static char buffer[1024];
    memset(buffer, 0, sizeof(buffer));

    va_list args;
    va_start(args, fmt);
    vsnprintf(buffer, sizeof(buffer), fmt, args);
    va_end(args);

    return buffer;
}

str read_whole_file(const char *fname) {
    FILE *fp = fopen(fname, "rb");
    if (!fp) return {};

    fseek(fp, 0, SEEK_END);
    auto len = ftell(fp);
    fseek(fp, 0, SEEK_SET);

    str out;
    out.len = len;
    out.buf = (char *)malloc(len + 1);
    fread(out.buf, 1, len, fp);
    out.buf[len] = '\0';
    fclose(fp);

    return out;
}

vec<str_view> split_lines(const str &string) {
    vec<str_view> out;

    usize from = 0;
    for(usize i = 0; i < string.len; ++i) {
        if(string[i] == '\n') {
            auto line = string.to_slice(from, i);
            from = i+1;
            if (line.len == 0) continue;
            out.append(line);
        }
    }

    return out;
}