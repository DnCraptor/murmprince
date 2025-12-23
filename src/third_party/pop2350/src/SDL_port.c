#include "SDL_port.h"
#include "HDMI.h"
#include "ps2.h"
#include "pico/stdlib.h"
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include "embedded_data.h"
#include "psram_allocator.h"

#define STB_IMAGE_IMPLEMENTATION
#define STBI_NO_STDIO
#define STBI_ONLY_PNG
#define STBI_MALLOC psram_malloc
#define STBI_FREE psram_free
#define STBI_REALLOC psram_realloc
#include "stb_image.h"

#define IS_PSRAM(ptr) ((uintptr_t)(ptr) >= 0x11000000 && (uintptr_t)(ptr) < 0x12000000)
#define SDL_PALETTE_FLAG_OWNS_COLORS 0x1

static SDL_Palette *SDL_CreatePaletteInternal(int ncolors) {
    SDL_Palette *pal = (SDL_Palette *)psram_malloc(sizeof(SDL_Palette));
    bool pal_in_psram = pal != NULL;
    if (!pal) pal = (SDL_Palette *)malloc(sizeof(SDL_Palette));
    if (!pal) return NULL;

    pal->colors = (SDL_Color *)psram_malloc(sizeof(SDL_Color) * ncolors);
    bool colors_in_psram = pal->colors != NULL;
    if (!pal->colors) pal->colors = (SDL_Color *)malloc(sizeof(SDL_Color) * ncolors);
    if (!pal->colors) {
        if (pal_in_psram) psram_free(pal); else free(pal);
        return NULL;
    }

    memset(pal->colors, 0, sizeof(SDL_Color) * ncolors);
    pal->ncolors = ncolors;
    pal->refcount = 1;
    pal->flags = SDL_PALETTE_FLAG_OWNS_COLORS;
    return pal;
}

SDL_Palette *SDL_CreatePalette(int ncolors) {
    if (ncolors <= 0) return NULL;
    return SDL_CreatePaletteInternal(ncolors);
}

void SDL_PaletteAddRef(SDL_Palette *palette) {
    if (palette) {
        palette->refcount++;
    }
}

void SDL_PaletteRelease(SDL_Palette *palette) {
    if (!palette) return;
    palette->refcount--;
    if (palette->refcount <= 0) {
        if (palette->colors) {
            if (palette->flags & SDL_PALETTE_FLAG_OWNS_COLORS) {
                if (IS_PSRAM(palette->colors)) psram_free(palette->colors);
                else free(palette->colors);
            }
        }
        if (IS_PSRAM(palette)) psram_free(palette);
        else free(palette);
    }
}

void SDL_SurfaceAdoptPalette(SDL_Surface *surface, SDL_Palette *palette) {
    if (!surface || !surface->format) return;
    if (surface->format->palette == palette) return;
    if (palette) SDL_PaletteAddRef(palette);
    if (surface->format->palette) {
        SDL_PaletteRelease(surface->format->palette);
    }
    surface->format->palette = palette;
}

extern SDL_Surface* onscreen_surface_;

static SDL_Palette* get_screen_palette(void) {
    if (onscreen_surface_ && onscreen_surface_->format) {
        return onscreen_surface_->format->palette;
    }
    return NULL;
}

static Uint8 find_best_palette_index(const SDL_Color *src_color, const SDL_Palette *dst_palette) {
    int best_distance = INT_MAX;
    Uint8 best_index = 0;
    if (!dst_palette || dst_palette->ncolors <= 0) {
        return 0;
    }

    for (int i = 0; i < dst_palette->ncolors; ++i) {
        const SDL_Color *dst_color = &dst_palette->colors[i];
        int dr = (int)src_color->r - (int)dst_color->r;
        int dg = (int)src_color->g - (int)dst_color->g;
        int db = (int)src_color->b - (int)dst_color->b;
        int distance = dr * dr + dg * dg + db * db;
        if (distance < best_distance) {
            best_distance = distance;
            best_index = (Uint8)i;
            if (distance == 0) {
                break;
            }
        }
    }
    return best_index;
}

// Globals
static SDL_Surface *screen_surface = NULL;
static uint32_t start_time = 0;

int SDL_Init(Uint32 flags) {
    stdio_init_all();
    // HDMI and PS2 are initialized in main.c before calling pop_main
    // But if pop_main calls SDL_Init, we can just return success.
    start_time = time_us_32() / 1000;
    return 0;
}

int SDL_InitSubSystem(Uint32 flags) {
    return 0;
}

void SDL_Quit(void) {
    printf("SDL_Quit called\n");
    // Blink LED rapidly to indicate quit
    gpio_init(25);
    gpio_set_dir(25, GPIO_OUT);
    for(int i=0; i<50; i++) {
        gpio_put(25, 1); sleep_ms(50);
        gpio_put(25, 0); sleep_ms(50);
    }
}

const char *SDL_GetError(void) {
    return "Unknown Error";
}

void SDL_SetHint(const char *name, const char *value) {
}

SDL_Window *SDL_CreateWindow(const char *title, int x, int y, int w, int h, Uint32 flags) {
    return (SDL_Window *)1; // Dummy pointer
}

SDL_Renderer *SDL_CreateRenderer(SDL_Window *window, int index, Uint32 flags) {
    return (SDL_Renderer *)1; // Dummy pointer
}

int SDL_GetRendererInfo(SDL_Renderer *renderer, SDL_RendererInfo *info) {
    info->flags = SDL_RENDERER_SOFTWARE;
    return 0;
}

SDL_Texture *SDL_CreateTexture(SDL_Renderer *renderer, Uint32 format, int access, int w, int h) {
    return (SDL_Texture *)1; // Dummy pointer
}

void SDL_DestroyTexture(SDL_Texture *texture) {
}

void SDL_DestroyRenderer(SDL_Renderer *renderer) {
}

void SDL_DestroyWindow(SDL_Window *window) {
}

