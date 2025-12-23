#ifndef DIRENT_H
#define DIRENT_H

// We are using VFS, so we don't need these stubs.
// The types and functions are provided by vfs.h and redirected via macros in common.h.
// If this file is included, it should do nothing if VFS is active.

#ifndef opendir
// FatFS defines a typedef named DIR in ff.h. Some SDLPoP sources include ff.h
// before including this header (e.g. on POP_RP2350), so avoid redefining it.
#if !defined(FF_DEFINED)
typedef struct DIR DIR;
#endif
struct dirent {
    char d_name[256];
};

static inline DIR *opendir(const char *name) { return 0; }
static inline int closedir(DIR *dirp) { return 0; }
static inline struct dirent *readdir(DIR *dirp) { return 0; }
#endif

#endif
