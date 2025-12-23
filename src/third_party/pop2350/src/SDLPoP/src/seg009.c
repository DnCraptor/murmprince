/*
SDLPoP, a port/conversion of the DOS game Prince of Persia.
Copyright (C) 2013-2025  Dávid Nagy

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <https://www.gnu.org/licenses/>.

The authors of this program may be contacted at https://forum.princed.org
*/

#include "common.h"
#include "psram_allocator.h"
#include <time.h>
#include <errno.h>
#include "../../HDMI.h"

#ifdef _WIN32
#include <windows.h>
#include <wchar.h>
#else
#include "../../dirent.h"
#endif

#ifdef __amigaos4__
#include <workbench/startup.h>
#include <proto/dos.h>
#endif

// Most functions in this file are different from those in the original game.

void sdlperror(const char* header) {
	const char* error = SDL_GetError();
	printf("%s: %s\n",header,error);
	//quit(1);
}

char exe_dir[POP_MAX_PATH] = ".";
bool found_exe_dir = false;
#if ! (defined WIN32 || _WIN32 || WIN64 || _WIN64)
char home_dir[POP_MAX_PATH];
bool found_home_dir = false;
char share_dir[POP_MAX_PATH];
bool found_share_dir = false;
#endif

void find_exe_dir(void) {
	if (found_exe_dir) return;
#ifdef __amigaos4__
	if(g_argc == 0) { // from Workbench
		struct WBStartup *WBenchMsg = (struct WBStartup *)g_argv;
		NameFromLock( WBenchMsg->sm_ArgList->wa_Lock, exe_dir, sizeof(exe_dir) );
	}
	else { // from Shell/CLI
		NameFromLock( GetProgramDir(), exe_dir, sizeof(exe_dir) );
	}
#else
	snprintf_check(exe_dir, sizeof(exe_dir), "%s", g_argv[0]);
	char* last_slash = NULL;
	char* pos = exe_dir;
	for (char c = *pos; c != '\0'; ++pos, c = *pos) {
		if (c == '/' || c == '\\') {
			last_slash = pos;
		}
	}
	if (last_slash != NULL) {
		*last_slash = '\0';
	}
#endif
	found_exe_dir = true;
}

#if ! (defined WIN32 || _WIN32 || WIN64 || _WIN64)
void find_home_dir(void) {
	if (found_home_dir) return;
	const char* home_path = getenv("HOME");
	snprintf_check(home_dir, POP_MAX_PATH - 1, "%s/.%s", home_path, POP_DIR_NAME);
	if(file_exists(home_dir))
		found_home_dir = true;
}

void find_share_dir(void) {
	if (found_share_dir) return;
	snprintf_check(share_dir, POP_MAX_PATH - 1, "%s/%s", SHARE_PATH, POP_DIR_NAME);
	if(file_exists(share_dir))
		found_share_dir = true;
}
#endif

bool file_exists(const char* filename) {
	return (access(filename, F_OK) != -1);
}

const char* find_first_file_match(char* dst, int size, char* format, const char* filename) {
	find_exe_dir();
#if defined WIN32 || _WIN32 || WIN64 || _WIN64
	snprintf_check(dst, size, format, exe_dir, filename);
#else
	find_home_dir();
	find_share_dir();
	char* dirs[3] = {home_dir, share_dir, exe_dir};
	for (int i = 0; i < 3; i++) {
		snprintf_check(dst, size, format, dirs[i], filename);
		if(file_exists(dst))
			break;
	}
#endif
	return (const char*) dst;
}

const char* locate_save_file_(const char* filename, char* dst, int size) {
	find_exe_dir();
#if defined WIN32 || _WIN32 || WIN64 || _WIN64
	snprintf_check(dst, size, "%s/%s", exe_dir, filename);
#else
	find_home_dir();
	find_share_dir();
	char* dirs[3] = {home_dir, share_dir, exe_dir};
	for (int i = 0; i < 3; i++) {
		struct stat path_stat;
		int result = stat(dirs[i], &path_stat);
		if (result == 0 && S_ISDIR(path_stat.st_mode) && access(dirs[i], W_OK) == 0) {
			snprintf_check(dst, size, "%s/%s", dirs[i], filename);
			break;
		}
	}
#endif
	return (const char*) dst;
}
const char* locate_file_(const char* filename, char* path_buffer, int buffer_size) {
	if(file_exists(filename)) {
		return filename;
	} else {
		// If failed, it may be that SDLPoP is being run from the wrong different working directory.
		// We can try to rescue the situation by loading from the directory of the executable.
		return find_first_file_match(path_buffer, buffer_size, "%s/%s", filename);
	}
}

#ifdef _WIN32
// These macros are from the SDL2 source. (src/core/windows/SDL_windows.h)
// The pointers returned by these macros must be freed with SDL_free().
#define WIN_StringToUTF8(S) SDL_iconv_string("UTF-8", "UTF-16LE", (char *)(S), (SDL_wcslen(S)+1)*sizeof(WCHAR))
#define WIN_UTF8ToString(S) (WCHAR *)SDL_iconv_string("UTF-16LE", "UTF-8", (char *)(S), SDL_strlen(S)+1)

// This hack is needed because SDL uses UTF-8 everywhere (even in argv!), but fopen on Windows uses whatever code page is currently set.
FILE* fopen_UTF8(const char* filename_UTF8, const char* mode_UTF8) {
	WCHAR* filename_UTF16 = WIN_UTF8ToString(filename_UTF8);
	WCHAR* mode_UTF16 = WIN_UTF8ToString(mode_UTF8);
	FILE* result = _wfopen(filename_UTF16, mode_UTF16);
	SDL_free(mode_UTF16);
	SDL_free(filename_UTF16);
	return result;
}

int chdir_UTF8(const char* path_UTF8) {
	WCHAR* path_UTF16 = WIN_UTF8ToString(path_UTF8);
	int result = _wchdir(path_UTF16);
	SDL_free(path_UTF16);
	return result;
}

int mkdir_UTF8(const char* path_UTF8) {
	WCHAR* path_UTF16 = WIN_UTF8ToString(path_UTF8);
	int result = _wmkdir(path_UTF16);
	SDL_free(path_UTF16);
	return result;
}

int access_UTF8(const char* filename_UTF8, int mode) {
	WCHAR* filename_UTF16 = WIN_UTF8ToString(filename_UTF8);
	int result = _waccess(filename_UTF16, mode);
	SDL_free(filename_UTF16);
	return result;
}

int stat_UTF8(const char *filename_UTF8, struct stat *_Stat) {
	WCHAR* filename_UTF16 = WIN_UTF8ToString(filename_UTF8);
#ifdef _MSC_VER
	int result = _wstat(filename_UTF16, _Stat);
#else
	// There is a _wstat() function on Dev-C++ as well, but it expects the second argument to be a different type than stat().
	int result = wstat(filename_UTF16, _Stat);
#endif
	SDL_free(filename_UTF16);
	return result;
}

#endif //_WIN32

// OS abstraction for listing directory contents
// Directory listing using dirent.h is available using MinGW on Windows, but not using MSVC (need to use Win32 API).
// - Under GNU/Linux, etc (or if compiling with MinGW on Windows), we can use dirent.h
// - Under Windows, we'd like to directly call the Win32 API. (Note: MSVC does not include dirent.h)
// NOTE: If we are using MinGW, we'll opt to use the Win32 API as well: dirent.h would just wrap Win32 anyway!

#ifdef _WIN32
struct directory_listing_type {
	WIN32_FIND_DATAW find_data;
	HANDLE search_handle;
	char* current_filename_UTF8;
};

directory_listing_type* create_directory_listing_and_find_first_file(const char* directory, const char* extension) {
	directory_listing_type* directory_listing = calloc(1, sizeof(directory_listing_type));
	char search_pattern[POP_MAX_PATH];
	snprintf_check(search_pattern, POP_MAX_PATH, "%s/*.%s", directory, extension);
	WCHAR* search_pattern_UTF16 = WIN_UTF8ToString(search_pattern);
	directory_listing->search_handle = FindFirstFileW( search_pattern_UTF16, &directory_listing->find_data );
	SDL_free(search_pattern_UTF16);
	if (directory_listing->search_handle != INVALID_HANDLE_VALUE) {
		return directory_listing;
	} else {
		free(directory_listing);
		return NULL;
	}
}

char* get_current_filename_from_directory_listing(directory_listing_type* data) {
	SDL_free(data->current_filename_UTF8);
	data->current_filename_UTF8 = NULL;
	data->current_filename_UTF8 = WIN_StringToUTF8(data->find_data.cFileName);
	return data->current_filename_UTF8;
}

bool find_next_file(directory_listing_type* data) {
	return (bool) FindNextFileW( data->search_handle, &data->find_data );
}

void close_directory_listing(directory_listing_type* data) {
	FindClose(data->search_handle);
	SDL_free(data->current_filename_UTF8);
	data->current_filename_UTF8 = NULL;
	free(data);
}

#else // use dirent.h API for listing files

struct directory_listing_type {
	DIR* dp;
	char* found_filename;
	const char* extension;
};

directory_listing_type* create_directory_listing_and_find_first_file(const char* directory, const char* extension) {
	directory_listing_type* data = calloc(1, sizeof(directory_listing_type));
	bool ok = false;
	data->dp = opendir(directory);
	if (data->dp != NULL) {
		struct dirent* ep;
		while ((ep = readdir(data->dp))) {
			char *ext = strrchr(ep->d_name, '.');
			if (ext != NULL && strcasecmp(ext+1, extension) == 0) {
				data->found_filename = ep->d_name;
				data->extension = extension;
				ok = true;
				break;
			}
		}
	}
	if (ok) {
		return data;
	} else {
		free(data);
		return NULL;
	}
}

char* get_current_filename_from_directory_listing(directory_listing_type* data) {
	return data->found_filename;
}

bool find_next_file(directory_listing_type* data) {
	bool ok = false;
	struct dirent* ep;
	while ((ep = readdir(data->dp))) {
		char *ext = strrchr(ep->d_name, '.');
		if (ext != NULL && strcasecmp(ext+1, data->extension) == 0) {
			data->found_filename = ep->d_name;
			ok = true;
			break;
		}
	}
	return ok;
}

void close_directory_listing(directory_listing_type *data) {
	closedir(data->dp);
	free(data);
}

#endif //_WIN32

dat_type* dat_chain_ptr = NULL;

char last_text_input;

// seg009:000D
int read_key() {
	// stub
	int key = last_key_scancode;
	last_key_scancode = 0;
	return key;
}

// seg009:019A
void clear_kbd_buf() {
	// stub
	last_key_scancode = 0;
	last_text_input = 0;
}

// seg009:040A
word prandom(word max) {
	if (!seed_was_init) {
		// init from current time
		random_seed = (dword)time(NULL);
		seed_was_init = 1;
	}
	random_seed = random_seed * 214013 + 2531011;
	return (random_seed >> 16) % (max + 1);
}

// seg009:0467
int round_xpos_to_byte(int xpos,int round_direction) {
	// stub
	return xpos;
}

// seg009:0C7A
void quit(int exit_code) {
	restore_stuff();
	exit(exit_code);
}

// seg009:0C90
void restore_stuff() {
	SDL_Quit();
}

// seg009:0E33
int key_test_quit() {
	word key = read_key();
	if (key == (SDL_SCANCODE_Q | WITH_CTRL)) { // Ctrl+Q

		#ifdef USE_REPLAY
		if (recording) save_recorded_replay_dialog();
		#endif
		#ifdef USE_MENU
		if (is_menu_shown) menu_was_closed();
		#endif

		quit(0);
	}
	return key;
}

// seg009:0E54
const char* check_param(const char* param) {
	// stub
	for (short arg_index = 1; arg_index < g_argc; ++arg_index) {

		char* curr_arg = g_argv[arg_index];

		// Filenames (e.g. replays) should never be a valid 'normal' param so we should skip these to prevent conflicts.
		// We can lazily distinguish filenames from non-filenames by checking whether they have a dot in them.
		// (Assumption: all relevant files, e.g. replay files, have some file extension anyway)
		if (strchr(curr_arg, '.') != NULL) {
			continue;
		}

		// List of params that expect a specifier ('sub-') arg directly after it (e.g. the mod's name, after "mod" arg)
		// Such sub-args may conflict with the normal params (so, we should 'skip over' them)
		static const char params_with_one_subparam[][16] = { "mod", "validate", /*...*/ };

		bool curr_arg_has_one_subparam = false;
		for (int i = 0; i < COUNT(params_with_one_subparam); ++i) {
			if (strncasecmp(curr_arg, params_with_one_subparam[i], strlen(params_with_one_subparam[i])) == 0) {
				curr_arg_has_one_subparam = true;
				break;
			}
		}

		if (curr_arg_has_one_subparam) {
			// Found an arg that has one sub-param, so we want to:
			// 1: skip over the next arg                (if we are NOT checking for this specific param)
			// 2: return a pointer below to the SUB-arg (if we ARE checking for this specific param)
			++arg_index;
			if (!(arg_index < g_argc)) return NULL; // not enough arguments
		}

		if (/*strnicmp*/strncasecmp(curr_arg, param, strlen(param)) == 0) {
			return g_argv[arg_index];
		}
	}
	return NULL;
}

// seg009:0EDF
int pop_wait(int timer_index,int time) {
	start_timer(timer_index, time);
	return do_wait(timer_index);
}

static FILE* open_dat_from_root_or_data_dir(const char* filename) {
	FILE* fp = NULL;
	fp = fopen(filename, "rb");

	// if failed, try if the DAT file can be opened in the data/ directory, instead of the main folder
	if (fp == NULL) {
		char data_path[POP_MAX_PATH];
		snprintf_check(data_path, sizeof(data_path), "data/%s", filename);

		if (!file_exists(data_path)) {
			find_first_file_match(data_path, sizeof(data_path), "%s/data/%s", filename);
		}

		// verify that this is a regular file and not a directory (otherwise, don't open)
		struct stat path_stat;
		stat(data_path, &path_stat);
		if (S_ISREG(path_stat.st_mode)) {
			fp = fopen(data_path, "rb");
		}
	}
	return fp;
}

int showmessage(char* text,int arg_4,void* arg_0);

// seg009:0F58
dat_type* open_dat(const char* filename, int optional) {
	FILE* fp = NULL;
	if (!use_custom_levelset) {
		fp = open_dat_from_root_or_data_dir(filename);
	}
	else {
		// Don't complain about missing data files if we are only looking in the mod folder, because they might exist in the data folder.
		// (This is possible only if open_dat() was called by load_all_sounds().)
		if (!skip_mod_data_files && skip_normal_data_files) optional = 1;

		if (!skip_mod_data_files && !(always_use_original_graphics && optional == 'G')) {
			char filename_mod[POP_MAX_PATH];
			// before checking the root directory, first try mods/MODNAME/
			snprintf_check(filename_mod, sizeof(filename_mod), "%s/%s", mod_data_path, filename);
			fp = fopen(filename_mod, "rb");
		}
		if (fp == NULL && !skip_normal_data_files) {
			fp = open_dat_from_root_or_data_dir(filename);
		}
	}
	dat_header_type dat_header;
	dat_table_type* dat_table = NULL;

	dat_type* pointer = (dat_type*) calloc(1, sizeof(dat_type));
	snprintf_check(pointer->filename, sizeof(pointer->filename), "%s", filename);
	pointer->next_dat = dat_chain_ptr;
	dat_chain_ptr = pointer;

	if (fp != NULL) {
		if (fread(&dat_header, 6, 1, fp) != 1)
			goto failed;
		dat_table = (dat_table_type*) malloc(SDL_SwapLE16(dat_header.table_size));
		if (dat_table == NULL ||
		    fseek(fp, SDL_SwapLE32(dat_header.table_offset), SEEK_SET) ||
		    fread(dat_table, SDL_SwapLE16(dat_header.table_size), 1, fp) != 1)
			goto failed;
		pointer->handle = fp;
		pointer->dat_table = dat_table;
	} else if (optional == 0) {
		// showmessage will crash if we call it before certain things are initialized!
		// Solution: In pop_main(), I moved the first open_dat() call after init_copyprot_dialog().
		// /*
		// There is no DAT file, verify whether the corresponding directory exists.
		char filename_no_ext[POP_MAX_PATH];
		// strip the .DAT file extension from the filename (use folders simply named TITLE, KID, VPALACE, etc.)
		strncpy(filename_no_ext, pointer->filename, sizeof(filename_no_ext));
		size_t len = strlen(filename_no_ext);
		if (len >= 5 && filename_no_ext[len-4] == '.') {
			filename_no_ext[len-4] = '\0'; // terminate, so ".DAT" is deleted from the filename
		}
		char foldername[POP_MAX_PATH];
		snprintf_check(foldername,sizeof(foldername),"data/%s",filename_no_ext);
		const char* data_path = locate_file(foldername);
		struct stat path_stat;
		int result = stat(data_path, &path_stat);
		if (result != 0 || !S_ISDIR(path_stat.st_mode)) {
			char error_message[256];
			snprintf_check(error_message, sizeof(error_message), "Cannot find a required data file: %s or folder: %s\nPress any key to quit.", filename, foldername);
			if (onscreen_surface_ != NULL && copyprot_dialog != NULL) { // otherwise showmessage will crash
				showmessage(error_message, 1, &key_test_quit);
				quit(1);
			}
		}
		// */
	}
out:
	// stub
	return pointer;
failed:
	perror(filename);
	if (fp)
		fclose(fp);
	if (dat_table)
		free(dat_table);
	goto out;
}

// seg009:9CAC
void set_loaded_palette(dat_pal_type* palette_ptr) {
	int dest_row, dest_index, source_row;
	for (dest_row = dest_index = source_row = 0; dest_row < 16; ++dest_row, dest_index += 0x10) {
		if (palette_ptr->row_bits & (1 << dest_row)) {
			set_pal_arr(dest_index, 16, palette_ptr->vga + source_row*0x10);
			++source_row;
		}
	}
}

// data:3356
word chtab_palette_bits = 1;

static bool should_stream_dat(const char* dat_name) {
	if (!dat_name || !dat_name[0]) return false;
	if (strcasecmp(dat_name, "KID.DAT") == 0) return true;
	if (strncasecmp(dat_name, "GUARD", 5) == 0) return true;
	return false;
}

static void adopt_shared_palette(SDL_Surface* image, SDL_Palette* shared) {
	if (!image || !shared) return;
	SDL_SurfaceAdoptPalette(image, shared);
}

static void stream_unload_images(chtab_type* chtab) {
	if (!chtab || !chtab->extra || !chtab->extra->streaming_enabled) return;
	for (int i = 0; i < chtab->n_images; ++i) {
		if (chtab->images[i]) {
			SDL_FreeSurface(chtab->images[i]);
			chtab->images[i] = NULL;
		}
	}
}

// seg009:104E
chtab_type* load_sprites_from_file(int resource,int palette_bits, int quit_on_error, const char* dat_name) {
    // printf("load_sprites_from_file(%d)\n", resource);
	dat_shpl_type* shpl = (dat_shpl_type*) load_from_opendats_alloc(resource, "pal", NULL, NULL);
	if (shpl == NULL) {
		printf("Can't load sprites from resource %d.\n", resource);
		if (quit_on_error) {
			char error_message[256];
			snprintf_check(error_message, sizeof(error_message), "Can't load sprites from resource %d.\nThe last opened data file is: %s\nPress any key to quit.", resource, dat_chain_ptr ? dat_chain_ptr->filename : "<unknown>");
			showmessage(error_message, 1, &key_test_quit);
			quit(1);
		}
		return NULL;
	}

	dat_pal_type* pal_ptr = &shpl->palette;
    word original_row_bits = pal_ptr->row_bits;
	if (graphics_mode == gmMcgaVga) {
		if (palette_bits == 0) {
			// leave palette bits unchanged
		} else {
			chtab_palette_bits |= palette_bits;
		}
		pal_ptr->row_bits = palette_bits;
	}

	int n_images = shpl->n_images;
	size_t alloc_size = sizeof(chtab_type) + sizeof(void *) * n_images;
	chtab_type* chtab = (chtab_type*) psram_malloc(alloc_size);
	if (!chtab) {
		// printf("load_sprites_from_file: OOM allocating chtab!\n");
		return NULL;
	}
	memset(chtab, 0, alloc_size);
	chtab->n_images = n_images;
	bool streaming_enabled = should_stream_dat(dat_name);
	chtab->extra = (chtab_extra*)psram_malloc(sizeof(chtab_extra));
	if (chtab->extra) {
		memset(chtab->extra, 0, sizeof(chtab_extra));
		chtab->extra->streaming_enabled = streaming_enabled ? 1 : 0;
		chtab->extra->base_resource = resource;
		if (dat_name) {
			snprintf_check(chtab->extra->dat_name, sizeof(chtab->extra->dat_name), "%s", dat_name);
		}
		chtab->extra->palette_copy = *pal_ptr;
		if (streaming_enabled) {
			chtab->extra->resource_ids = (int*)psram_malloc(sizeof(int) * n_images);
			if (chtab->extra->resource_ids) {
				memset(chtab->extra->resource_ids, 0, sizeof(int) * n_images);
			}
			chtab->extra->shared_palette = SDL_CreatePalette(256);
			if (chtab->extra->shared_palette) {
				SDL_Color colors[256];
                // Initialize with current screen palette to ensure correct colors for non-overridden rows
                if (onscreen_surface_ && onscreen_surface_->format->palette) {
                    memcpy(colors, onscreen_surface_->format->palette->colors, sizeof(colors));
                } else {
				    memset(colors, 0, sizeof(colors));
                }

                // Determine which row to use for indices 0-15
                // The vga array is packed: it only contains rows where row_bits is set.
                int source_offset = 0;
                int packed_index = 0;
                bool found_row = false;
                
                if (palette_bits != 0) {
                    for (int r = 0; r < 16; ++r) {
                        // Check if this row is present in the loaded palette data
                        if (original_row_bits & (1 << r)) {
                            // Check if this is the row we want for the sprite
                            if (palette_bits & (1 << r)) {
                                source_offset = packed_index * 16;
                                found_row = true;
                                break;
                            }
                            packed_index++;
                        }
                    }
                }

				for (int i = 0; i < 16; ++i) {
					colors[i].r = pal_ptr->vga[source_offset + i].r << 2;
					colors[i].g = pal_ptr->vga[source_offset + i].g << 2;
					colors[i].b = pal_ptr->vga[source_offset + i].b << 2;
					colors[i].a = 255;
				}
				SDL_SetPaletteColors(chtab->extra->shared_palette, colors, 0, 256);
			}
		}
	}

	for (int i = 1; i <= n_images; i++) {
        // printf("load_sprites_from_file: loading image %d/%d (res %d)\n", i, n_images, resource + i);
        fflush(stdout);
		SDL_Surface* image = load_image(resource + i, pal_ptr);
		if (image && chtab->extra && chtab->extra->shared_palette) {
			adopt_shared_palette(image, chtab->extra->shared_palette);
		}
		chtab->images[i-1] = image;
		if (streaming_enabled && chtab->extra && chtab->extra->resource_ids) {
			chtab->extra->resource_ids[i-1] = resource + i;
		}
	}
    // printf("load_sprites_from_file: loop done\n");
    fflush(stdout);

	if (streaming_enabled) {
        // printf("load_sprites_from_file: stream_unload_images\n");
		stream_unload_images(chtab);
	}

	set_loaded_palette(pal_ptr);
    
    // Ensure shared_palette is updated with the latest screen palette if it was created before the screen palette was fully populated
    if (chtab->extra && chtab->extra->shared_palette && onscreen_surface_ && onscreen_surface_->format->palette) {
        // We only want to update the colors that are NOT part of the sprite's own 16 colors (indices 0-15)
        // But wait, shared_palette has 256 colors. Indices 0-15 are the sprite's colors.
        // Indices 16-255 should match the screen palette.
        // So let's copy indices 16-255 from screen palette to shared_palette.
        SDL_Color* screen_colors = onscreen_surface_->format->palette->colors;
        SDL_SetPaletteColors(chtab->extra->shared_palette, screen_colors + 16, 16, 240);
    }

    // printf("load_sprites_from_file: done\n");
	return chtab;
}

image_type* reload_streamed_image(chtab_type* chtab, int id) {
    // printf("reload_streamed_image: id=%d\n", id);
	if (!chtab || id < 0 || id >= chtab->n_images) return NULL;
	if (!chtab->extra || !chtab->extra->streaming_enabled) {
		return chtab->images[id];
	}
	if (chtab->images[id]) {
		return chtab->images[id];
	}
	if (!chtab->extra->dat_name[0] || !chtab->extra->resource_ids) {
		return NULL;
	}
	dat_type* dathandle = open_dat(chtab->extra->dat_name, 'G');
    // printf("reload_streamed_image: calling load_image(%d)\n", chtab->extra->resource_ids[id]);
	SDL_Surface* image = load_image(chtab->extra->resource_ids[id], &chtab->extra->palette_copy);
	if (dathandle) close_dat(dathandle);
	if (!image) {
        // printf("reload_streamed_image: load_image failed\n");
		return NULL;
	}
	if (chtab->extra->shared_palette) {
        // printf("reload_streamed_image: adopt_shared_palette\n");
		adopt_shared_palette(image, chtab->extra->shared_palette);
	}
	chtab->images[id] = image;
    // printf("reload_streamed_image: done\n");
	return image;
}

// seg009:11A8
void free_chtab(chtab_type *chtab_ptr) {
	image_type* curr_image;
	if (graphics_mode == gmMcgaVga && chtab_ptr->has_palette_bits) {
		chtab_palette_bits &= ~ chtab_ptr->chtab_palette_bits;
	}
	word n_images = chtab_ptr->n_images;
	for (word id = 0; id < n_images; ++id) {
		curr_image = chtab_ptr->images[id];
		if (curr_image) {
			SDL_FreeSurface(curr_image);
		}
	}
	if (chtab_ptr->extra) {
		if (chtab_ptr->extra->shared_palette) {
			SDL_PaletteRelease(chtab_ptr->extra->shared_palette);
		}
		if (chtab_ptr->extra->resource_ids) {
			psram_free(chtab_ptr->extra->resource_ids);
		}
		psram_free(chtab_ptr->extra);
	}
	psram_free(chtab_ptr);
}