SDL_Surface *SDL_CreateRGBSurface(Uint32 flags, int width, int height, int depth, Uint32 Rmask, Uint32 Gmask, Uint32 Bmask, Uint32 Amask) {
    bool force_full_palette = (flags & SDL_FORCE_FULL_PALETTE) != 0;
    Uint32 stored_flags = flags & ~SDL_FORCE_FULL_PALETTE;
    // printf("SDL_CreateRGBSurface: %dx%d %d bpp\n", width, height, depth);
    // Try to allocate everything in PSRAM to save SRAM
    SDL_Surface *s = (SDL_Surface *)psram_malloc(sizeof(SDL_Surface));
    bool using_psram_struct = (s != NULL);
    if (!s) {
        printf("SDL_CreateRGBSurface: psram_malloc(SDL_Surface) failed\n");
        s = (SDL_Surface *)malloc(sizeof(SDL_Surface));
    }
    if (!s) return NULL;

    s->flags = stored_flags;
    s->w = width;
    s->h = height;
    
    s->format = (SDL_PixelFormat *)psram_malloc(sizeof(SDL_PixelFormat));
    if (!s->format) {
        printf("SDL_CreateRGBSurface: psram_malloc(SDL_PixelFormat) failed\n");
        s->format = (SDL_PixelFormat *)malloc(sizeof(SDL_PixelFormat));
    }
    if (!s->format) { 
        if (using_psram_struct) psram_free(s); else free(s); 
        return NULL; 
    }

    s->format->BitsPerPixel = depth;
    s->format->BytesPerPixel = (depth + 7) / 8;
    
    // Optimization: Allocate fewer colors for sprites (width < 320) to save RAM
    // Also, don't allocate palette for > 8bpp surfaces to save RAM
    if (depth <= 8) {
        int ncolors = 256;
        // if (!force_full_palette && width < 320) {
        //     ncolors = 16;
        // }

        SDL_Palette *palette = SDL_CreatePalette(ncolors);
        if (!palette) {
            if (IS_PSRAM(s->format)) psram_free(s->format);
            else free(s->format);
            if (using_psram_struct) psram_free(s); else free(s);
            return NULL;
        }
        s->format->palette = palette;
    } else {
        s->format->palette = NULL;
    }
    
    s->pitch = width * s->format->BytesPerPixel;
    
    // Use PSRAM for pixel data if available
    s->pixels = psram_malloc(s->pitch * height);
    if (!s->pixels) {
        printf("SDL_CreateRGBSurface: psram_malloc(pixels) failed. Size: %d\n", s->pitch * height);
        s->pixels = calloc(1, s->pitch * height);
    } else {
        memset(s->pixels, 0, s->pitch * height);
    }

    if (!s->pixels) {
        printf("SDL_CreateRGBSurface: calloc(pixels) failed\n");
        return NULL;
    }

    s->refcount = 1;
    s->colorkey = 0;
    s->use_colorkey = SDL_FALSE;
    s->clip_rect = (SDL_Rect){0, 0, width, height};
    s->clip_enabled = SDL_FALSE;
    return s;
}

void SDL_FreeSurface(SDL_Surface *surface) {
    if (surface) {
        if (surface->pixels) {
            if (IS_PSRAM(surface->pixels)) psram_free(surface->pixels);
            else free(surface->pixels);
        }
        if (surface->format) {
            if (surface->format->palette) {
                SDL_PaletteRelease(surface->format->palette);
            }
            if (IS_PSRAM(surface->format)) psram_free(surface->format);
            else free(surface->format);
        }
        if (IS_PSRAM(surface)) psram_free(surface);
        else free(surface);
    }
}

int SDL_SetPaletteColors(SDL_Palette *palette, const SDL_Color *colors, int firstcolor, int ncolors) {
    if (!palette || !colors || ncolors <= 0) return -1;

    SDL_Palette* screen_palette = get_screen_palette();
    bool update_hardware = (palette == screen_palette);
    // printf("SDL_SetPaletteColors: first=%d n=%d\n", firstcolor, ncolors);
    for (int i = 0; i < ncolors; i++) {
        int idx = firstcolor + i;
        if (idx < palette->ncolors) {
            palette->colors[idx] = colors[i];
            if (update_hardware) {
                uint32_t color888 = (colors[i].r << 16) | (colors[i].g << 8) | colors[i].b;
                graphics_set_palette(idx, color888);
                // if (i < 5) printf("Pal[%d] = %06X\n", idx, color888);
            }
        }
    }
    return 0;
}

static bool is_palette_empty(SDL_Palette *pal) {
    if (!pal || !pal->colors) return true;
    // Check if all colors are black.
    for (int i = 0; i < pal->ncolors; i++) {
        if (pal->colors[i].r != 0 || pal->colors[i].g != 0 || pal->colors[i].b != 0) {
            return false;
        }
    }
    return true;
}

