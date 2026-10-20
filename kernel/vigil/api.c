// kernel/vigil/api.c

#include "api.h"
#include "graphics.h"
#include "mouse.h"
#include "button.h"
#include "kstring.h"
#include "fs.h"
#include "timer.h"
#include "heap.h"
#include "menu.h"
#include "prompt.h"
#include "launcher.h"

// The table itself which is filled in at compile time
//
// "const" puts the table in rodata, and because its built by the
// compiler theres no setup code to run so nothing can forget to
// initialize it
static const vigil_api_t api = {
    .version                = VIGIL_API_VERSION,
    .struct_size            = sizeof(vigil_api_t),

    .gfx_clear              = gfx_clear,
    .gfx_put_pixel          = gfx_put_pixel,
    .gfx_get_pixel          = gfx_get_pixel,
    .gfx_draw_char          = gfx_draw_char,
    .gfx_draw_char_bg       = gfx_draw_char_bg,
    .gfx_draw_string        = gfx_draw_string,
    .gfx_fill_rect          = gfx_fill_rect,
    .gfx_draw_rect_outline  = gfx_draw_rect_outline,
    .gfx_screen_width       = gfx_screen_width,
    .gfx_screen_height      = gfx_screen_height,

    .mouse_hide_cursor      = mouse_hide_cursor,

    .button_draw            = button_draw,
    .button_find_at         = button_find_at,

    .kstrlen                = kstrlen,
    .kstrcmp                = kstrcmp,
    .kuitoa                 = kuitoa,
    .kitoa                  = kitoa,

    .fs_write_file          = fs_write_file,
    .fs_read_file           = fs_read_file,
    .fs_delete_file         = fs_delete_file,
    .fs_make_folder         = fs_make_folder,
    .fs_rename              = fs_rename,
    .fs_stat                = fs_stat,
    .fs_list_dir            = fs_list_dir,

    .timer_get_ticks        = timer_get_ticks,
    .timer_get_frequency    = timer_get_frequency,

    .kmalloc                = kmalloc,
    .kfree                  = kfree,

    .menu_open              = menu_open,
    .prompt_open            = prompt_open,
    .launcher_open_file     = launcher_open_file,
};

const vigil_api_t* api_get(void) {
    return &api;
}
