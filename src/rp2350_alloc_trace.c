#include "rp2350_alloc_trace.h"
#include "board_config.h"

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

// Keep this extremely low-noise: only print when size == 1552 (the allocation
// that immediately precedes the first observed onscreen corruption).

static volatile int g_alloc_trace_in_hook = 0;

static inline void rp2350_trace_if_1552(const char *kind, size_t bytes, void *ptr, const char *file, int line) {
    if (g_alloc_trace_in_hook) return;
    if (bytes != 1552u) return;

    g_alloc_trace_in_hook = 1;
    void *caller = __builtin_return_address(0);
    DBG_PRINTF("ALLOC1552: %s bytes=%u ptr=%p at %s:%d caller=%p\\n",
           kind, (unsigned)bytes, ptr, file ? file : "?", line, caller);
    g_alloc_trace_in_hook = 0;
}

void *rp2350_malloc_traced(size_t size, const char *file, int line) {
    void *p = malloc(size);
    rp2350_trace_if_1552("malloc", size, p, file, line);
    return p;
}

void *rp2350_calloc_traced(size_t nmemb, size_t size, const char *file, int line) {
    void *p = calloc(nmemb, size);
    size_t bytes = nmemb * size;
    rp2350_trace_if_1552("calloc", bytes, p, file, line);
    return p;
}

void *rp2350_realloc_traced(void *ptr, size_t size, const char *file, int line) {
    void *p = realloc(ptr, size);
    rp2350_trace_if_1552("realloc", size, p, file, line);
    return p;
}

void rp2350_free_traced(void *ptr, const char *file, int line) {
    (void)file;
    (void)line;
    free(ptr);
}
