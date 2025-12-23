#pragma once
#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>

// Basic types
typedef uint8_t Uint8;
typedef int8_t Sint8;
typedef uint16_t Uint16;
typedef int16_t Sint16;
typedef uint32_t Uint32;
typedef int32_t Sint32;
typedef uint64_t Uint64;
typedef int64_t Sint64;

// SDL Constants
#define SDL_INIT_VIDEO 0x00000020
#define SDL_INIT_TIMER 0x00000001
#define SDL_INIT_NOPARACHUTE 0x00100000
#define SDL_INIT_GAMECONTROLLER 0x00002000
#define SDL_INIT_HAPTIC 0x00001000

#define SDL_WINDOW_FULLSCREEN_DESKTOP 0x00001000
#define SDL_WINDOW_RESIZABLE 0x00000020
#define SDL_WINDOW_ALLOW_HIGHDPI 0x00002000

#define SDL_RENDERER_SOFTWARE 0x00000001
#define SDL_RENDERER_ACCELERATED 0x00000002
#define SDL_RENDERER_TARGETTEXTURE 0x00000008
#define SDL_FORCE_FULL_PALETTE 0x80000000u

#define SDL_HINT_WINDOWS_DISABLE_THREAD_NAMING "SDL_WINDOWS_DISABLE_THREAD_NAMING"
#define SDL_HINT_RENDER_SCALE_QUALITY "SDL_RENDER_SCALE_QUALITY"

typedef enum {
    SDL_FALSE = 0,
    SDL_TRUE = 1
} SDL_bool;

// Structures
typedef struct SDL_Rect {
    int x, y;
    int w, h;
} SDL_Rect;

typedef struct SDL_Color {
    Uint8 r;
    Uint8 g;
    Uint8 b;
    Uint8 a;
} SDL_Color;

typedef struct SDL_Palette {
    int ncolors;
    SDL_Color *colors;
    int refcount;
    Uint32 flags;
} SDL_Palette;

typedef struct SDL_PixelFormat {
    Uint32 format;
    SDL_Palette *palette;
    Uint8 BitsPerPixel;
    Uint8 BytesPerPixel;
    Uint32 Rmask;
    Uint32 Gmask;
    Uint32 Bmask;
    Uint32 Amask;
} SDL_PixelFormat;

typedef struct SDL_Surface {
    Uint32 flags;
    SDL_PixelFormat *format;
    int w, h;
    int pitch;
    void *pixels;
    void *userdata;
    int refcount;
    Uint32 colorkey;
    SDL_bool use_colorkey;
    SDL_Rect clip_rect;      // Clipping rectangle
    SDL_bool clip_enabled;   // Whether clipping is active
} SDL_Surface;

typedef struct SDL_Window SDL_Window;
typedef struct SDL_Renderer SDL_Renderer;
typedef struct SDL_Texture SDL_Texture;

typedef struct SDL_GameController SDL_GameController;
typedef struct SDL_Joystick SDL_Joystick;
typedef struct SDL_Haptic SDL_Haptic;

typedef struct SDL_RendererInfo {
    const char *name;
    Uint32 flags;
    Uint32 num_texture_formats;
    Uint32 texture_formats[16];
    int max_texture_width;
    int max_texture_height;
} SDL_RendererInfo;

// Keys
typedef int SDL_Keycode;
typedef int SDL_Scancode;

#define SDL_PRESSED 1
#define SDL_RELEASED 0
#define KMOD_NONE 0

#define SDLK_ESCAPE 27
#define SDLK_RETURN 13
#define SDLK_SPACE 32
#define SDLK_UP 1
#define SDLK_DOWN 2
#define SDLK_LEFT 3
#define SDLK_RIGHT 4
// Add more keys as needed

