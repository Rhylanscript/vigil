// kernel/vigil/api.h
//
// The "kernel API table": a list of pointers to kernel functions that
// loaded apps can call.
//
// A loaded app is compiled separately from kernel, so it doesnt know 
// where kernel functions are in memory (bc their addresses change every
// build) Instead the kernel hands app a pointer to this table and the
// app calls everything through it, e.g.
//     api->gfx_fill_rect(10, 10, 50, 50, color);
//
// (Windows and Linux solve the same problem with system calls)

#ifndef VIGIL_API_H
#define VIGIL_API_H

#include <stdint.h>
#include <stddef.h>
#include "graphics.h"
#include "button.h"
#include "fs.h"
#include "menu.h"
#include "prompt.h"
#include "dialog.h"
#include "launcher.h"

// bumped whenever the table changes such that old apps cant gracefully handle
#define VIGIL_API_VERSION 1

// RULE FOR FUTURE: only ever add new entries at the END of this struct, as
// apps built against older versions expect every existing entry to stay in
// the same position and moving one would make them call the wrong function
typedef struct {
    uint32_t version;       // VIGIL_API_VERSION
    uint32_t struct_size;   // sizeof(vigil_api_t); lets an app check the table is big enough

    // --- graphics ---
    void        (*gfx_clear)(gfx_color_t color);
    void        (*gfx_put_pixel)(uint32_t x, uint32_t y, gfx_color_t color);
    gfx_color_t (*gfx_get_pixel)(uint32_t x, uint32_t y);
    void        (*gfx_draw_char)(uint32_t x, uint32_t y, char c, gfx_color_t color);
    void        (*gfx_draw_char_bg)(uint32_t x, uint32_t y, char c, gfx_color_t fg, gfx_color_t bg);
    void        (*gfx_draw_string)(uint32_t x, uint32_t y, const char* str, gfx_color_t color);
    void        (*gfx_fill_rect)(uint32_t x, uint32_t y, uint32_t width, uint32_t height, gfx_color_t color);
    void        (*gfx_draw_rect_outline)(uint32_t x, uint32_t y, uint32_t width, uint32_t height, gfx_color_t color);
    uint32_t    (*gfx_screen_width)(void);
    uint32_t    (*gfx_screen_height)(void);

    // --- mouse ---
    void        (*mouse_hide_cursor)(void);

    // --- buttons ---
    void        (*button_draw)(const button_t* button);
    int         (*button_find_at)(const button_t* buttons, int count, int32_t px, int32_t py);

    // --- strings ---
    uint32_t    (*kstrlen)(const char* str);
    int         (*kstrcmp)(const char* a, const char* b);
    void        (*kuitoa)(uint32_t value, char* buffer);
    void        (*kitoa)(int32_t value, char* buffer);

    // --- filesystem ---
    int         (*fs_write_file)(const char* path, const uint8_t* data, uint32_t size);
    int         (*fs_read_file)(const char* path, uint8_t* buffer, uint32_t buffer_size, uint32_t* out_size);
    int         (*fs_delete_file)(const char* path);
    int         (*fs_make_folder)(const char* path);
    int         (*fs_rename)(const char* path, const char* new_name);
    int         (*fs_stat)(const char* path, fs_info_t* out);
    int         (*fs_list_dir)(const char* path, fs_info_t out[FS_MAX_FILES]);

    // --- timer ---
    uint32_t    (*timer_get_ticks)(void);
    uint32_t    (*timer_get_frequency)(void);

    // --- heap ---
    void*       (*kmalloc)(size_t size);
    void        (*kfree)(void* ptr);

    // --- menu (right-click popup) ---
    void        (*menu_open)(int32_t x, int32_t y, const char* const* labels, int count);

    // --- prompt (name-entry box) ---
    void        (*prompt_open)(const char* title, const char* initial_text, int max_length, const char* blocked_chars);

    // --- launcher ---
    void        (*launcher_open_file)(const char* path);

    // --- dialog (modal choice box) ---
    void        (*dialog_open)(const char* title, const char* message, const char* const* labels, int count);
    void        (*launcher_close_app)(void);
} vigil_api_t;

// returns the shared table
const vigil_api_t* api_get(void);

#endif
