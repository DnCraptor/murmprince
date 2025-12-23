#pragma once

// This header is intentionally *SDL2-internal-free*.
// It defines the minimal glue we need between an SDL2 backend and the existing
// RP2350 firmware subsystems (HDMI scanout, input).

#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

// HDMI scanout is 320x240 8bpp indexed.
// SDLPoP renders 320x200; we typically center vertically (20px top/bottom).
#define RP2350_SCANOUT_W 320
#define RP2350_SCANOUT_H 240

// Palette indices reserved for HDMI control/sync codes.
// All other indices should be usable for game palettes.
#define RP2350_HDMI_RESERVED_FIRST 240
#define RP2350_HDMI_RESERVED_LAST  243

typedef struct rp2350_palette_entry {
    uint8_t r;
    uint8_t g;
    uint8_t b;
    uint8_t a;
} rp2350_palette_entry;

// Returns pointer to the scanout framebuffer (8bpp, pitch == RP2350_SCANOUT_W).
// The buffer is owned by the platform.
uint8_t *rp2350_video_get_scanout_fb(void);

// Replace a contiguous range of palette entries.
// The implementation must preserve reserved indices 240..243.
void rp2350_video_set_palette(int first, int count, const rp2350_palette_entry *entries);

// Present/copy a 320x200 8bpp “game surface” into scanout.
// - `src_pitch` is bytes per row in src (typically 320).
void rp2350_video_present_game_320x200(const uint8_t *src, int src_pitch);

// Input polling hook (optional for early bring-up). Returns a bitmask.
// You can wire this to GPIO buttons or USB HID.
uint32_t rp2350_input_poll_buttons(void);

#ifdef __cplusplus
}
#endif