#define SDL_NUM_SCANCODES 512
#define SDL_SCANCODE_UNKNOWN 0
#define SDL_SCANCODE_A 4
#define SDL_SCANCODE_B 5
#define SDL_SCANCODE_C 6
#define SDL_SCANCODE_D 7
#define SDL_SCANCODE_E 8
#define SDL_SCANCODE_F 9
#define SDL_SCANCODE_G 10
#define SDL_SCANCODE_H 11
#define SDL_SCANCODE_I 12
#define SDL_SCANCODE_J 13
#define SDL_SCANCODE_K 14
#define SDL_SCANCODE_L 15
#define SDL_SCANCODE_M 16
#define SDL_SCANCODE_N 17
#define SDL_SCANCODE_O 18
#define SDL_SCANCODE_P 19
#define SDL_SCANCODE_Q 20
#define SDL_SCANCODE_R 21
#define SDL_SCANCODE_S 22
#define SDL_SCANCODE_T 23
#define SDL_SCANCODE_U 24
#define SDL_SCANCODE_V 25
#define SDL_SCANCODE_W 26
#define SDL_SCANCODE_X 27
#define SDL_SCANCODE_Y 28
#define SDL_SCANCODE_Z 29
#define SDL_SCANCODE_1 30
#define SDL_SCANCODE_2 31
#define SDL_SCANCODE_3 32
#define SDL_SCANCODE_4 33
#define SDL_SCANCODE_5 34
#define SDL_SCANCODE_6 35
#define SDL_SCANCODE_7 36
#define SDL_SCANCODE_8 37
#define SDL_SCANCODE_9 38
#define SDL_SCANCODE_0 39
#define SDL_SCANCODE_RETURN 40
#define SDL_SCANCODE_ESCAPE 41
#define SDL_SCANCODE_BACKSPACE 42
#define SDL_SCANCODE_TAB 43
#define SDL_SCANCODE_SPACE 44
#define SDL_SCANCODE_MINUS 45
#define SDL_SCANCODE_EQUALS 46
#define SDL_SCANCODE_LEFTBRACKET 47
#define SDL_SCANCODE_RIGHTBRACKET 48
#define SDL_SCANCODE_BACKSLASH 49
#define SDL_SCANCODE_NONUSHASH 50
#define SDL_SCANCODE_SEMICOLON 51
#define SDL_SCANCODE_APOSTROPHE 52
#define SDL_SCANCODE_GRAVE 53
#define SDL_SCANCODE_COMMA 54
#define SDL_SCANCODE_PERIOD 55
#define SDL_SCANCODE_SLASH 56
#define SDL_SCANCODE_CAPSLOCK 57
#define SDL_SCANCODE_F1 58
#define SDL_SCANCODE_F2 59
#define SDL_SCANCODE_F3 60
#define SDL_SCANCODE_F4 61
#define SDL_SCANCODE_F5 62
#define SDL_SCANCODE_F6 63
#define SDL_SCANCODE_F7 64
#define SDL_SCANCODE_F8 65
#define SDL_SCANCODE_F9 66
#define SDL_SCANCODE_F10 67
#define SDL_SCANCODE_F11 68
#define SDL_SCANCODE_F12 69
#define SDL_SCANCODE_PRINTSCREEN 70
#define SDL_SCANCODE_SCROLLLOCK 71
#define SDL_SCANCODE_PAUSE 72
#define SDL_SCANCODE_INSERT 73
#define SDL_SCANCODE_HOME 74
#define SDL_SCANCODE_PAGEUP 75
#define SDL_SCANCODE_DELETE 76
#define SDL_SCANCODE_END 77
#define SDL_SCANCODE_PAGEDOWN 78
#define SDL_SCANCODE_RIGHT 79
#define SDL_SCANCODE_LEFT 80
#define SDL_SCANCODE_DOWN 81
#define SDL_SCANCODE_UP 82
#define SDL_SCANCODE_NUMLOCKCLEAR 83
#define SDL_SCANCODE_KP_DIVIDE 84
#define SDL_SCANCODE_KP_MULTIPLY 85
#define SDL_SCANCODE_KP_MINUS 86
#define SDL_SCANCODE_KP_PLUS 87
#define SDL_SCANCODE_KP_ENTER 88
#define SDL_SCANCODE_KP_1 89
#define SDL_SCANCODE_KP_2 90
#define SDL_SCANCODE_KP_3 91
#define SDL_SCANCODE_KP_4 92
#define SDL_SCANCODE_KP_5 93
#define SDL_SCANCODE_KP_6 94
#define SDL_SCANCODE_KP_7 95
#define SDL_SCANCODE_KP_8 96
#define SDL_SCANCODE_KP_9 97
#define SDL_SCANCODE_KP_0 98
#define SDL_SCANCODE_KP_PERIOD 99
#define SDL_SCANCODE_LCTRL 224
#define SDL_SCANCODE_LSHIFT 225
#define SDL_SCANCODE_LALT 226
#define SDL_SCANCODE_LGUI 227
#define SDL_SCANCODE_RCTRL 228
#define SDL_SCANCODE_RSHIFT 229
#define SDL_SCANCODE_RALT 230
#define SDL_SCANCODE_RGUI 231
#define SDL_CONTROLLER_AXIS_LEFTX 0
#define SDL_CONTROLLER_AXIS_LEFTY 1
#define SDL_CONTROLLER_AXIS_RIGHTX 2
#define SDL_CONTROLLER_AXIS_RIGHTY 3
#define SDL_CONTROLLER_AXIS_TRIGGERLEFT 4
#define SDL_CONTROLLER_AXIS_TRIGGERRIGHT 5