// seg009:8CE6
void decompress_rle_lr(byte* destination,const byte* source,int dest_length) {
	const byte* src_pos = source;
	byte* dest_pos = destination;
	short rem_length = dest_length;
	while (rem_length) {
		sbyte count = *src_pos;
		src_pos++;
		if (count >= 0) { // copy
			++count;
			do {
				*dest_pos = *src_pos;
				dest_pos++;
				src_pos++;
				--rem_length;
				--count;
			} while (count && rem_length);
		} else { // repeat
			byte al = *src_pos;
			src_pos++;
			count = -count;
			do {
				*dest_pos = al;
				dest_pos++;
				--rem_length;
				--count;
			} while (count && rem_length);
		}
	}
}

// seg009:8D1C
void decompress_rle_ud(byte* destination,const byte* source,int dest_length,int width,int height) {
	short rem_height = height;
	const byte* src_pos = source;
	byte* dest_pos = destination;
	short rem_length = dest_length;
	--dest_length;
	--width;
	while (rem_length) {
		sbyte count = *src_pos;
		src_pos++;
		if (count >= 0) { // copy
			++count;
			do {
				*dest_pos = *src_pos;
				dest_pos++;
				src_pos++;
				dest_pos += width;
				--rem_height;
				if (rem_height == 0) {
					dest_pos -= dest_length;
					rem_height = height;
				}
				--rem_length;
				--count;
			} while (count && rem_length);
		} else { // repeat
			byte al = *src_pos;
			src_pos++;
			count = -count;
			do {
				*dest_pos = al;
				dest_pos++;
				dest_pos += width;
				--rem_height;
				if (rem_height == 0) {
					dest_pos -= dest_length;
					rem_height = height;
				}
				--rem_length;
				--count;
			} while (count && rem_length);
		}
	}
}

// seg009:90FA
static byte lzg_window[0x400];
byte* decompress_lzg_lr(byte* dest,const byte* source,int dest_length) {
	byte* window = lzg_window;
	// if (window == NULL) return NULL;
	memset(window, 0, 0x400);
	byte* window_pos = window + 0x400 - 0x42; // bx
	short remaining = dest_length; // cx
	byte* window_end = window + 0x400; // dx
	const byte* source_pos = source;
	byte* dest_pos = dest;
	word mask = 0;
	do {
		mask >>= 1;
		if ((mask & 0xFF00) == 0) {
			mask = *source_pos | 0xFF00;
			source_pos++;
		}
		if (mask & 1) {
			*window_pos = *dest_pos = *source_pos;
			window_pos++;
			dest_pos++;
			source_pos++;
			if (window_pos >= window_end) window_pos = window;
			--remaining;
		} else {
			word copy_info = *source_pos;
			source_pos++;
			copy_info = (copy_info << 8) | *source_pos;
			source_pos++;
			byte* copy_source = window + (copy_info & 0x3FF);
			byte copy_length = (copy_info >> 10) + 3;
			do {
				*window_pos = *dest_pos = *copy_source;
				window_pos++;
				dest_pos++;
				copy_source++;
				if (copy_source >= window_end) copy_source = window;
				if (window_pos >= window_end) window_pos = window;
				--remaining;
				--copy_length;
			} while (remaining && copy_length);
		}
	} while (remaining);
//	end:
	// free(window);
	return dest;
}

// seg009:91AD
byte* decompress_lzg_ud(byte* dest,const byte* source,int dest_length,int stride,int height) {
	byte* window = lzg_window;
	// if (window == NULL) return NULL; // Static buffer
	memset(window, 0, 0x400);
	byte* window_pos = window + 0x400 - 0x42; // bx
	short remaining = height; // cx
	byte* window_end = window + 0x400; // dx
	const byte* source_pos = source;
	byte* dest_pos = dest;
	word mask = 0;
	short dest_end = dest_length - 1;
	do {
		mask >>= 1;
		if ((mask & 0xFF00) == 0) {
			mask = *source_pos | 0xFF00;
			source_pos++;
		}
		if (mask & 1) {
			*window_pos = *dest_pos = *source_pos;
			window_pos++;
			source_pos++;
			dest_pos += stride;
			--remaining;
			if (remaining == 0) {
				dest_pos -= dest_end;
				remaining = height;
			}
			if (window_pos >= window_end) window_pos = window;
			--dest_length;
		} else {
			word copy_info = *source_pos;
			source_pos++;
			copy_info = (copy_info << 8) | *source_pos;
			source_pos++;
			byte* copy_source = window + (copy_info & 0x3FF);
			byte copy_length = (copy_info >> 10) + 3;
			do {
				*window_pos = *dest_pos = *copy_source;
				window_pos++;
				copy_source++;
				dest_pos += stride;
				--remaining;
				if (remaining == 0) {
					dest_pos -= dest_end;
					remaining = height;
				}
				if (copy_source >= window_end) copy_source = window;
				if (window_pos >= window_end) window_pos = window;
				--dest_length;
				--copy_length;
			} while (dest_length && copy_length);
		}
	} while (dest_length);
//	end:
	// free(window);
	return dest;
}

// seg009:938E
void decompr_img(byte* dest,const image_data_type* source,int decomp_size,int cmeth, int stride) {
	switch (cmeth) {
		case 0: // RAW left-to-right
			memcpy(dest, &source->data, decomp_size);
		break;
		case 1: // RLE left-to-right
			decompress_rle_lr(dest, source->data, decomp_size);
		break;
		case 2: // RLE up-to-down
			decompress_rle_ud(dest, source->data, decomp_size, stride, SDL_SwapLE16(source->height));
		break;
		case 3: // LZG left-to-right
			decompress_lzg_lr(dest, source->data, decomp_size);
		break;
		case 4: // LZG up-to-down
			decompress_lzg_ud(dest, source->data, decomp_size, stride, SDL_SwapLE16(source->height));
		break;
	}
}

int calc_stride(image_data_type* image_data) {
	int width = SDL_SwapLE16(image_data->width);
	int flags = SDL_SwapLE16(image_data->flags);
	int depth = ((flags >> 12) & 7) + 1;
	return (depth * width + 7) / 8;
}

byte* conv_to_8bpp(byte* in_data, int width, int height, int stride, int depth) {
	byte* out_data = (byte*) malloc(width * height);
	int pixels_per_byte = 8 / depth;
	int mask = (1 << depth) - 1;
	for (int y = 0; y < height; ++y) {
		byte* in_pos = in_data + y*stride;
		byte* out_pos = out_data + y*width;
		for (int x_pixel = 0, x_byte = 0; x_byte < stride; ++x_byte) {
			byte v = *in_pos;
			int shift = 8;
			for (int pixel_in_byte = 0; pixel_in_byte < pixels_per_byte && x_pixel < width; ++pixel_in_byte, ++x_pixel) {
				shift -= depth;
				*out_pos = (v >> shift) & mask;
				++out_pos;
			}
			++in_pos;
		}
	}
	return out_data;
}

image_type* decode_image(image_data_type* image_data, dat_pal_type* palette) {
	int height = SDL_SwapLE16(image_data->height);
	if (height == 0) return NULL;
	int width = SDL_SwapLE16(image_data->width);
	int flags = SDL_SwapLE16(image_data->flags);
	int depth = ((flags >> 12) & 7) + 1;
	int cmeth = (flags >> 8) & 0x0F;
	int stride = calc_stride(image_data);
	int dest_size = stride * height;
    // printf("decode_image: w=%d h=%d d=%d cmeth=%d size=%d\n", width, height, depth, cmeth, dest_size);
    fflush(stdout);

	// Use PSRAM scratch buffer for decompression
	byte* dest = (byte*) psram_get_scratch_1(dest_size);
	if (!dest) {
		// Fallback to malloc if too big (unlikely)
        // printf("decode_image: scratch1 too small (%d), using psram_malloc\n", dest_size);
		dest = (byte*) psram_malloc(dest_size);
	}
	memset(dest, 0, dest_size);
	decompr_img(dest, image_data, dest_size, cmeth, stride);
	
	// Use PSRAM scratch buffer for 8bpp conversion
	byte* image_8bpp = (byte*) psram_get_scratch_2(width * height);
	bool free_8bpp = false;
	if (!image_8bpp) {
        // printf("decode_image: scratch2 too small (%d), using psram_malloc\n", width * height);
		image_8bpp = (byte*) psram_malloc(width * height);
		free_8bpp = true;
	} else {
		// conv_to_8bpp expects a buffer, but the original function allocated it.
		// We need to modify conv_to_8bpp or inline it.
		// Actually, conv_to_8bpp allocates internally.
		// Let's rewrite conv_to_8bpp to take a buffer.
	}
	
	// Let's inline the conversion here to use our buffer
	int pixels_per_byte = 8 / depth;
	int mask = (1 << depth) - 1;
	for (int y = 0; y < height; ++y) {
		byte* in_pos = dest + y*stride;
		byte* out_pos = image_8bpp + y*width;
		for (int x_pixel = 0, x_byte = 0; x_byte < stride; ++x_byte) {
			byte v = *in_pos;
			int shift = 8;
			for (int pixel_in_byte = 0; pixel_in_byte < pixels_per_byte && x_pixel < width; ++pixel_in_byte, ++x_pixel) {
				shift -= depth;
				*out_pos = (v >> shift) & mask;
				++out_pos;
			}
			++in_pos;
		}
	}

	// byte* image_8bpp = conv_to_8bpp(dest, width, height, stride, depth);
	
	// If we used malloc for dest, free it.
	if (dest != psram_get_scratch_1(dest_size)) {
		psram_free(dest);
	}
	dest = NULL;
	
    // printf("decode_image: SDL_CreateRGBSurface\n");
	image_type* image = SDL_CreateRGBSurface(0, width, height, 8, 0, 0, 0, 0);
	if (image == NULL) {
		sdlperror("decode_image: SDL_CreateRGBSurface");
		quit(1);
	}
	if (SDL_LockSurface(image) != 0) {
		sdlperror("decode_image: SDL_LockSurface");
	}
	for (int y = 0; y < height; ++y) {
		// fill image with data
		memcpy((byte*)image->pixels + y*image->pitch, image_8bpp + y*width, width);
	}
	SDL_UnlockSurface(image);

	// If we used malloc for image_8bpp, free it.
	if (free_8bpp) {
		psram_free(image_8bpp);
	}
	image_8bpp = NULL;
	SDL_Color colors[16];
	for (int i = 0; i < 16; ++i) {
		colors[i].r = palette->vga[i].r << 2;
		colors[i].g = palette->vga[i].g << 2;
		colors[i].b = palette->vga[i].b << 2;
		colors[i].a = SDL_ALPHA_OPAQUE;   // SDL2's SDL_Color has a fourth alpha component
	}
	// Palette index 0 is used for transparency via colorkey (set in load_image)
	// Don't force it to black - use the actual palette color
	// The colorkey mechanism handles transparency, not the RGB value
	colors[0].a = SDL_ALPHA_TRANSPARENT;
	
	SDL_SetPaletteColors(image->format->palette, colors, 0, 16); // SDL_SetColors = deprecated
	return image;
}

// seg009:121A
image_type* load_image(int resource_id, dat_pal_type* palette) {
	// stub
    // printf("load_image(%d)\n", resource_id);
    //printf("load_image(%d) called from %p\n", resource_id, __builtin_return_address(0));
	data_location result;
	int size;
	void* image_data = load_from_opendats_alloc(resource_id, "png", &result, &size);
    //printf("load_image(%d): size=%d data=%p result=%d\n", resource_id, size, image_data, result);
    //if (image_data) {
    //    unsigned char* p = (unsigned char*)image_data;
    //    printf("Header: %02X %02X %02X %02X\n", p[0], p[1], p[2], p[3]);
    //}
    // printf("load_image: result=%d size=%d data=%p\n", result, size, image_data);
	image_type* image = NULL;
	switch (result) {
		case data_none:
			return NULL;
		break;
		case data_DAT: { // DAT
            //printf("load_image: calling decode_image\n");
            //fflush(stdout);
			image = decode_image((image_data_type*) image_data, palette);
		} break;
		case data_directory: { // directory
            //printf("load_image: calling IMG_Load_RW\n");
            //fflush(stdout);
			SDL_RWops* rw = SDL_RWFromConstMem(image_data, size);
            //printf("load_image: rw=%p\n", rw);
			if (rw == NULL) {
				sdlperror("load_image: SDL_RWFromConstMem");
				return NULL;
			}
			image = IMG_Load_RW(rw, 0);
            //printf("load_image: IMG_Load_RW returned %p\n", image);
			if (image == NULL) {
				// printf("load_image: IMG_Load_RW: %s\n", IMG_GetError());
			}
			if (SDL_RWclose(rw) != 0) {
				sdlperror("load_image: SDL_RWclose");
			}
		} break;
	}
	// if (image_data != NULL) free(image_data);
    // Don't free image_data because it's in PSRAM (either file buffer or malloc)
    // and psram_free is a no-op.
    // If it was file buffer, it will be overwritten next time.
    // If it was malloc, it leaks (but we have 8MB).


	if (image != NULL) {
		// Set colorkey for indexed images immediately after loading
		// Palette index 0 is always transparent for indexed images
		if (image->format->BitsPerPixel == 8) {
			if (SDL_SetColorKey(image, SDL_TRUE, 0) != 0) {
				sdlperror("load_image: SDL_SetColorKey");
				quit(1);
			}
		}
		// RGBA images will use blend mode set in method_6_blit_img_to_scr
//		printf("bpp = %d\n", image->format->BitsPerPixel);
/*
		if (SDL_SetSurfaceAlphaMod(image, 0) != 0) { //sdl 1.2: SDL_SetAlpha removed
			sdlperror("load_image: SDL_SetAlpha");
			quit(1);
		}
*/
//		image_type* colored_image = SDL_ConvertSurfaceFormat(image, SDL_PIXELFORMAT_ARGB8888, 0);
//		if (!colored_image) {
//			sdlperror("load_image: SDL_ConvertSurfaceFormat");
//			quit(1);
//		}
//		SDL_FreeSurface(image);
//		image = colored_image;
	}
    // printf("load_image: returning %p\n", image);
	return image;
}

// seg009:13C4
void draw_image_transp(image_type* image,image_type* mask,int xpos,int ypos) {
	if (graphics_mode == gmMcgaVga) {
		draw_image_transp_vga(image, xpos, ypos);
	} else {
		// ...
	}
}

// seg009:157E
int set_joy_mode() {
	// stub
	if (SDL_NumJoysticks() < 1) {
		is_joyst_mode = 0;
	} else {
		if (gamecontrollerdb_file[0] != '\0') {
			SDL_GameControllerAddMappingsFromFile(gamecontrollerdb_file);
		}

		if (SDL_IsGameController(0)) {
			sdl_controller_ = SDL_GameControllerOpen(0);
			if (sdl_controller_ == NULL) {
				is_joyst_mode = 0;
			} else {
				is_joyst_mode = 1;
			}
		}
		// We have a joystick connected, but it's NOT compatible with the SDL_GameController
		// interface, so we resort to the classic SDL_Joystick interface instead
		else {
			sdl_joystick_ = SDL_JoystickOpen(0);
			is_joyst_mode = 1;
			using_sdl_joystick_interface = 1;
		}
	}
	if (enable_controller_rumble && is_joyst_mode) {
		sdl_haptic = SDL_HapticOpen(0);
		SDL_HapticRumbleInit(sdl_haptic); // initialize the device for simple rumble
	} else {
		sdl_haptic = NULL;
	}

	is_keyboard_mode = !is_joyst_mode;
	return is_joyst_mode;
}

// seg009:178B
surface_type* make_offscreen_buffer(const rect_type* rect) {
	// stub
#ifndef USE_ALPHA
	// Bit order matches onscreen buffer, good for fading.
	surface_type* surface = SDL_CreateRGBSurface(0, rect->right, rect->bottom, 8, 0, 0, 0, 0);
#else
	surface_type* surface = SDL_CreateRGBSurface(0, rect->right, rect->bottom, 32, Rmsk, Gmsk, Bmsk, Amsk);
#endif
    if (surface) {
        if (onscreen_surface_) {
            SDL_SetSurfacePalette(surface, onscreen_surface_->format->palette);
        }
    }
	return surface;
}

// seg009:17BD
void free_surface(surface_type* surface) {
	SDL_FreeSurface(surface);
}

// seg009:17EA
void free_peel(peel_type* peel_ptr) {
	// Mark peel as invalid so restore_peel() will skip it
	// With pool system, this prevents stale peels from being restored after screen transitions
	if (peel_ptr == NULL) {
		return;
	}
	
	// Our pool returns manual_peel_type* cast to peel_type*
	// The first field of both is a pointer (SDL_Surface* vs byte*)
	// Setting it to NULL marks the peel as freed
	peel_ptr->peel = NULL;
	
	// Also need to mark the pool slot as not in_use
	// Since peel_type and manual_peel_type both start with a pointer and rect,
	// we can safely access the fields that come after by treating it as manual_peel_type
	// The in_use flag is at offset: sizeof(byte*) + sizeof(rect_type) + sizeof(int)*2
	// But simpler: just zero the rect to make dimensions invalid
	peel_ptr->rect.right = peel_ptr->rect.left;
	peel_ptr->rect.bottom = peel_ptr->rect.top;
}

// seg009:182F
void set_hc_pal() {
	// stub
	if (graphics_mode == gmMcgaVga) {
		set_pal_arr(0, 16, custom->vga_palette);
	} else {
		// ...
	}
}

// seg009:2446
void flip_not_ega(byte* memory,int height,int stride) {
	byte* row_buffer = (byte*) malloc(stride);
	byte* top_ptr;
	byte* bottom_ptr;
	bottom_ptr = top_ptr = memory;
	bottom_ptr += (height - 1) * stride;
	short rem_rows = height >> 1;
	do {
		memcpy(row_buffer, top_ptr, stride);
		memcpy(top_ptr, bottom_ptr, stride);
		memcpy(bottom_ptr, row_buffer, stride);
		top_ptr += stride;
		bottom_ptr -= stride;
		--rem_rows;
	} while (rem_rows);
	free(row_buffer);
}

// seg009:19B1
void flip_screen(surface_type* surface) {
	// stub
	if (graphics_mode != gmEga) {
		if (SDL_LockSurface(surface) != 0) {
			sdlperror("flip_screen: SDL_LockSurface");
			quit(1);
		}
		flip_not_ega((byte*) surface->pixels, surface->h, surface->pitch);
		SDL_UnlockSurface(surface);
	} else {
		// ...
	}
}

#ifndef USE_FADE
// seg009:19EF
void fade_in_2(surface_type* source_surface,int which_rows) {
	// stub
	method_1_blit_rect(onscreen_surface_, source_surface, &screen_rect, &screen_rect, 0);
    update_screen();
}

// seg009:1CC9
void fade_out_2(int rows) {
	// stub
    update_screen();
}
#endif // USE_FADE

// seg009:2288
void draw_image_transp_vga(image_type* image,int xpos,int ypos) {
	//printf("draw_image_transp_vga: image=%p, x=%d, y=%d\n", image, xpos, ypos);
    //fflush(stdout);
	// stub
	method_6_blit_img_to_scr(image, xpos, ypos, blitters_10h_transp);
}

#ifdef USE_TEXT


