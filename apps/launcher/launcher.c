// apps/launcher/launcher.c

#include "launcher.h"
#include "app.h"
#include "button.h"
#include "graphics.h"
#include "font.h"
#include "kstring.h"
#include "keyboard.h"
#include "mouse.h"
#include "terminal.h"
#include "title_bar.h"
#include "taskbar.h"
#include "menu.h"
#include "fs.h"
#include "prompt.h"
#include "loader.h"
#include "shell.h"
#include "shell_icon.h"

// --- home screen layout (px) ---

#define HOME_TITLE              "VIGIL"
#define HOME_HINT               "Click '-' to minimize an app, click 'X' to close it"

#define HOME_TITLE_Y            120
#define HOME_FIRST_BUTTON_Y     200
#define HOME_HINT_MARGIN        (TASKBAR_HEIGHT + 40)
#define HOME_BUTTON_COLOR       ((gfx_color_t){0x30, 0x30, 0x30})
#define HOME_RUNNING_COLOR      GFX_GREEN

#define HOME_COLUMNS            4
#define HOME_TILE_WIDTH         128
#define HOME_TILE_HEIGHT        112
#define HOME_TILE_GAP           24
#define HOME_TILE_PAD           8

#define HOME_ICON_SCALE         2
#define HOME_ICON_PX            (APP_ICON_SIZE * HOME_ICON_SCALE)
#define HOME_LABEL_GAP          12

#define HOME_BUTTON_WIDTH       240
#define HOME_BUTTON_HEIGHT      48
#define HOME_BUTTON_GAP         16

// --- apps ---

static void start_terminal(void) {
    terminal_set_top_margin(TITLE_BAR_HEIGHT);
    terminal_set_bottom_margin(TASKBAR_HEIGHT);
    terminal_initialize();
    terminal_print("Terminal - '-' minimizes, 'X' closes\n");
    shell_init();
}

static const app_t builtin_apps[] = {
    {
        .name = "Terminal",
        .start = start_terminal,
        .redraw = terminal_redraw,
        .on_key = shell_handle_char,
        .on_tick = terminal_update_cursor,
        .icon = shell_icon,
    },
};

#define BUILTIN_COUNT ((int) (sizeof(builtin_apps) / sizeof(builtin_apps[0])))
#define MAX_APPS TASKBAR_MAX_ITEMS

typedef char builtin_apps_must_fit[(BUILTIN_COUNT <= MAX_APPS) ? 1 : -1];

static app_t apps[MAX_APPS];
static int app_count = 0;

static button_t home_buttons[MAX_APPS];

// --- state ---

typedef enum {
    APP_STATE_STOPPED = 0,
    APP_STATE_FOREGROUND,
    APP_STATE_MINIMIZED,
} app_state_t;

static app_state_t app_state[MAX_APPS];

static const app_t* volatile running_app = 0;

static int taskbar_slot_app[MAX_APPS];

// --- helpers ---

static int app_index(const app_t* app) {
    return (int) (app - apps);
}

static void copy_app(app_t* dest, const app_t* src) {
    dest->name = src->name;
    dest->start = src->start;
    dest->redraw = src->redraw;
    dest->on_key = src->on_key;
    dest->on_tick = src->on_tick;
    dest->on_click = src->on_click;
    dest->on_right_click = src->on_right_click;
    dest->on_menu_pick = src->on_menu_pick;
    dest->on_prompt_done = src->on_prompt_done;
    dest->on_open_file = src->on_open_file;
    dest->icon = src->icon;
}

static void place_home_button(int index) {
    int column = index % HOME_COLUMNS;
    int row = index / HOME_COLUMNS;

    uint32_t grid_width = HOME_COLUMNS * HOME_TILE_WIDTH + (HOME_COLUMNS - 1) * HOME_TILE_GAP;
    uint32_t grid_x = 0;
    if (gfx_screen_width() > grid_width) {
        grid_x = (gfx_screen_width() - grid_width) / 2;
    }

    home_buttons[index].x = grid_x + (uint32_t) column * (HOME_TILE_WIDTH + HOME_TILE_GAP);
    home_buttons[index].y = HOME_FIRST_BUTTON_Y + (uint32_t) row * (HOME_TILE_HEIGHT + HOME_TILE_GAP);
    home_buttons[index].width = HOME_TILE_WIDTH;
    home_buttons[index].height = HOME_TILE_HEIGHT;
    home_buttons[index].label = apps[index].name;
    home_buttons[index].color = HOME_BUTTON_COLOR;
    home_buttons[index].on_click = 0;
}

int launcher_register_app(const app_t* app) {
    if (app == 0 || app_count >= MAX_APPS) {
        return -1;
    }

    int index = app_count;

    copy_app(&apps[index], app);
    app_state[index] = APP_STATE_STOPPED;
    place_home_button(index);

    app_count++;
    return index;
}