int SDL_BlitSurface(SDL_Surface *src, const SDL_Rect *srcrect, SDL_Surface *dst, SDL_Rect *dstrect) {
    if (!src || !dst) return -1;

    SDL_Rect s_rect = (SDL_Rect){0, 0, src->w, src->h};
    if (srcrect) s_rect = *srcrect;

    SDL_Rect d_rect = (SDL_Rect){0, 0, dst->w, dst->h};
    if (dstrect) {
        d_rect.x = dstrect->x;
        d_rect.y = dstrect->y;
        d_rect.w = s_rect.w;
        d_rect.h = s_rect.h;
    }

    // Clipping to surface bounds
    if (d_rect.x < 0) { s_rect.x -= d_rect.x; s_rect.w += d_rect.x; d_rect.w += d_rect.x; d_rect.x = 0; }
    if (d_rect.y < 0) { s_rect.y -= d_rect.y; s_rect.h += d_rect.y; d_rect.h += d_rect.y; d_rect.y = 0; }
    if (d_rect.x + d_rect.w > dst->w) { int diff = (d_rect.x + d_rect.w) - dst->w; s_rect.w -= diff; d_rect.w -= diff; }
    if (d_rect.y + d_rect.h > dst->h) { int diff = (d_rect.y + d_rect.h) - dst->h; s_rect.h -= diff; d_rect.h -= diff; }

    // Additional clipping to destination clip rect if enabled
    if (dst->clip_enabled) {
        // Clip left
        if (d_rect.x < dst->clip_rect.x) {
            int diff = dst->clip_rect.x - d_rect.x;
            s_rect.x += diff; s_rect.w -= diff; d_rect.w -= diff; d_rect.x = dst->clip_rect.x;
        }
        // Clip top
        if (d_rect.y < dst->clip_rect.y) {
            int diff = dst->clip_rect.y - d_rect.y;
            s_rect.y += diff; s_rect.h -= diff; d_rect.h -= diff; d_rect.y = dst->clip_rect.y;
        }
        // Clip right
        int clip_right = dst->clip_rect.x + dst->clip_rect.w;
        if (d_rect.x + d_rect.w > clip_right) {
            int diff = (d_rect.x + d_rect.w) - clip_right;
            s_rect.w -= diff; d_rect.w -= diff;
        }
        // Clip bottom
        int clip_bottom = dst->clip_rect.y + dst->clip_rect.h;
        if (d_rect.y + d_rect.h > clip_bottom) {
            int diff = (d_rect.y + d_rect.h) - clip_bottom;
            s_rect.h -= diff; d_rect.h -= diff;
        }
    }

    if (s_rect.w <= 0 || s_rect.h <= 0) return 0;

    const int src_bpp = src->format ? src->format->BytesPerPixel : 1;
    const int dst_bpp = dst->format ? dst->format->BytesPerPixel : 1;
    const bool paletted_copy = (src_bpp == 1 && dst_bpp == 1);
    Uint8 palette_map[256];
    bool use_palette_map = false;

    if (paletted_copy && src->format && dst->format) {
        SDL_Palette *src_palette = src->format->palette;
        SDL_Palette *dst_palette = dst->format->palette;
        
        // Only map if source palette is NOT empty (has colors) AND differs from dest
        if (src_palette && dst_palette && src_palette != dst_palette && !is_palette_empty(src_palette)) {
            bool palettes_differ = true;
            if (src_palette->ncolors == dst_palette->ncolors) {
                 if (memcmp(src_palette->colors, dst_palette->colors, src_palette->ncolors * sizeof(SDL_Color)) == 0) {
                     palettes_differ = false;
                 }
            }

            if (palettes_differ) {
                use_palette_map = true;
                for (int i = 0; i < 256; ++i) {
                    palette_map[i] = (Uint8)i;
                }
                int max_src_colors = src_palette->ncolors;
                if (max_src_colors > 256) max_src_colors = 256;
                for (int i = 0; i < max_src_colors; ++i) {
                    palette_map[i] = find_best_palette_index(&src_palette->colors[i], dst_palette);
                }
            }
        }
    }

    Uint8 *src_pixels = (Uint8 *)src->pixels;
    Uint8 *dst_pixels = (Uint8 *)dst->pixels;

    // Debug print for blit (throttled)
    static int blit_debug_count = 0;
    if (blit_debug_count < 10) {
        // printf("Blit: %dx%d bpp:%d->%d key:%d val:%d map:%d\n", 
        //     s_rect.w, s_rect.h, src_bpp, dst_bpp, src->use_colorkey, src->colorkey, use_palette_map);
        blit_debug_count++;
    }

    for (int y = 0; y < s_rect.h; y++) {
        Uint8 *s_row = src_pixels + (s_rect.y + y) * src->pitch + s_rect.x * src_bpp;
        Uint8 *d_row = dst_pixels + (d_rect.y + y) * dst->pitch + d_rect.x * dst_bpp;
        
        if (paletted_copy) {
            for (int x = 0; x < s_rect.w; ++x) {
                Uint8 pixel = s_row[x];
                if (src->use_colorkey && pixel == (Uint8)src->colorkey) {
                    continue;
                }
                Uint8 mapped_pixel = use_palette_map ? palette_map[pixel] : pixel;
                d_row[x] = mapped_pixel;
            }
        } else if (src_bpp == 3 && dst_bpp == 1) {
            // 24bpp -> 8bpp conversion (No Alpha)
            // Used for fonts/images loaded as RGB
            SDL_Palette *dst_pal = dst->format->palette;
            if (!dst_pal) dst_pal = get_screen_palette();

            for (int x = 0; x < s_rect.w; ++x) {
                Uint8 r = s_row[x * 3];
                Uint8 g = s_row[x * 3 + 1];
                Uint8 b = s_row[x * 3 + 2];

                // Check colorkey if enabled
                if (src->use_colorkey) {
                    Uint32 pixel = (r << 16) | (g << 8) | b;
                    // Colorkey for 24bpp is usually packed RGB
                    if ((pixel & 0xFFFFFF) == (src->colorkey & 0xFFFFFF)) {
                        continue; // Transparent
                    }
                }

                if (dst_pal) {
                    SDL_Color c = {r, g, b, 255};
                    d_row[x] = find_best_palette_index(&c, dst_pal);
                } else {
                    d_row[x] = 15; // White fallback
                }
            }
        } else if (src_bpp == 4 && dst_bpp == 1) {
            // 32bpp -> 8bpp conversion
            Uint32 *s_row_32 = (Uint32*)s_row;
            SDL_Palette *dst_pal = dst->format->palette;
            if (!dst_pal) dst_pal = get_screen_palette();

            for (int x = 0; x < s_rect.w; ++x) {
                Uint32 pixel = s_row_32[x];
                Uint8 a = (pixel >> 24) & 0xFF;
                
                // If alpha is transparent, do NOT write to destination (preserve background)
                if (a < 128) continue; 

                if (dst_pal) {
                    SDL_Color c;
                    c.r = pixel & 0xFF;          // stbi loads RGBA (little-endian)
                    c.g = (pixel >> 8) & 0xFF;
                    c.b = (pixel >> 16) & 0xFF;
                    d_row[x] = find_best_palette_index(&c, dst_pal);
                } else {
                    d_row[x] = 15; // White fallback
                }
            }
        } else if (src_bpp == 1 && dst_bpp == 4) {
             // 8bpp -> 32bpp conversion (used for read_peel_from_screen)
             Uint32 *d_row_32 = (Uint32*)d_row;
             SDL_Palette *src_pal = src->format->palette;
             if (!src_pal) src_pal = get_screen_palette();
             
             for (int x = 0; x < s_rect.w; ++x) {
                 Uint8 idx = s_row[x];
                if (src->use_colorkey && idx == src->colorkey) {
                    // Transparent source pixel -> Transparent destination pixel
                    // Or should we preserve destination?
                    // Usually blit overwrites unless alpha blending is on.
                    // But for peel reading, we want the exact pixel.
                    // If source is transparent, we write transparent black?
                    // Or do we skip?
                    // If we are reading FROM screen, screen usually doesn't have transparency in the way we think.
                    // But if it does (index 0), we should map it.
                    // Let's just map the color.
                    continue;
                }                 if (src_pal) {
                     SDL_Color c = src_pal->colors[idx];
                     d_row_32[x] = (255 << 24) | (c.r << 16) | (c.g << 8) | c.b;
                 } else {
                     d_row_32[x] = (255 << 24) | (idx << 16) | (idx << 8) | idx; // Grayscale fallback
                 }
             }
        } else if (src_bpp == dst_bpp) {
            if (src->use_colorkey) {
                if (src_bpp == 1) {
                    Uint8 key = (Uint8)src->colorkey;
                    for (int x = 0; x < s_rect.w; ++x) {
                        if (s_row[x] != key) {
                            d_row[x] = s_row[x];
                        }
                    }
                } else {
                    // Fallback for other depths or if implementation is missing
                    memcpy(d_row, s_row, s_rect.w * src_bpp);
                }
            } else {
                memcpy(d_row, s_row, s_rect.w * src_bpp);
            }
        } else {
            int copy_bytes = s_rect.w * (src_bpp < dst_bpp ? src_bpp : dst_bpp);
            memcpy(d_row, s_row, copy_bytes);
        }
    }
    return 0;
}