/*const*/ byte hc_font_data[] = {
0x20,0x83,0x07,0x00,0x02,0x00,0x01,0x00,0x01,0x00,0xD2,0x00,0xD8,0x00,0xE5,0x00,
0xEE,0x00,0xFA,0x00,0x07,0x01,0x14,0x01,0x21,0x01,0x2A,0x01,0x37,0x01,0x44,0x01,
0x50,0x01,0x5C,0x01,0x6A,0x01,0x74,0x01,0x81,0x01,0x8E,0x01,0x9B,0x01,0xA8,0x01,
0xB5,0x01,0xC2,0x01,0xCF,0x01,0xDC,0x01,0xE9,0x01,0xF6,0x01,0x03,0x02,0x10,0x02,
0x1C,0x02,0x2A,0x02,0x37,0x02,0x42,0x02,0x4F,0x02,0x5C,0x02,0x69,0x02,0x76,0x02,
0x83,0x02,0x90,0x02,0x9D,0x02,0xAA,0x02,0xB7,0x02,0xC4,0x02,0xD1,0x02,0xDE,0x02,
0xEB,0x02,0xF8,0x02,0x05,0x03,0x12,0x03,0x1F,0x03,0x2C,0x03,0x39,0x03,0x46,0x03,
0x53,0x03,0x60,0x03,0x6D,0x03,0x7A,0x03,0x87,0x03,0x94,0x03,0xA1,0x03,0xAE,0x03,
0xBB,0x03,0xC8,0x03,0xD5,0x03,0xE2,0x03,0xEB,0x03,0xF9,0x03,0x02,0x04,0x0F,0x04,
0x1C,0x04,0x29,0x04,0x36,0x04,0x43,0x04,0x50,0x04,0x5F,0x04,0x6C,0x04,0x79,0x04,
0x88,0x04,0x95,0x04,0xA2,0x04,0xAF,0x04,0xBC,0x04,0xC9,0x04,0xD8,0x04,0xE7,0x04,
0xF4,0x04,0x01,0x05,0x0E,0x05,0x1B,0x05,0x28,0x05,0x35,0x05,0x42,0x05,0x51,0x05,
0x5E,0x05,0x6B,0x05,0x78,0x05,0x85,0x05,0x8D,0x05,0x9A,0x05,0xA7,0x05,0xBB,0x05,
0xD9,0x05,0x00,0x00,0x03,0x00,0x00,0x00,0x07,0x00,0x02,0x00,0x01,0x00,0xC0,0xC0,
0xC0,0xC0,0xC0,0x00,0xC0,0x03,0x00,0x05,0x00,0x01,0x00,0xD8,0xD8,0xD8,0x06,0x00,
0x07,0x00,0x01,0x00,0x00,0x6C,0xFE,0x6C,0xFE,0x6C,0x07,0x00,0x07,0x00,0x01,0x00,
0x10,0x7C,0xD0,0x7C,0x16,0x7C,0x10,0x07,0x00,0x08,0x00,0x01,0x00,0xC3,0xC6,0x0C,
0x18,0x30,0x63,0xC3,0x07,0x00,0x08,0x00,0x01,0x00,0x38,0x6C,0x38,0x7A,0xCC,0xCE,
0x7B,0x03,0x00,0x03,0x00,0x01,0x00,0x60,0x60,0xC0,0x07,0x00,0x04,0x00,0x01,0x00,
0x30,0x60,0xC0,0xC0,0xC0,0x60,0x30,0x07,0x00,0x04,0x00,0x01,0x00,0xC0,0x60,0x30,
0x30,0x30,0x60,0xC0,0x06,0x00,0x07,0x00,0x01,0x00,0x00,0x6C,0x38,0xFE,0x38,0x6C,
0x06,0x00,0x06,0x00,0x01,0x00,0x00,0x30,0x30,0xFC,0x30,0x30,0x08,0x00,0x03,0x00,
0x01,0x00,0x00,0x00,0x00,0x00,0x00,0x60,0x60,0xC0,0x04,0x00,0x04,0x00,0x01,0x00,
0x00,0x00,0x00,0xF0,0x07,0x00,0x02,0x00,0x01,0x00,0x00,0x00,0x00,0x00,0x00,0xC0,
0xC0,0x07,0x00,0x08,0x00,0x01,0x00,0x03,0x06,0x0C,0x18,0x30,0x60,0xC0,0x07,0x00,
0x06,0x00,0x01,0x00,0x78,0xCC,0xCC,0xCC,0xCC,0xCC,0x78,0x07,0x00,0x06,0x00,0x01,
0x00,0x30,0x70,0xF0,0x30,0x30,0x30,0xFC,0x07,0x00,0x06,0x00,0x01,0x00,0x78,0xCC,
0x0C,0x18,0x30,0x60,0xFC,0x07,0x00,0x06,0x00,0x01,0x00,0x78,0xCC,0x0C,0x18,0x0C,
0xCC,0x78,0x07,0x00,0x07,0x00,0x01,0x00,0x1C,0x3C,0x6C,0xCC,0xFE,0x0C,0x0C,0x07,
0x00,0x06,0x00,0x01,0x00,0xF8,0xC0,0xC0,0xF8,0x0C,0x0C,0xF8,0x07,0x00,0x06,0x00,
0x01,0x00,0x78,0xC0,0xC0,0xF8,0xCC,0xCC,0x78,0x07,0x00,0x06,0x00,0x01,0x00,0xFC,
0x0C,0x18,0x30,0x30,0x30,0x30,0x07,0x00,0x06,0x00,0x01,0x00,0x78,0xCC,0xCC,0x78,
0xCC,0xCC,0x78,0x07,0x00,0x06,0x00,0x01,0x00,0x78,0xCC,0xCC,0x7C,0x0C,0xCC,0x78,
0x06,0x00,0x02,0x00,0x01,0x00,0x00,0xC0,0xC0,0x00,0xC0,0xC0,0x08,0x00,0x03,0x00,
0x01,0x00,0x00,0x60,0x60,0x00,0x00,0x60,0x60,0xC0,0x07,0x00,0x05,0x00,0x01,0x00,
0x18,0x30,0x60,0xC0,0x60,0x30,0x18,0x05,0x00,0x04,0x00,0x01,0x00,0x00,0x00,0xF0,
0x00,0xF0,0x07,0x00,0x05,0x00,0x01,0x00,0xC0,0x60,0x30,0x18,0x30,0x60,0xC0,0x07,
0x00,0x06,0x00,0x01,0x00,0x78,0xCC,0x0C,0x18,0x30,0x00,0x30,0x07,0x00,0x06,0x00,
0x01,0x00,0x78,0xCC,0xDC,0xDC,0xD8,0xC0,0x78,0x07,0x00,0x06,0x00,0x01,0x00,0x78,
0xCC,0xCC,0xFC,0xCC,0xCC,0xCC,0x07,0x00,0x06,0x00,0x01,0x00,0xF8,0xCC,0xCC,0xF8,
0xCC,0xCC,0xF8,0x07,0x00,0x06,0x00,0x01,0x00,0x78,0xCC,0xC0,0xC0,0xC0,0xCC,0x78,
0x07,0x00,0x06,0x00,0x01,0x00,0xF8,0xCC,0xCC,0xCC,0xCC,0xCC,0xF8,0x07,0x00,0x05,
0x00,0x01,0x00,0xF8,0xC0,0xC0,0xF0,0xC0,0xC0,0xF8,0x07,0x00,0x05,0x00,0x01,0x00,
0xF8,0xC0,0xC0,0xF0,0xC0,0xC0,0xC0,0x07,0x00,0x06,0x00,0x01,0x00,0x78,0xCC,0xC0,
0xDC,0xCC,0xCC,0x78,0x07,0x00,0x06,0x00,0x01,0x00,0xCC,0xCC,0xCC,0xFC,0xCC,0xCC,
0xCC,0x07,0x00,0x04,0x00,0x01,0x00,0xF0,0x60,0x60,0x60,0x60,0x60,0xF0,0x07,0x00,
0x06,0x00,0x01,0x00,0x0C,0x0C,0x0C,0x0C,0x0C,0xCC,0x78,0x07,0x00,0x07,0x00,0x01,
0x00,0xC6,0xCC,0xD8,0xF0,0xD8,0xCC,0xC6,0x07,0x00,0x05,0x00,0x01,0x00,0xC0,0xC0,
0xC0,0xC0,0xC0,0xC0,0xF8,0x07,0x00,0x08,0x00,0x01,0x00,0xC3,0xE7,0xFF,0xDB,0xC3,
0xC3,0xC3,0x07,0x00,0x06,0x00,0x01,0x00,0xCC,0xCC,0xEC,0xFC,0xDC,0xCC,0xCC,0x07,
0x00,0x06,0x00,0x01,0x00,0x78,0xCC,0xCC,0xCC,0xCC,0xCC,0x78,0x07,0x00,0x06,0x00,
0x01,0x00,0xF8,0xCC,0xCC,0xF8,0xC0,0xC0,0xC0,0x07,0x00,0x06,0x00,0x01,0x00,0x78,
0xCC,0xCC,0xCC,0xCC,0xD8,0x6C,0x07,0x00,0x06,0x00,0x01,0x00,0xF8,0xCC,0xCC,0xF8,
0xD8,0xCC,0xCC,0x07,0x00,0x06,0x00,0x01,0x00,0x78,0xCC,0xC0,0x78,0x0C,0xCC,0x78,
0x07,0x00,0x06,0x00,0x01,0x00,0xFC,0x30,0x30,0x30,0x30,0x30,0x30,0x07,0x00,0x06,
0x00,0x01,0x00,0xCC,0xCC,0xCC,0xCC,0xCC,0xCC,0x7C,0x07,0x00,0x06,0x00,0x01,0x00,
0xCC,0xCC,0xCC,0xCC,0xCC,0x78,0x30,0x07,0x00,0x08,0x00,0x01,0x00,0xC3,0xC3,0xC3,
0xDB,0xFF,0xE7,0xC3,0x07,0x00,0x06,0x00,0x01,0x00,0xCC,0xCC,0x78,0x30,0x78,0xCC,
0xCC,0x07,0x00,0x06,0x00,0x01,0x00,0xCC,0xCC,0xCC,0x78,0x30,0x30,0x30,0x07,0x00,
0x08,0x00,0x01,0x00,0xFF,0x06,0x0C,0x18,0x30,0x60,0xFF,0x07,0x00,0x04,0x00,0x01,
0x00,0xF0,0xC0,0xC0,0xC0,0xC0,0xC0,0xF0,0x07,0x00,0x08,0x00,0x01,0x00,0xC0,0x60,
0x30,0x18,0x0C,0x06,0x03,0x07,0x00,0x04,0x00,0x01,0x00,0xF0,0x30,0x30,0x30,0x30,
0x30,0xF0,0x03,0x00,0x06,0x00,0x01,0x00,0x30,0x78,0xCC,0x08,0x00,0x06,0x00,0x01,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0xFC,0x03,0x00,0x04,0x00,0x01,0x00,0xC0,
0x60,0x30,0x07,0x00,0x06,0x00,0x01,0x00,0x00,0x00,0x78,0x0C,0x7C,0xCC,0x7C,0x07,
0x00,0x06,0x00,0x01,0x00,0xC0,0xC0,0xF8,0xCC,0xCC,0xCC,0xF8,0x07,0x00,0x06,0x00,
0x01,0x00,0x00,0x00,0x78,0xCC,0xC0,0xCC,0x78,0x07,0x00,0x06,0x00,0x01,0x00,0x0C,
0x0C,0x7C,0xCC,0xCC,0xCC,0x7C,0x07,0x00,0x06,0x00,0x01,0x00,0x00,0x00,0x78,0xCC,0xFC,0xC0,0x7C,0x07,0x00,0x05,0x00,
0x01,0x00,0x38,0x60,0xF8,0x60,0x60,0x60,0x60,0x09,0x00,0x06,0x00,0x01,0x00,0x00,
0x00,0x78,0xCC,0xCC,0xCC,0x7C,0x0C,0x78,0x07,0x00,0x06,0x00,0x01,0x00,0xC0,0xC0,
0xF8,0xCC,0xCC,0xCC,0xCC,0x07,0x00,0x02,0x00,0x01,0x00,0xC0,0x00,0xC0,0xC0,0xC0,
0xC0,0x09,0x00,0x04,0x00,0x01,0x00,0x30,0x00,0x30,0x30,0x30,0x30,0x30,0x30,0xE0,
0x07,0x00,0x06,0x00,0x01,0x00,0xC0,0xC0,0xCC,0xD8,0xF0,0xD8,0xCC,0x07,0x00,0x02,
0x00,0x01,0x00,0xC0,0xC0,0xC0,0xC0,0xC0,0xC0,0xC0,0x07,0x00,0x08,0x00,0x01,0x00,
0x00,0x00,0xFE,0xDB,0xDB,0xDB,0xDB,0x07,0x00,0x06,0x00,0x01,0x00,0x00,0x00,0xF8,
0xCC,0xCC,0xCC,0xCC,0x07,0x00,0x06,0x00,0x01,0x00,0x00,0x00,0x78,0xCC,0xCC,0xCC,
0xF8,0xC0,0xC0,0x09,0x00,0x06,0x00,0x01,0x00,0x00,0x00,0x78,0xCC,0xCC,0xCC,0x7C,
0x0C,0x0C,0x07,0x00,0x06,0x00,0x01,0x00,0x00,0x00,0x78,0xCC,0xC0,0xC0,0xC0,0x07,
0x00,0x06,0x00,0x01,0x00,0x00,0x00,0x78,0xC0,0x78,0x0C,0xF8,0x07,0x00,0x05,0x00,
0x01,0x00,0x60,0x60,0xF8,0x60,0x60,0x60,0x38,0x07,0x00,0x06,0x00,0x01,0x00,0x00,
0x00,0xCC,0xCC,0xCC,0xCC,0x7C,0x07,0x00,0x06,0x00,0x01,0x00,0x00,0x00,0xCC,0xCC,
0xCC,0x78,0x30,0x07,0x00,0x08,0x00,0x01,0x00,0x00,0x00,0xC3,0xC3,0xDB,0xFF,0x66,
0x07,0x00,0x06,0x00,0x01,0x00,0x00,0x00,0xCC,0x78,0x30,0x78,0xCC,0x09,0x00,0x06,
0x00,0x01,0x00,0x00,0x00,0xCC,0xCC,0xCC,0xCC,0x7C,0x0C,0x78,0x07,0x00,0x06,0x00,
0x01,0x00,0x00,0x00,0xFC,0x18,0x30,0x60,0xFC,0x07,0x00,0x04,0x00,0x01,0x00,0x30,
0x60,0x60,0xC0,0x60,0x60,0x30,0x07,0x00,0x02,0x00,0x01,0x00,0xC0,0xC0,0xC0,0x00,
0xC0,0xC0,0xC0,0x07,0x00,0x04,0x00,0x01,0x00,0xC0,0x60,0x60,0x30,0x60,0x60,0xC0,
0x02,0x00,0x07,0x00,0x01,0x00,0x76,0xDC,0x07,0x00,0x07,0x00,0x01,0x00,0x00,0x00,
0x70,0xC4,0xCC,0x8C,0x38,0x07,0x00,0x07,0x00,0x01,0x00,0x00,0x06,0x0C,0xD8,0xF0,
0xE0,0xC0,0x08,0x00,0x10,0x00,0x02,0x00,0x7F,0xFE,0xCD,0xC7,0xB5,0xEF,0xB5,0xEF,
0x85,0xEF,0xB5,0xEF,0xB4,0x6F,0x08,0x00,0x13,0x00,0x03,0x00,0x7F,0xFF,0xC0,0xCC,
0x46,0xE0,0xB6,0xDA,0xE0,0xBE,0xDA,0xE0,0xBE,0xC6,0xE0,0xB6,0xDA,0xE0,0xCE,0xDA,
0x20,0x7F,0xFF,0xC0,0x08,0x00,0x11,0x00,0x03,0x00,0x7F,0xFF,0x00,0xC6,0x73,0x80,
0xDD,0xAD,0x80,0xCE,0xEF,0x80,0xDF,0x6F,0x80,0xDD,0xAD,0x80,0xC6,0x73,0x80,0x7F,
0xFF,0x00
};

static void load_font_character_offsets(rawfont_type* data) {
	int n_chars = data->last_char - data->first_char + 1;
	byte* pos = (byte*) &data->offsets[n_chars];
	for (int index = 0; index < n_chars; ++index) {
		data->offsets[index] = SDL_SwapLE16(pos - (byte*) data);
		image_data_type* image_data = (image_data_type*) pos;
		int image_bytes = SDL_SwapLE16(image_data->height) * calc_stride(image_data);
		pos = (byte*) &image_data->data + image_bytes;
	}
}

font_type load_font_from_data(/*const*/ rawfont_type* data) {
	font_type font;
	font.first_char = data->first_char;
	font.last_char = data->last_char;
	font.height_above_baseline = SDL_SwapLE16(data->height_above_baseline);
	font.height_below_baseline = SDL_SwapLE16(data->height_below_baseline);
	font.space_between_lines = SDL_SwapLE16(data->space_between_lines);
	font.space_between_chars = SDL_SwapLE16(data->space_between_chars);
	int n_chars = font.last_char - font.first_char + 1;
	// Allow loading a font even if the offsets for each character image were not supplied in the raw data.
	if (SDL_SwapLE16(data->offsets[0]) == 0) {
		load_font_character_offsets(data);
	}
	chtab_type* chtab = malloc(sizeof(chtab_type) + sizeof(image_type*) * n_chars);
	// Make a dummy palette for decode_image().
	dat_pal_type dat_pal;
	memset(&dat_pal, 0, sizeof(dat_pal));
	dat_pal.vga[1].r = dat_pal.vga[1].g = dat_pal.vga[1].b = 0x3F; // white
	for (int index = 0, chr = data->first_char; chr <= data->last_char; ++index, ++chr) {
		/*const*/ image_data_type* image_data = (/*const*/ image_data_type*)((/*const*/ byte*)data + SDL_SwapLE16(data->offsets[index]));
		//image_data->flags=0;
		if (image_data->height == SDL_SwapLE16(0)) image_data->height = SDL_SwapLE16(1); // HACK: decode_image() returns NULL if height==0.
		image_type* image;
		chtab->images[index] = image = decode_image(image_data, &dat_pal);
		if (SDL_SetColorKey(image, SDL_TRUE, 0) != 0) {
			sdlperror("load_font_from_data: SDL_SetColorKey");
			quit(1);
		}
	}
	font.chtab = chtab;
	return font;
}

// Small font data (hardcoded), defined in menu.c
extern byte hc_small_font_data[];

void load_font(void) {
	// Try to load font from a file.
	const char* const dat_name = "font";
	dat_type* dathandle = open_dat(dat_name, 1);
	hc_font.chtab = load_sprites_from_file(1000, 1<<1, 0, dat_name);
	close_dat(dathandle);
	if (hc_font.chtab == NULL) {
		// Use built-in font.
		hc_font = load_font_from_data((/*const*/ rawfont_type*)hc_font_data);
	}

#ifdef USE_MENU
	hc_small_font = load_font_from_data((rawfont_type*)hc_small_font_data);
#endif

}

// seg009:35C5
int get_char_width(byte character) {
	font_type* font = textstate.ptr_font;
	int width = 0;
	if (character <= font->last_char && character >= font->first_char) {
		image_type* image = font->chtab->images[character - font->first_char];
		if (image != NULL) {
			width += image->w; //char_ptrs[character - font->first_char]->width;
			if (width) width += font->space_between_chars;
		}
	}
	return width;
}

// seg009:3E99
int find_linebreak(const char* text,int length,int break_width,int x_align) {
	int curr_char_pos = 0;
	short last_break_pos = 0; // in characters
	short curr_line_width = 0; // in pixels
	const char* text_pos = text;
	while (curr_char_pos < length) {
		curr_line_width += get_char_width(*text_pos);
		if (curr_line_width <= break_width) {
			++curr_char_pos;
			char curr_char = *text_pos;
			text_pos++;
			if (curr_char == '\n') {
				return curr_char_pos;
			}
			if (curr_char == '-' ||
				(x_align <= 0 && (curr_char == ' ' || *text_pos == ' ')) ||
				(*text_pos == ' ' && curr_char == ' ')
			) {
				// May break here.
				last_break_pos = curr_char_pos;
			}
		} else {
			if (last_break_pos == 0) {
				// If the first word is wider than break_width then break it.
				return curr_char_pos;
			} else {
				// Otherwise break at the last space.
				return last_break_pos;
			}
		}
	}
	return curr_char_pos;
}

// seg009:403F
int get_line_width(const char* text,int length) {
	int width = 0;
	const char* text_pos = text;
	while (--length >= 0) {
		width += get_char_width(*text_pos);
		text_pos++;
	}
	return width;
}

// seg009:3706
int draw_text_character(byte character) {
	//printf("going to do draw_text_character...\n");
	font_type* font = textstate.ptr_font;
	int width = 0;
	if (character <= font->last_char && character >= font->first_char) {
		image_type* image = font->chtab->images[character - font->first_char]; //char_ptrs[character - font->first_char];
		if (image != NULL) {
			method_3_blit_mono(image, textstate.current_x, textstate.current_y - font->height_above_baseline, textstate.textblit, textstate.textcolor);
			width = font->space_between_chars + image->w;
		}
	}
	textstate.current_x += width;
	return width;
}

// seg009:377F
int draw_text_line(const char* text,int length) {
	//hide_cursor();
	int width = 0;
	const char* text_pos = text;
	while (--length >= 0) {
		width += draw_text_character(*text_pos);
		text_pos++;
	}
	//show_cursor();
	return width;
}

// seg009:3755
int draw_cstring(const char* string) {
	//hide_cursor();
	int width = 0;
	const char* text_pos = string;
	while ('\0' != *text_pos) {
		width += draw_text_character(*text_pos);
		text_pos++;
	}
	//show_cursor();
	return width;
}

// seg009:3F01
const rect_type* draw_text(const rect_type* rect_ptr,int x_align,int y_align,const char* text,int length) {
	//printf("going to do draw_text()...\n");
	short rect_top;
	short rect_height;
	short rect_width;
	//textinfo_type var_C;
	short num_lines;
	short font_line_distance;
	//hide_cursor();
	//get_textinfo(&var_C);
	set_clip_rect(rect_ptr);
	rect_width = rect_ptr->right - rect_ptr->left;
	rect_top = rect_ptr->top;
	rect_height = rect_ptr->bottom - rect_ptr->top;
	num_lines = 0;
	int rem_length = length;
	const char* line_start = text;
	#define MAX_LINES 100
	const char* line_starts[MAX_LINES];
	int line_lengths[MAX_LINES];
	do {
		int line_length = find_linebreak(line_start, rem_length, rect_width, x_align);
		if (line_length == 0) break;
		if (num_lines >= MAX_LINES) {
			//... ERROR!
			printf("draw_text(): Too many lines!\n");
			quit(1);
		}
		line_starts[num_lines] = line_start;
		line_lengths[num_lines] = line_length;
		++num_lines;
		line_start += line_length;
		rem_length -= line_length;
	} while(rem_length);
	font_type* font = textstate.ptr_font;
	font_line_distance = font->height_above_baseline + font->height_below_baseline + font->space_between_lines;
	int text_height = font_line_distance * num_lines - font->space_between_lines;
	int text_top = rect_top;
	if (y_align >= 0) {
		if (y_align <= 0) {
			// middle
			// The +1 is for simulating SHR + ADC/SBB.
			text_top += (rect_height+1)/2 - (text_height+1)/2;
		} else {
			// bottom
			text_top += rect_height - text_height;
		}
	}
	textstate.current_y = text_top + font->height_above_baseline;
	for (int i = 0; i < num_lines; ++i) {
		const char* line_pos = line_starts[i];
		int line_length = line_lengths[i];
		if (x_align < 0 &&
			*line_pos == ' ' &&
			i != 0 &&
			*(line_pos-1) != '\n'
		) {
			// Skip over space if it's not at the beginning of a line.
			++line_pos;
			--line_length;
			if (line_length != 0 &&
				*line_pos == ' ' &&
				*(line_pos-2) == '.'
			) {
				// Skip over second space after point.
				++line_pos;
				--line_length;
			}
		}
		int line_width = get_line_width(line_pos,line_length);
		int text_left = rect_ptr->left;
		if (x_align >= 0) {
			if (x_align <= 0) {
				// center
				text_left += rect_width/2 - line_width/2;
			} else {
				// right
				text_left += rect_width - line_width;
			}
		}
		textstate.current_x = text_left;
		//printf("going to draw text line...\n");
		draw_text_line(line_pos,line_length);
		textstate.current_y += font_line_distance;
	}
	reset_clip_rect();
	//set_textinfo(...);
	//show_cursor();
	return rect_ptr;
}

// seg009:3E4F
void show_text(const rect_type* rect_ptr,int x_align,int y_align,const char* text) {
	// stub
	//printf("show_text: %s\n",text);
	draw_text(rect_ptr, x_align, y_align, text, (int)strlen(text));
}

// seg009:04FF
void show_text_with_color(const rect_type* rect_ptr,int x_align,int y_align, const char* text,int color) {
	short saved_textcolor;
	saved_textcolor = textstate.textcolor;
	textstate.textcolor = color;
	show_text(rect_ptr, x_align, y_align, text);
	textstate.textcolor = saved_textcolor;
}

// seg009:3A91
void set_curr_pos(int xpos,int ypos) {
	textstate.current_x = xpos;
	textstate.current_y = ypos;
}

// seg009:145A
void init_copyprot_dialog() {
	copyprot_dialog = make_dialog_info(&dialog_settings, &dialog_rect_1, &dialog_rect_1, NULL);
	copyprot_dialog->peel = read_peel_from_screen(&copyprot_dialog->peel_rect);
}

// seg009:0838
int showmessage(char* text,int arg_4,void *arg_0) {
	word key;
	rect_type rect;
	//font_type* saved_font_ptr;
	//surface_type* old_target;
	//old_target = current_target_surface;
	//current_target_surface = onscreen_surface_;
	// In the disassembly there is some messing with the current_target_surface and font (?)
	// However, this does not seem to be strictly necessary
	if (NULL == offscreen_surface) offscreen_surface = make_offscreen_buffer(&screen_rect); // In case we get an error before there is an offsceen buffer
	method_1_blit_rect(offscreen_surface, onscreen_surface_, &copyprot_dialog->peel_rect, &copyprot_dialog->peel_rect, 0);
	draw_dialog_frame(copyprot_dialog);
	//saved_font_ptr = textstate.ptr_font;
	//saved_font_ptr = current_target_surface->ptr_font;
	//current_target_surface->ptr_font = ptr_font;
	shrink2_rect(&rect, &copyprot_dialog->text_rect, 2, 1);
	show_text_with_color(&rect, halign_center, valign_middle, text, color_15_brightwhite);
	//textstate.ptr_font = saved_font_ptr;
	//current_target_surface->ptr_font = saved_font_ptr;
	clear_kbd_buf();
	do {
		idle();
		key = key_test_quit(); // Press any key to continue...
	} while(key == 0);
	//restore_dialog_peel_2(copyprot_dialog->peel);
	//current_target_surface = old_target;
	need_full_redraw = 1; // lazy: instead of neatly restoring only the relevant part, just redraw the whole screen
	return key;
}

// seg009:08FB
dialog_type* make_dialog_info(dialog_settings_type* settings, rect_type* dialog_rect,
                                            rect_type* text_rect, peel_type* dialog_peel) {
	dialog_type* dialog_info;
	dialog_info = malloc(sizeof(dialog_type));
	dialog_info->settings = settings;
	dialog_info->has_peel = 0;
	dialog_info->peel = dialog_peel;
	if (text_rect != NULL)
		dialog_info->text_rect = *text_rect;
	calc_dialog_peel_rect(dialog_info);
	if (text_rect != NULL) {        // does not seem to be quite right; see seg009:0948 (?)
		read_dialog_peel(dialog_info);
	}
	return dialog_info;
}

// seg009:0BE7
void calc_dialog_peel_rect(dialog_type* dialog) {
	dialog_settings_type* settings;
	settings = dialog->settings;
	dialog->peel_rect.left = dialog->text_rect.left - settings->left_border;
	dialog->peel_rect.top = dialog->text_rect.top - settings->top_border;
	dialog->peel_rect.right = dialog->text_rect.right + settings->right_border + settings->shadow_right;
	dialog->peel_rect.bottom = dialog->text_rect.bottom + settings->bottom_border + settings->shadow_bottom;
}

// seg009:0BB0
void read_dialog_peel(dialog_type* dialog) {
	if (dialog->has_peel) {
		if (dialog->peel == NULL) {
			dialog->peel = read_peel_from_screen(&dialog->peel_rect);
		}
		dialog->has_peel = 1;
		draw_dialog_frame(dialog);
	}
}

// seg009:09DE
void draw_dialog_frame(dialog_type* dialog) {
	dialog->settings->method_2_frame(dialog);
}

// A pointer to this function is the first field of dialog_settings (data:2944)
// Perhaps used when replacing a dialog's text with another text (?)
// seg009:096F
void add_dialog_rect(dialog_type* dialog) {
	draw_rect(&dialog->text_rect, color_0_black);
}

// seg009:09F0
void dialog_method_2_frame(dialog_type* dialog) {
	rect_type rect;
	short shadow_right = dialog->settings->shadow_right;
	short shadow_bottom = dialog->settings->shadow_bottom;
	short bottom_border = dialog->settings->bottom_border;
	short outer_border = dialog->settings->outer_border;
	short peel_top = dialog->peel_rect.top;
	short peel_left = dialog->peel_rect.left;
	short peel_bottom = dialog->peel_rect.bottom;
	short peel_right = dialog->peel_rect.right;
	short text_top = dialog->text_rect.top;
	short text_left = dialog->text_rect.left;
	short text_bottom = dialog->text_rect.bottom;
	short text_right = dialog->text_rect.right;
	// Draw outer border
	rect = (rect_type) { peel_top, peel_left, peel_bottom - shadow_bottom, peel_right - shadow_right };
	draw_rect(&rect, color_0_black);
	// Draw shadow (right)
	rect = (rect_type) { text_top, peel_right - shadow_right, peel_bottom, peel_right };
	draw_rect(&rect, get_text_color(0, color_8_darkgray /*dialog's shadow*/, 0));
	// Draw shadow (bottom)
	rect = (rect_type) { peel_bottom - shadow_bottom, text_left, peel_bottom, peel_right };
	draw_rect(&rect, get_text_color(0, color_8_darkgray /*dialog's shadow*/, 0));
	// Draw inner border (left)
	rect = (rect_type) { peel_top + outer_border, peel_left + outer_border, text_bottom, text_left };
	draw_rect(&rect, color_15_brightwhite);
	// Draw inner border (top)
	rect = (rect_type) { peel_top + outer_border, text_left, text_top, text_right + dialog->settings->right_border - outer_border };
	draw_rect(&rect, color_15_brightwhite);
	// Draw inner border (right)
	rect.top = text_top;
	rect.left =  text_right;
	rect.bottom = text_bottom + bottom_border - outer_border;           // (rect.right stays the same)
	draw_rect(&rect, color_15_brightwhite);
	// Draw inner border (bottom)
	rect = (rect_type) { text_bottom, peel_left + outer_border, text_bottom + bottom_border - outer_border, text_right };
	draw_rect(&rect, color_15_brightwhite);
}

// seg009:0C44
void show_dialog(const char* text) {
	char string[256];
	snprintf(string, sizeof(string), "%s\n\nPress any key to continue.", text);
	showmessage(string, 1, &key_test_quit);
}

// seg009:0791
int get_text_center_y(const rect_type* rect) {
	const font_type* font;
	short empty_height; // height of empty space above+below the line of text
	font = &hc_font;//current_target_surface->ptr_font;
	empty_height = rect->bottom - font->height_above_baseline - font->height_below_baseline - rect->top;
	return ((empty_height - empty_height % 2) >> 1) + font->height_above_baseline + empty_height % 2 + rect->top;
}

// seg009:3E77
int get_cstring_width(const char* text) {
	int width = 0;
	const char* text_pos = text;
	char curr_char;
	while ('\0' != (curr_char = *text_pos)) {
		text_pos++;
		width += get_char_width(curr_char);
	}
	return width;
}

// seg009:0767
void draw_text_cursor(int xpos,int ypos,int color) {
	set_curr_pos(xpos, ypos);
	/*current_target_surface->*/textstate.textcolor = color;
	draw_text_character('_');
	//restore_curr_color();
	textstate.textcolor = 15;
}

