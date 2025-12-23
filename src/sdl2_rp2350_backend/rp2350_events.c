// SDL2 RP2350 event pump sketch.
//
// Like rp2350_video_driver.c, this is intended to be compiled within SDL2.

#include "rp2350_platform_glue.h"

#if !defined(SDL_INTERNAL_BUILD)

int rp2350_sdl2_events_placeholder(void) { return 0; }

#else

#include "SDL_internal.h"
#include "events/SDL_events_c.h"
#include "events/SDL_keyboard_c.h"

// Minimal mapping for SDLPoP:
// - arrows
// - space/enter/escape
//
// TODO: Define a stable button bitmask in rp2350_input_poll_buttons() and map it.

static void rp2350_post_key(SDL_Scancode sc, SDL_bool down) {
    SDL_SendKeyboardKey(0, down ? SDL_PRESSED : SDL_RELEASED, sc);
}

void RP2350_PumpEvents(void) {
    const uint32_t buttons = rp2350_input_poll_buttons();

    // NOTE: bit positions are TBD.
    (void)buttons;

    // Example (disabled until bit assignments exist):
    // rp2350_post_key(SDL_SCANCODE_LEFT,  (buttons & (1u << 0)) != 0);
    // rp2350_post_key(SDL_SCANCODE_RIGHT, (buttons & (1u << 1)) != 0);
    // rp2350_post_key(SDL_SCANCODE_UP,    (buttons & (1u << 2)) != 0);
    // rp2350_post_key(SDL_SCANCODE_DOWN,  (buttons & (1u << 3)) != 0);
    // rp2350_post_key(SDL_SCANCODE_SPACE, (buttons & (1u << 4)) != 0);
    // rp2350_post_key(SDL_SCANCODE_RETURN,(buttons & (1u << 5)) != 0);
    // rp2350_post_key(SDL_SCANCODE_ESCAPE,(buttons & (1u << 6)) != 0);
}

#endif