// --- drawing ---

static void draw_centered_string(uint32_t y, const char* text, gfx_color_t color) {
    uint32_t text_width = kstrlen(text) * FONT_WIDTH;
    uint32_t x = 0;
    if (text_width < gfx_screen_width()) {
        x = (gfx_screen_width() - text_width) / 2;
    }
    gfx_draw_string(x, y, text, color);
}

static void draw_taskbar(void) {
    taskbar_item_t items[MAX_APPS];
    int count = 0;

    for (int i = 0; i < app_count; i++) {
        if (app_state[i] == APP_STATE_STOPPED) {
            continue;
        }
        items[count].label = apps[i].name;
        items[count].highlighted = (app_state[i] == APP_STATE_FOREGROUND);
        items[count].icon = apps[i].icon;
        taskbar_slot_app[count] = i;
        count++;
    }

    taskbar_draw(items, count);
}

static void draw_home_tile(int index) {
    const button_t* tile = &home_buttons[index];
    const char* name = apps[index].name;

    uint32_t icon_x = tile->x + (tile->width - HOME_ICON_PX) / 2;
    uint32_t icon_y = tile->y + HOME_TILE_PAD;

    if (apps[index].icon != 0) {
        gfx_draw_icon(icon_x, icon_y, apps[index].icon, APP_ICON_SIZE, HOME_ICON_SCALE);
    } else {
        gfx_fill_rect(icon_x, icon_y, HOME_ICON_PX, HOME_ICON_PX, HOME_BUTTON_COLOR);
        gfx_draw_rect_outline(icon_x, icon_y, HOME_ICON_PX, HOME_ICON_PX, GFX_WHITE);
        gfx_draw_char(
            icon_x + (HOME_ICON_PX - FONT_WIDTH) / 2,
            icon_y + (HOME_ICON_PX - FONT_HEIGHT) / 2,
            name[0],
            GFX_WHITE
        );
    }

    uint32_t text_width = kstrlen(name) * FONT_WIDTH;
    uint32_t text_x = tile->x;
    if (text_width < tile->width) {
        text_x += (tile->width - text_width) / 2;
    }

    gfx_color_t label_color = GFX_WHITE;
    if (app_state[index] != APP_STATE_STOPPED) {
        label_color = HOME_RUNNING_COLOR;
    }
    gfx_draw_string(text_x, icon_y + HOME_ICON_PX + HOME_LABEL_GAP, name, label_color);
}

static void draw_home_screen(void) {
    mouse_hide_cursor();
    gfx_clear(GFX_BLACK);

    draw_centered_string(HOME_TITLE_Y, HOME_TITLE, GFX_GREEN);
    draw_taskbar();

    for (int i = 0; i < app_count; i++) {
        draw_home_tile(i);
    }

    draw_centered_string(gfx_screen_height() - HOME_HINT_MARGIN, HOME_HINT, GFX_WHITE);
}

static void show_home_screen(void) {
    running_app = 0;
    menu_close();
    prompt_close();
    mouse_left_clicked();
    draw_home_screen();
}

// --- switching between apps ---

static void launch_app(int index) {
    mouse_hide_cursor();

    if (app_state[index] == APP_STATE_MINIMIZED && apps[index].redraw != 0) {
        apps[index].redraw();
    } else {
        apps[index].start();
    }

    app_state[index] = APP_STATE_FOREGROUND;

    title_bar_draw(apps[index].name);
    draw_taskbar();

    running_app = &apps[index];
}

static void close_foreground_app(void) {
    const app_t* app = running_app;
    if (app == 0) {
        return;
    }

    app_state[app_index(app)] = APP_STATE_STOPPED;
    show_home_screen();
}

static void minimize_foreground_app(void) {
    const app_t* app = running_app;
    if (app == 0) {
        return;
    }

    app_state[app_index(app)] = APP_STATE_MINIMIZED;
    show_home_screen();
}

static void switch_to_app(int index) {
    const app_t* current = running_app;

    if (current != 0) {
        int current_index = app_index(current);

        if (current_index == index) {
            minimize_foreground_app();
            return;
        }

        app_state[current_index] = APP_STATE_MINIMIZED;
        running_app = 0;
    }

    launch_app(index);
}

// --- opening files in notepad ---

static char pending_open_path[FS_MAX_PATH];
static int open_pending = 0;

static int find_app_index(const char* name) {
    for (int i = 0; i < app_count; i++) {
        if (kstrcmp(apps[i].name, name) == 0) {
            return i;
        }
    }
    return -1;
}

static void bring_to_front(int index) {
    const app_t* current = running_app;

    if (current != 0) {
        int current_index = app_index(current);
        if (current_index == index) {
            return;
        }
        app_state[current_index] = APP_STATE_MINIMIZED;
        running_app = 0;
    }

    launch_app(index);
}

