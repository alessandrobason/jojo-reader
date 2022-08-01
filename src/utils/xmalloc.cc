#include "xmalloc.h"

#undef malloc
#undef calloc
#undef realloc

#include <stdio.h>

void *xmalloc(size_t size) {
    void *ptr = malloc(size);
    if (!ptr) {
        abort();
    }
    return ptr;
}

void *xcalloc(size_t nmemb, size_t size) {
    void *ptr = calloc(nmemb, size);
    if (!ptr) {
        abort();
    }
    return ptr;
}

void *xrealloc(void *ptr, size_t newsize) {
    void *newptr = realloc(ptr, newsize);
    if (!newptr) {
        abort();
    }
    return newptr;
}