// seg009:053C
int input_str(const rect_type* rect,char* buffer,int max_length,const char *initial,int has_initial,int arg_4,int color,int bgcolor) {
	// Display the screen keyboard if supported.
	//SDL_SetHint(SDL_HINT_IME_SHOW_UI, "1");
	SDL_Rect sdlrect;
	rect_to_sdlrect(rect, &sdlrect);
	SDL_SetTextInputRect(&sdlrect);
	SDL_StartTextInput();

	word key;
	short current_xpos;
	short length = 0;
	short cursor_visible = 0;
	draw_rect(rect, bgcolor);
	short init_length = strlen(initial);
	if (has_initial) {
		strcpy(buffer, initial);
		length = init_length;
	}
	current_xpos = rect->left + arg_4;
	short ypos = get_text_center_y(rect);
	set_curr_pos(current_xpos, ypos);
	/*current_target_surface->*/textstate.textcolor = color;
	draw_cstring(initial);
	//restore_curr_pos?();
	current_xpos += get_cstring_width(initial) + (init_length != 0) * arg_4;
	do {
		key = 0;
		do {
			if (cursor_visible) {
				draw_text_cursor(current_xpos, ypos, color);
			} else {
				draw_text_cursor(current_xpos, ypos, bgcolor);
			}
			cursor_visible = !cursor_visible;
			start_timer(timer_0, 6);
			if (key) {
				if (cursor_visible) {
					draw_text_cursor(current_xpos, ypos, color);
					cursor_visible = !cursor_visible;
				}
				if (key == SDL_SCANCODE_RETURN) { // Enter
					buffer[length] = 0;
					SDL_StopTextInput();
					return length;
				} else break;
			}
			while (!has_timer_stopped(timer_0) && (key = key_test_quit()) == 0) idle();
		} while (1);
		// Only use the printable ASCII chars (UTF-8 encoding)
		// We check SDL_TEXTINPUT instead of SDL_KEYDOWN.
		// If '+' is on Shift+something then we can't detect it in SDL_KEYDOWN,
		// because event.key.keysym.sym only tells us what character would the key type without shift.
		char entered_char = last_text_input <= 0x7E ? last_text_input : 0;
		clear_kbd_buf();

		if (key == SDL_SCANCODE_ESCAPE) { // Esc
			draw_rect(rect, bgcolor);
			buffer[0] = 0;
			SDL_StopTextInput();
			return -1;
		}
		if (length != 0 && (key == SDL_SCANCODE_BACKSPACE ||
				key == SDL_SCANCODE_DELETE)) { // Backspace, Delete
			--length;
			draw_text_cursor(current_xpos, ypos, bgcolor);
			current_xpos -= get_char_width(buffer[length]);
			set_curr_pos(current_xpos, ypos);
			/*current_target_surface->*/textstate.textcolor = bgcolor;
			draw_text_character(buffer[length]);
			//restore_curr_pos?();
			draw_text_cursor(current_xpos, ypos, color);
		}
		else if (entered_char >= 0x20 && entered_char <= 0x7E && length < max_length) {
			// Would the new character make the cursor go past the right side of the rect?
			if (get_char_width('_') + get_char_width(entered_char) + current_xpos < rect->right) {
				draw_text_cursor(current_xpos, ypos, bgcolor);
				set_curr_pos(current_xpos, ypos);
				/*current_target_surface->*/textstate.textcolor = color;
				buffer[length] = entered_char;
				length++;
				current_xpos += draw_text_character(entered_char);
			}
		}
	} while(1);
}

#else // USE_TEXT

// seg009:3706
int draw_text_character(byte character) {
	// stub
	// printf("draw_text_character: %c\n",character);
	return 0;
}

// seg009:3E4F
void show_text(const rect_type* rect_ptr,int x_align,int y_align,const char *text) {
	// stub
	// printf("show_text: %s\n",text);
}

// seg009:04FF
void show_text_with_color(const rect_type* rect_ptr,int x_align,int y_align,const char* text,int color) {
	//short saved_textcolor;
	//saved_textcolor = textstate.textcolor;
	//textstate.textcolor = color;
	show_text(rect_ptr, x_align, y_align, text);
	//textstate.textcolor = saved_textcolor;
}

// seg009:3A91
void set_curr_pos(int xpos,int ypos) {
	// stub
}

// seg009:0C44
void show_dialog(const char *text) {
	// stub
	puts(text);
}

// seg009:053C
int input_str(const rect_type* rect,char* buffer,int max_length,const char* initial,int has_initial,int arg_4,int color,int bgcolor) {
	// stub
	strncpy(buffer, "dummy input text", max_length);
	return strlen(buffer);
}

int showmessage(char* text,int arg_4,void *arg_0) {
	// stub
	puts(text);
	return 0;
}

void init_copyprot_dialog() {
	// stub
}

void draw_dialog_frame(dialog_type *dialog) {
	// stub
}

void add_dialog_rect(dialog_type *dialog) {
	// stub
}

void dialog_method_2_frame(dialog_type *dialog) {
	// stub
}

#endif // USE_TEXT

// seg009:37E8
void draw_rect(const rect_type* rect,int color) {
	method_5_rect(rect, blitters_0_no_transp, color);
}

// seg009:3985
surface_type *rect_sthg(surface_type* surface,const rect_type* rect) {
	// stub
	return surface;
}

// seg009:39CE
rect_type *shrink2_rect(rect_type* target_rect,const rect_type* source_rect,int delta_x,int delta_y) {
	target_rect->top    = source_rect->top    + delta_y;
	target_rect->left   = source_rect->left   + delta_x;
	target_rect->bottom = source_rect->bottom - delta_y;
	target_rect->right  = source_rect->right  - delta_x;
	return target_rect;
}

// Peel pool system to avoid PSRAM allocator memory leaks
// Key insight: Allocate in size buckets for better reuse
// Instead of exact sizes, round up to power-of-2 to allow slot reuse
#define MAX_PEEL_POOL_SIZE 50
#define POOL_SURFACE_WIDTH 320
#define POOL_SURFACE_HEIGHT 200
#define MAX_PEEL_MEMORY_BUDGET (1200 * 1024)  // 1.2MB budget

// Size buckets for efficient reuse (power of 2)
#define PEEL_SIZE_SMALL (2 * 1024)    // 2KB - small sprites
#define PEEL_SIZE_MEDIUM (8 * 1024)   // 8KB - medium sprites  
#define PEEL_SIZE_LARGE (32 * 1024)   // 32KB - large sprites
#define PEEL_SIZE_XLARGE (128 * 1024) // 128KB - very large/full screen

static size_t total_peel_memory_used = 0;

static size_t round_up_to_bucket(size_t size) {
	if (size <= PEEL_SIZE_SMALL) return PEEL_SIZE_SMALL;
	if (size <= PEEL_SIZE_MEDIUM) return PEEL_SIZE_MEDIUM;
	if (size <= PEEL_SIZE_LARGE) return PEEL_SIZE_LARGE;
	return PEEL_SIZE_XLARGE;
}

typedef struct manual_peel_type {
	byte* pixels;  // Exact-size pixel buffer (allocated on first use)
	rect_type rect;
	int pitch;
	int allocated_size;  // Track actual allocation size
	byte in_use;
	byte data_valid;  // Flag: was pixel data actually copied?
} manual_peel_type;

static manual_peel_type manual_peel_pool[MAX_PEEL_POOL_SIZE];
static manual_peel_type dummy_peel;  // Invalid peel returned when allocation fails
static int peel_pool_initialized = 0;
static int peel_pool_next_free = 0;
static int max_peels_used = 0;

void clear_all_peels(void) {
	// Mark all peels as free without deallocating buffers
	// Used during screen transitions to prevent stale peels
	for (int i = 0; i < MAX_PEEL_POOL_SIZE; i++) {
		manual_peel_pool[i].in_use = 0;
		manual_peel_pool[i].data_valid = 0;
		manual_peel_pool[i].rect = (rect_type){0, 0, 0, 0};
	}
}

void invalidate_peels_in_rect(const rect_type* rect) {
	if (!peel_pool_initialized || rect == NULL) {
		return;
	}
	for (int i = 0; i < MAX_PEEL_POOL_SIZE; i++) {
		manual_peel_type* slot = &manual_peel_pool[i];
		if (!slot->in_use || !slot->data_valid) {
			continue;
		}
		if (slot->rect.right <= slot->rect.left || slot->rect.bottom <= slot->rect.top) {
			continue;
		}
		if (rect->left < slot->rect.right && rect->right > slot->rect.left &&
		    rect->top < slot->rect.bottom && rect->bottom > slot->rect.top) {
			slot->in_use = 0;
			slot->data_valid = 0;
		}
	}
}

static void init_peel_pool() {
	if (peel_pool_initialized) return;
	
	// Don't pre-allocate! Allocate exact size needed per peel
	for (int i = 0; i < MAX_PEEL_POOL_SIZE; i++) {
		manual_peel_pool[i].pixels = NULL;
		manual_peel_pool[i].pitch = 0;
		manual_peel_pool[i].allocated_size = 0;
		manual_peel_pool[i].rect = (rect_type){0, 0, 0, 0};
		manual_peel_pool[i].in_use = 0;
		manual_peel_pool[i].data_valid = 0;
	}
	
	// Initialize dummy peel for failed allocations
	dummy_peel.pixels = NULL;
	dummy_peel.pitch = 0;
	dummy_peel.allocated_size = 0;
	dummy_peel.rect = (rect_type){0, 0, 0, 0};
	dummy_peel.in_use = 0;
	
	peel_pool_initialized = 1;
	total_peel_memory_used = 0;
	// printf("Peel pool initialized: %d slots, bucketed allocation (budget: %d KB)\n", 
	//        MAX_PEEL_POOL_SIZE, MAX_PEEL_MEMORY_BUDGET / 1024);
	// printf("  Buckets: 2KB, 8KB, 32KB, 128KB for efficient reuse\n");
}

// seg009:3BBA
void restore_peel(peel_type* peel_ptr) {
	// NULL safety and dummy peel check
	if (peel_ptr == NULL || peel_ptr == (peel_type*)&dummy_peel) {
		return;
	}
	
	// Check if peel was freed (marked invalid by free_peel)
	if (peel_ptr->peel == NULL) {
		// Also mark pool slot as free since this peel is done
		manual_peel_type* freed_peel = (manual_peel_type*)peel_ptr;
		freed_peel->in_use = 0;
		freed_peel->data_valid = 0;
		return;  // Skip freed peels
	}
	
	// This is now a wrapper that extracts the manual peel data
	manual_peel_type* manual_peel = (manual_peel_type*)peel_ptr;
	
	int width = manual_peel->rect.right - manual_peel->rect.left;
	int height = manual_peel->rect.bottom - manual_peel->rect.top;
	int top = manual_peel->rect.top;
	
	// Clip to game area (y < 192) to prevent artifacts on health bar
	if (top >= 192) {
		manual_peel->in_use = 0;
		return;  // Entirely in health bar area, skip
	}
	if (top + height > 192) {
		height = 192 - top;  // Clip height
	}
	
	if (width <= 0 || height <= 0 || width > POOL_SURFACE_WIDTH || height > POOL_SURFACE_HEIGHT) {
		manual_peel->in_use = 0;
		return;
	}
	
	// If peel has no buffer (budget exceeded), skip gracefully
	if (manual_peel->pixels == NULL) {
		manual_peel->in_use = 0;
		return;
	}
	
	// Check if pixel data is valid (was actually copied)
	if (!manual_peel->data_valid) {
		manual_peel->in_use = 0;
		return;  // Skip peels that were never properly saved
	}
	
	// Lock target surface to write pixels
	if (SDL_LockSurface(current_target_surface) != 0) {
		sdlperror("restore_peel: SDL_LockSurface");
		return;
	}
	
	// Copy pixels from our buffer to the screen
	byte* src_pixels = manual_peel->pixels;
	byte* dst_pixels = (byte*)current_target_surface->pixels;
	int dst_pitch = current_target_surface->pitch;
	int bytes_per_pixel = current_target_surface->format->BytesPerPixel;
	
	for (int y = 0; y < height; y++) {
		int src_offset = y * manual_peel->pitch;
		int dst_offset = ((top + y) * dst_pitch) + 
		                 (manual_peel->rect.left * bytes_per_pixel);
		memcpy(dst_pixels + dst_offset, src_pixels + src_offset, width * bytes_per_pixel);
	}
	
	SDL_UnlockSurface(current_target_surface);
	
	// Mark this peel as free
	manual_peel->in_use = 0;
}

// seg009:3BE9
peel_type* read_peel_from_screen(const rect_type* rect) {
	init_peel_pool();
	
	int width = rect->right - rect->left;
	int height = rect->bottom - rect->top;
	int bytes_per_pixel = current_target_surface->format->BytesPerPixel;
	size_t buffer_size_needed = width * height * bytes_per_pixel;
	size_t buffer_size = round_up_to_bucket(buffer_size_needed);  // Round to bucket
	
	// Find a suitable slot: free slot OR large enough slot that's not in use
	int found_index = -1;
	for (int i = 0; i < MAX_PEEL_POOL_SIZE; i++) {
		int check_index = (peel_pool_next_free + i) % MAX_PEEL_POOL_SIZE;
		manual_peel_type* slot = &manual_peel_pool[check_index];
		
		if (!slot->in_use) {
			// Free slot - perfect, use it
			found_index = check_index;
			peel_pool_next_free = (check_index + 1) % MAX_PEEL_POOL_SIZE;
			break;
		}
		// Don't steal occupied slots - it causes corruption when old peel is restored
	}
	
	if (found_index == -1) {
		static int exhaustion_count = 0;
		if (exhaustion_count < 5) {
			// Count how many slots are actually in use
			int in_use_count = 0;
			for (int i = 0; i < MAX_PEEL_POOL_SIZE; i++) {
				if (manual_peel_pool[i].in_use) in_use_count++;
			}
			printf("ERROR: Peel pool exhausted! No free slots for %dx%d (%zu->%zu bytes)\n", 
			       width, height, buffer_size_needed, buffer_size);
			printf("  Pool status: %d/%d slots in use\n", in_use_count, MAX_PEEL_POOL_SIZE);
			exhaustion_count++;
		}
		// Return dummy peel that will be safely ignored by restore_peel
		return (peel_type*)&dummy_peel;
	}
	
	manual_peel_type* result = &manual_peel_pool[found_index];
	result->in_use = 1;
	result->rect = *rect;
	result->data_valid = 0;  // Mark as invalid until we successfully copy pixels
	
	// Debug: log peel operations (disabled - too verbose)
	// if (found_index <= 10) {
	// 	printf("Peel[%d]: saving %dx%d at (%d,%d)-(%d,%d)\n",
	// 	       found_index, width, height, rect->left, rect->top, rect->right, rect->bottom);
	// }
	
	if (width > POOL_SURFACE_WIDTH || height > POOL_SURFACE_HEIGHT) {
		printf("ERROR: Peel size (%dx%d) exceeds max size (%dx%d)\n", 
		       width, height, POOL_SURFACE_WIDTH, POOL_SURFACE_HEIGHT);
		quit(1);
	}
	
	// Allocate buffer in rounded bucket size for better reuse
	// buffer_size already rounded to bucket above
	
	if (result->pixels == NULL || result->allocated_size < buffer_size) {
		// Need to allocate or reallocate
		
		// Check budget
		size_t size_delta = buffer_size - (result->pixels ? result->allocated_size : 0);
		if (total_peel_memory_used + size_delta > MAX_PEEL_MEMORY_BUDGET) {
			static int budget_warning_shown = 0;
			if (!budget_warning_shown) {
				printf("WARNING: Peel budget reached (%zu KB / %d KB)\n",
				       total_peel_memory_used / 1024, MAX_PEEL_MEMORY_BUDGET / 1024);
				printf("  Allocated %d peels so far. Further peels will cause artifacts.\n", found_index);
				budget_warning_shown = 1;
			}
			// Return dummy peel instead of NULL
			return (peel_type*)&dummy_peel;
		}
		
		// Allocate new buffer
		result->pixels = (byte*)psram_malloc(buffer_size);
		if (result->pixels == NULL) {
			printf("ERROR: PSRAM exhausted allocating %zu bytes for peel\n", buffer_size);
			return (peel_type*)&dummy_peel;
		}
		result->allocated_size = buffer_size;
		result->pitch = width * bytes_per_pixel;
		total_peel_memory_used += buffer_size;
		
		if (found_index <= 5 || found_index % 10 == 0) {
			// printf("Peel %d: %dx%d (%zu->%zu KB), total: %zu KB / %d KB\n", 
			//        found_index, width, height, 
			//        buffer_size_needed / 1024, buffer_size / 1024,
			//        total_peel_memory_used / 1024, MAX_PEEL_MEMORY_BUDGET / 1024);
		}
	}
	
	// Update pitch for current size
	result->pitch = width * bytes_per_pixel;
	
	// If peel has no buffer (budget exceeded), skip - game will have artifacts but continue
	if (result->pixels == NULL) {
		return (peel_type*)result;
	}
	
	// Lock source surface to read pixels
	if (SDL_LockSurface(current_target_surface) != 0) {
		sdlperror("read_peel_from_screen: SDL_LockSurface");
		return (peel_type*)result;
	}
	
	// Copy pixels from screen to our buffer
	byte* src_pixels = (byte*)current_target_surface->pixels;
	byte* dst_pixels = result->pixels;
	int src_pitch = current_target_surface->pitch;
	
	for (int y = 0; y < height; y++) {
		int src_offset = ((rect->top + y) * src_pitch) + (rect->left * bytes_per_pixel);
		int dst_offset = y * result->pitch;
		memcpy(dst_pixels + dst_offset, src_pixels + src_offset, width * bytes_per_pixel);
	}
	
	SDL_UnlockSurface(current_target_surface);
	
	// Mark data as valid - pixels were successfully copied
	result->data_valid = 1;
	
	// Track usage
	int in_use_count = 0;
	for (int i = 0; i < MAX_PEEL_POOL_SIZE; i++) {
		if (manual_peel_pool[i].in_use) in_use_count++;
	}
	if (in_use_count > max_peels_used) {
		max_peels_used = in_use_count;
		if (max_peels_used % 5 == 0) {
			// printf("Max peels used: %d/%d\n", max_peels_used, MAX_PEEL_POOL_SIZE);
		}
	}
	
	return (peel_type*)result;
}

// seg009:3D95
int intersect_rect(rect_type* output,const rect_type* input1,const rect_type* input2) {
	short left = MAX(input1->left, input2->left);
	short right = MIN(input1->right, input2->right);
	if (left < right) {
		output->left = left;
		output->right = right;
		short top = MAX(input1->top, input2->top);
		short bottom = MIN(input1->bottom, input2->bottom);
		if (top < bottom) {
			output->top = top;
			output->bottom = bottom;
			return 1;
		}
	}
	memset(output, 0, sizeof(rect_type));
	return 0;
}

// seg009:4063
rect_type* union_rect(rect_type* output,const rect_type* input1,const rect_type* input2) {
	short top = MIN(input1->top, input2->top);
	short left = MIN(input1->left, input2->left);
	short bottom = MAX(input1->bottom, input2->bottom);
	short right = MAX(input1->right, input2->right);
	output->top = top;
	output->left = left;
	output->bottom = bottom;
	output->right = right;
	return output;
}

enum userevents {
	userevent_SOUND,
	userevent_TIMER,
};

short speaker_playing = 0;
short digi_playing = 0;
short midi_playing = 0;
short ogg_playing = 0;

// The currently playing sound buffer for the PC speaker.
speaker_type* current_speaker_sound;
// Index for which note is currently playing.
int speaker_note_index;
// Tracks how long the last (partially played) speaker note has been playing (for the audio callback).
int current_speaker_note_samples_already_emitted;

void speaker_sound_stop(void) {
	if (!speaker_playing) return;
	SDL_LockAudio();
	speaker_playing = 0;
	current_speaker_sound = NULL;
	speaker_note_index = 0;
	current_speaker_note_samples_already_emitted = 0;
	SDL_UnlockAudio();
}

// The current buffer, holds the resampled sound data.
byte* digi_buffer = NULL;
// The current position in digi_buffer.
byte* digi_remaining_pos = NULL;
// The remaining length.
int digi_remaining_length = 0;

// The properties of the audio device.
SDL_AudioSpec* digi_audiospec = NULL;
// The desired samplerate. Everything will be resampled to this.
const int digi_samplerate = 44100;

void stop_digi(void) {
//	SDL_PauseAudio(1);
	if (!digi_playing) return;
	SDL_LockAudio();
	digi_playing = 0;
	/*
//	if (SDL_GetAudioStatus() == SDL_AUDIO_PLAYING) {
		SDL_PauseAudio(1);
		SDL_CloseAudio();
//	}
	if (digi_audiospec != NULL) {
		free(digi_audiospec);
		digi_audiospec = NULL;
	}
	*/
	digi_buffer = NULL;
	digi_remaining_length = 0;
	digi_remaining_pos = NULL;
	SDL_UnlockAudio();
}

// Decoder for the currently playing OGG sound. (This also holds the playback position.)
stb_vorbis* ogg_decoder;

void stop_ogg(void) {
	SDL_PauseAudio(1);
	if (!ogg_playing) return;
	ogg_playing = 0;
	SDL_LockAudio();
	ogg_decoder = NULL;
	SDL_UnlockAudio();
}

// seg009:7214
void stop_sounds() {
	// stub
	stop_digi();
	stop_midi();
	speaker_sound_stop();
	stop_ogg();
}

short square_wave_state = 4000; // If the amplitude is too high, the speaker sounds will be really loud!
float square_wave_samples_since_last_flip;

void generate_square_wave(byte* stream, float note_freq, int samples) {
	int channels = digi_audiospec->channels;
	float half_period_in_samples = (digi_audiospec->freq / note_freq) * 0.5f;

	int samples_left = samples;
	while (samples_left > 0) {
		if (square_wave_samples_since_last_flip > half_period_in_samples) {
			// Produce a square wave by flipping the signal.
			square_wave_state = ~square_wave_state;
			// Note(Falcury): not completely sure that this is the right way to prevent glitches in the sound...
			// Because I can still hear some hiccups, e.g. in the music. Especially when switching between notes.
			square_wave_samples_since_last_flip -= half_period_in_samples;
		} else {
			int samples_until_next_flip = (int)(half_period_in_samples - square_wave_samples_since_last_flip);
			++samples_until_next_flip; // round up.

			int samples_to_emit = MIN(samples_until_next_flip, samples_left);
			for (int i = 0; i < samples_to_emit * channels; ++i) {
				*(short*)stream = square_wave_state;
				stream += sizeof(short);
			}
			samples_left -= samples_to_emit;
			square_wave_samples_since_last_flip += samples_to_emit;
		}
	}
}

void speaker_callback(void *userdata, Uint8 *stream, int len) {
	int output_channels = digi_audiospec->channels;
	int bytes_per_sample = sizeof(short) * output_channels;
	int samples_requested = len / bytes_per_sample;

	if (current_speaker_sound == NULL) return;
	word tempo = SDL_SwapLE16(current_speaker_sound->tempo);

	int total_samples_left = samples_requested;
	while (total_samples_left > 0) {
		note_type* note = current_speaker_sound->notes + speaker_note_index;
		if (SDL_SwapLE16(note->frequency) == 0x12 /*end*/) {
			speaker_playing = 0;
			current_speaker_sound = NULL;
			speaker_note_index = 0;
			current_speaker_note_samples_already_emitted = 0;
			SDL_Event event;
			memset(&event, 0, sizeof(event));
			event.type = SDL_USEREVENT;
			event.user.code = userevent_SOUND;
			SDL_PushEvent(&event);
			return;
		}

		int note_length_in_samples = (note->length * digi_audiospec->freq) / tempo;
		int note_samples_to_emit = MIN(note_length_in_samples - current_speaker_note_samples_already_emitted, total_samples_left);
		total_samples_left -= note_samples_to_emit;
		size_t copy_len = (size_t)note_samples_to_emit * bytes_per_sample;
		if (SDL_SwapLE16(note->frequency) <= 0x01 /*rest*/) {
			memset(stream, digi_audiospec->silence, copy_len);
		} else {
			generate_square_wave(stream, (float)SDL_SwapLE16(note->frequency), note_samples_to_emit);
		}
		stream += copy_len;

		int note_samples_emitted = current_speaker_note_samples_already_emitted + note_samples_to_emit;
		if (note_samples_emitted < note_length_in_samples) {
			current_speaker_note_samples_already_emitted += note_samples_to_emit;
		} else {
			++speaker_note_index;
			current_speaker_note_samples_already_emitted = 0;
		}
	}
}

// seg009:7640
void play_speaker_sound(sound_buffer_type* buffer) {
	speaker_sound_stop();
	stop_sounds();
	current_speaker_sound = &buffer->speaker;
	speaker_note_index = 0;
	speaker_playing = 1;
	SDL_PauseAudio(0);
}

void digi_callback(void *userdata, Uint8 *stream, int len) {
	// Don't go over the end of either the input or the output buffer.
	size_t copy_len = MIN(len, digi_remaining_length);
	//printf("digi_callback(): copy_len = %d\n", copy_len);
	//printf("digi_callback(): len = %d\n", len);
	if (is_sound_on) {
		// Copy the next part of the input of the output.
		memcpy(stream, digi_remaining_pos, copy_len);
		// In case the sound does not fill the buffer: fill the rest of the buffer with silence.
		memset(stream + copy_len, digi_audiospec->silence, len - copy_len);
	} else {
		// If sound is off: Mute the sound but keep track of where we are.
		memset(stream, digi_audiospec->silence, len);
	}
	// If the sound ended, push an event.
	if (digi_playing && digi_remaining_length == 0) {
		//printf("digi_callback(): sound ended\n");
		SDL_Event event;
		memset(&event, 0, sizeof(event));
		event.type = SDL_USEREVENT;
		event.user.code = userevent_SOUND;
		digi_playing = 0;
		SDL_PushEvent(&event);
	}
	// Advance the pointer.
	digi_remaining_length -= copy_len;
	digi_remaining_pos += copy_len;
}

void ogg_callback(void *userdata, Uint8 *stream, int len) {
	int output_channels = digi_audiospec->channels;
	int bytes_per_sample = sizeof(short) * output_channels;
	int samples_requested = len / bytes_per_sample;

	int samples_filled;
	if (is_sound_on) {
		samples_filled = stb_vorbis_get_samples_short_interleaved(ogg_decoder, output_channels,
                                                                      (short*) stream, len / sizeof(short));
		if (samples_filled < samples_requested) {
			// In case the sound does not fill the buffer: fill the rest of the buffer with silence.
			int bytes_filled = samples_filled * bytes_per_sample;
			int remaining_bytes = (samples_requested - samples_filled) * bytes_per_sample;
			memset(stream + bytes_filled, digi_audiospec->silence, remaining_bytes);
		}
	} else {
		// If sound is off: Mute the sound, but keep track of where we are.
		memset(stream, digi_audiospec->silence, len);
		// Let the decoder run normally (to advance the position), but discard the result.
		byte* discarded_samples = alloca(len);
		samples_filled = stb_vorbis_get_samples_short_interleaved(ogg_decoder, output_channels,
																  (short*) discarded_samples, len / sizeof(short));
	}
	// Push an event if the sound has ended.
	if (samples_filled == 0) {
		//printf("ogg_callback(): sound ended\n");
		SDL_Event event;
		memset(&event, 0, sizeof(event));
		event.type = SDL_USEREVENT;
		event.user.code = userevent_SOUND;
		ogg_playing = 0;
		SDL_PushEvent(&event);
	}
}

#ifdef USE_FAST_FORWARD
int audio_speed = 1; // =1 normally, >1 during fast forwarding
#endif