#define SDL_SCANCODE_CLEAR 100

#define SDL_USEREVENT 0x8000
#define SDL_LASTEVENT 0xFFFF
#define SDL_TEXTINPUT 0x300
#define SDL_WINDOWEVENT 0x200
#define SDL_MOUSEWHEEL 0x403
#define SDL_CONTROLLERAXISMOTION 0x650
#define SDL_CONTROLLERBUTTONDOWN 0x651
#define SDL_CONTROLLERBUTTONUP 0x652
#define SDL_CONTROLLERDEVICEADDED 0x653
#define SDL_CONTROLLERDEVICEREMOVED 0x654
#define SDL_JOYAXISMOTION 0x600
#define SDL_JOYBUTTONDOWN 0x603
#define SDL_JOYBUTTONUP 0x604

#define SDL_WINDOWEVENT_SIZE_CHANGED 5
#define SDL_WINDOWEVENT_EXPOSED 3
#define SDL_WINDOWEVENT_FOCUS_GAINED 12

#define SDL_BUTTON_LEFT 1
#define SDL_BUTTON_RIGHT 3
#define SDL_BUTTON_X1 4

#define KMOD_SHIFT 0x0003
#define KMOD_CTRL 0x00C0
#define KMOD_ALT 0x0300

#define SDL_SCANCODE_APPLICATION 101
#define SDL_SCANCODE_VOLUMEUP 128
#define SDL_SCANCODE_VOLUMEDOWN 129
#define SDL_SCANCODE_MUTE 127
#define SDL_SCANCODE_AUDIOMUTE 262

#define SDL_CONTROLLER_BUTTON_A 0
#define SDL_CONTROLLER_BUTTON_B 1
#define SDL_CONTROLLER_BUTTON_X 2
#define SDL_CONTROLLER_BUTTON_Y 3
#define SDL_CONTROLLER_BUTTON_BACK 4
#define SDL_CONTROLLER_BUTTON_START 6
#define SDL_CONTROLLER_BUTTON_DPAD_UP 11
#define SDL_CONTROLLER_BUTTON_DPAD_DOWN 12
#define SDL_CONTROLLER_BUTTON_DPAD_LEFT 13
#define SDL_CONTROLLER_BUTTON_DPAD_RIGHT 14

#define SDL_JOYSTICK_X_AXIS 0
#define SDL_JOYSTICK_Y_AXIS 1
#ifndef SDL_JOYSTICK_BUTTON_X
#define SDL_JOYSTICK_BUTTON_X 0
#endif
#ifndef SDL_JOYSTICK_BUTTON_Y
#define SDL_JOYSTICK_BUTTON_Y 1
#endif

#define SDL_PIXELFORMAT_RGB24 3
#define SDL_PIXELFORMAT_RGB565 4
#define SDL_PIXELFORMAT_ARGB8888 6
#define SDL_TEXTUREACCESS_STREAMING 1
#define SDL_TEXTUREACCESS_TARGET 2
#define SDL_WINDOWPOS_UNDEFINED 0x1FFF0000
#define SDL_HINT_RENDER_VSYNC "SDL_RENDER_VSYNC"

#define AUDIO_S16SYS 0x8010
typedef Uint16 SDL_AudioFormat;
typedef Sint32 SDL_JoystickID;

typedef struct SDL_version {
    Uint8 major;
    Uint8 minor;
    Uint8 patch;
} SDL_version;

#define SDL_VERSION(x) do { (x)->major = 2; (x)->minor = 0; (x)->patch = 0; } while(0)

#define SDL_VERSIONNUM(X, Y, Z)                     \
    ((X)*1000 + (Y)*100 + (Z))

#define SDL_COMPILEDVERSION \
    SDL_VERSIONNUM(2, 0, 0)

#define SDL_VERSION_ATLEAST(X, Y, Z) \
    (SDL_COMPILEDVERSION >= SDL_VERSIONNUM(X, Y, Z))

#define SDL_COMPILE_TIME_ASSERT(name, x) typedef int SDL_compile_time_assert_ ## name[(x) * 2 - 1]

typedef int SDL_TimerID;
typedef Uint32 (*SDL_TimerCallback)(Uint32 interval, void *param);