int SDL_FillRect(SDL_Surface *dst, const SDL_Rect *rect, Uint32 color) {
    if (!dst) return -1;
    SDL_Rect d_rect = {0, 0, dst->w, dst->h};
    if (rect) d_rect = *rect;

    // Clipping to surface bounds
    if (d_rect.x < 0) { d_rect.w += d_rect.x; d_rect.x = 0; }
    if (d_rect.y < 0) { d_rect.h += d_rect.y; d_rect.y = 0; }
    if (d_rect.x + d_rect.w > dst->w) { d_rect.w = dst->w - d_rect.x; }
    if (d_rect.y + d_rect.h > dst->h) { d_rect.h = dst->h - d_rect.y; }

    // Clip to destination surface's clip rect if enabled
    if (dst->clip_enabled) {
        int clip_left = dst->clip_rect.x;
        int clip_top = dst->clip_rect.y;
        int clip_right = dst->clip_rect.x + dst->clip_rect.w;
        int clip_bottom = dst->clip_rect.y + dst->clip_rect.h;
        
        // Clip left edge
        if (d_rect.x < clip_left) {
            d_rect.w -= (clip_left - d_rect.x);
            d_rect.x = clip_left;
        }
        // Clip top edge
        if (d_rect.y < clip_top) {
            d_rect.h -= (clip_top - d_rect.y);
            d_rect.y = clip_top;
        }
        // Clip right edge
        if (d_rect.x + d_rect.w > clip_right) {
            d_rect.w = clip_right - d_rect.x;
        }
        // Clip bottom edge
        if (d_rect.y + d_rect.h > clip_bottom) {
            d_rect.h = clip_bottom - d_rect.y;
        }
    }

    if (d_rect.w <= 0 || d_rect.h <= 0) return 0;

    Uint8 *dst_pixels = (Uint8 *)dst->pixels;
    for (int y = 0; y < d_rect.h; y++) {
        Uint8 *d_row = dst_pixels + (d_rect.y + y) * dst->pitch + d_rect.x;
        memset(d_row, (Uint8)color, d_rect.w);
    }
    return 0;
}

int SDL_SetColorKey(SDL_Surface *surface, int flag, Uint32 key) {
    if (!surface) return -1;
    surface->use_colorkey = flag ? SDL_TRUE : SDL_FALSE;
    if (surface->format && surface->format->BytesPerPixel == 1) {
        surface->colorkey = key & 0xFF;
    } else {
        surface->colorkey = key;
    }
    return 0;
}

int SDL_RenderClear(SDL_Renderer *renderer) {
    return 0;
}

int SDL_RenderCopy(SDL_Renderer *renderer, SDL_Texture *texture, const SDL_Rect *srcrect, const SDL_Rect *dstrect) {
    // In SDLPoP, RenderCopy is used to copy the final texture to screen.
    // We intercept UpdateTexture instead, or just assume the texture is the screen.
    return 0;
}

extern uint8_t graphics_buffer[]; // Defined in main.c

int SDL_UpdateTexture(SDL_Texture *texture, const SDL_Rect *rect, const void *pixels, int pitch) {
    // Copy pixels to graphics_buffer
    // Assuming 320x200 input and 320x240 output
    // We center it vertically?
    
    Uint8 *src = (Uint8 *)pixels;
    Uint8 *dst = graphics_buffer;
    
    int w = 320;
    int h = 200;
    int dst_pitch = 320;
    const int output_height = 240;
    
    // Center vertically: (240 - 200) / 2 = 20 offset
    int y_offset = (output_height - h) / 2;
    if (y_offset < 0) y_offset = 0;
    int bottom_pad = output_height - (y_offset + h);
    if (bottom_pad < 0) bottom_pad = 0;
    
    // Debug print every 60 frames
    static int frame_count = 0;
    frame_count++;
    if (frame_count == 1 || frame_count % 60 == 0) {
        // printf("SDL_UpdateTexture: src=%p dst=%p pitch=%d\n", src, dst, pitch);
        // Visual heartbeat: Blink a pixel at center (White/Black)
        // 320 * 120 + 160 = 38560
        if (IS_PSRAM(graphics_buffer)) {
             graphics_buffer[38560] = (frame_count % 120 < 60) ? 15 : 0;
             graphics_buffer[38561] = (frame_count % 120 < 60) ? 15 : 0;
             graphics_buffer[38562] = (frame_count % 120 < 60) ? 15 : 0;
             graphics_buffer[38563] = (frame_count % 120 < 60) ? 15 : 0;
        }
    }

    if (y_offset > 0) {
        memset(dst, 0, dst_pitch * y_offset);
    }
    if (bottom_pad > 0) {
        memset(dst + (y_offset + h) * dst_pitch, 0, dst_pitch * bottom_pad);
    }

    for (int y = 0; y < h; y++) {
        memcpy(dst + (y + y_offset) * dst_pitch, src + y * pitch, w);
    }
    
    // DEBUG: Clear status bar AFTER copying to dst (graphics_buffer)
    // This should definitely work - clear rows 192-199 in the OUTPUT
    // y=192 in input = row (192 + y_offset) in output = row 212 in graphics_buffer
    for (int y = 192; y < 200; y++) {
        memset(dst + (y + y_offset) * dst_pitch, 0, w);
    }
    
    // DEBUG: Draw a bright line at y=191 to confirm this code runs (palette index 4 = red)
    memset(dst + (191 + y_offset) * dst_pitch, 4, w);
    
    return 0;
}