void audio_callback(void* userdata, Uint8* stream_orig, int len_orig) {

	Uint8* stream;
	int len;
#ifdef USE_FAST_FORWARD
	if (audio_speed > 1) {
		len = len_orig * audio_speed;
		stream = malloc(len);
	} else
#endif
	{
		len = len_orig;
		stream = stream_orig;
	}

	memset(stream, digi_audiospec->silence, len);
	if (digi_playing) {
		digi_callback(userdata, stream, len);
	} else if (speaker_playing) {
		speaker_callback(userdata, stream, len);
	}
	// Note: music sounds and digi sounds are allowed to play simultaneously (will be blended together)
	// I.e., digi sounds and music will not cut each other short.
	if (midi_playing) {
		midi_callback(userdata, stream, len);
	} else if (ogg_playing) {
		ogg_callback(userdata, stream, len);
	}

#ifdef USE_FAST_FORWARD
	if (audio_speed > 1) {

#ifdef FAST_FORWARD_MUTE
		memset(stream_orig, digi_audiospec->silence, len_orig);
#else
#ifdef FAST_FORWARD_RESAMPLE_SOUND
		static SDL_AudioCVT cvt;
		static bool cvt_initialized = false;
		if (!cvt_initialized) {
			SDL_BuildAudioCVT(&cvt,
				digi_audiospec->format, digi_audiospec->channels, digi_audiospec->freq * audio_speed,
				digi_audiospec->format, digi_audiospec->channels, digi_audiospec->freq);
			cvt_initialized = true;
		}
		//realloc(stream, len * cvt.len_mult);
		//cvt.buf = stream;
		cvt.len = len;
		cvt.buf = malloc(cvt.len * cvt.len_mult);
		memcpy(cvt.buf, stream, cvt.len);
		//printf("cvt.needed = %d\n", cvt.needed);
		//printf("cvt.len_mult = %d\n", cvt.len_mult);
		//printf("cvt.len_ratio = %lf\n", cvt.len_ratio);
		SDL_ConvertAudio(&cvt);

		memcpy(stream_orig, cvt.buf, len_orig);
		free(cvt.buf);
		cvt.buf = NULL;
#else
		// Hack: use the beginning of the buffer instead of resampling.
		memcpy(stream_orig, stream, len_orig);
#endif
#endif

		free(stream);
	}
#endif

}

int digi_unavailable = 0;
void init_digi() {
	if (digi_unavailable) return;
	if (digi_audiospec != NULL) return;
	// Open the audio device. Called once.
	//printf("init_digi(): called\n");

	SDL_AudioFormat desired_audioformat;
	SDL_version version;
	SDL_GetVersion(&version);
	//printf("SDL Version = %d.%d.%d\n", version.major, version.minor, version.patch);
	if (version.major <= 2 && version.minor <= 0 && version.patch <= 3) {
		// In versions before 2.0.4, 16-bit audio samples don't work properly (the sound becomes garbled).
		// See: https://bugzilla.libsdl.org/show_bug.cgi?id=2389
		// Workaround: set the audio format to 8-bit, if we are linking against an older SDL2 version.
		desired_audioformat = AUDIO_U8;
		printf("Your SDL.dll is older than 2.0.4. Using 8-bit audio format to work around resampling bug.");
	} else {
		desired_audioformat = AUDIO_S16SYS;
	}

	SDL_AudioSpec *desired;
	desired = (SDL_AudioSpec *)malloc(sizeof(SDL_AudioSpec));
	memset(desired, 0, sizeof(SDL_AudioSpec));
	desired->freq = digi_samplerate; //buffer->digi.sample_rate;
	desired->format = desired_audioformat;
	desired->channels = 2;
	desired->samples = 1024;
	desired->callback = audio_callback;
	desired->userdata = NULL;
    
    // Disable audio for debugging
    printf("init_digi: Audio disabled by user request.\n");
    digi_unavailable = 1;
    return;

	if (SDL_OpenAudio(desired, NULL) != 0) {
		sdlperror("init_digi: SDL_OpenAudio");
		//quit(1);
		digi_unavailable = 1;
		return;
	}
	//SDL_PauseAudio(0);
	digi_audiospec = desired;
}

const int sound_channel = 0;
const int max_sound_id = 58;

void load_sound_names() {
	const char* names_path = locate_file("data/music/names.txt");
	if (sound_names != NULL) return;
	FILE* fp = fopen(names_path,"rt");
	if (fp==NULL) return;
	sound_names = (char**) calloc(sizeof(char*) * max_sound_id, 1);
    char line[512];
	while (fgets(line, sizeof(line), fp)) {
		int index;
		char name[POP_MAX_PATH];
		if (sscanf(line, "%d=%255s", &index, name) != 2) {
			// perror(names_path);
			continue;
		}
		//if (feof(fp)) break;
		//printf("sound_names[%d] = %s\n",index,name);
		if (index >= 0 && index < max_sound_id) {
			sound_names[index] = strdup(name);
		}
	}
	fclose(fp);
}

char* sound_name(int index) {
	if (sound_names != NULL && index >= 0 && index < max_sound_id) {
		return sound_names[index];
	} else {
		return NULL;
	}
}

sound_buffer_type* convert_digi_sound(sound_buffer_type* digi_buffer);

sound_buffer_type* load_sound(int index) {
	sound_buffer_type* result = NULL;
	//printf("load_sound(%d)\n", index);
	init_digi();
	if (enable_music && !digi_unavailable && result == NULL && index >= 0 && index < max_sound_id) {
		//printf("Trying to load from music folder\n");

		//load_sound_names();  // Moved to load_sounds()
		if (sound_names != NULL && sound_name(index) != NULL) {
			//printf("Loading from music folder\n");
			do {
				FILE* fp = NULL;
				char filename[POP_MAX_PATH];
				if (!skip_mod_data_files) {
					// before checking the root directory, first try mods/MODNAME/
					snprintf_check(filename, sizeof(filename), "%s/music/%s.ogg", mod_data_path, sound_name(index));
					fp = fopen(filename, "rb");
				}
				if (fp == NULL && !skip_normal_data_files) {
					snprintf_check(filename, sizeof(filename), "data/music/%s.ogg", sound_name(index));
					fp = fopen(locate_file(filename), "rb");
				}
				if (fp == NULL) {
					break;
				}
				// Read the entire file (undecoded) into memory.
				struct stat info;
				if (fstat(fileno(fp), &info))
					break;
				size_t file_size = (size_t) MAX(0, info.st_size);
				byte* file_contents = malloc(file_size);
				if (fread(file_contents, 1, file_size, fp) != file_size) {
					free(file_contents);
					fclose(fp);
					break;
				}
				fclose(fp);

				// Decoding the entire file immediately would make the loading time much longer.
				// However, we can also create the decoder now, and only use it when we are actually playing the file.
				// (In the audio callback, we'll decode chunks of samples to the output stream, as needed).
				int error = 0;
				stb_vorbis* decoder = stb_vorbis_open_memory(file_contents, (int)file_size, &error, NULL);
				if (decoder == NULL) {
					printf("Error %d when creating decoder from file \"%s\"!\n", error, filename);
					free(file_contents);
					break;
				}
				result = malloc(sizeof(sound_buffer_type));
				result->type = sound_ogg;
				result->ogg.total_length = stb_vorbis_stream_length_in_samples(decoder) * sizeof(short);
				result->ogg.file_contents = file_contents; // Remember in case we want to free the sound later.
				result->ogg.decoder = decoder;
			} while(0); // do once (breakable block)
		} else {
			//printf("sound_names = %p\n", sound_names);
			//printf("sound_names[%d] = %p\n", index, sound_name(index));
		}
	}
	if (result == NULL) {
		//printf("Trying to load from DAT\n");
		result = (sound_buffer_type*) load_from_opendats_alloc(index + 10000, "bin", NULL, NULL);
	}
	if (result != NULL && (result->type & 7) == sound_digi) {
		sound_buffer_type* converted = convert_digi_sound(result);
		psram_free(result);
		result = converted;
	}
	if (result == NULL && !skip_normal_data_files) {
		fprintf(stderr, "Failed to load sound %d '%s'\n", index, sound_name(index));
	}
	return result;
}

void play_ogg_sound(sound_buffer_type *buffer) {
	init_digi();
	if (digi_unavailable) return;
	stop_sounds();

	// Need to rewind the music, or else the decoder might continue where it left off, the last time this sound played.
	stb_vorbis_seek_start(buffer->ogg.decoder);

	SDL_LockAudio();
	ogg_decoder = buffer->ogg.decoder;
	SDL_UnlockAudio();
	SDL_PauseAudio(0);

	ogg_playing = 1;
}

int wave_version = -1;

typedef struct waveinfo_type {
	int sample_rate, sample_size, sample_count;
	byte* samples;
} waveinfo_type;

bool determine_wave_version(sound_buffer_type *buffer, waveinfo_type* waveinfo) {
	int version = wave_version;
	if (version == -1) {
		// Determine the version of the wave data.
		version = 0;
		if (buffer->digi.sample_size == 8) version += 1;
		if (buffer->digi_new.sample_size == 8) version += 2;
		if (version == 1 || version == 2) wave_version = version;
	}

	switch (version) {
		case 1: // 1.0 and 1.1
			waveinfo->sample_rate = SDL_SwapLE16(buffer->digi.sample_rate);
			waveinfo->sample_size = buffer->digi.sample_size;
			waveinfo->sample_count = SDL_SwapLE16(buffer->digi.sample_count);
			waveinfo->samples = buffer->digi.samples;
			return true;
		case 2: // 1.3 and 1.4 (and PoP2)
			waveinfo->sample_rate = SDL_SwapLE16(buffer->digi_new.sample_rate);
			waveinfo->sample_size = buffer->digi_new.sample_size;
			waveinfo->sample_count = SDL_SwapLE16(buffer->digi_new.sample_count);
			waveinfo->samples = buffer->digi_new.samples;
			return true;
		case 3: // ambiguous
			printf("Warning: Ambiguous wave version.\n");
			return false;
		default: // case 0, unknown
			printf("Warning: Can't determine wave version.\n");
			return false;
	}
}

sound_buffer_type* convert_digi_sound(sound_buffer_type* digi_buffer) {
	init_digi();
	if (digi_unavailable) return NULL;
	waveinfo_type waveinfo;
	if (false == determine_wave_version(digi_buffer, &waveinfo)) return NULL;

	float freq_ratio = (float)waveinfo.sample_rate /  (float)digi_audiospec->freq;

	int source_length = waveinfo.sample_count;
	int expanded_frames = source_length * digi_audiospec->freq / waveinfo.sample_rate;
	int expanded_length = expanded_frames * 2 * sizeof(short);
	sound_buffer_type* converted_buffer = psram_malloc(sizeof(sound_buffer_type));
    if (!converted_buffer) return NULL;

	converted_buffer->type = sound_digi_converted;
	converted_buffer->converted.length = expanded_length;

	byte* source = waveinfo.samples;
	//short* dest = converted_buffer->converted.samples;
	short* dest = psram_malloc(converted_buffer->converted.length);
    if (!dest) return NULL;
        converted_buffer->converted.samples = dest;

	for (int i = 0; i < expanded_frames; ++i) {
		float src_frame_float = i * freq_ratio;
		int src_frame_0 = (int) src_frame_float; // truncation

		int sample_0 = (source[src_frame_0] | (source[src_frame_0] << 8)) - 32768;
		short interpolated_sample;
		if (src_frame_0 >= waveinfo.sample_count-1) {
			interpolated_sample = (short)sample_0;
		} else {
			int src_frame_1 = src_frame_0 + 1;
			float alpha = src_frame_float - src_frame_0;
			int sample_1 = (source[src_frame_1] | (source[src_frame_1] << 8)) - 32768;
			interpolated_sample = (short)((1.0f - alpha) * sample_0 + alpha * sample_1);
		}
		for (int channel = 0; channel < digi_audiospec->channels; ++channel) {
			*dest++ = interpolated_sample;
		}
	}

	return converted_buffer;
}

// seg009:74F0
void play_digi_sound(sound_buffer_type* buffer) {
	//if (!is_sound_on) return;
	init_digi();
	if (digi_unavailable) return;
	stop_digi();
//	stop_sounds();
	//printf("play_digi_sound(): called\n");
	if ((buffer->type & 7) != sound_digi_converted) {
		printf("Tried to play unconverted digi sound.\n");
		return;
	}
	SDL_LockAudio();
	digi_buffer = (byte*) buffer->converted.samples;
	digi_playing = 1;
	digi_remaining_length = buffer->converted.length;
	digi_remaining_pos = digi_buffer;
	SDL_UnlockAudio();
	SDL_PauseAudio(0);
}

void free_sound(sound_buffer_type* buffer) {
	if (buffer == NULL) return;
	if (buffer->type == sound_ogg) {
		stb_vorbis_close(buffer->ogg.decoder);
		free(buffer->ogg.file_contents);
	}
	free(buffer);
}

// seg009:7220
void play_sound_from_buffer(sound_buffer_type* buffer) {
    return; // Disable sound playback

#ifdef USE_REPLAY
	if (replaying && skipping_replay) return;
#endif

	// stub
	if (buffer == NULL) {
		printf("Tried to play NULL sound.\n");
		//quit(1);
		return;
	}
	switch (buffer->type & 7) {
		case sound_speaker:
			play_speaker_sound(buffer);
		break;
		case sound_digi_converted:
		case sound_digi:
			play_digi_sound(buffer);
		break;
		case sound_midi:
			play_midi_sound(buffer);
		break;
		case sound_ogg:
			play_ogg_sound(buffer);
		break;
		default:
			printf("Tried to play unimplemented sound type %d.\n", buffer->type);
			quit(1);
		break;
	}
}

void turn_music_on_off(byte new_state) {
	enable_music = new_state;
	turn_sound_on_off(is_sound_on);
}

// seg009:7273
void turn_sound_on_off(byte new_state) {
	// stub
	is_sound_on = new_state;
	//if (!is_sound_on) stop_sounds();
}

// seg009:7299
int check_sound_playing() {
	return speaker_playing || digi_playing || midi_playing || ogg_playing;
}

void apply_aspect_ratio() {
	// Allow us to use a consistent set of screen co-ordinates, even if the screen size changes
	if (use_correct_aspect_ratio) {
		SDL_RenderSetLogicalSize(renderer_, 320 * 5, 200 * 6); // 4:3
	} else {
		SDL_RenderSetLogicalSize(renderer_, 320, 200); // 16:10
	}
	window_resized();
}

void window_resized() {
#if SDL_VERSION_ATLEAST(2,0,5) // SDL_RenderSetIntegerScale
	if (use_integer_scaling) {
		int window_width, window_height;
		// On high-DPI screens, this is what we need instead of SDL_GetWindowSize().
		//SDL_GL_GetDrawableSize(window_, &window_width, &window_height);
		SDL_GetRendererOutputSize(renderer_, &window_width, &window_height);
		int render_width, render_height;
		SDL_RenderGetLogicalSize(renderer_, &render_width, &render_height);
		// Disable integer scaling if it would result in downscaling.
		// Because then the only suitable integer scaling factor is zero, i.e. the picture disappears.
		SDL_bool makes_sense = (window_width >= render_width && window_height >= render_height);
		SDL_RenderSetIntegerScale(renderer_, makes_sense);
	}
#endif
}

void init_overlay(void) {
	static bool initialized = false;
	if (!initialized) {
#ifdef USE_MENU
		overlay_surface = SDL_CreateRGBSurface(0, 320, 200, 32, Rmsk, Gmsk, Bmsk, Amsk);
		merged_surface = SDL_CreateRGBSurface(0, 320, 200, 32, Rmsk, Gmsk, Bmsk, 0);
#endif
	
		initialized = true;
	}
}

SDL_Surface* onscreen_surface_2x;

void init_scaling(void) {
	// Don't crash in validate mode.
	if (renderer_ == NULL) return;

	if (texture_sharp == NULL) {
		texture_sharp = SDL_CreateTexture(renderer_, SDL_PIXELFORMAT_RGB565, SDL_TEXTUREACCESS_STREAMING, 320, 200);
	}
	if (scaling_type == 1) {
		if (!is_renderer_targettexture_supported && onscreen_surface_2x == NULL) {
#ifdef __amigaos4__
			overlay_surface = SDL_CreateRGBSurface(0, 320*2, 200*2, 32, 0x00FF0000, 0x0000FF00, 0x000000FF, 0xFF000000);
#else
			onscreen_surface_2x = SDL_CreateRGBSurface(0, 320*2, 200*2, 32, 0x00FF0000, 0x0000FF00, 0x000000FF, 0xFF000000);
#endif
		}
		if (texture_fuzzy == NULL) {
			SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "1");
			int access = is_renderer_targettexture_supported ? SDL_TEXTUREACCESS_TARGET : SDL_TEXTUREACCESS_STREAMING;
			texture_fuzzy = SDL_CreateTexture(renderer_, SDL_PIXELFORMAT_RGB565, access, 320*2, 200*2);
			SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "0");
		}
		target_texture = texture_fuzzy;
	} else if (scaling_type == 2) {
		if (texture_blurry == NULL) {
			SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "1");
			texture_blurry = SDL_CreateTexture(renderer_, SDL_PIXELFORMAT_RGB565, SDL_TEXTUREACCESS_STREAMING, 320, 200);
			SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "0");
		}
		target_texture = texture_blurry;
	} else {
		target_texture = texture_sharp;
	}
	if (target_texture == NULL) {
		sdlperror("init_scaling: SDL_CreateTexture");
		quit(1);
	}
}

// seg009:38ED
void set_gr_mode(byte grmode) {
#ifdef SDL_HINT_WINDOWS_DISABLE_THREAD_NAMING
	SDL_SetHint(SDL_HINT_WINDOWS_DISABLE_THREAD_NAMING, "1");
#endif
	if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER | SDL_INIT_NOPARACHUTE | SDL_INIT_GAMECONTROLLER) != 0) {
		sdlperror("set_gr_mode: SDL_Init");
		quit(1);
	}
	if (enable_controller_rumble) {
		if (SDL_InitSubSystem(SDL_INIT_HAPTIC) != 0) {
			printf("Warning: Haptic subsystem unavailable, ignoring enable_controller_rumble = true\n");
		}
	}

	//SDL_EnableUNICODE(1); //deprecated
	Uint32 flags = 0;
	if (!start_fullscreen) start_fullscreen = check_param("full") != NULL;
	if (start_fullscreen) flags |= SDL_WINDOW_FULLSCREEN_DESKTOP;
	flags |= SDL_WINDOW_RESIZABLE;
	flags |= SDL_WINDOW_ALLOW_HIGHDPI; // for Retina displays

	// Should use different default window dimensions when using 4:3 aspect ratio
	if (use_correct_aspect_ratio && pop_window_width == 640 && pop_window_height == 400) {
		pop_window_height = 480;
	}

#if _WIN32
	// Tell Windows that the application is DPI aware, to prevent unwanted bitmap stretching.
	// SetProcessDPIAware() is only available on Windows Vista and later, so we need to load it dynamically.
	typedef BOOL (WINAPI *dpiaware)(void);
	HMODULE user32dll = LoadLibraryA("User32.dll");
	if (user32dll) {
		dpiaware SetProcessDPIAware = (dpiaware)GetProcAddress(user32dll, "SetProcessDPIAware");
		if (SetProcessDPIAware) {
			SetProcessDPIAware();
		}
		FreeLibrary(user32dll);
	}
#endif

#ifdef USE_REPLAY
	if (!is_validate_mode) // run without a window if validating a replay
#endif
	window_ = SDL_CreateWindow(WINDOW_TITLE,
	                           SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
	                           pop_window_width, pop_window_height, flags);
	// Make absolutely sure that VSync will be off, to prevent timer issues.
	SDL_SetHint(SDL_HINT_RENDER_VSYNC, "0");
	flags = 0;
	switch (use_hardware_acceleration) {
		case 0:  flags |= SDL_RENDERER_SOFTWARE;    break;
		case 1:  flags |= SDL_RENDERER_ACCELERATED; break;
		case 2:  // let SDL decide
		         // fallthrough!
		default: break;
	}
	renderer_ = SDL_CreateRenderer(window_, -1 , flags | SDL_RENDERER_TARGETTEXTURE);
	SDL_RendererInfo renderer_info;
	if (SDL_GetRendererInfo(renderer_, &renderer_info) == 0) {
		if (renderer_info.flags & SDL_RENDERER_TARGETTEXTURE) {
			is_renderer_targettexture_supported = true;
		}
	}
	if (use_integer_scaling) {
#if SDL_VERSION_ATLEAST(2,0,5) // SDL_RenderSetIntegerScale
		SDL_RenderSetIntegerScale(renderer_, SDL_TRUE);
#else
		printf("Warning: You need to compile with SDL 2.0.5 or newer for the use_integer_scaling option.\n");
#endif
	}

	SDL_Surface* icon = IMG_Load(locate_file("data/icon.png"));
    /*
	if (icon == NULL) {
		sdlperror("set_gr_mode: Could not load icon");
	} else {
		SDL_SetWindowIcon(window_, icon);
	}
    */
    if (icon) SDL_FreeSurface(icon);

	apply_aspect_ratio();
	window_resized();

	/* Migration to SDL2: everything is still blitted to onscreen_surface_, however:
	 * SDL2 renders textures to the screen instead of surfaces; so, every screen
	 * update causes the onscreen_surface_ to be copied into the target_texture, which is
	 * subsequently displayed.
	 * The function handling the screen updates is update_screen()
	 * */
	onscreen_surface_ = SDL_CreateRGBSurface(0, 320, 200, 8, 0, 0, 0, 0);
	if (onscreen_surface_ == NULL) {
		sdlperror("set_gr_mode: SDL_CreateRGBSurface");
		quit(1);
	}
	// Clear onscreen surface to prevent title screen artifacts
	if (SDL_LockSurface(onscreen_surface_) == 0) {
		memset(onscreen_surface_->pixels, 0, onscreen_surface_->pitch * onscreen_surface_->h);
		SDL_UnlockSurface(onscreen_surface_);
	}
    // Initialize palette with full 250 color range (HDMI limit)
    SDL_Color colors[256];
    for(int i=0; i<250; ++i) {
        // Create a varied palette covering RGB space
        colors[i].r = ((i & 0x07) << 5) | ((i & 0x07) << 2);
        colors[i].g = (((i >> 3) & 0x07) << 5) | (((i >> 3) & 0x07) << 2);
        colors[i].b = (((i >> 6) & 0x03) << 6) | (((i >> 6) & 0x03) << 4);
        colors[i].a = 255;
    }
    // Last 6 entries black
    for(int i=250; i<256; ++i) {
        colors[i].r = colors[i].g = colors[i].b = 0;
        colors[i].a = 255;
    }
    SDL_SetPaletteColors(onscreen_surface_->format->palette, colors, 0, 256);

    scaling_type = 0; // Force no scaling to avoid complexity
	init_overlay();
	init_scaling();
	if (start_fullscreen) {
		SDL_ShowCursor(SDL_DISABLE);
	}


	//SDL_WM_SetCaption(WINDOW_TITLE, NULL);
//	if (SDL_EnableKeyRepeat(SDL_DEFAULT_REPEAT_DELAY, SDL_DEFAULT_REPEAT_INTERVAL) != 0) {  //deprecated
//		sdlperror("set_gr_mode: SDL_EnableKeyRepeat");
//		quit(1);
//	}
	graphics_mode = gmMcgaVga;
#ifdef USE_TEXT
	load_font();
#endif
    // debug_blink(18); // Blink 18: End of set_gr_mode
}

SDL_Surface* get_final_surface() {
	if (!is_overlay_displayed) {
		return onscreen_surface_;
	} else {
		return merged_surface;
	}
}

void draw_overlay(void) {
#ifdef USE_MENU
	int overlay = 0;
	is_overlay_displayed = false;
#ifdef USE_DEBUG_CHEATS
	if (is_timer_displayed && start_level > 0) overlay = 1; // Timer overlay
	else if (fixes->fix_quicksave_during_feather &&
				is_feather_timer_displayed &&
				start_level > 0 &&
				is_feather_fall > 0) {
		overlay = 3; // Feather timer overlay
	}
#endif
#ifdef USE_MENU
	// Menu overlay - not drawn here directly, only copied from the overlay surface.
	if (is_paused && is_menu_shown) overlay = 2;
#endif
	if (overlay != 0) {
		is_overlay_displayed = true;
		surface_type* saved_target_surface = current_target_surface;
		current_target_surface = overlay_surface;
		rect_type drawn_rect;
		if (overlay == 1) {
#ifdef USE_DEBUG_CHEATS
			char timer_text[32];
			if (rem_min < 0) {
				snprintf(timer_text, sizeof(timer_text), "%02d:%02d:%02d",
				         -(rem_min + 1), (719 - rem_tick) / 12, (719 - rem_tick) % 12);
			} else {
				snprintf(timer_text, sizeof(timer_text), "%02d:%02d:%02d",
				         rem_min - 1, rem_tick / 12, rem_tick % 12);
			}
			int expected_numeric_chars = 6;
			int extra_numeric_chars = MAX(0, (int)strnlen(timer_text, sizeof(timer_text)) - 8);
			int line_width = 5 + (expected_numeric_chars + extra_numeric_chars) * 9;

			rect_type timer_box_rect = {0, 0, 11, 2 + line_width};
			rect_type timer_text_rect = {2, 2, 10, 100};
			draw_rect_with_alpha(&timer_box_rect, color_0_black, 128);
			show_text(&timer_text_rect, halign_left, valign_top, timer_text);

#ifdef USE_REPLAY
			// During playback, display the number of ticks since start, if the timer is shown (debug cheats: T).
			if (replaying) {
				char ticks_text[12];
				snprintf(ticks_text, sizeof(ticks_text), "T: %d", curr_tick);
				rect_type ticks_box_rect = timer_box_rect;
				ticks_box_rect.top += 12;
				ticks_box_rect.bottom += 12;
				rect_type ticks_text_rect = timer_text_rect;
				ticks_text_rect.top += 12;
				ticks_text_rect.bottom += 12;

				draw_rect_with_alpha(&ticks_box_rect, color_0_black, 128);
				show_text(&ticks_text_rect, halign_left, valign_top, ticks_text);

				timer_box_rect.bottom += 12;
			}
#endif

			drawn_rect = timer_box_rect; // Only need to blit this bit to the merged_surface.
#endif
		} else if (overlay == 3) { // Feather timer
#ifdef USE_DEBUG_CHEATS
			char timer_text[32];
			int ticks_per_sec = get_ticks_per_sec(timer_1);
			snprintf(timer_text, sizeof(timer_text), "%02d:%02d", is_feather_fall / ticks_per_sec, is_feather_fall % ticks_per_sec);
			int expected_numeric_chars = 6;
			int extra_numeric_chars = MAX(0, (int)strnlen(timer_text, sizeof(timer_text)) - 8);
			int line_width = 5 + (expected_numeric_chars + extra_numeric_chars) * 9;

			rect_type timer_box_rect = {0, 0, 11, 2 + line_width};
			rect_type timer_text_rect = {2, 2, 10, 100};
			draw_rect_with_alpha(&timer_box_rect, color_0_black, 128);
			show_text_with_color(&timer_text_rect, halign_left, valign_top, timer_text, color_10_brightgreen);

			drawn_rect = timer_box_rect; // Only need to blit this bit to the merged_surface.
#endif
		} else {
			drawn_rect = screen_rect; // We'll blit the whole contents of overlay_surface to the merged_surface.
		}
		SDL_Rect sdl_rect;
		rect_to_sdlrect(&drawn_rect, &sdl_rect);
		SDL_BlitSurface(onscreen_surface_, NULL, merged_surface, NULL);
		SDL_BlitSurface(overlay_surface, &sdl_rect, merged_surface, &sdl_rect);
		current_target_surface = saved_target_surface;
	}
#endif
}