// Events
typedef enum {
    SDL_FIRSTEVENT = 0,
    SDL_QUIT,
    SDL_KEYDOWN,
    SDL_KEYUP,
    SDL_MOUSEMOTION,
    SDL_MOUSEBUTTONDOWN,
    SDL_MOUSEBUTTONUP
} SDL_EventType;

typedef struct SDL_KeyboardEvent {
    Uint32 type;
    Uint32 timestamp;
    Uint32 windowID;
    Uint8 state;
    Uint8 repeat;
    struct {
        SDL_Scancode scancode;
        SDL_Keycode sym;
        Uint16 mod;
    } keysym;
} SDL_KeyboardEvent;

typedef struct SDL_UserEvent {
    Uint32 type;
    Uint32 timestamp;
    Uint32 windowID;
    Sint32 code;
    void *data1;
    void *data2;
} SDL_UserEvent;

typedef struct SDL_ControllerAxisEvent {
    Uint32 type;
    Uint32 timestamp;
    SDL_JoystickID which;
    Uint8 axis;
    Sint16 value;
} SDL_ControllerAxisEvent;

typedef struct SDL_ControllerButtonEvent {
    Uint32 type;
    Uint32 timestamp;
    SDL_JoystickID which;
    Uint8 button;
    Uint8 state;
} SDL_ControllerButtonEvent;

typedef struct SDL_ControllerDeviceEvent {
    Uint32 type;
    Uint32 timestamp;
    Sint32 which;
} SDL_ControllerDeviceEvent;

typedef struct SDL_JoyAxisEvent {
    Uint32 type;
    Uint32 timestamp;
    SDL_JoystickID which;
    Uint8 axis;
    Sint16 value;
} SDL_JoyAxisEvent;

typedef struct SDL_JoyButtonEvent {
    Uint32 type;
    Uint32 timestamp;
    SDL_JoystickID which;
    Uint8 button;
    Uint8 state;
} SDL_JoyButtonEvent;

typedef struct SDL_TextInputEvent {
    Uint32 type;
    Uint32 timestamp;
    Uint32 windowID;
    char text[32];
} SDL_TextInputEvent;

typedef struct SDL_WindowEvent {
    Uint32 type;
    Uint32 timestamp;
    Uint32 windowID;
    Uint8 event;
    Sint32 data1;
    Sint32 data2;
} SDL_WindowEvent;

typedef struct SDL_MouseWheelEvent {
    Uint32 type;
    Uint32 timestamp;
    Uint32 windowID;
    Uint32 which;
    Sint32 x;
    Sint32 y;
    Uint32 direction;
} SDL_MouseWheelEvent;

typedef struct SDL_MouseButtonEvent {
    Uint32 type;
    Uint32 timestamp;
    Uint32 windowID;
    Uint32 which;
    Uint8 button;
    Uint8 state;
    Uint8 clicks;
    Sint32 x;
    Sint32 y;
} SDL_MouseButtonEvent;

typedef union SDL_Event {
    Uint32 type;
    SDL_KeyboardEvent key;
    SDL_UserEvent user;
    SDL_ControllerAxisEvent caxis;
    SDL_ControllerButtonEvent cbutton;
    SDL_ControllerDeviceEvent cdevice;
    SDL_JoyAxisEvent jaxis;
    SDL_JoyButtonEvent jbutton;
    SDL_TextInputEvent text;
    SDL_WindowEvent window;
    SDL_MouseWheelEvent wheel;
    SDL_MouseButtonEvent button;
} SDL_Event;

// Functions
int SDL_Init(Uint32 flags);
int SDL_InitSubSystem(Uint32 flags);
void SDL_Quit(void);
const char *SDL_GetError(void);
void SDL_SetHint(const char *name, const char *value);

SDL_Window *SDL_CreateWindow(const char *title, int x, int y, int w, int h, Uint32 flags);
SDL_Renderer *SDL_CreateRenderer(SDL_Window *window, int index, Uint32 flags);
int SDL_GetRendererInfo(SDL_Renderer *renderer, SDL_RendererInfo *info);
SDL_Texture *SDL_CreateTexture(SDL_Renderer *renderer, Uint32 format, int access, int w, int h);
void SDL_DestroyTexture(SDL_Texture *texture);
void SDL_DestroyRenderer(SDL_Renderer *renderer);
void SDL_DestroyWindow(SDL_Window *window);

