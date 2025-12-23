#pragma once
#include "../SDL_port.h"

// SDL_image stubs
#define IMG_INIT_PNG 0
int IMG_Init(int flags);
void IMG_Quit(void);
SDL_Surface *IMG_Load(const char *file);
SDL_Surface *IMG_ReadXPMFromArray(char **xpm);
const char *IMG_GetError(void);
int IMG_SavePNG(SDL_Surface *surface, const char *file);