void update_screen() {
	draw_overlay();
	SDL_Surface* surface = get_final_surface();
	init_scaling();
	if (scaling_type == 1) {
		// Make "fuzzy pixels" like DOSBox does:
		// First scale to double size with nearest-neighbor scaling, then scale to full screen with smooth scaling.
		// The result is not as blurry as if we did only a smooth scaling, but not as sharp as if we did only nearest-neighbor scaling.
		if (is_renderer_targettexture_supported) {
			SDL_UpdateTexture(texture_sharp, NULL, surface->pixels, surface->pitch);
			SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "1");
			SDL_SetRenderTarget(renderer_, target_texture);
			SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "0");
			SDL_RenderClear(renderer_);
			SDL_RenderCopy(renderer_, texture_sharp, NULL, NULL);
			SDL_SetRenderTarget(renderer_, NULL);
		} else {
			SDL_BlitScaled(surface, NULL, onscreen_surface_2x, NULL);
			surface = onscreen_surface_2x;
			SDL_UpdateTexture(target_texture, NULL, surface->pixels, surface->pitch);
		}
	} else {
			SDL_UpdateTexture(target_texture, NULL, surface->pixels, surface->pitch);
	}
	SDL_RenderClear(renderer_);
	SDL_RenderCopy(renderer_, target_texture, NULL, NULL);
	SDL_RenderPresent(renderer_);
}

// seg009:9289
void set_pal_arr(int start,int count,const rgb_type* array) {
	// stub
    SDL_Color colors[256];
	for (int i = 0; i < count; ++i) {
		if (array) {
			set_pal(start + i, array[i].r, array[i].g, array[i].b);
            colors[i].r = array[i].r << 2;
            colors[i].g = array[i].g << 2;
            colors[i].b = array[i].b << 2;
            colors[i].a = 255;
		} else {
			set_pal(start + i, 0, 0, 0);
            colors[i].r = 0;
            colors[i].g = 0;
            colors[i].b = 0;
            colors[i].a = 255;
		}
	}
    if (onscreen_surface_ && onscreen_surface_->format->palette) {
        SDL_SetPaletteColors(onscreen_surface_->format->palette, colors, start, count);
    }

    // Update chtab palettes if they share the global palette range
    for (int i = 0; i < 10; ++i) {
        chtab_type* chtab = chtab_addrs[i];
        if (chtab && chtab->extra) {
            // Update shared_palette directly with the full range
            if (chtab->extra->shared_palette) {
                 SDL_SetPaletteColors(chtab->extra->shared_palette, colors, start, count);
            } else {
                // Fallback for individual images
                for (int img_idx = 0; img_idx < chtab->n_images; ++img_idx) {
                    if (chtab->images[img_idx]) {
                        SDL_SetPaletteColors(chtab->images[img_idx]->format->palette, colors, start, count);
                    }
                }
            }

            // Update palette_copy (keep respecting row_bits as it tracks the sprite's *defined* palette)
            for (int row = 0; row < 16; ++row) {
                if (chtab->extra->palette_copy.row_bits & (1 << row)) {
                    int row_start = row * 16;
                    int row_end = row_start + 16;
                    
                    int overlap_start = (start > row_start) ? start : row_start;
                    int overlap_end = ((start + count) < row_end) ? (start + count) : row_end;
                    
                    if (overlap_start < overlap_end) {
                        int num_update = overlap_end - overlap_start;
                        
                        // Update shared_palette
                        if (chtab->extra->shared_palette) {
                             SDL_Color update_colors[16];
                             for(int k=0; k<num_update; ++k) {
                                 int global_idx = overlap_start + k;
                                 int colors_idx = overlap_start + k - start;
                                 update_colors[k] = colors[colors_idx];
                                 // Ensure index 0 is transparent for the local mapping
                                 // For the global mapping, we keep alpha as is (usually 255)
                                 update_colors[k].a = 255; 
                             }
                             
                             // 1. Update the sprite's local palette (indices 0-15)
                             // This is what the sprite pixels (0-15) actually reference.
                             // We must set alpha=0 for the first color (transparent).
                             SDL_Color local_colors[16];
                             for(int k=0; k<num_update; ++k) {
                                 local_colors[k] = update_colors[k];
                                 if ((overlap_start - row_start + k) == 0) {
                                     local_colors[k].a = 0;
                                 }
                             }
                             SDL_SetPaletteColors(chtab->extra->shared_palette, local_colors, overlap_start - row_start, num_update);

                             // 2. Update the global range in shared_palette (e.g. 128-143)
                             // This ensures that if the sprite somehow references global indices, they are also correct.
                             SDL_SetPaletteColors(chtab->extra->shared_palette, update_colors, overlap_start, num_update);
                        } else {
                            // Fallback for individual images (no shared palette)
                            for (int img_idx = 0; img_idx < chtab->n_images; ++img_idx) {
                                if (chtab->images[img_idx]) {
                                    SDL_SetPaletteColors(chtab->images[img_idx]->format->palette, colors, start, count); // This updates global range?
                                    // Wait, if individual images use 0-15, we need to update 0-15!
                                    // But SDL_SetPaletteColors takes 'colors' which is the source array.
                                    // And 'start' is the destination index.
                                    // If we pass 'start' (e.g. 128), we update 128.
                                    // We need to update 0!
                                    
                                    // Correct logic for individual images:
                                    SDL_Color local_colors[16];
                                    for(int k=0; k<num_update; ++k) {
                                        int colors_idx = overlap_start + k - start;
                                        local_colors[k] = colors[colors_idx];
                                        if ((overlap_start - row_start + k) == 0) local_colors[k].a = 0;
                                        else local_colors[k].a = 255;
                                    }
                                    SDL_SetPaletteColors(chtab->images[img_idx]->format->palette, local_colors, overlap_start - row_start, num_update);
                                }
                            }
                        }

                        // Update palette_copy
                         for (int k = 0; k < num_update; ++k) {
                             int idx = overlap_start - row_start + k;
                             int colors_idx = overlap_start + k - start;
                             if (idx >= 0 && idx < 16) {
                                 chtab->extra->palette_copy.vga[idx].r = colors[colors_idx].r >> 2;
                                 chtab->extra->palette_copy.vga[idx].g = colors[colors_idx].g >> 2;
                                 chtab->extra->palette_copy.vga[idx].b = colors[colors_idx].b >> 2;
                             }
                         }
                    }
                }
            }
        }
    }
}

rgb_type palette[256];

// seg009:92DF
void set_pal(int index,int red,int green,int blue) {
	// stub
	//palette[index] = ((red&0x3F)<<2)|((green&0x3F)<<2<<8)|((blue&0x3F)<<2<<16);
	palette[index].r = red;
	palette[index].g = green;
	palette[index].b = blue;
    
    if (onscreen_surface_ && onscreen_surface_->format->palette) {
        SDL_Color color;
        color.r = red << 2;
        color.g = green << 2;
        color.b = blue << 2;
        color.a = 255;
        SDL_SetPaletteColors(onscreen_surface_->format->palette, &color, index, 1);
    }
}

// seg009:969C
int add_palette_bits(byte n_colors) {
	// stub
	return 0;
}

// seg009:9C36
int find_first_pal_row(int which_rows_mask) {
	word which_row = 0;
	word row_mask = 1;
	do {
		if (row_mask & which_rows_mask) {
			return which_row;
		}
		++which_row;
		row_mask <<= 1;
	} while (which_row < 16);
	return 0;
}

// seg009:9C6C
int get_text_color(int cga_color,int low_half,int high_half_mask) {
	if (graphics_mode == gmCga || graphics_mode == gmHgaHerc) {
		return cga_color;
	} else if (graphics_mode == gmMcgaVga && high_half_mask != 0) {
		return (find_first_pal_row(high_half_mask) << 4) + low_half;
	} else {
		return low_half;
	}
}

void load_from_opendats_metadata(int resource_id, const char* extension, FILE** out_fp, data_location* result, byte* checksum, int* size, dat_type** out_pointer) {
	char image_filename[POP_MAX_PATH];
	FILE* fp = NULL;
	*result = data_none;
	// Go through all open DAT files.
	for (dat_type* pointer = dat_chain_ptr; fp == NULL && pointer != NULL; pointer = pointer->next_dat) {
		*out_pointer = pointer;
		if (pointer->handle != NULL) {
			// If it's an actual DAT file:
			fp = pointer->handle;
			dat_table_type* dat_table = pointer->dat_table;
			int i;
			for (i = 0; i < SDL_SwapLE16(dat_table->res_count); ++i) {
				if (SDL_SwapLE16(dat_table->entries[i].id) == resource_id) {
					break;
				}
			}
			if (i < SDL_SwapLE16(dat_table->res_count)) {
				// found
				*result = data_DAT;
				*size = SDL_SwapLE16(dat_table->entries[i].size);
				if (strcmp(extension,"png") == 0 && *size <= 2) {
					// Skip empty images in DATs, so we can fall back to directories.
					// This is useful for teleport graphics for example.
					fp = NULL;
					*result = data_none;
					*size = 0;
				} else
				if (fseek(fp, SDL_SwapLE32(dat_table->entries[i].offset), SEEK_SET) ||
				    fread(checksum, 1, 1, fp) != 1
				) {
					printf("Cannot seek or cannot read checksum: ");
					perror(pointer->filename);
					fp = NULL;
				}
			} else {
				// not found
				fp = NULL;
			}
		}
		// If the image is not in the DAT then try the directory as well.
		if (*result == data_none) {
			// If it's a directory:
			char filename_no_ext[POP_MAX_PATH];
			// strip the .DAT file extension from the filename (use folders simply named TITLE, KID, VPALACE, etc.)
			strncpy(filename_no_ext, pointer->filename, sizeof(filename_no_ext));
			size_t len = strlen(filename_no_ext);
			if (len >= 5 && filename_no_ext[len-4] == '.') {
				filename_no_ext[len-4] = '\0'; // terminate, so ".DAT" is deleted from the filename
			}
			snprintf_check(image_filename,sizeof(image_filename),"data/%s/res%d.%s",filename_no_ext, resource_id, extension);
			if (!use_custom_levelset) {
				//printf("loading (binary) %s",image_filename);
				fp = fopen(locate_file(image_filename), "rb");
			}
			else {
				if (!skip_mod_data_files) {
					char image_filename_mod[POP_MAX_PATH];
					// before checking data/, first try mods/MODNAME/data/
					snprintf_check(image_filename_mod, sizeof(image_filename_mod), "%s/%s", mod_data_path, image_filename);
					//printf("loading (binary) %s",image_filename_mod);
					fp = fopen(locate_file(image_filename_mod), "rb");
				}
							if (fp == NULL && !skip_normal_data_files) {
					fp = fopen(locate_file(image_filename), "rb");
				}
			}

			if (fp != NULL) {
				struct stat buf;
				if (fstat(fileno(fp), &buf) == 0) {
					*result = data_directory;
					*size = (int)buf.st_size;
				} else {
					printf("Cannot fstat: ");
					perror(image_filename);
					fclose(fp);
					fp = NULL;
				}
			}
		}
	}
	*out_fp = fp;
	if (fp == NULL) {
		*result = data_none;
//		printf(" FAILED\n");
		//return NULL;
	}
	//...
}

// seg009:9F34
void close_dat(dat_type* pointer) {
	dat_type** prev = &dat_chain_ptr;
	dat_type* curr = dat_chain_ptr;
	while (curr != NULL) {
		if (curr == pointer) {
			*prev = curr->next_dat;
			if (curr->handle) fclose(curr->handle);
			if (curr->dat_table) free(curr->dat_table);
			free(curr);
			return;
		}
		curr = curr->next_dat;
		prev = &((*prev)->next_dat);
	}
	// stub
}

// seg009:9F80
void *load_from_opendats_alloc(int resource, const char* extension, data_location* out_result, int* out_size) {
	// stub
	//printf("id = %d\n",resource);
	dat_type* pointer;
	data_location result;
	byte checksum;
	int size;
	FILE* fp = NULL;
	load_from_opendats_metadata(resource, extension, &fp, &result, &checksum, &size, &pointer);
	if (out_result != NULL) *out_result = result;
	if (out_size != NULL) *out_size = size;
	if (result == data_none) return NULL;
	
    void* area = NULL;
    // Use PSRAM file buffer for PNG/DAT resources (temporary)
    if (strcmp(extension, "png") == 0 || strcmp(extension, "DAT") == 0) {
        area = psram_get_file_buffer(size);
        if (!area) {
            // Fallback to psram_malloc if too big (unlikely)
            area = psram_malloc(size);
        }
    } else {
        // Use PSRAM malloc for persistent data (palettes, sounds)
        area = psram_malloc(size);
        if (strcmp(extension, "bin") == 0) {
            // printf("Allocated sound/bin resource %d: %d bytes at %p\n", resource, size, area);
        }
    }

    if (area == NULL) {
        fprintf(stderr, "%s: Out of memory (PSRAM) for resource %d\n", __func__, resource);
        if (result == data_directory) fclose(fp);
        return NULL;
    }

    // printf("load_from_opendats_alloc: reading %d bytes to %p from fp %p\n", size, area, fp);
	//read(fd, area, size);
	if (fread(area, size, 1, fp) != 1) {
		fprintf(stderr, "%s: %s, resource %d, size %d, failed: %s\n",
			__func__, pointer->filename, resource,
			size, strerror(errno));
        // Only free if it was malloc'd (not file buffer)
        // But psram_free is no-op, so it's safe.
		// free(area); 
		area = NULL;
	}
    // printf("load_from_opendats_alloc: read complete\n");
	if (result == data_directory) fclose(fp);
	/* XXX: check checksum */
	return area;
}

// seg009:A172
int load_from_opendats_to_area(int resource,void* area,int length, const char* extension) {
	// stub
	//return 0;
	dat_type* pointer;
	data_location result;
	byte checksum;
	int size;
	FILE* fp = NULL;
	load_from_opendats_metadata(resource, extension, &fp, &result, &checksum, &size, &pointer);
	if (result == data_none) return 0;
	if (fread(area, MIN(size, length), 1, fp) != 1) {
		fprintf(stderr, "%s: %s, resource %d, size %d, failed: %s\n",
			__func__, pointer->filename, resource,
			size, strerror(errno));
		memset(area, 0, MIN(size, length));
	}
	if (result == data_directory) fclose(fp);
	/* XXX: check checksum */
	return 0;
}

// SDL-specific implementations

void rect_to_sdlrect(const rect_type* rect, SDL_Rect* sdlrect) {
	sdlrect->x = rect->left;
	sdlrect->y = rect->top;
	sdlrect->w = rect->right - rect->left;
	sdlrect->h = rect->bottom - rect->top;
}

void method_1_blit_rect(surface_type* target_surface,surface_type* source_surface,const rect_type* target_rect, const rect_type* source_rect,int blit) {
	SDL_Rect src_rect;
	rect_to_sdlrect(source_rect, &src_rect);
	SDL_Rect dest_rect;
	rect_to_sdlrect(target_rect, &dest_rect);

	if (blit == blitters_0_no_transp) {
		// Disable transparency.
		if (SDL_SetColorKey(source_surface, 0, 0) != 0) {
			sdlperror("method_1_blit_rect: SDL_SetColorKey");
			quit(1);
		}
	} else {
		// Enable transparency.
		if (SDL_SetColorKey(source_surface, SDL_TRUE, 0) != 0) {
			sdlperror("method_1_blit_rect: SDL_SetColorKey");
			quit(1);
		}
	}
	if (SDL_BlitSurface(source_surface, &src_rect, target_surface, &dest_rect) != 0) {
		sdlperror("method_1_blit_rect: SDL_BlitSurface");
		quit(1);
	}
}

image_type* method_3_blit_mono(image_type* image,int xpos,int ypos,int blitter,byte color) {
	//printf("method_3_blit_mono: start. image=%p, x=%d, y=%d\n", image, xpos, ypos);
    //fflush(stdout);
	int w = image->w;
	int h = image->h;
	if (SDL_SetColorKey(image, SDL_TRUE, 0) != 0) {
		sdlperror("method_3_blit_mono: SDL_SetColorKey");
		quit(1);
	}
    //printf("method_3_blit_mono: SDL_SetColorKey done\n");
    //fflush(stdout);

    // Save temp offset to allow nested allocations (e.g. inside a peel)
    size_t saved_offset = psram_get_temp_offset();

	// RP2350 Port: Handle 8-bit target surface
	if (current_target_surface->format->BitsPerPixel == 8) {
        // Use temp allocator for this short-lived surface
        psram_set_temp_mode(1);
		// Create a temporary 8-bit surface
		SDL_Surface* colored_image = SDL_CreateRGBSurface(0, w, h, 8, 0, 0, 0, 0);
        psram_set_temp_mode(0);

		if (!colored_image) {
			sdlperror("method_3_blit_mono: SDL_CreateRGBSurface");
            psram_set_temp_offset(saved_offset);
			quit(1);
		}

        // RP2350 Fix: Copy palette to ensure no remapping occurs (or maps 1:1)
        if (current_target_surface->format->palette) {
            SDL_SetPaletteColors(colored_image->format->palette, 
                                 current_target_surface->format->palette->colors, 
                                 0, current_target_surface->format->palette->ncolors);
        }
		
		// Copy pixels and apply color
		if (image->format->BitsPerPixel == 8) {
			for (int y = 0; y < h; y++) {
				Uint8* src_row = (Uint8*)image->pixels + y * image->pitch;
				Uint8* dst_row = (Uint8*)colored_image->pixels + y * colored_image->pitch;
				for (int x = 0; x < w; x++) {
					if (src_row[x] != 0) // Assuming 0 is transparent
						dst_row[x] = color;
					else
						dst_row[x] = 0;
				}
			}
		} else if (image->format->BitsPerPixel == 1) {
            // 1-bit bitmap (Fonts often use this)
            // Assuming packed bits? Or 1 byte per pixel?
            // SDL 1-bit surfaces usually pack 8 pixels per byte.
            // But let's check if SDLPoP uses 1 byte per pixel for "1-bit" (unlikely).
            // Actually, SDL_ConvertSurface might be better here, but let's try to handle it manually.
            // If it's standard SDL 1-bit:
            for (int y = 0; y < h; y++) {
                Uint8* src_row = (Uint8*)image->pixels + y * image->pitch;
                Uint8* dst_row = (Uint8*)colored_image->pixels + y * colored_image->pitch;
                for (int x = 0; x < w; x++) {
                    // Extract bit
                    Uint8 byte = src_row[x / 8];
                    Uint8 bit = (byte >> (7 - (x % 8))) & 1;
                    if (bit)
                        dst_row[x] = color;
                    else
                        dst_row[x] = 0;
                }
            }
        } else if (image->format->BitsPerPixel == 32 || image->format->BitsPerPixel == 24) {
            // 32/24-bit (PNG loaded fonts)
            // Check alpha or colorkey
            int bpp = image->format->BytesPerPixel;
            for (int y = 0; y < h; y++) {
                Uint8* src_row = (Uint8*)image->pixels + y * image->pitch;
                Uint8* dst_row = (Uint8*)colored_image->pixels + y * colored_image->pitch;
                for (int x = 0; x < w; x++) {
                    Uint8 r = src_row[x * bpp];
                    Uint8 g = src_row[x * bpp + 1];
                    Uint8 b = src_row[x * bpp + 2];
                    Uint8 a = (bpp == 4) ? src_row[x * bpp + 3] : 255;
                    
                    bool transparent = false;
                    if (bpp == 4 && a < 128) transparent = true;
                    // Check colorkey (black is usually transparent for fonts)
                    if (r == 0 && g == 0 && b == 0) transparent = true;
                    
                    if (!transparent) {
                        dst_row[x] = color;
                    } else {
                        dst_row[x] = 0;
                    }
                }
            }
        } else {
			// Fallback: just fill with color (ignoring transparency for now if not 8-bit)
			// Ideally we should convert, but let's assume 8-bit for now.
			memset(colored_image->pixels, color, colored_image->pitch * h);
		}
		
		SDL_SetColorKey(colored_image, SDL_TRUE, 0);
		
		SDL_Rect src_rect = {0, 0, w, h};
		SDL_Rect dest_rect = {xpos, ypos, w, h};
		
        // printf("method_3_blit_mono: calling SDL_BlitSurface to %p\n", current_target_surface);
        fflush(stdout);
		if (SDL_BlitSurface(colored_image, &src_rect, current_target_surface, &dest_rect) != 0) {
			sdlperror("method_3_blit_mono: SDL_BlitSurface");
            psram_set_temp_offset(saved_offset);
			quit(1);
		}
		// SDL_FreeSurface(colored_image); // No-op in PSRAM
        psram_set_temp_offset(saved_offset); // Free the temp surface
		return image;
	}

    // Use temp allocator for this short-lived surface
    psram_set_temp_mode(1);
    // Create 32-bit surface (ARGB8888) manually to ensure PSRAM usage and correct alpha
    SDL_Surface* colored_image = SDL_CreateRGBSurface(0, w, h, 32, 0x00FF0000, 0x0000FF00, 0x000000FF, 0xFF000000);
    psram_set_temp_mode(0);
    
    if (!colored_image) {
        sdlperror("method_3_blit_mono: SDL_CreateRGBSurface (32-bit)");
        psram_set_temp_offset(saved_offset);
        quit(1);
    }

    // Prepare target color
	rgb_type palette_color = palette[color];
    uint8_t r = palette_color.r << 2;
    uint8_t g = palette_color.g << 2;
    uint8_t b = palette_color.b << 2;
    uint32_t color_val = (255 << 24) | (r << 16) | (g << 8) | b;

    // Fill pixels
    if (image->format->BitsPerPixel == 8) {
        for (int y = 0; y < h; y++) {
            Uint8* src_row = (Uint8*)image->pixels + y * image->pitch;
            Uint32* dst_row = (Uint32*)((Uint8*)colored_image->pixels + y * colored_image->pitch);
            for (int x = 0; x < w; x++) {
                if (src_row[x] != 0) {
                    dst_row[x] = color_val;
                } else {
                    dst_row[x] = 0;
                }
            }
        }
    } else if (image->format->BitsPerPixel == 1) {
         for (int y = 0; y < h; y++) {
            Uint8* src_row = (Uint8*)image->pixels + y * image->pitch;
            Uint32* dst_row = (Uint32*)((Uint8*)colored_image->pixels + y * colored_image->pitch);
            for (int x = 0; x < w; x++) {
                Uint8 byte = src_row[x / 8];
                Uint8 bit = (byte >> (7 - (x % 8))) & 1;
                if (bit) {
                    dst_row[x] = color_val;
                } else {
                    dst_row[x] = 0;
                }
            }
        }
    } else {
        // Fallback for 24/32 bit source
        int bpp = image->format->BytesPerPixel;
        for (int y = 0; y < h; y++) {
            Uint8* src_row = (Uint8*)image->pixels + y * image->pitch;
            Uint32* dst_row = (Uint32*)((Uint8*)colored_image->pixels + y * colored_image->pitch);
            for (int x = 0; x < w; x++) {
                Uint8 sr = src_row[x * bpp];
                Uint8 sg = src_row[x * bpp + 1];
                Uint8 sb = src_row[x * bpp + 2];
                Uint8 a = (bpp == 4) ? src_row[x * bpp + 3] : 255;
                dst_row[x] = color_val;
            }
        }
    }

	SDL_Rect src_rect = {0, 0, image->w, image->h};
	SDL_Rect dest_rect = {xpos, ypos, image->w, image->h};

	SDL_SetSurfaceBlendMode(colored_image, SDL_BLENDMODE_BLEND);
	SDL_SetSurfaceBlendMode(current_target_surface, SDL_BLENDMODE_BLEND);
	SDL_SetSurfaceAlphaMod(colored_image, 255);
	if (SDL_BlitSurface(colored_image, &src_rect, current_target_surface, &dest_rect) != 0) {
		sdlperror("method_3_blit_mono: SDL_BlitSurface");
        psram_set_temp_offset(saved_offset);
		quit(1);
	}
	// SDL_FreeSurface(colored_image); // No-op in PSRAM
    psram_set_temp_offset(saved_offset); // Free the temp surface

	return image;
}

// Workaround for a bug in SDL2 (before v2.0.4):
// https://bugzilla.libsdl.org/show_bug.cgi?id=2986
// SDL_FillRect onto a 24-bit surface swaps Red and Blue component

bool RGB24_bug_checked = false;
bool RGB24_bug_affected;

bool RGB24_bug_check(void) {
	if (!RGB24_bug_checked) {
		// Check if the bug occurs in this version of SDL.
		SDL_Surface* test_surface = SDL_CreateRGBSurface(0, 1, 1, 32, 0, 0, 0, 0);
		if (NULL == test_surface) sdlperror("SDL_CreateSurface in RGB24_bug_check");
		// Fill with red.
		SDL_FillRect(test_surface, NULL, SDL_MapRGB(test_surface->format, 0xFF, 0, 0));
		if (0 != SDL_LockSurface(test_surface)) sdlperror("SDL_LockSurface in RGB24_bug_check");
		// Read red component of pixel.
		RGB24_bug_affected = (*(Uint32*)test_surface->pixels & test_surface->format->Rmask) == 0;
		SDL_UnlockSurface(test_surface);
		SDL_FreeSurface(test_surface);
		RGB24_bug_checked = true;
	}
	return RGB24_bug_affected;
}

