#pragma once

// Feature switches for the minimal RP2350 SDL2-ish shim (rp_sdl).
//
// These are intended to make it explicit what subsystems are *not* implemented
// (and should remain as small stubs) for the SDLPoP port.
//
// You can override these from CMake with target_compile_definitions(rp_sdl ...).

#ifndef RP_SDL_FEATURE_WINDOW
#define RP_SDL_FEATURE_WINDOW 0
#endif

#ifndef RP_SDL_FEATURE_MESSAGEBOX
#define RP_SDL_FEATURE_MESSAGEBOX 0
#endif

#ifndef RP_SDL_FEATURE_AUDIO
#define RP_SDL_FEATURE_AUDIO 0
#endif

#ifndef RP_SDL_FEATURE_JOYSTICK
#define RP_SDL_FEATURE_JOYSTICK 0
#endif

#ifndef RP_SDL_FEATURE_GAMECONTROLLER
#define RP_SDL_FEATURE_GAMECONTROLLER 0
#endif

#ifndef RP_SDL_FEATURE_HAPTIC
#define RP_SDL_FEATURE_HAPTIC 0
#endif

// Fix for dark/black fringes around RGBA sprites:
// Some PNG assets are effectively premultiplied against black. When those are
// blended or quantized, edge pixels become too dark.
//
// 0 = off
// 1 = auto-detect premultiplied alpha and unpremultiply
// 2 = force unpremultiply for all RGBA loads
#ifndef RP_SDL_STBI_UNPREMULTIPLY_ALPHA_MODE
#define RP_SDL_STBI_UNPREMULTIPLY_ALPHA_MODE 1
#endif
