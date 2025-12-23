// stb_image implementation unit for embedded PNG loading.
// We keep it in a dedicated TU to avoid including the implementation in other files.

#include "psram_allocator.h"

#define STBI_ONLY_PNG
#define STBI_NO_STDIO

// Route stb_image allocations through our PSRAM allocator.
// On RP2350 the Pico SDK's malloc wrapper can panic on OOM; decoding sprite PNGs
// can require large temporary buffers (e.g. 320x200x4 = 256000 bytes).
#define STBI_MALLOC(sz) psram_malloc(sz)
#define STBI_REALLOC(p, sz) psram_realloc((p), (sz))
#define STBI_FREE(p) psram_free(p)

#define STB_IMAGE_IMPLEMENTATION
#include "third_party/stb/stb_image.h"
