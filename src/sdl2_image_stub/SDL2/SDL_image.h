#pragma once

// Minimal SDL2_image header shim for RP2350 bring-up.
// This intentionally implements only the tiny subset SDLPoP uses.
//
// When a full SDL2_image port is added later, remove this include directory
// (`src/sdl2_image_stub`) from the build.

#include <SDL2/SDL.h>

#ifdef __cplusplus
extern "C" {
#endif

#define IMG_INIT_PNG 0x00000002

int IMG_Init(int flags);
void IMG_Quit(void);

const char *IMG_GetError(void);

SDL_Surface *IMG_Load(const char *file);
SDL_Surface *IMG_Load_RW(SDL_RWops *src, int freesrc);

// Not currently supported in the RP2350 bring-up.
int IMG_SavePNG(SDL_Surface *surface, const char *file);

#ifdef __cplusplus
}
#endif