void SDL_RenderPresent(SDL_Renderer *renderer) {
    // Nothing to do, DMA handles it
}

int SDL_SetRenderDrawColor(SDL_Renderer *renderer, Uint8 r, Uint8 g, Uint8 b, Uint8 a) {
    return 0;
}

int SDL_RenderFillRect(SDL_Renderer *renderer, const SDL_Rect *rect) {
    return 0;
}

void SDL_Delay(Uint32 ms) {
    sleep_ms(ms);
}

Uint32 SDL_GetTicks(void) {
    return (time_us_32() / 1000) - start_time;
}

static Uint8 keyboard_state[SDL_NUM_SCANCODES];

SDL_Scancode PS2_to_SDL_Scancode(uint8_t scancode, bool is_e0) {
    if (is_e0) {
        switch (scancode) {
            case 0x75: return SDL_SCANCODE_UP;
            case 0x72: return SDL_SCANCODE_DOWN;
            case 0x6B: return SDL_SCANCODE_LEFT;
            case 0x74: return SDL_SCANCODE_RIGHT;
            case 0x6C: return SDL_SCANCODE_HOME;
            case 0x69: return SDL_SCANCODE_END;
            case 0x7D: return SDL_SCANCODE_PAGEUP;
            case 0x7A: return SDL_SCANCODE_PAGEDOWN;
            case 0x70: return SDL_SCANCODE_INSERT;
            case 0x71: return SDL_SCANCODE_DELETE;
            case 0x5A: return SDL_SCANCODE_KP_ENTER;
            case 0x4A: return SDL_SCANCODE_KP_DIVIDE;
            case 0x11: return SDL_SCANCODE_RALT;
            case 0x14: return SDL_SCANCODE_RCTRL;
        }
    } else {
        switch (scancode) {
            case 0x1C: return SDL_SCANCODE_A;
            case 0x32: return SDL_SCANCODE_B;
            case 0x21: return SDL_SCANCODE_C;
            case 0x23: return SDL_SCANCODE_D;
            case 0x24: return SDL_SCANCODE_E;
            case 0x2B: return SDL_SCANCODE_F;
            case 0x34: return SDL_SCANCODE_G;
            case 0x33: return SDL_SCANCODE_H;
            case 0x43: return SDL_SCANCODE_I;
            case 0x3B: return SDL_SCANCODE_J;
            case 0x42: return SDL_SCANCODE_K;
            case 0x4B: return SDL_SCANCODE_L;
            case 0x3A: return SDL_SCANCODE_M;
            case 0x31: return SDL_SCANCODE_N;
            case 0x44: return SDL_SCANCODE_O;
            case 0x4D: return SDL_SCANCODE_P;
            case 0x15: return SDL_SCANCODE_Q;
            case 0x2D: return SDL_SCANCODE_R;
            case 0x1B: return SDL_SCANCODE_S;
            case 0x2C: return SDL_SCANCODE_T;
            case 0x3C: return SDL_SCANCODE_U;
            case 0x2A: return SDL_SCANCODE_V;
            case 0x1D: return SDL_SCANCODE_W;
            case 0x22: return SDL_SCANCODE_X;
            case 0x35: return SDL_SCANCODE_Y;
            case 0x1A: return SDL_SCANCODE_Z;
            
            case 0x16: return SDL_SCANCODE_1;
            case 0x1E: return SDL_SCANCODE_2;
            case 0x26: return SDL_SCANCODE_3;
            case 0x25: return SDL_SCANCODE_4;
            case 0x2E: return SDL_SCANCODE_5;
            case 0x36: return SDL_SCANCODE_6;
            case 0x3D: return SDL_SCANCODE_7;
            case 0x3E: return SDL_SCANCODE_8;
            case 0x46: return SDL_SCANCODE_9;
            case 0x45: return SDL_SCANCODE_0;
            
            case 0x5A: return SDL_SCANCODE_RETURN;
            case 0x76: return SDL_SCANCODE_ESCAPE;
            case 0x66: return SDL_SCANCODE_BACKSPACE;
            case 0x0D: return SDL_SCANCODE_TAB;
            case 0x29: return SDL_SCANCODE_SPACE;
            
            case 0x12: return SDL_SCANCODE_LSHIFT;
            case 0x59: return SDL_SCANCODE_RSHIFT;
            case 0x14: return SDL_SCANCODE_LCTRL;
            case 0x11: return SDL_SCANCODE_LALT;
            
            case 0x70: return SDL_SCANCODE_KP_0;
            case 0x69: return SDL_SCANCODE_KP_1;
            case 0x72: return SDL_SCANCODE_KP_2;
            case 0x7A: return SDL_SCANCODE_KP_3;
            case 0x6B: return SDL_SCANCODE_KP_4;
            case 0x73: return SDL_SCANCODE_KP_5;
            case 0x74: return SDL_SCANCODE_KP_6;
            case 0x6C: return SDL_SCANCODE_KP_7;
            case 0x75: return SDL_SCANCODE_KP_8;
            case 0x7D: return SDL_SCANCODE_KP_9;
            case 0x71: return SDL_SCANCODE_KP_PERIOD;
            case 0x79: return SDL_SCANCODE_KP_PLUS;
            case 0x7B: return SDL_SCANCODE_KP_MINUS;
            case 0x7C: return SDL_SCANCODE_KP_MULTIPLY;
            case 0x77: return SDL_SCANCODE_NUMLOCKCLEAR;
            
            case 0x05: return SDL_SCANCODE_F1;
            case 0x06: return SDL_SCANCODE_F2;
            case 0x04: return SDL_SCANCODE_F3;
            case 0x0C: return SDL_SCANCODE_F4;
            case 0x03: return SDL_SCANCODE_F5;
            case 0x0B: return SDL_SCANCODE_F6;
            case 0x83: return SDL_SCANCODE_F7;
            case 0x0A: return SDL_SCANCODE_F8;
            case 0x01: return SDL_SCANCODE_F9;
            case 0x09: return SDL_SCANCODE_F10;
            case 0x78: return SDL_SCANCODE_F11;
            case 0x07: return SDL_SCANCODE_F12;
        }
    }
    return SDL_SCANCODE_UNKNOWN;
}