SDL_Surface *SDL_CreateRGBSurface(Uint32 flags, int width, int height, int depth, Uint32 Rmask, Uint32 Gmask, Uint32 Bmask, Uint32 Amask);
void SDL_FreeSurface(SDL_Surface *surface);
int SDL_SetPaletteColors(SDL_Palette *palette, const SDL_Color *colors, int firstcolor, int ncolors);
SDL_Palette *SDL_CreatePalette(int ncolors);
void SDL_PaletteAddRef(SDL_Palette *palette);
void SDL_PaletteRelease(SDL_Palette *palette);
void SDL_SurfaceAdoptPalette(SDL_Surface *surface, SDL_Palette *palette);
int SDL_BlitSurface(SDL_Surface *src, const SDL_Rect *srcrect, SDL_Surface *dst, SDL_Rect *dstrect);
int SDL_FillRect(SDL_Surface *dst, const SDL_Rect *rect, Uint32 color);
int SDL_SetColorKey(SDL_Surface *surface, int flag, Uint32 key);

int SDL_RenderClear(SDL_Renderer *renderer);
int SDL_RenderCopy(SDL_Renderer *renderer, SDL_Texture *texture, const SDL_Rect *srcrect, const SDL_Rect *dstrect);
void SDL_RenderPresent(SDL_Renderer *renderer);
int SDL_UpdateTexture(SDL_Texture *texture, const SDL_Rect *rect, const void *pixels, int pitch);
int SDL_SetRenderDrawColor(SDL_Renderer *renderer, Uint8 r, Uint8 g, Uint8 b, Uint8 a);
int SDL_RenderFillRect(SDL_Renderer *renderer, const SDL_Rect *rect);

void SDL_Delay(Uint32 ms);
Uint32 SDL_GetTicks(void);

int SDL_PollEvent(SDL_Event *event);
const Uint8 *SDL_GetKeyboardState(int *numkeys);

void SDL_ShowCursor(int toggle);
void SDL_SetWindowTitle(SDL_Window *window, const char *title);
void SDL_SetWindowIcon(SDL_Window *window, SDL_Surface *icon);

// Audio (stubs for now)
typedef void (*SDL_AudioCallback)(void *userdata, Uint8 *stream, int len);
typedef struct SDL_AudioSpec {
    int freq;
    Uint16 format;
    Uint8 channels;
    Uint8 silence;
    Uint16 samples;
    Uint16 padding;
    Uint32 size;
    SDL_AudioCallback callback;
    void *userdata;
} SDL_AudioSpec;

int SDL_OpenAudio(SDL_AudioSpec *desired, SDL_AudioSpec *obtained);
void SDL_PauseAudio(int pause_on);
void SDL_CloseAudio(void);
void SDL_LockAudio(void);
void SDL_UnlockAudio(void);

#define AUDIO_S16 0x8010
#define AUDIO_U8 0x0008

// RWops (File I/O)
typedef struct SDL_RWops {
    size_t (*read)(struct SDL_RWops *context, void *ptr, size_t size, size_t maxnum);
    size_t (*write)(struct SDL_RWops *context, const void *ptr, size_t size, size_t num);
    Sint32 (*seek)(struct SDL_RWops *context, Sint32 offset, int whence);
    int (*close)(struct SDL_RWops *context);
    
    // Custom data for memory reading
    const Uint8 *base;
    const Uint8 *here;
    const Uint8 *stop;
    Uint32 type; // 0 = unknown, 1 = memory
} SDL_RWops;

SDL_RWops *SDL_RWFromFile(const char *file, const char *mode);
size_t SDL_RWread(SDL_RWops *context, void *ptr, size_t size, size_t maxnum);
size_t SDL_RWwrite(SDL_RWops *context, const void *ptr, size_t size, size_t num);
int SDL_RWclose(SDL_RWops *context);
Sint32 SDL_RWseek(SDL_RWops *context, Sint32 offset, int whence);
Sint32 SDL_RWtell(SDL_RWops *context);

#define RW_SEEK_SET 0
#define RW_SEEK_CUR 1
#define RW_SEEK_END 2

#define SDL_ALPHA_OPAQUE 255
#define SDL_ALPHA_TRANSPARENT 0

typedef enum {
    SDL_BLENDMODE_NONE = 0x00000000,
    SDL_BLENDMODE_BLEND = 0x00000001,
    SDL_BLENDMODE_ADD = 0x00000002,
    SDL_BLENDMODE_MOD = 0x00000004
} SDL_BlendMode;