int safe_SDL_FillRect(SDL_Surface* dst, const SDL_Rect* rect, Uint32 color) {
	if (dst->format->BitsPerPixel == 24 && RGB24_bug_check()) {
		// In the buggy version, SDL_FillRect swaps R and B, so we swap it once more.
		color = ((color & 0xFF) << 16) | (color & 0xFF00) | ((color & 0xFF0000) >> 16);
	}
	return SDL_FillRect(dst, rect, color);
}
// End of workaround.

const rect_type* method_5_rect(const rect_type* rect,int blit,byte color) {
	SDL_Rect dest_rect;
	rect_to_sdlrect(rect, &dest_rect);

	// Optimization for 8-bit surfaces (RP2350 port)
	if (current_target_surface->format->BitsPerPixel == 8) {
		if (safe_SDL_FillRect(current_target_surface, &dest_rect, color) != 0) {
			sdlperror("method_5_rect: SDL_FillRect");
			quit(1);
		}
		return rect;
	}

	rgb_type palette_color = palette[color];
#ifndef USE_ALPHA
	uint32_t rgb_color = SDL_MapRGBA(current_target_surface->format, palette_color.r<<2, palette_color.g<<2, palette_color.b<<2, 0xFF);
#else
	uint32_t rgb_color = SDL_MapRGBA(current_target_surface->format, palette_color.r<<2, palette_color.g<<2, palette_color.b<<2, color == 0 ? SDL_ALPHA_TRANSPARENT : SDL_ALPHA_OPAQUE);
#endif
	if (safe_SDL_FillRect(current_target_surface, &dest_rect, rgb_color) != 0) {
		sdlperror("method_5_rect: SDL_FillRect");
		quit(1);
	}
	return rect;
}

void draw_rect_with_alpha(const rect_type* rect, byte color, byte alpha) {
	SDL_Rect dest_rect;
	rect_to_sdlrect(rect, &dest_rect);
	rgb_type palette_color = palette[color];
	uint32_t rgb_color = SDL_MapRGBA(overlay_surface->format, palette_color.r<<2, palette_color.g<<2, palette_color.b<<2, alpha);
	if (safe_SDL_FillRect(current_target_surface, &dest_rect, rgb_color) != 0) {
		sdlperror("draw_rect_with_alpha: SDL_FillRect");
		quit(1);
	}
}

void draw_rect_contours(const rect_type* rect, byte color) {
	// TODO: handle 24 bit surfaces? (currently, 32 bit surface is assumed)
	if (current_target_surface->format->BitsPerPixel != 32) {
		printf("draw_rect_contours: not implemented for %d bit surfaces\n", current_target_surface->format->BitsPerPixel);
		return;
	}
	SDL_Rect dest_rect;
	rect_to_sdlrect(rect, &dest_rect);
	rgb_type palette_color = palette[color];
	uint32_t rgb_color = SDL_MapRGBA(overlay_surface->format, palette_color.r<<2, palette_color.g<<2, palette_color.b<<2, 0xFF);
	if (SDL_LockSurface(current_target_surface) != 0) {
		sdlperror("draw_rect_contours: SDL_LockSurface");
		quit(1);
	}
	int bytes_per_pixel = current_target_surface->format->BytesPerPixel;
	int pitch = current_target_surface->pitch;
	byte* pixels = current_target_surface->pixels;
	int xmin = MIN(dest_rect.x,               current_target_surface->w);
	int xmax = MIN(dest_rect.x + dest_rect.w, current_target_surface->w);
	int ymin = MIN(dest_rect.y,               current_target_surface->h);
	int ymax = MIN(dest_rect.y + dest_rect.h, current_target_surface->h);
	byte* row = pixels + ymin*pitch;
	uint32_t* pixel =  (uint32_t*) (row + xmin*bytes_per_pixel);
	for (int x = xmin; x < xmax; ++x) {
		*pixel++ = rgb_color;
	}
	for (int y = ymin+1; y < ymax-1; ++y) {
		row += pitch;
		*(uint32_t*)(row + xmin*bytes_per_pixel) = rgb_color;
		*(uint32_t*)(row + (xmax-1)*bytes_per_pixel) = rgb_color;
	}
	pixel = (uint32_t*) (pixels + (ymax-1)*pitch + xmin*bytes_per_pixel);
	for (int x = xmin; x < xmax; ++x) {
		*pixel++ = rgb_color;
	}

	SDL_UnlockSurface(current_target_surface);
}

void blit_xor(SDL_Surface* target_surface, SDL_Rect* dest_rect, SDL_Surface* image, SDL_Rect* src_rect) {
	if (dest_rect->w != src_rect->w || dest_rect->h != src_rect->h) {
		printf("blit_xor: dest_rect and src_rect have different sizes\n");
		quit(1);
	}

    // Save temp offset
    size_t saved_offset = psram_get_temp_offset();

    // Use temp allocator
    psram_set_temp_mode(1);
    // Use 32-bit surface (XRGB) to avoid 24-bit issues and ensure alignment
	SDL_Surface* helper_surface = SDL_CreateRGBSurface(0, dest_rect->w, dest_rect->h, 32, Rmsk, Gmsk, Bmsk, 0);
    psram_set_temp_mode(0);

	if (helper_surface == NULL) {
		sdlperror("blit_xor: SDL_CreateRGBSurface");
        psram_set_temp_offset(saved_offset);
		quit(1);
	}

    // Use temp allocator
    psram_set_temp_mode(1);
	SDL_Surface* image_24 = SDL_ConvertSurface(image, helper_surface->format, 0);
    psram_set_temp_mode(0);

	//SDL_CreateRGBSurface(0, src_rect->w, src_rect->h, 24, Rmsk, Gmsk, Bmsk, 0);
	if (image_24 == NULL) {
		sdlperror("blit_xor: SDL_CreateRGBSurface");
        psram_set_temp_offset(saved_offset);
		quit(1);
	}
	SDL_Rect dest_rect2 = *src_rect;
	// Read what is currently where we want to draw the new image.
	if (SDL_BlitSurface(target_surface, dest_rect, helper_surface, &dest_rect2) != 0) {
		sdlperror("blit_xor: SDL_BlitSurface");
        psram_set_temp_offset(saved_offset);
		quit(1);
	}
	if (SDL_LockSurface(image_24) != 0) {
		sdlperror("blit_xor: SDL_LockSurface");
        psram_set_temp_offset(saved_offset);
		quit(1);
	}
	if (SDL_LockSurface(helper_surface) != 0) {
		sdlperror("blit_xor: SDL_LockSurface");
        psram_set_temp_offset(saved_offset);
		quit(1);
	}
	int size = helper_surface->h * helper_surface->pitch;
	byte *p_src = (byte*) image_24->pixels;
	byte *p_dest = (byte*) helper_surface->pixels;

	// Xor the old area with the image.
	for (int i = 0; i < size; ++i) {
		*p_dest ^= *p_src;
		++p_src; ++p_dest;
	}
	SDL_UnlockSurface(image_24);
	SDL_UnlockSurface(helper_surface);
	// Put the new area in place of the old one.
	if (SDL_BlitSurface(helper_surface, src_rect, target_surface, dest_rect) != 0) {
		sdlperror("blit_xor: SDL_BlitSurface 2065");
        psram_set_temp_offset(saved_offset);
		quit(1);
	}
	// SDL_FreeSurface(image_24);
	// SDL_FreeSurface(helper_surface);
    psram_set_temp_offset(saved_offset);
}

#ifdef USE_COLORED_TORCHES
void draw_colored_torch(int color, SDL_Surface* image, int xpos, int ypos) {
	if (SDL_SetColorKey(image, SDL_TRUE, 0) != 0) {
		sdlperror("draw_colored_torch: SDL_SetColorKey");
		quit(1);
	}

    // Save temp offset
    size_t saved_offset = psram_get_temp_offset();
    
    // Use temp allocator
    psram_set_temp_mode(1);
    // Use 8-bit surface for torches to match screen and avoid conversion
	SDL_Surface* colored_image = SDL_CreateRGBSurface(SDL_FORCE_FULL_PALETTE, image->w, image->h, 8, 0, 0, 0, 0);
    psram_set_temp_mode(0);

    if (!colored_image) {
        sdlperror("draw_colored_torch: SDL_CreateRGBSurface");
        psram_set_temp_offset(saved_offset);
        quit(1);
    }

    // Copy palette from screen
    if (current_target_surface->format->palette) {
        SDL_SetPaletteColors(colored_image->format->palette, current_target_surface->format->palette->colors, 0, 256);
    }

    // Calculate new color index
	int iRed = ((color >> 4) & 3) * 85;
	int iGreen = ((color >> 2) & 3) * 85;
	int iBlue = ((color >> 0) & 3) * 85;
    
    // Find index for new color
    Uint32 new_color_idx = SDL_MapRGB(colored_image->format, iRed, iGreen, iBlue);
    
    // Find index for old color (orange)
    Uint32 old_color_idx = SDL_MapRGB(colored_image->format, 0xFC, 0x84, 0x00);

    // Copy pixels and replace color
    int w = image->w;
    int h = image->h;
    if (image->format->BitsPerPixel == 8) {
        for (int y = 0; y < h; y++) {
            Uint8* src_row = (Uint8*)image->pixels + y * image->pitch;
            Uint8* dst_row = (Uint8*)colored_image->pixels + y * colored_image->pitch;
            for (int x = 0; x < w; x++) {
                Uint8 idx = src_row[x];
                if (idx == old_color_idx) {
                    dst_row[x] = (Uint8)new_color_idx;
                } else {
                    dst_row[x] = idx;
                }
            }
        }
    } else {
        // Fallback if source is not 8-bit (unlikely)
        // Just copy 0
        memset(colored_image->pixels, 0, colored_image->h * colored_image->pitch);
    }

    // Set colorkey (0 is usually transparent for torches)
    SDL_SetColorKey(colored_image, SDL_TRUE, 0);

	SDL_Rect src_rect = {0, 0, image->w, image->h};
	SDL_Rect dest_rect = {xpos, ypos, image->w, image->h};

	if (SDL_BlitSurface(colored_image, &src_rect, current_target_surface, &dest_rect) != 0) {
		sdlperror("draw_colored_torch: SDL_BlitSurface");
        psram_set_temp_offset(saved_offset);
		quit(1);
	}
	// SDL_FreeSurface(colored_image);
    psram_set_temp_offset(saved_offset);
}
#endif

// Cache to avoid redundant SDL state changes (huge performance boost on RP2350)
typedef struct {
	image_type* last_image;
	int last_blit_mode;
	SDL_BlendMode last_blend_mode;
	SDL_bool last_colorkey;
} blit_state_cache_t;

static blit_state_cache_t blit_cache = {NULL, -1, (SDL_BlendMode)-1, SDL_FALSE};

// Reset cache when switching surfaces or starting new frame
void reset_blit_cache() {
	blit_cache.last_image = NULL;
	blit_cache.last_blit_mode = -1;
	blit_cache.last_blend_mode = (SDL_BlendMode)-1;
	blit_cache.last_colorkey = SDL_FALSE;
}

image_type* method_6_blit_img_to_scr(image_type* image,int xpos,int ypos,int blit) {
	if (image == NULL) {
		//printf("method_6_blit_img_to_scr: image == NULL\n");
        //fflush(stdout);
		return NULL;
	}
	if (current_target_surface == NULL) {
		printf("method_6_blit_img_to_scr: current_target_surface == NULL\n");
        fflush(stdout);
		return NULL;
	}

	// Debug output disabled for performance
	//printf("method_6_blit_img_to_scr: image=%p x=%d y=%d blit=%d w=%d h=%d\n", image, xpos, ypos, blit, image->w, image->h);
    //fflush(stdout);

	if (blit == blitters_9_black) {
		method_3_blit_mono(image, xpos, ypos, blitters_9_black, 0);
		return image;
	}

	SDL_Rect src_rect = {0, 0, image->w, image->h};
	SDL_Rect dest_rect = {xpos, ypos, image->w, image->h};

	if (blit == blitters_3_xor) {
		blit_xor(current_target_surface, &dest_rect, image, &src_rect);
		return image;
	}

#ifdef USE_COLORED_TORCHES
	if (blit >= blitters_colored_flame && blit <= blitters_colored_flame_last) {
		draw_colored_torch(blit - blitters_colored_flame, image, xpos, ypos);
		return image;
	}
#endif

	// OPTIMIZATION: Only set SDL state if it actually changed
	// This avoids expensive SDL calls for every sprite
	SDL_bool need_state_change = (blit_cache.last_image != image || blit_cache.last_blit_mode != blit);
	
	if (need_state_change) {
		SDL_BlendMode target_blend;
		SDL_bool target_colorkey;
		
		// Determine target state
		if (blit == blitters_0_no_transp) {
			target_blend = SDL_BLENDMODE_NONE;
			target_colorkey = SDL_FALSE;
		} else {
			if (image->format->BitsPerPixel == 8) {
				target_blend = SDL_BLENDMODE_NONE;
				target_colorkey = SDL_TRUE;
			} else {
				target_blend = SDL_BLENDMODE_BLEND;
				target_colorkey = SDL_FALSE;
			}
		}
		
		// Only call SDL functions if state actually changed
		if (blit_cache.last_blend_mode != target_blend) {
			SDL_SetSurfaceBlendMode(image, target_blend);
			blit_cache.last_blend_mode = target_blend;
		}
		
		if (blit_cache.last_colorkey != target_colorkey || blit_cache.last_image != image) {
			SDL_SetColorKey(image, target_colorkey, 0);
			blit_cache.last_colorkey = target_colorkey;
		}
		
		SDL_SetSurfaceAlphaMod(image, 255);
		
		// Update cache
		blit_cache.last_image = image;
		blit_cache.last_blit_mode = blit;
	}

	if (SDL_BlitSurface(image, &src_rect, current_target_surface, &dest_rect) != 0) {
		sdlperror("method_6_blit_img_to_scr: SDL_BlitSurface 2247");
		//quit(1);
	}
    // Debug output disabled for performance
    //printf("method_6_blit_img_to_scr: SDL_BlitSurface done\n");
    //fflush(stdout);
/*
	if (SDL_SetSurfaceAlphaMod(image, 0) != 0) {
		sdlperror("method_6_blit_img_to_scr: SDL_SetAlpha");
		quit(1);
	}
*/
	return image;
}

#ifndef USE_COMPAT_TIMER
int fps = BASE_FPS;
float milliseconds_per_tick = (1000.0f / (BASE_FPS*1.0f));
Uint64 timer_last_counter[NUM_TIMERS];
#endif
int wait_time[NUM_TIMERS];


#ifdef USE_COMPAT_TIMER
Uint32 timer_callback(Uint32 interval, void *param) {
	SDL_Event event;
	memset(&event, 0, sizeof(event));
	event.type = SDL_USEREVENT;
	event.user.code = userevent_TIMER;
	event.user.data1 = param;
	SDL_PushEvent(&event);
	return interval;
}
#endif

void reset_timer(int timer_index) {
#ifndef USE_COMPAT_TIMER
	timer_last_counter[timer_index] = SDL_GetPerformanceCounter();
#endif
}

double get_ticks_per_sec(int timer_index) {
	return (double) fps / wait_time[timer_index];
}

void recalculate_feather_fall_timer(double previous_ticks_per_second, double ticks_per_second) {
	if (is_feather_fall <= MAX(previous_ticks_per_second, ticks_per_second) ||
			previous_ticks_per_second == ticks_per_second) {
		return;
	}
	// there are more ticks per second in base mode vs fight mode so
	// feather fall length needs to be recalculated
	is_feather_fall = is_feather_fall / previous_ticks_per_second * ticks_per_second;
}

void set_timer_length(int timer_index, int length) {
	if (!fixes->fix_quicksave_during_feather) {
		wait_time[timer_index] = length;
		return;
	}
	if (is_feather_fall == 0 ||
			wait_time[timer_index] < custom->base_speed ||
			wait_time[timer_index] > custom->fight_speed) {
		wait_time[timer_index] = length;
		return;
	}
	double previous_ticks_per_second, ticks_per_second;
	previous_ticks_per_second = get_ticks_per_sec(timer_index);
	wait_time[timer_index] = length;
	ticks_per_second = get_ticks_per_sec(timer_index);
	recalculate_feather_fall_timer(previous_ticks_per_second, ticks_per_second);
}

void start_timer(int timer_index, int length) {
#ifdef USE_REPLAY
	if (replaying && skipping_replay) return;
#endif
#ifndef USE_COMPAT_TIMER
	timer_last_counter[timer_index] = SDL_GetPerformanceCounter();
#endif
	wait_time[timer_index] = length;
}

void toggle_fullscreen(void) {
	uint32_t flags = SDL_GetWindowFlags(window_);
	if (flags & SDL_WINDOW_FULLSCREEN_DESKTOP) {
		SDL_SetWindowFullscreen(window_, 0);
		SDL_ShowCursor(SDL_ENABLE);
	}
	else {
		SDL_SetWindowFullscreen(window_, SDL_WINDOW_FULLSCREEN_DESKTOP);
		SDL_ShowCursor(SDL_DISABLE);
	}
}

bool ignore_tab = false;

void process_events() {
	// Process all events in the queue.
	// Previously, this procedure would wait for *one* event and process it, then return.
	// Much like the x86 HLT instruction.
	// (We still want to process all events in the queue. For instance, there might be
	// simultaneous SDL2 KEYDOWN and TEXTINPUT events.)
	SDL_Event event;
	while (SDL_PollEvent(&event) == 1) { // while there are still events to be processed
		switch (event.type) {
			case SDL_KEYDOWN:
			{
				int modifier = event.key.keysym.mod;
				int scancode = event.key.keysym.scancode;

				// Handle these separately, so they won't interrupt things that are usually interrupted by a keypress. (pause, cutscene)
#ifdef USE_FAST_FORWARD
				if (scancode == SDL_SCANCODE_GRAVE) {
					init_timer(BASE_FPS * FAST_FORWARD_RATIO); // fast-forward on
					audio_speed = FAST_FORWARD_RATIO;
					break;
				}
#endif
#ifdef USE_SCREENSHOT
				if (scancode == SDL_SCANCODE_F12) {
					if (modifier & KMOD_SHIFT) {
						save_level_screenshot((modifier & KMOD_CTRL) != 0);
					} else {
						save_screenshot();
					}
				} else
#endif
#ifdef USE_MENU
				if (escape_key_suppressed &&
						(scancode == SDL_SCANCODE_BACKSPACE || (enable_pause_menu && scancode == SDL_SCANCODE_ESCAPE))
				) {
					break; // Prevent repeated keystrokes opening/closing the menu as long as the key is held down.
				} else
#endif
				if ((modifier & KMOD_ALT) &&
				    scancode == SDL_SCANCODE_RETURN)
				{
					// Only if the Enter key was pressed down right now.
					if ((key_states[scancode] & KEYSTATE_HELD) == 0) {
						// Alt+Enter: toggle fullscreen mode
						toggle_fullscreen();
						key_states[scancode] |= KEYSTATE_HELD | KEYSTATE_HELD_NEW;
					}
				} else {
					last_any_key_scancode = scancode; // for showmessage_any_key
					key_states[scancode] |= KEYSTATE_HELD | KEYSTATE_HELD_NEW;
					switch (scancode) {
						// Keys that are ignored by themselves:
						case SDL_SCANCODE_LCTRL:
						case SDL_SCANCODE_LSHIFT:
						case SDL_SCANCODE_LALT:
						case SDL_SCANCODE_LGUI:
						case SDL_SCANCODE_RCTRL:
						case SDL_SCANCODE_RSHIFT:
						case SDL_SCANCODE_RALT:
						case SDL_SCANCODE_RGUI:
						case SDL_SCANCODE_CAPSLOCK:
						case SDL_SCANCODE_SCROLLLOCK:
						case SDL_SCANCODE_NUMLOCKCLEAR:
						case SDL_SCANCODE_APPLICATION:
						case SDL_SCANCODE_PRINTSCREEN:
						case SDL_SCANCODE_VOLUMEUP:
						case SDL_SCANCODE_VOLUMEDOWN:
						// Why are there two mute key codes?
						case SDL_SCANCODE_MUTE:
						case SDL_SCANCODE_AUDIOMUTE:
						case SDL_SCANCODE_PAUSE:
							break;

						default:
							// If Alt is held down from Alt+Tab: ignore it until it's released.
							if (scancode == SDL_SCANCODE_TAB && ignore_tab) break;

							last_key_scancode = scancode;
							if (modifier & KMOD_SHIFT) last_key_scancode |= WITH_SHIFT;
							if (modifier & KMOD_CTRL ) last_key_scancode |= WITH_CTRL ;
							if (modifier & KMOD_ALT  ) last_key_scancode |= WITH_ALT  ;
					}

#ifdef USE_AUTO_INPUT_MODE
					switch (scancode) {
						// Keys that are used for keyboard control:
						case SDL_SCANCODE_LSHIFT:
						case SDL_SCANCODE_RSHIFT:
						case SDL_SCANCODE_LEFT:
						case SDL_SCANCODE_RIGHT:
						case SDL_SCANCODE_UP:
						case SDL_SCANCODE_DOWN:
						case SDL_SCANCODE_CLEAR:
						case SDL_SCANCODE_HOME:
						case SDL_SCANCODE_PAGEUP:
						case SDL_SCANCODE_KP_2:
						case SDL_SCANCODE_KP_4:
						case SDL_SCANCODE_KP_5:
						case SDL_SCANCODE_KP_6:
						case SDL_SCANCODE_KP_7:
						case SDL_SCANCODE_KP_8:
						case SDL_SCANCODE_KP_9:
							if (!is_keyboard_mode) {
								is_keyboard_mode = 1;
								is_joyst_mode = 0;
							}
					}
#endif
				}
				break;
			}
			case SDL_KEYUP:
				// If Alt was held down from Alt+Tab but now it's released: stop ignoring Tab.
				if (event.key.keysym.scancode == SDL_SCANCODE_TAB && ignore_tab) ignore_tab = false;

#ifdef USE_FAST_FORWARD
				if (event.key.keysym.scancode == SDL_SCANCODE_GRAVE) {
					init_timer(BASE_FPS); // fast-forward off
					audio_speed = 1;
					break;
				}
#endif

				key_states[event.key.keysym.scancode] &= ~KEYSTATE_HELD;
#ifdef USE_MENU
				// Prevent repeated keystrokes opening/closing the menu as long as the key is held down.
				if (event.key.keysym.scancode == SDL_SCANCODE_BACKSPACE || event.key.keysym.scancode == SDL_SCANCODE_ESCAPE) {
					escape_key_suppressed = false;
				}
#endif
				break;
			case SDL_CONTROLLERAXISMOTION:
				if (event.caxis.axis < 6) {
					joy_axis[event.caxis.axis] = event.caxis.value;
					if (abs(event.caxis.value) > abs(joy_axis_max[event.caxis.axis]))
						joy_axis_max[event.caxis.axis] = event.caxis.value;

#ifdef USE_AUTO_INPUT_MODE
					if (!is_joyst_mode && (event.caxis.value >= joystick_threshold || event.caxis.value <= -joystick_threshold)) {
						is_joyst_mode = 1;
						is_keyboard_mode = 0;
					}
#endif
				}
				break;
			case SDL_CONTROLLERDEVICEADDED:
				SDL_GameControllerOpen(event.cdevice.which);
				if (gamecontrollerdb_file[0] != '\0') {
					SDL_GameControllerAddMappingsFromFile(gamecontrollerdb_file);
				}
				is_joyst_mode = 1;
				using_sdl_joystick_interface = 0;
				break;
			case SDL_CONTROLLERDEVICEREMOVED:
				if (sdl_controller_ == SDL_GameControllerFromInstanceID(event.cdevice.which)) {
					sdl_controller_ = NULL;
					is_joyst_mode = 0;
					is_keyboard_mode = 1;
				}
				SDL_GameControllerClose(SDL_GameControllerFromInstanceID(event.cdevice.which));
				break;
			case SDL_CONTROLLERBUTTONDOWN:
				//Make sure sdl_controller_ always points to the active controller
				sdl_controller_ = SDL_GameControllerFromInstanceID(event.cdevice.which);
#ifdef USE_AUTO_INPUT_MODE
				if (!is_joyst_mode) {
					is_joyst_mode = 1;
				 is_keyboard_mode = 0;
				}
#endif
				switch (event.cbutton.button)
				{
					case SDL_CONTROLLER_BUTTON_DPAD_LEFT:  joy_button_states[JOYINPUT_DPAD_LEFT] |= KEYSTATE_HELD | KEYSTATE_HELD_NEW; break; // left
					case SDL_CONTROLLER_BUTTON_DPAD_RIGHT: joy_button_states[JOYINPUT_DPAD_RIGHT] |= KEYSTATE_HELD | KEYSTATE_HELD_NEW; break; // right
					case SDL_CONTROLLER_BUTTON_DPAD_UP:    joy_button_states[JOYINPUT_DPAD_UP] |= KEYSTATE_HELD | KEYSTATE_HELD_NEW; break; // up
					case SDL_CONTROLLER_BUTTON_DPAD_DOWN:  joy_button_states[JOYINPUT_DPAD_DOWN] |= KEYSTATE_HELD | KEYSTATE_HELD_NEW; break; // down

					case SDL_CONTROLLER_BUTTON_A:          joy_button_states[JOYINPUT_A] |= KEYSTATE_HELD | KEYSTATE_HELD_NEW; break; /*** A (down) ***/
					case SDL_CONTROLLER_BUTTON_Y:          joy_button_states[JOYINPUT_Y] |= KEYSTATE_HELD | KEYSTATE_HELD_NEW; break; /*** Y (up) ***/
					case SDL_CONTROLLER_BUTTON_X:          joy_button_states[JOYINPUT_X] |= KEYSTATE_HELD | KEYSTATE_HELD_NEW; break; /*** X (Shift) ***/
					case SDL_CONTROLLER_BUTTON_B:          joy_button_states[JOYINPUT_B] |= KEYSTATE_HELD | KEYSTATE_HELD_NEW; break; /*** B (unused) ***/

					case SDL_CONTROLLER_BUTTON_START:
					case SDL_CONTROLLER_BUTTON_BACK:
						if(event.cbutton.button == SDL_CONTROLLER_BUTTON_START)
							joy_button_states[JOYINPUT_START] |= KEYSTATE_HELD | KEYSTATE_HELD_NEW;
						else if(event.cbutton.button == SDL_CONTROLLER_BUTTON_BACK)
							joy_button_states[JOYINPUT_BACK] |= KEYSTATE_HELD | KEYSTATE_HELD_NEW;
#ifdef USE_MENU
						last_key_scancode = SDL_SCANCODE_BACKSPACE;  /*** bring up pause menu ***/
#else
						last_key_scancode = SDL_SCANCODE_ESCAPE;  /*** back (pause game) ***/
#endif
						break;

					default: break;
				}
				break;
			case SDL_CONTROLLERBUTTONUP:
				switch (event.cbutton.button)
				{
					case SDL_CONTROLLER_BUTTON_DPAD_LEFT:  joy_button_states[JOYINPUT_DPAD_LEFT] &= ~KEYSTATE_HELD; break; // left
					case SDL_CONTROLLER_BUTTON_DPAD_RIGHT: joy_button_states[JOYINPUT_DPAD_RIGHT] &= ~KEYSTATE_HELD; break; // right
					case SDL_CONTROLLER_BUTTON_DPAD_UP:    joy_button_states[JOYINPUT_DPAD_UP] &= ~KEYSTATE_HELD; break; // up
					case SDL_CONTROLLER_BUTTON_DPAD_DOWN:  joy_button_states[JOYINPUT_DPAD_DOWN] &= ~KEYSTATE_HELD; break; // down

					case SDL_CONTROLLER_BUTTON_A:          joy_button_states[JOYINPUT_A] &= ~KEYSTATE_HELD; break; /*** A (down) ***/
					case SDL_CONTROLLER_BUTTON_Y:          joy_button_states[JOYINPUT_Y] &= ~KEYSTATE_HELD; break; /*** Y (up) ***/
					case SDL_CONTROLLER_BUTTON_X:          joy_button_states[JOYINPUT_X] &= ~KEYSTATE_HELD; break; /*** X (Shift) ***/
					case SDL_CONTROLLER_BUTTON_B:          joy_button_states[JOYINPUT_B] &= ~KEYSTATE_HELD; break; /*** B (unused) ***/

					case SDL_CONTROLLER_BUTTON_START:      joy_button_states[JOYINPUT_START] &= ~KEYSTATE_HELD; break;
					case SDL_CONTROLLER_BUTTON_BACK:       joy_button_states[JOYINPUT_BACK] &= ~KEYSTATE_HELD; break;

					default: break;
				}
				break;
			case SDL_JOYBUTTONDOWN:
			case SDL_JOYBUTTONUP:
			case SDL_JOYAXISMOTION:
				// Only handle the event if the joystick is incompatible with the SDL_GameController interface.
				// (Otherwise it will interfere with the normal action of the SDL_GameController API.)
				if (!using_sdl_joystick_interface) {
					break;
				}
				if (event.type == SDL_JOYAXISMOTION) {
					int axis = -1;
					if (event.jaxis.axis == SDL_JOYSTICK_X_AXIS) {
						axis = SDL_CONTROLLER_AXIS_LEFTX;
					}
					else if (event.jaxis.axis == SDL_JOYSTICK_Y_AXIS) {
						axis = SDL_CONTROLLER_AXIS_LEFTY;
					}
					if (axis == -1)
						break;
					joy_axis[axis] = event.jaxis.value;
					if (abs(event.jaxis.value) > abs(joy_axis_max[axis]))
						joy_axis_max[axis] = event.jaxis.value;

					// Disregard SDL_JOYAXISMOTION events within joystick 'dead zone'
					int joy_x = joy_axis[SDL_CONTROLLER_AXIS_LEFTX];
					int joy_y = joy_axis[SDL_CONTROLLER_AXIS_LEFTY];
					if ((dword)(joy_x*joy_x) + (dword)(joy_y*joy_y) < (dword)(joystick_threshold*joystick_threshold)) {
						break;
					}
				}
#ifdef USE_AUTO_INPUT_MODE
				if (!is_joyst_mode) {
					is_joyst_mode = 1;
					is_keyboard_mode = 0;
				}
#endif
				if (event.type == SDL_JOYBUTTONDOWN) {
					if      (event.jbutton.button == SDL_JOYSTICK_BUTTON_Y)   joy_button_states[JOYINPUT_Y] |= KEYSTATE_HELD | KEYSTATE_HELD_NEW; // Y (up)
					else if (event.jbutton.button == SDL_JOYSTICK_BUTTON_X)   joy_button_states[JOYINPUT_X] |= KEYSTATE_HELD | KEYSTATE_HELD_NEW;    // X (Shift)
				}
				else if (event.type == SDL_JOYBUTTONUP) {
					if      (event.jbutton.button == SDL_JOYSTICK_BUTTON_Y)   joy_button_states[JOYINPUT_Y] &= ~KEYSTATE_HELD;  // Y (up)
					else if (event.jbutton.button == SDL_JOYSTICK_BUTTON_X)   joy_button_states[JOYINPUT_X] &= ~KEYSTATE_HELD;    // X (Shift)
				}
				break;

			case SDL_TEXTINPUT:
				last_text_input = event.text.text[0]; // UTF-8 formatted char text input

				// Make the +/- keys work on the main keyboard, on any keyboard layout.
				// We check SDL_TEXTINPUT instead of SDL_KEYDOWN.
				// If '+' is on Shift+something then we can't detect it in SDL_KEYDOWN,
				// because event.key.keysym.sym only tells us what character would the key type without shift.
				switch (last_text_input) {
					case '-': last_key_scancode = SDL_SCANCODE_KP_MINUS; break;
					case '+': last_key_scancode = SDL_SCANCODE_KP_PLUS;  break;
				}

				break;
			case SDL_WINDOWEVENT:
				// In case the user switches away while holding a key: do as if all keys were released.
				// (DOSBox does the same.)

/* // not implemented in SDL2 for now
 *
			if ((event.active.state & SDL_APPINPUTFOCUS) && event.active.gain == 0) {
				memset(key_states, 0, sizeof(key_states));
			}
			// Note: event.active.state can contain multiple flags or'ed.
			// If the game is in full screen, and I switch away (Alt+Tab) and back, most of the screen will be black, until it is redrawn.
			if ((event.active.state & SDL_APPACTIVE) && event.active.gain == 1) {
				update_screen();
			}
*/
				switch (event.window.event) {
#ifdef __amigaos4__
					case SDL_WINDOWEVENT_MINIMIZED: /* pause game */
						if (!is_menu_shown) {
							last_key_scancode = SDL_SCANCODE_BACKSPACE;
						}
						break;
					case SDL_WINDOWEVENT_RESTORED: /* show "game paused/menu" */
						update_screen();
						break;
#endif
					case SDL_WINDOWEVENT_SIZE_CHANGED:
						window_resized();
						// fallthrough!
					//case SDL_WINDOWEVENT_MOVED:
					//case SDL_WINDOWEVENT_RESTORED:
					case SDL_WINDOWEVENT_EXPOSED:
						update_screen();
						break;

					case SDL_WINDOWEVENT_FOCUS_GAINED:
					// Fix for this bug: When playing back a recording, Alt+Tabbing back to SDLPoP stops the replay if Alt is released before Tab.
					{ // If Alt is held down from Alt+Tab: ignore it until it's released.
						const Uint8 *state = SDL_GetKeyboardState(NULL);
						if (state[SDL_SCANCODE_TAB]) ignore_tab = true;
					}
					break;
				}
				break;
			case SDL_USEREVENT:
				if (event.user.code == userevent_TIMER /*&& event.user.data1 == (void*)timer_index*/) {
#ifdef USE_COMPAT_TIMER
					for (int index = 0; index < NUM_TIMERS; ++index) {
						if (wait_time[index] > 0) --wait_time[index];
					}
#endif
				} else if (event.user.code == userevent_SOUND) {
					//sound_timer = 0;
					//stop_sounds();
				}
				break;
#ifdef USE_MENU
			case SDL_MOUSEBUTTONDOWN:
				switch(event.button.button) {
					case SDL_BUTTON_LEFT:
						if (!is_menu_shown) {
							last_key_scancode = SDL_SCANCODE_BACKSPACE;
						} else {
							mouse_clicked = true;
						}
						break;
					case SDL_BUTTON_RIGHT:
					case SDL_BUTTON_X1: // 'Back' button (on mice that have these extra buttons).
						mouse_button_clicked_right = true;
						break;
					default: break;
				}

				break;
			case SDL_MOUSEWHEEL:
				if (is_menu_shown) {
					menu_control_scroll_y = -event.wheel.y;
				}
				break;
#endif
			case SDL_QUIT:
#ifdef USE_MENU
				if (is_menu_shown) {
					menu_was_closed();
				}
#endif
				quit(0);
				break;
		}
	}
}

