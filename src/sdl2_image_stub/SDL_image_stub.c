#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>

#include <stdio.h>
#include <string.h>

#include "psram_allocator.h"

// stb_image is provided by `src/stb_image_impl.c` (STBI_NO_STDIO), so use memory decode.
#include "third_party/stb/stb_image.h"

static const char *g_img_error = "";

int IMG_Init(int flags) {
    (void)flags;
    return IMG_INIT_PNG;
}

void IMG_Quit(void) {}

const char *IMG_GetError(void) {
    return g_img_error;
}

SDL_Surface *IMG_Load(const char *file) {
    if (!file) {
        g_img_error = "IMG_Load: NULL filename";
        return NULL;
    }

    SDL_RWops *rw = SDL_RWFromFile(file, "rb");
    if (!rw) {
        g_img_error = "IMG_Load: SDL_RWFromFile failed";
        return NULL;
    }
    return IMG_Load_RW(rw, 1);
}

SDL_Surface *IMG_Load_RW(SDL_RWops *src, int freesrc) {
    if (!src) {
        g_img_error = "IMG_Load_RW: NULL src";
        return NULL;
    }

    const Sint64 size64 = SDL_RWsize(src);
    if (size64 <= 0 || size64 > (Sint64)(16 * 1024 * 1024)) {
        g_img_error = "IMG_Load_RW: invalid/too large input";
        if (freesrc) SDL_RWclose(src);
        return NULL;
    }

    const size_t size = (size_t)size64;
    unsigned char *png = (unsigned char *)psram_malloc(size);
    if (!png) {
        g_img_error = "IMG_Load_RW: OOM";
        if (freesrc) SDL_RWclose(src);
        return NULL;
    }

    const size_t read = SDL_RWread(src, png, 1, size);
    if (freesrc) SDL_RWclose(src);

    if (read != size) {
        psram_free(png);
        g_img_error = "IMG_Load_RW: short read";
        return NULL;
    }

    int w = 0, h = 0, comp_in_file = 0;
    unsigned char *rgba = stbi_load_from_memory(png, (int)size, &w, &h, &comp_in_file, 4);
    psram_free(png);

    if (!rgba) {
        g_img_error = stbi_failure_reason();
        return NULL;
    }

    // Use a canonical 32bpp surface format; SDLPoP will convert/blit as needed.
    SDL_Surface *surf = SDL_CreateRGBSurfaceWithFormat(0, w, h, 32, SDL_PIXELFORMAT_RGBA32);
    if (!surf) {
        stbi_image_free(rgba);
        g_img_error = "IMG_Load_RW: SDL_CreateRGBSurfaceWithFormat failed";
        return NULL;
    }

    // Copy row-by-row to respect pitch.
    const size_t src_pitch = (size_t)w * 4u;
    for (int y = 0; y < h; y++) {
        memcpy((Uint8 *)surf->pixels + (size_t)y * (size_t)surf->pitch,
               rgba + (size_t)y * src_pitch,
               src_pitch);
    }

    stbi_image_free(rgba);
    g_img_error = "";
    return surf;
}

int IMG_SavePNG(SDL_Surface *surface, const char *file) {
    (void)surface;
    (void)file;
    g_img_error = "IMG_SavePNG: not supported";
    return -1;
}
