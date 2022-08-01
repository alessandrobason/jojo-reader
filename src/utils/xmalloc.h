#pragma once

#include <stdlib.h>
#include <stdint.h>

#define malloc xmalloc
#define calloc xcalloc
#define realloc xrealloc

void *xmalloc(size_t size);
void *xcalloc(size_t nmemb, size_t size);
void *xrealloc(void *ptr, size_t newsize);