int SDL_PollEvent(SDL_Event *event) {
    uint8_t scancode;
    bool pressed, is_e0, is_e1;
    
    if (decode_PS2(&scancode, &pressed, &is_e0, &is_e1)) {
        SDL_Scancode sdl_scancode = PS2_to_SDL_Scancode(scancode, is_e0);
        
        if (sdl_scancode != SDL_SCANCODE_UNKNOWN) {
            if (pressed) {
                event->type = SDL_KEYDOWN;
                event->key.state = SDL_PRESSED;
                keyboard_state[sdl_scancode] = 1;
            } else {
                event->type = SDL_KEYUP;
                event->key.state = SDL_RELEASED;
                keyboard_state[sdl_scancode] = 0;
            }
            event->key.keysym.scancode = sdl_scancode;
            event->key.keysym.sym = sdl_scancode; 
            event->key.keysym.mod = KMOD_NONE;
            
            return 1;
        }
    }
    return 0;
}

const Uint8 *SDL_GetKeyboardState(int *numkeys) {
    if (numkeys) *numkeys = SDL_NUM_SCANCODES;
    return keyboard_state;
}

void SDL_ShowCursor(int toggle) {}
void SDL_SetWindowTitle(SDL_Window *window, const char *title) {}
void SDL_SetWindowIcon(SDL_Window *window, SDL_Surface *icon) {}

// Audio stubs
int SDL_OpenAudio(SDL_AudioSpec *desired, SDL_AudioSpec *obtained) {
    // Fake success for now to allow the game to start
    return 0; 
}
void SDL_PauseAudio(int pause_on) {}
void SDL_CloseAudio(void) {}
void SDL_LockAudio(void) {}
void SDL_UnlockAudio(void) {}

// RWops implementation for memory
size_t mem_read(SDL_RWops *context, void *ptr, size_t size, size_t maxnum) {
    size_t total_bytes = size * maxnum;
    size_t bytes_left = context->stop - context->here;
    
    if (total_bytes == 0) return 0;
    
    if (total_bytes > bytes_left) {
        total_bytes = bytes_left;
    }
    
    memcpy(ptr, context->here, total_bytes);
    context->here += total_bytes;
    
    return total_bytes / size;
}

size_t mem_write(SDL_RWops *context, const void *ptr, size_t size, size_t num) {
    return 0; // Read-only
}

Sint32 mem_seek(SDL_RWops *context, Sint32 offset, int whence) {
    const Uint8 *new_pos;
    
    switch (whence) {
        case RW_SEEK_SET:
            new_pos = context->base + offset;
            break;
        case RW_SEEK_CUR:
            new_pos = context->here + offset;
            break;
        case RW_SEEK_END:
            new_pos = context->stop + offset;
            break;
        default:
            return -1;
    }
    
    if (new_pos < context->base) new_pos = context->base;
    if (new_pos > context->stop) new_pos = context->stop;
    
    context->here = new_pos;
    return (Sint32)(context->here - context->base);
}

int mem_close(SDL_RWops *context) {
    if (context) {
        free(context);
    }
    return 0;
}

Sint32 mem_tell(SDL_RWops *context) {
    return (Sint32)(context->here - context->base);
}

SDL_RWops *SDL_RWFromConstMem(const void *mem, int size) {
    SDL_RWops *rw = (SDL_RWops *)malloc(sizeof(SDL_RWops));
    if (!rw) return NULL;
    
    rw->base = (const Uint8 *)mem;
    rw->here = rw->base;
    rw->stop = rw->base + size;
    rw->type = 1;
    
    rw->read = mem_read;
    rw->write = mem_write;
    rw->seek = mem_seek;
    rw->close = mem_close;
    
    return rw;
}

SDL_RWops *SDL_RWFromMem(void *mem, int size) {
    return SDL_RWFromConstMem(mem, size);
}

SDL_RWops *SDL_RWFromFile(const char *file, const char *mode) {
    // Normalize path: replace backslashes with forward slashes
    char normalized_path[256];
    strncpy(normalized_path, file, sizeof(normalized_path) - 1);
    normalized_path[255] = '\0';
    
    for (int i = 0; normalized_path[i]; i++) {
        if (normalized_path[i] == '\\') normalized_path[i] = '/';
    }
    
    // Remove "data/" prefix if present, as our embedded names are relative to data/
    const char *search_name = normalized_path;
    if (strncmp(search_name, "data/", 5) == 0) {
        search_name += 5;
    }
    
    // Search in embedded files
    for (int i = 0; embedded_files[i].name != NULL; i++) {
        // Case-insensitive comparison might be needed, but let's try exact match first
        // Actually, let's do case-insensitive to be safe
        if (strcasecmp(embedded_files[i].name, search_name) == 0) {
            // Found!
            return SDL_RWFromConstMem(embedded_files[i].data, embedded_files[i].size);
        }
    }
    
    // Not found
    printf("File not found: %s (searched for %s)\n", file, search_name);
    return NULL;
}

size_t SDL_RWread(SDL_RWops *context, void *ptr, size_t size, size_t maxnum) {
    if (context && context->read) return context->read(context, ptr, size, maxnum);
    return 0;
}

size_t SDL_RWwrite(SDL_RWops *context, const void *ptr, size_t size, size_t num) {
    if (context && context->write) return context->write(context, ptr, size, num);
    return 0;
}

