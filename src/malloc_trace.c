#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

// Linker wrappers enabled via -Wl,--wrap=...
void *__real_malloc(size_t size);
void *__real_calloc(size_t nmemb, size_t size);
void *__real_realloc(void *ptr, size_t size);
void __real_free(void *ptr);

static volatile int g_malloc_trace_in_hook = 0;

static inline void *rp2350_caller_pc(void) {
    // Note: on ARM Thumb the return address has bit0 set.
    // Keep as-is for logging; when using addr2line you may want to clear bit0.
    return __builtin_return_address(0);
}

static inline void rp2350_trace_alloc_if_interesting(const char *kind, size_t bytes, void *ptr) {
    // Avoid recursion if printf allocates.
    if (g_malloc_trace_in_hook) return;
    // DISABLED: Memory tracing spam
    return;

    g_malloc_trace_in_hook = 1;
    void *pc = rp2350_caller_pc();
    printf("MALLOC_TRACE: %s bytes=%u ptr=%p caller=%p\n", kind, (unsigned)bytes, ptr, pc);
    g_malloc_trace_in_hook = 0;
}

void *__wrap_malloc(size_t size) {
    void *p = __real_malloc(size);
    rp2350_trace_alloc_if_interesting("malloc", size, p);
    return p;
}

void *__wrap_calloc(size_t nmemb, size_t size) {
    void *p = __real_calloc(nmemb, size);
    // Beware overflow; this is only used for a specific size match.
    size_t bytes = nmemb * size;
    rp2350_trace_alloc_if_interesting("calloc", bytes, p);
    return p;
}

void *__wrap_realloc(void *ptr, size_t size) {
    void *p = __real_realloc(ptr, size);
    rp2350_trace_alloc_if_interesting("realloc", size, p);
    return p;
}

void __wrap_free(void *ptr) {
    __real_free(ptr);
}
