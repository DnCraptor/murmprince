#pragma once

// Allocation tracing for SDLPoP bring-up.
// We only log a single suspicious size to keep serial output manageable.
// This header is force-included into SDLPoP source files via CMake.

// If a build accidentally force-includes this into an assembler unit, do nothing.
#ifdef __ASSEMBLER__
#define RP2350_ALLOC_TRACE_IS_ACTIVE 0
#else

#include <stddef.h>
#include <stdlib.h>

#ifdef __cplusplus
extern "C" {
#endif

void *rp2350_malloc_traced(size_t size, const char *file, int line);
void *rp2350_calloc_traced(size_t nmemb, size_t size, const char *file, int line);
void *rp2350_realloc_traced(void *ptr, size_t size, const char *file, int line);
void rp2350_free_traced(void *ptr, const char *file, int line);

#ifdef __cplusplus
}
#endif

// Only affect SDLPoP compilation units that explicitly enable it.
#ifndef RP2350_ALLOC_TRACE_ENABLE
#define RP2350_ALLOC_TRACE_ENABLE 0
#endif

#if RP2350_ALLOC_TRACE_ENABLE

#ifndef RP2350_ALLOC_TRACE_IS_ACTIVE
#define RP2350_ALLOC_TRACE_IS_ACTIVE 1
#endif

#ifndef RP2350_TRACE_ALLOC_1552
#define RP2350_TRACE_ALLOC_1552 1
#endif

#if RP2350_TRACE_ALLOC_1552
#define malloc(sz) rp2350_malloc_traced((sz), __FILE__, __LINE__)
#define calloc(nmemb, sz) rp2350_calloc_traced((nmemb), (sz), __FILE__, __LINE__)
#define realloc(ptr, sz) rp2350_realloc_traced((ptr), (sz), __FILE__, __LINE__)
#define free(ptr) rp2350_free_traced((ptr), __FILE__, __LINE__)
#endif

#endif

#endif // __ASSEMBLER__
