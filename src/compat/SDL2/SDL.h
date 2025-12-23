#pragma once

// Compatibility header so SDLPoP's `#include <SDL2/SDL.h>` works when building
// against upstream SDL2 from source (which typically provides headers as
// `<SDL.h>` within its `include/` directory).

#include <SDL.h>
