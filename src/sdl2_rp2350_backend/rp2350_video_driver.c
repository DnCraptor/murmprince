// SDL2 RP2350 video driver sketch.
//
// This file is meant to be compiled *inside* an upstream SDL2 build
// (or as an overlay target_sources() into SDL2-static).
// It intentionally avoids being referenced by the default shim build.
//
// NOTE: This is a skeleton to establish the shape of the integration.
// The real implementation will need to include SDL2 internal headers
// (e.g. SDL_sysvideo.h) and fill in the driver structs.

#include "rp2350_platform_glue.h"

// When compiled within SDL2, SDL will define internal build macros.
// Keep this file harmless if accidentally compiled elsewhere.
#if !defined(SDL_INTERNAL_BUILD)

int rp2350_sdl2_video_driver_placeholder(void) {
    // If you see this referenced in a link, something wired it incorrectly.
    return 0;
}

#else

#include "SDL_internal.h"
#include "video/SDL_sysvideo.h"
#include "video/SDL_pixels_c.h"

// --- Driver state ---

typedef struct RP2350_VideoData {
    int dummy;
} RP2350_VideoData;

// --- Prototypes ---

static SDL_VideoDevice *RP2350_CreateDevice(void);
static void RP2350_DeleteDevice(SDL_VideoDevice *device);

static int RP2350_VideoInit(SDL_VideoDevice *device);
static void RP2350_VideoQuit(SDL_VideoDevice *device);

static int RP2350_CreateWindow(SDL_VideoDevice *device, SDL_Window *window);
static void RP2350_DestroyWindow(SDL_VideoDevice *device, SDL_Window *window);

static int RP2350_SetWindowSize(SDL_VideoDevice *device, SDL_Window *window);

static SDL_Surface *RP2350_GetWindowSurface(SDL_VideoDevice *device, SDL_Window *window);
static int RP2350_UpdateWindowSurface(SDL_VideoDevice *device, SDL_Window *window);

// --- Bootstrap ---

static int RP2350_Available(void) {
    // On bare metal, if this driver is compiled in, we assume it’s available.
    return 1;
}

VideoBootStrap RP2350_bootstrap = {
    "rp2350",
    "SDL RP2350 (HDMI 8bpp indexed)",
    RP2350_Available,
    RP2350_CreateDevice
};

// --- Implementation ---

static SDL_VideoDevice *RP2350_CreateDevice(void) {
    SDL_VideoDevice *device = (SDL_VideoDevice *)SDL_calloc(1, sizeof(SDL_VideoDevice));
    if (!device) {
        return NULL;
    }

    device->driverdata = SDL_calloc(1, sizeof(RP2350_VideoData));
    if (!device->driverdata) {
        SDL_free(device);
        return NULL;
    }

    // Minimal set of hooks for SDLPoP:
    device->VideoInit = RP2350_VideoInit;
    device->VideoQuit = RP2350_VideoQuit;
    device->CreateSDLWindow = RP2350_CreateWindow;
    device->DestroyWindow = RP2350_DestroyWindow;
    device->SetWindowSize = RP2350_SetWindowSize;

    // Surface-based rendering path (SDLPoP uses surfaces heavily).
    device->GetWindowSurface = RP2350_GetWindowSurface;
    device->UpdateWindowSurface = RP2350_UpdateWindowSurface;

    device->free = RP2350_DeleteDevice;

    return device;
}

static void RP2350_DeleteDevice(SDL_VideoDevice *device) {
    if (!device) return;
    if (device->driverdata) SDL_free(device->driverdata);
    SDL_free(device);
}

static int RP2350_VideoInit(SDL_VideoDevice *device) {
    (void)device;

    // TODO: Register a single display mode.
    // We can expose 320x240 (scanout) or 320x200 (game logical) depending on
    // what keeps SDL’s assumptions simplest.

    return 0;
}

static void RP2350_VideoQuit(SDL_VideoDevice *device) {
    (void)device;
}

static int RP2350_CreateWindow(SDL_VideoDevice *device, SDL_Window *window) {
    (void)device;
    (void)window;

    // TODO: Create exactly one fullscreen window.
    // Enforce fixed size; ignore resizable.

    return 0;
}

static void RP2350_DestroyWindow(SDL_VideoDevice *device, SDL_Window *window) {
    (void)device;
    (void)window;
}

static int RP2350_SetWindowSize(SDL_VideoDevice *device, SDL_Window *window) {
    (void)device;
    (void)window;
    // Fixed-size target: accept but no-op.
    return 0;
}

static SDL_Surface *RP2350_GetWindowSurface(SDL_VideoDevice *device, SDL_Window *window) {
    (void)device;
    (void)window;

    // TODO: Provide an 8bpp paletted surface whose pixels are a dedicated
    // 320x200 “game buffer” (not the scanout buffer), to avoid fighting SDL
    // assumptions about pitch/ownership.

    SDL_SetError("RP2350_GetWindowSurface not implemented yet");
    return NULL;
}

static int RP2350_UpdateWindowSurface(SDL_VideoDevice *device, SDL_Window *window) {
    (void)device;
    (void)window;

    // TODO:
    // - Locate the SDL window surface
    // - Copy/center 320x200 -> 320x240 scanout via rp2350_video_present_game_320x200

    return 0;
}

#endif
