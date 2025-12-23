#pragma once
// Compatibility wrapper for SDLPoP when building with upstream SDL2.
// This keeps SDLPoP from needing to include the full `src/` include root
// (which contains the SDL shim headers under `src/SDL2`).
#include "../dirent.h"