int SDL_RWclose(SDL_RWops *context) {
    if (context && context->close) return context->close(context);
    return 0;
}

Sint32 SDL_RWseek(SDL_RWops *context, Sint32 offset, int whence) {
    if (context && context->seek) return context->seek(context, offset, whence);
    return -1;
}

Sint32 SDL_RWtell(SDL_RWops *context) {
    if (context && context->type == 1) return mem_tell(context);
    return -1;
}

// SDL_image stubs
int IMG_Init(int flags) { return 0; }
void IMG_Quit(void) {}
SDL_Surface *IMG_Load(const char *file) { return NULL; } // TODO: Implement using vfs_fopen + IMG_Load_RW
SDL_Surface *IMG_ReadXPMFromArray(char **xpm) { return NULL; }

// IMG_GetError is implemented below with stb_image

int IMG_SavePNG(SDL_Surface *surface, const char *file) {
    return -1;
}

// BlendMode stubs
int SDL_SetSurfaceBlendMode(SDL_Surface *surface, SDL_BlendMode blendMode) {
    return 0; // Stub
}

Uint32 SDL_MapRGBA(const SDL_PixelFormat *format, Uint8 r, Uint8 g, Uint8 b, Uint8 a) {
    if (format && format->palette) {
        SDL_Color c = {r, g, b, a};
        return find_best_palette_index(&c, format->palette);
    }
    // Assuming ARGB8888 for 32-bit if no palette
    return ((Uint32)a << 24) | ((Uint32)r << 16) | ((Uint32)g << 8) | (Uint32)b;
}

Uint32 SDL_MapRGB(const SDL_PixelFormat *format, Uint8 r, Uint8 g, Uint8 b) {
    if (format && format->palette) {
        SDL_Color c = {r, g, b, 255};
        return find_best_palette_index(&c, format->palette);
    }
    return ((Uint32)r << 16) | ((Uint32)g << 8) | (Uint32)b;
}

void SDL_RenderGetScale(SDL_Renderer *renderer, float *scale_x, float *scale_y) {
    if (scale_x) *scale_x = 1.0f;
    if (scale_y) *scale_y = 1.0f;
}

void SDL_RenderGetLogicalSize(SDL_Renderer *renderer, int *w, int *h) {
    if (w) *w = 320;
    if (h) *h = 200;
}

void SDL_RenderGetViewport(SDL_Renderer *renderer, SDL_Rect *rect) {
    if (rect) {
        rect->x = 0;
        rect->y = 0;
        rect->w = 320;
        rect->h = 200;
    }
}

Uint32 SDL_GetMouseState(int *x, int *y) {
    if (x) *x = 0;
    if (y) *y = 0;
    return 0;
}

int SDL_SetWindowFullscreen(SDL_Window *window, Uint32 flags) {
    return 0;
}

const char *SDL_GetScancodeName(SDL_Scancode scancode) {
    return "Key";
}

Uint32 SDL_GetWindowFlags(SDL_Window *window) {
    return 0;
}

Uint64 SDL_GetPerformanceCounter(void) {
    return time_us_64();
}

Uint64 SDL_GetPerformanceFrequency(void) {
    return 1000000;
}

Uint32 SDL_SwapBE32(Uint32 x) {
    return __builtin_bswap32(x);
}

Uint16 SDL_SwapBE16(Uint16 x) {
    return __builtin_bswap16(x);
}

int SDL_ShowSimpleMessageBox(Uint32 flags, const char *title, const char *message, SDL_Window *window) {
    printf("MessageBox: %s - %s\n", title, message);
    // Blink LED rapidly to indicate error
    gpio_init(25);
    gpio_set_dir(25, GPIO_OUT);
    for(int i=0; i<20; i++) {
        gpio_put(25, 1); sleep_ms(50);
        gpio_put(25, 0); sleep_ms(50);
    }
    return 0;
}

SDL_RWops *SDL_RWFromMem(void *mem, int size); // Forward declaration

void SDL_GetVersion(SDL_version *ver) {
    if (ver) {
        ver->major = 2;
        ver->minor = 0;
        ver->patch = 4;
    }
}

SDL_TimerID SDL_AddTimer(Uint32 interval, SDL_TimerCallback callback, void *param) {
    // TODO: Implement timer
    return 0;
}

SDL_Haptic *SDL_HapticOpen(int device_index) {
    return NULL; // Stub
}

int SDL_HapticRumbleInit(SDL_Haptic *haptic) {
    return -1; // Stub
}

int SDL_HapticRumblePlay(SDL_Haptic *haptic, float strength, Uint32 length) {
    return -1; // Stub
}

SDL_Surface *SDL_ConvertSurface(SDL_Surface *src, const SDL_PixelFormat *fmt, Uint32 flags) {
    if (!src || !fmt) return NULL;
    
    SDL_Surface *new_surf = SDL_CreateRGBSurface(flags, src->w, src->h, fmt->BitsPerPixel, 0, 0, 0, 0);
    if (!new_surf) return NULL;

    // Copy palette if exists
    if (src->format->palette && new_surf->format->palette) {
        SDL_SetPaletteColors(new_surf->format->palette, src->format->palette->colors, 0, src->format->palette->ncolors);
    } else if (new_surf->format->palette && fmt->palette) {
         SDL_SetPaletteColors(new_surf->format->palette, fmt->palette->colors, 0, fmt->palette->ncolors);
    }

    // Conversion logic
    if (src->format->BitsPerPixel == 32 && fmt->BitsPerPixel == 8) {
        // 32-bit RGBA to 8-bit Indexed
        SDL_Palette *dst_pal = fmt->palette;
        if (!dst_pal && new_surf->format->palette) dst_pal = new_surf->format->palette;
        
        if (dst_pal) {
            Uint32 *src_pixels = (Uint32 *)src->pixels;
            Uint8 *dst_pixels = (Uint8 *)new_surf->pixels;
            for (int i = 0; i < src->w * src->h; i++) {
                Uint32 pixel = src_pixels[i];
                SDL_Color c;
                c.r = pixel & 0xFF;
                c.g = (pixel >> 8) & 0xFF;
                c.b = (pixel >> 16) & 0xFF;
                c.a = (pixel >> 24) & 0xFF;
                
                if (c.a < 128) {
                    dst_pixels[i] = 0; // Transparent? Assuming 0 is transparent
                } else {
                    dst_pixels[i] = find_best_palette_index(&c, dst_pal);
                }
            }
        }
    } else if (src->format->BitsPerPixel == fmt->BitsPerPixel) {
        // Same depth, just copy
        memcpy(new_surf->pixels, src->pixels, src->pitch * src->h);
    } else {
        // Fallback: clear to black
        memset(new_surf->pixels, 0, new_surf->pitch * new_surf->h);
    }

    return new_surf;
}