int SDL_SetSurfaceBlendMode(SDL_Surface *surface, SDL_BlendMode blendMode);
Uint32 SDL_MapRGBA(const SDL_PixelFormat *format, Uint8 r, Uint8 g, Uint8 b, Uint8 a);
Uint32 SDL_MapRGB(const SDL_PixelFormat *format, Uint8 r, Uint8 g, Uint8 b);

// Additional stubs
#define SDL_ENABLE 1
#define SDL_DISABLE 0

#define SDL_CONTROLLER_AXIS_LEFTX 0
#define SDL_CONTROLLER_AXIS_LEFTY 1
#define SDL_CONTROLLER_AXIS_RIGHTX 2
#define SDL_CONTROLLER_AXIS_RIGHTY 3
#define SDL_CONTROLLER_AXIS_TRIGGERLEFT 4
#define SDL_CONTROLLER_AXIS_TRIGGERRIGHT 5

#define SDL_SCANCODE_CLEAR 100

void SDL_RenderGetScale(SDL_Renderer *renderer, float *scale_x, float *scale_y);
void SDL_RenderGetLogicalSize(SDL_Renderer *renderer, int *w, int *h);
void SDL_RenderGetViewport(SDL_Renderer *renderer, SDL_Rect *rect);
Uint32 SDL_GetMouseState(int *x, int *y);
int SDL_SetWindowFullscreen(SDL_Window *window, Uint32 flags);
const char *SDL_GetScancodeName(SDL_Scancode scancode);
Uint32 SDL_GetWindowFlags(SDL_Window *window);
Uint64 SDL_GetPerformanceCounter(void);
Uint64 SDL_GetPerformanceFrequency(void);

#define SDL_memset memset
#define SDL_memcpy memcpy

Uint32 SDL_SwapBE32(Uint32 x);
Uint16 SDL_SwapBE16(Uint16 x);
#define SDL_SwapLE16(x) (x)
#define SDL_SwapLE32(x) (x)

#define SDL_MESSAGEBOX_ERROR 0x00000010

int SDL_ShowSimpleMessageBox(Uint32 flags, const char *title, const char *message, SDL_Window *window);
SDL_RWops *SDL_RWFromMem(void *mem, int size);

void SDL_GetVersion(SDL_version * ver);
SDL_TimerID SDL_AddTimer(Uint32 interval, SDL_TimerCallback callback, void *param);

SDL_Haptic *SDL_HapticOpen(int device_index);
int SDL_HapticRumbleInit(SDL_Haptic *haptic);
int SDL_HapticRumblePlay(SDL_Haptic *haptic, float strength, Uint32 length);

SDL_Surface *SDL_ConvertSurface(SDL_Surface *src, const SDL_PixelFormat *fmt, Uint32 flags);
int SDL_SetSurfacePalette(SDL_Surface *surface, SDL_Palette *palette);
int SDL_SetSurfaceAlphaMod(SDL_Surface *surface, Uint8 alpha);

int SDL_LockSurface(SDL_Surface *surface);
void SDL_UnlockSurface(SDL_Surface *surface);
SDL_RWops *SDL_RWFromConstMem(const void *mem, int size);
SDL_Surface *IMG_Load_RW(SDL_RWops *src, int freesrc);
int SDL_NumJoysticks(void);
int SDL_GameControllerAddMappingsFromFile(const char *file);
SDL_bool SDL_IsGameController(int joystick_index);
SDL_GameController *SDL_GameControllerOpen(int joystick_index);
SDL_Joystick *SDL_JoystickOpen(int device_index);
void SDL_SetTextInputRect(const SDL_Rect *rect);
void SDL_StartTextInput(void);
void SDL_StopTextInput(void);
int SDL_PushEvent(SDL_Event *event);
void SDL_RenderSetLogicalSize(SDL_Renderer *renderer, int w, int h);
int SDL_SetRenderTarget(SDL_Renderer *renderer, SDL_Texture *texture);
int SDL_BlitScaled(SDL_Surface *src, const SDL_Rect *srcrect, SDL_Surface *dst, SDL_Rect *dstrect);
SDL_Surface *SDL_ConvertSurfaceFormat(SDL_Surface *src, Uint32 pixel_format, Uint32 flags);
SDL_bool SDL_ISPIXELFORMAT_INDEXED(Uint32 format);
SDL_GameController *SDL_GameControllerFromInstanceID(SDL_JoystickID joyid);
void SDL_GameControllerClose(SDL_GameController *gamecontroller);
void SDL_SetClipRect(SDL_Surface *surface, const SDL_Rect *rect);

