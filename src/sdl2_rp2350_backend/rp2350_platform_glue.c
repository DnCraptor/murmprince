// Platform glue default stubs.
//
// These are intentionally weak placeholders. The real implementation should
// live next to the existing HDMI/input code and call into drivers/HDMI.c.

#include "rp2350_platform_glue.h"

uint8_t *rp2350_video_get_scanout_fb(void) {
    return (uint8_t *)0;
}

void rp2350_video_set_palette(int first, int count, const rp2350_palette_entry *entries) {
    (void)first;
    (void)count;
    (void)entries;
}

void rp2350_video_present_game_320x200(const uint8_t *src, int src_pitch) {
    (void)src;
    (void)src_pitch;
}

uint32_t rp2350_input_poll_buttons(void) {
    return 0;
}