int SDL_SetSurfacePalette(SDL_Surface *surface, SDL_Palette *palette) {
    if (!surface || !palette) return -1;
    if (surface->format->palette) {
        // We should probably free the old palette if we owned it, but SDL memory management is tricky.
        // For now, let's just copy colors if sizes match, or replace the pointer?
        // SDL_SetSurfacePalette documentation says: "The palette is copied."
        SDL_SetPaletteColors(surface->format->palette, palette->colors, 0, palette->ncolors);
    }
    return 0;
}

int SDL_SetSurfaceAlphaMod(SDL_Surface *surface, Uint8 alpha) {
    return 0; // Stub
}

int SDL_LockSurface(SDL_Surface *surface) { return 0; }
void SDL_UnlockSurface(SDL_Surface *surface) {}
// SDL_RWFromConstMem is implemented above

const char *IMG_GetError(void) {
    return stbi_failure_reason();
}

SDL_Surface *IMG_Load_RW(SDL_RWops *src, int freesrc) {
    if (!src) return NULL;

    const Uint8 *data = src->base;
    int size = (int)(src->stop - src->base);
    
    int w, h, channels;
    
    // Save temp offset
    size_t saved_offset = psram_get_temp_offset();
    
    // Use temp allocator for stbi
    psram_set_temp_mode(1);
    // printf("IMG_Load_RW: calling stbi_load_from_memory(size=%d)\n", size);
    unsigned char *pixels = stbi_load_from_memory(data, size, &w, &h, &channels, 4); // Force 4 channels (RGBA)
    // printf("IMG_Load_RW: stbi returned %p, w=%d, h=%d, channels=%d\n", pixels, w, h, channels);
    psram_set_temp_mode(0);
    
    if (!pixels) {
        printf("IMG_Load_RW: stbi failed: %s\n", stbi_failure_reason());
        psram_set_temp_offset(saved_offset); // Reset temp memory even on failure
        if (freesrc) SDL_RWclose(src);
        return NULL;
    }
    
    // Create SDL_Surface (32-bit RGBA)
    Uint32 rmask = 0x000000FF;
    Uint32 gmask = 0x0000FF00;
    Uint32 bmask = 0x00FF0000;
    Uint32 amask = 0xFF000000;
    
    SDL_Surface *new_surf = SDL_CreateRGBSurface(0, w, h, 32, rmask, gmask, bmask, amask);
    if (new_surf) {
        // printf("IMG_Load_RW: copying pixels to surface %p\n", new_surf);
        memcpy(new_surf->pixels, pixels, w * h * 4);
    } else {
        printf("IMG_Load_RW: SDL_CreateRGBSurface failed\n");
    }
    
    // stbi_image_free(pixels); // No-op in PSRAM allocator, but good practice to call it if we weren't resetting
    
    // Reset temp memory to reclaim space used by stbi
    psram_set_temp_offset(saved_offset);
    
    if (freesrc) SDL_RWclose(src);
    
    return new_surf;
}

int SDL_NumJoysticks(void) { return 0; }
int SDL_GameControllerAddMappingsFromFile(const char *file) { return 0; }
SDL_bool SDL_IsGameController(int joystick_index) { return SDL_FALSE; }
SDL_GameController *SDL_GameControllerOpen(int joystick_index) { return NULL; }
SDL_Joystick *SDL_JoystickOpen(int device_index) { return NULL; }
void SDL_SetTextInputRect(const SDL_Rect *rect) {}
void SDL_StartTextInput(void) {}
void SDL_StopTextInput(void) {}
int SDL_PushEvent(SDL_Event *event) { return 0; }
void SDL_RenderSetLogicalSize(SDL_Renderer *renderer, int w, int h) {}
int SDL_SetRenderTarget(SDL_Renderer *renderer, SDL_Texture *texture) { return 0; }
int SDL_BlitScaled(SDL_Surface *src, const SDL_Rect *srcrect, SDL_Surface *dst, SDL_Rect *dstrect) { 
    return SDL_BlitSurface(src, srcrect, dst, dstrect); 
}
SDL_Surface *SDL_ConvertSurfaceFormat(SDL_Surface *src, Uint32 pixel_format, Uint32 flags) { 
    // Just return a copy for now, ignoring format
    return SDL_ConvertSurface(src, NULL, flags); 
}
SDL_bool SDL_ISPIXELFORMAT_INDEXED(Uint32 format) { 
    // Assuming 8-bit is indexed
    return SDL_FALSE; 
}
SDL_GameController *SDL_GameControllerFromInstanceID(SDL_JoystickID joyid) { return NULL; }
void SDL_GameControllerClose(SDL_GameController *gamecontroller) {}
void SDL_SetClipRect(SDL_Surface *surface, const SDL_Rect *rect) {
    if (!surface) return;
    if (rect) {
        surface->clip_rect = *rect;
        surface->clip_enabled = SDL_TRUE;
    } else {
        surface->clip_rect = (SDL_Rect){0, 0, surface->w, surface->h};
        surface->clip_enabled = SDL_FALSE;
    }
}

int chdir(const char *path) { return 0; }
int mkdir(const char *path, int mode) { return 0; }

void debug_blink(int count) {
    gpio_init(25);
    gpio_set_dir(25, GPIO_OUT);
    for (int i = 0; i < count; i++) {
        gpio_put(25, 1);
        sleep_ms(400); // Slower blink
        gpio_put(25, 0);
        sleep_ms(400);
    }
    sleep_ms(1000); // Longer pause between codes
}