void idle() {
	process_events();
	update_screen();
}

void do_simple_wait(int timer_index) {
#ifdef USE_REPLAY
	if ((replaying && skipping_replay) || is_validate_mode) return;
#endif
	update_screen();
	while (! has_timer_stopped(timer_index)) {
		SDL_Delay(1);
		process_events();
	}
}

word word_1D63A = 1;
int do_wait(int timer_index) {
#ifdef USE_REPLAY
	if ((replaying && skipping_replay) || is_validate_mode) return 0;
#endif
	update_screen();
	while (! has_timer_stopped(timer_index)) {
		SDL_Delay(1);
		process_events();
		int key = do_paused();
		if (key != 0 && (word_1D63A !=  0 || key == 0x1B)) return 1;
	}
	return 0;
}

#ifdef USE_COMPAT_TIMER
SDL_TimerID global_timer = 0;
#endif
// seg009:78E9
void init_timer(int frequency) {
	perf_frequency = SDL_GetPerformanceFrequency();
#ifndef USE_COMPAT_TIMER
	fps = frequency;
	milliseconds_per_tick = 1000.0f / (float)fps;
	perf_counters_per_tick = perf_frequency / fps;
	milliseconds_per_counter = 1000.0f / perf_frequency;
#else
	global_timer = SDL_AddTimer(1000/frequency, timer_callback, NULL);
	if (global_timer != 0) {
		if (!SDL_RemoveTimer(global_timer)) {
			sdlperror("init_timer: SDL_RemoveTimer");
		}
	}
	if (global_timer == 0) {
		sdlperror("init_timer: SDL_AddTimer");
		quit(1);
	}
#endif
}

// seg009:35F6
void set_clip_rect(const rect_type* rect) {
	SDL_Rect clip_rect;
	rect_to_sdlrect(rect, &clip_rect);
	SDL_SetClipRect(current_target_surface, &clip_rect);
}

// seg009:365C
void reset_clip_rect() {
	SDL_SetClipRect(current_target_surface, NULL);
}

// seg009:1983
void set_bg_attr(int vga_pal_index,int hc_pal_index) {
	// stub
#ifdef USE_FLASH
	//palette[vga_pal_index] = vga_palette[hc_pal_index];
	if (!enable_flash) return;
	if (vga_pal_index == 0) {
		/*
		if (SDL_SetAlpha(offscreen_surface, SDL_SRCALPHA, 0) != 0) {
			sdlperror("set_bg_attr: SDL_SetAlpha");
			quit(1);
		}
		*/
		// Make the black pixels transparent.
		if (SDL_SetColorKey(offscreen_surface, SDL_TRUE, 0) != 0) {	// SDL_SRCCOLORKEY old
			sdlperror("set_bg_attr: SDL_SetColorKey");
			quit(1);
		}
		SDL_Rect rect = {0,0,0,0};
		rect.w = offscreen_surface->w;
		rect.h = offscreen_surface->h;
		rgb_type palette_color = palette[hc_pal_index];
		uint32_t rgb_color = SDL_MapRGB(onscreen_surface_->format, palette_color.r<<2, palette_color.g<<2, palette_color.b<<2) /*& 0xFFFFFF*/;
		//SDL_UpdateRect(onscreen_surface_, 0, 0, 0, 0);
		// First clear the screen with the color of the flash.
		if (safe_SDL_FillRect(onscreen_surface_, &rect, rgb_color) != 0) {
			sdlperror("set_bg_attr: SDL_FillRect");
			quit(1);
		}
		//SDL_UpdateRect(onscreen_surface_, 0, 0, 0, 0);
		if (upside_down) {
			flip_screen(offscreen_surface);
		}
		// Then draw the offscreen image onto it.
		if (SDL_BlitSurface(offscreen_surface, &rect, onscreen_surface_, &rect) != 0) {
			sdlperror("set_bg_attr: SDL_BlitSurface");
			quit(1);
		}
#ifdef USE_LIGHTING
		if (hc_pal_index == 0) update_lighting(&rect_top);
#endif
		if (upside_down) {
			flip_screen(offscreen_surface);
		}
		// And show it!
//		update_screen();
		// Give some time to show the flash.
		//SDL_Flip(onscreen_surface_);
//		if (hc_pal_index != 0) SDL_Delay(2*(1000/60));
		//SDL_Flip(onscreen_surface_);
		/*
		if (SDL_SetAlpha(offscreen_surface, 0, 0) != 0) {
			sdlperror("set_bg_attr: SDL_SetAlpha");
			quit(1);
		}
		*/
		if (SDL_SetColorKey(offscreen_surface, 0, 0) != 0) {
			sdlperror("set_bg_attr: SDL_SetColorKey");
			quit(1);
		}
	}
#endif // USE_FLASH
}

// seg009:07EB
rect_type* offset4_rect_add(rect_type* dest,const rect_type* source,int d_left,int d_top,int d_right,int d_bottom) {
	*dest = *source;
	dest->left += d_left;
	dest->top += d_top;
	dest->right += d_right;
	dest->bottom += d_bottom;
	return dest;
}

// seg009:3AA5
rect_type* offset2_rect(rect_type* dest,const rect_type *source,int delta_x,int delta_y) {
	dest->top    = source->top    + delta_y;
	dest->left   = source->left   + delta_x;
	dest->bottom = source->bottom + delta_y;
	dest->right  = source->right  + delta_x;
	return dest;
}

#ifdef USE_FADE
// seg009:19EF
void fade_in_2(surface_type* source_surface,int which_rows) {
	palette_fade_type* palette_buffer;
	if (graphics_mode == gmMcgaVga) {
		palette_buffer = make_pal_buffer_fadein(source_surface, which_rows, 2);
		while (fade_in_frame(palette_buffer) == 0) {
			process_events(); // modified
			do_paused();
		}
		pal_restore_free_fadein(palette_buffer);
	} else {
		// ...
	}
}

// seg009:1A51
palette_fade_type* make_pal_buffer_fadein(surface_type* source_surface,int which_rows,int wait_time) {
	palette_fade_type* palette_buffer;
	palette_buffer = (palette_fade_type*) malloc(sizeof(palette_fade_type));
	palette_buffer->which_rows = which_rows;
	palette_buffer->wait_time = wait_time;
	palette_buffer->fade_pos = 0x40;
	palette_buffer->proc_restore_free = &pal_restore_free_fadein;
	palette_buffer->proc_fade_frame = &fade_in_frame;
	read_palette_256(palette_buffer->original_pal);
	memcpy(palette_buffer->faded_pal, palette_buffer->original_pal, sizeof(palette_buffer->faded_pal));
	for (word curr_row = 0, curr_row_mask = 1; curr_row < 0x10; ++curr_row, curr_row_mask<<=1) {
		if (which_rows & curr_row_mask) {
			memset(palette_buffer->faded_pal + (curr_row<<4), 0, sizeof(rgb_type[0x10]));
			set_pal_arr(curr_row<<4, 0x10, NULL);
		}
	}
	//method_1_blit_rect(onscreen_surface_, source_surface, &screen_rect, &screen_rect, 0);
	// for RGB
	//method_5_rect(&screen_rect, 0, color_0_black);
	return palette_buffer;
}

// seg009:1B64
void pal_restore_free_fadein(palette_fade_type* palette_buffer) {
	set_pal_256(palette_buffer->original_pal);
	free(palette_buffer);
	// for RGB
	method_1_blit_rect(onscreen_surface_, offscreen_surface, &screen_rect, &screen_rect, 0);
}

// seg009:1B88
int fade_in_frame(palette_fade_type* palette_buffer) {
//	void* var_12;
	/**/start_timer(timer_1, palette_buffer->wait_time); // too slow?

	//printf("start ticks = %u\n",SDL_GetTicks());
	--palette_buffer->fade_pos;
	for (word start=0,current_row_mask=1; start<0x100; start+=0x10, current_row_mask<<=1) {
		if (palette_buffer->which_rows & current_row_mask) {
			//var_12 = palette_buffer->
			rgb_type* original_pal_ptr = palette_buffer->original_pal + start;
			rgb_type* faded_pal_ptr = palette_buffer->faded_pal + start;
			for (word column = 0; column<0x10; ++column) {
				if (original_pal_ptr[column].r > palette_buffer->fade_pos) {
					++faded_pal_ptr[column].r;
				}
				if (original_pal_ptr[column].g > palette_buffer->fade_pos) {
					++faded_pal_ptr[column].g;
				}
				if (original_pal_ptr[column].b > palette_buffer->fade_pos) {
					++faded_pal_ptr[column].b;
				}
			}
		}
	}
	for (word start = 0, current_row_mask = 1; start<0x100; start+=0x10, current_row_mask<<=1) {
		if (palette_buffer->which_rows & current_row_mask) {
			set_pal_arr(start, 0x10, palette_buffer->faded_pal + start);
		}
	}

	int h = offscreen_surface->h;
	if (SDL_LockSurface(onscreen_surface_) != 0) {
		sdlperror("fade_in_frame: SDL_LockSurface");
		quit(1);
	}
	if (SDL_LockSurface(offscreen_surface) != 0) {
		sdlperror("fade_in_frame: SDL_LockSurface");
		quit(1);
	}
	int on_stride = onscreen_surface_->pitch;
	int off_stride = offscreen_surface->pitch;
	int fade_pos = palette_buffer->fade_pos;
	for (int y = 0; y < h; ++y) {
		byte* on_pixel_ptr = (byte*)onscreen_surface_->pixels + on_stride * y;
		byte* off_pixel_ptr = (byte*)offscreen_surface->pixels + off_stride * y;
		for (int x = 0; x < on_stride; ++x) {
			//if (*off_pixel_ptr > palette_buffer->fade_pos) *pixel_ptr += 4;
			int v = *off_pixel_ptr - fade_pos*4;
			if (v<0) v=0;
			*on_pixel_ptr = v;
			++on_pixel_ptr; ++off_pixel_ptr;
		}
	}
	SDL_UnlockSurface(onscreen_surface_);
	SDL_UnlockSurface(offscreen_surface);

	return palette_buffer->fade_pos == 0;
}

// seg009:1CC9
void fade_out_2(int rows) {
	palette_fade_type* palette_buffer;
	if (graphics_mode == gmMcgaVga) {
		palette_buffer = make_pal_buffer_fadeout(rows, 2);
		while (fade_out_frame(palette_buffer) == 0) {
			process_events(); // modified
			do_paused();
		}
		pal_restore_free_fadeout(palette_buffer);
	} else {
		// ...
	}
}

// seg009:1D28
palette_fade_type* make_pal_buffer_fadeout(int which_rows,int wait_time) {
	palette_fade_type* palette_buffer;
	palette_buffer = (palette_fade_type*) malloc(sizeof(palette_fade_type));
	palette_buffer->which_rows = which_rows;
	palette_buffer->wait_time = wait_time;
	palette_buffer->fade_pos = 00; // modified
	palette_buffer->proc_restore_free = &pal_restore_free_fadeout;
	palette_buffer->proc_fade_frame = &fade_out_frame;
	read_palette_256(palette_buffer->original_pal);
	memcpy(palette_buffer->faded_pal, palette_buffer->original_pal, sizeof(palette_buffer->faded_pal));
	// for RGB
	method_1_blit_rect(onscreen_surface_, offscreen_surface, &screen_rect, &screen_rect, 0);
	return palette_buffer;
}

// seg009:1DAF
void pal_restore_free_fadeout(palette_fade_type* palette_buffer) {
	surface_type* surface;
	surface = current_target_surface;
	current_target_surface = onscreen_surface_;
	draw_rect(&screen_rect, color_0_black);
	current_target_surface = surface;
	set_pal_256(palette_buffer->original_pal);
	free(palette_buffer);
	// for RGB
	method_5_rect(&screen_rect, 0, color_0_black);
}

// seg009:1DF7
int fade_out_frame(palette_fade_type* palette_buffer) {
	word finished_fading = 1;
	++palette_buffer->fade_pos; // modified
	/**/start_timer(timer_1, palette_buffer->wait_time); // too slow?
	for (word start=0,current_row_mask=1; start<0x100; start+=0x10, current_row_mask<<=1) {
		if (palette_buffer->which_rows & current_row_mask) {
			//var_12 = palette_buffer->
			//original_pal_ptr = palette_buffer->original_pal + start;
			rgb_type* faded_pal_ptr = palette_buffer->faded_pal + start;
			for (word column = 0; column<0x10; ++column) {
				byte* curr_color_ptr = &faded_pal_ptr[column].r;
				if (*curr_color_ptr != 0) {
					--*curr_color_ptr;
					finished_fading = 0;
				}
				curr_color_ptr = &faded_pal_ptr[column].g;
				if (*curr_color_ptr != 0) {
					--*curr_color_ptr;
					finished_fading = 0;
				}
				curr_color_ptr = &faded_pal_ptr[column].b;
				if (*curr_color_ptr != 0) {
					--*curr_color_ptr;
					finished_fading = 0;
				}
			}
		}
	}
	for (word start = 0, current_row_mask = 1; start<0x100; start+=0x10, current_row_mask<<=1) {
		if (palette_buffer->which_rows & current_row_mask) {
			set_pal_arr(start, 0x10, palette_buffer->faded_pal + start);
		}
	}

	int h = offscreen_surface->h;
	if (SDL_LockSurface(onscreen_surface_) != 0) {
		sdlperror("fade_out_frame: SDL_LockSurface");
		quit(1);
	}
	if (SDL_LockSurface(offscreen_surface) != 0) {
		sdlperror("fade_out_frame: SDL_LockSurface");
		quit(1);
	}
	int on_stride = onscreen_surface_->pitch;
	int off_stride = offscreen_surface->pitch;
	int fade_pos = palette_buffer->fade_pos;
	for (int y = 0; y < h; ++y) {
		byte* on_pixel_ptr = (byte*)onscreen_surface_->pixels + on_stride * y;
		byte* off_pixel_ptr = (byte*)offscreen_surface->pixels + off_stride * y;
		for (int x = 0; x < on_stride; ++x) {
			//if (*pixel_ptr >= 4) *pixel_ptr -= 4;
			int v = *off_pixel_ptr - fade_pos*4;
			if (v<0) v=0;
			*on_pixel_ptr = v;
			++on_pixel_ptr; ++off_pixel_ptr;
		}
	}
	SDL_UnlockSurface(onscreen_surface_);
	SDL_UnlockSurface(offscreen_surface);

	return finished_fading;
}

// seg009:1F28
void read_palette_256(rgb_type* target) {
	for (int i = 0; i < 256; ++i) {
		target[i] = palette[i];
	}
}

// seg009:1F5E
void set_pal_256(rgb_type* source) {
	for (int i = 0; i < 256; ++i) {
		palette[i] = source[i];
	}
}
#endif // USE_FADE

void set_chtab_palette(chtab_type* chtab, byte* colors, int n_colors) {
	if (chtab != NULL) {
		// Update global palette if this chtab is assigned to a row
		if (chtab->extra && chtab->extra->palette_copy.row_bits) {
			for (int r = 0; r < 16; ++r) {
				if (chtab->extra->palette_copy.row_bits & (1 << r)) {
					set_pal_arr(r * 16, n_colors, (rgb_type*)colors);
					break;
				}
			}
		}

		SDL_Color* scolors = (SDL_Color*) malloc(n_colors*sizeof(SDL_Color));
		//printf("scolors\n",i);
		for (int i = 0; i < n_colors; ++i) {
			//printf("i=%d\n",i);
			scolors[i].r = *colors << 2; ++colors;
			scolors[i].g = *colors << 2; ++colors;
			scolors[i].b = *colors << 2; ++colors;
			scolors[i].a = SDL_ALPHA_OPAQUE; // the SDL2 SDL_Color struct has an alpha component
		}

		// Color 0 of the palette data is not used, it is replaced by the background color.
		// Needed for correct alternate colors (v1.3) of level 8.
		scolors[0].r = scolors[0].g = scolors[0].b = 0;
		scolors[0].a = SDL_ALPHA_TRANSPARENT;

		//printf("setcolors\n",i);
		for (int i = 0; i < chtab->n_images; ++i) {
			//printf("i=%d\n",i);
			image_type* current_image = chtab->images[i];
			if (current_image != NULL) {

				int n_colors_to_be_set = n_colors;
				SDL_Palette* current_palette = current_image->format->palette;

				// Fix crashing with the guard graphics of Christmas of Persia.
				if (current_palette != NULL) {
					// one of the guard images (i=25) is only a single transparent pixel
					// this caused SDL_SetPaletteColors to fail, I think because that palette contains only 2 colors
					if (current_palette->ncolors < n_colors_to_be_set) {
						n_colors_to_be_set = current_palette->ncolors;
					}
					if (SDL_SetPaletteColors(current_palette, scolors, 0, n_colors_to_be_set) != 0) {
						sdlperror("set_chtab_palette: SDL_SetPaletteColors");
						quit(1);
					}
				}
			}
		}
		free(scolors);
	}
}

int has_timer_stopped(int timer_index) {
#ifdef USE_COMPAT_TIMER
	return wait_time[timer_index] == 0;
#else
#ifdef USE_REPLAY
	if ((replaying && skipping_replay) || is_validate_mode) return true;
#endif
	//PSP: overshoot always too big, 333mhz mandatory to read input!
	Uint64 current_counter = SDL_GetPerformanceCounter();
	int ticks_elapsed = (int)((current_counter / perf_counters_per_tick) - (timer_last_counter[timer_index] / perf_counters_per_tick));
	int overshoot = ticks_elapsed - wait_time[timer_index];
	if (overshoot >= 0) {
//		float milliseconds_elapsed = (current_counter - timer_last_counter[timer_index]) * milliseconds_per_counter;
//		printf("timer %d:   frametime (ms) = %5.1f    fps = %.1f    timer ticks elapsed = %d\n", timer_index, milliseconds_elapsed, 1000.0f / milliseconds_elapsed, ticks_elapsed);
		if (overshoot > 0 && overshoot <= 3) {
			current_counter -= overshoot * perf_counters_per_tick;
		}
		timer_last_counter[timer_index] = current_counter;
		return true;
	} else {
		return false;
	}
#endif
}
