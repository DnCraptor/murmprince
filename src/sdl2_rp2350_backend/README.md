# RP2350 SDL2 backend sketch (WIP)

This folder contains a *sketch* of what an upstream SDL2 backend for RP2350 could look like.

It is **not compiled yet** in the current tree because upstream SDL2 isn’t vendored by default.

## Goal

- Make SDLPoP run against upstream SDL2 (instead of the shim), while reusing existing platform code:
  - HDMI scanout in `drivers/HDMI.c`
  - Palette rules: indices **240–243 reserved** for HDMI control/sync
  - 8bpp indexed framebuffer, game content 320×200 (centered/padded into 320×240 scanout)
  - Input via whatever RP2350 wiring you choose (GPIO, USB HID, etc.)

## Intended integration

Two viable integration options (we’ll pick one after SDL2 is vendored):

1. **Patch SDL2 source tree**
   - Add `src/video/rp2350/*` and register a new `RP2350` video bootstrap.

2. **Overlay build from top-level CMake** (preferred initially)
   - Keep these sources in this repo and add them to the SDL2 static target with `target_sources()`.
   - This avoids maintaining a long-lived fork just for early bring-up.

## Files

- `rp2350_video_driver.c`: skeleton video driver entry points (create device, init, create window, framebuffer, present)
- `rp2350_events.c`: skeleton event pump (keyboard/controller)
- `rp2350_platform_glue.h`: narrow glue surface between SDL2 and existing HDMI/input code

## Next steps

- Vendor SDL2 under `third_party/SDL2` and flip `USE_REAL_SDL2=ON`.
- Decide whether to run SDL2 in 8bpp-native (preferred) or 32bpp->8bpp conversion.
- Implement:
  - a single fullscreen “window”
  - an 8bpp streaming framebuffer + palette updates
  - input mapping into SDL key events