static void process_pending_open(void) {
    if (!open_pending) {
        return;
    }
    open_pending = 0;

    int index = find_app_index("Notepad");
    if (index < 0 || apps[index].on_open_file == 0) {
        return;
    }

    bring_to_front(index);
    apps[index].on_open_file(pending_open_path);
}

void launcher_open_file(const char* path) {
    int i = 0;
    while (path[i] != '\0' && i < FS_MAX_PATH - 1) {
        pending_open_path[i] = path[i];
        i++;
    }
    pending_open_path[i] = '\0';

    open_pending = 1;
}

// --- context menu ---

static void redraw_running_app(void) {
    const app_t* app = running_app;

    if (app != 0 && app->redraw != 0) {
        app->redraw();
    }
}

static void dismiss_menu(void) {
    mouse_hide_cursor();
    menu_close();
    redraw_running_app();
}

static void handle_menu_click(int32_t x, int32_t y) {
    mouse_hide_cursor();

    int pick = menu_click(x, y);
    redraw_running_app();

    const app_t* app = running_app;
    if (pick != MENU_NO_PICK && app != 0 && app->on_menu_pick != 0) {
        app->on_menu_pick(pick);
    }
}

// --- name prompt ---

static void handle_prompt_key(char c) {
    mouse_hide_cursor();

    prompt_result_t result = prompt_handle_key(c);
    if (result == PROMPT_CONTINUE) {
        return;
    }

    redraw_running_app();
    const app_t* app = running_app;
    if (result == PROMPT_ACCEPTED && app != 0 && app->on_prompt_done != 0) {
        app->on_prompt_done(prompt_text());
    }
}

// --- keyboard ---

static void launcher_handle_key(char c) {
    const app_t* app = running_app;

    if (app == 0) {
        return;
    }
    
    if (prompt_is_open()) {
        handle_prompt_key(c);
        return;
    }

    if (menu_is_open()) {
        if (c == KEY_ESCAPE) {
            dismiss_menu();
        }
        return;
    }

    if (c == KEY_ESCAPE) {
        minimize_foreground_app();
        return;
    }

    if (app->on_key != 0) {
        app->on_key(c);
    }
}

static void drain_key_queue(void) {
    char c;
    while (keyboard_pop(&c)) {
        launcher_handle_key(c);
    }
}

// --- public functions & stuff™ ---

static int click_is_in_app_band(int32_t py) {
    int32_t top = TITLE_BAR_HEIGHT;
    int32_t bottom = (int32_t) gfx_screen_height() - TASKBAR_HEIGHT;
    return py >= top && py < bottom;
}

void launcher_init(void) {
    app_count = 0;

    for (int i = 0; i < BUILTIN_COUNT; i++) {
        launcher_register_app(&builtin_apps[i]);
    }

    loader_load_folder(LOADER_APP_FOLDER);
    show_home_screen();
}

void launcher_update(void) {
    drain_key_queue();
    process_pending_open();

    int clicked = mouse_left_clicked();
    int right_clicked = mouse_right_clicked();
    int32_t mouse_x = mouse_get_x();
    int32_t mouse_y = mouse_get_y();

    if (prompt_is_open()) {
        return;
    }

    if (menu_is_open()) {
        if (clicked) {
            handle_menu_click(mouse_x, mouse_y);
        } else if (right_clicked) {
            dismiss_menu();
        }
        return;
    }

    if (clicked) {
        int slot = taskbar_hit_test(mouse_x, mouse_y);
        if (slot >= 0) {
            switch_to_app(taskbar_slot_app[slot]);
            return;
        }
    }

    const app_t* app = running_app;

    if (app != 0) {
        if (clicked) {
            title_bar_hit_t hit = title_bar_hit_test(mouse_x, mouse_y);

            if (hit == TITLE_BAR_HIT_CLOSE) {
                close_foreground_app();
                return;
            }
            if (hit == TITLE_BAR_HIT_MINIMIZE) {
                minimize_foreground_app();
                return;
            }
            if (app->on_click != 0 && click_is_in_app_band(mouse_y)) {
                mouse_hide_cursor();
                app->on_click(mouse_x, mouse_y);
            }
        }

        if (right_clicked) {
            if (app->on_right_click != 0 && click_is_in_app_band(mouse_y)) {
                mouse_hide_cursor();
                app->on_right_click(mouse_x, mouse_y);
            }
        }

        if (app->on_tick != 0) {
            app->on_tick();
        }
        return;
    }

    if (clicked) {
        int index = button_find_at(home_buttons, app_count, mouse_x, mouse_y);
        if (index >= 0) {
            launch_app(index);
        }
    }
}
