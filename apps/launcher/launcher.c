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
#include "shell.h"
#include "title_bar.h"
#include "taskbar.h"

// --- home screen layout (px) ---

#define HOME_TITLE              "VIGIL"
#define HOME_HINT               "Click '-' to minimize an app, click 'X' to close it"
#define HOME_TITLE_Y            120
#define HOME_FIRST_BUTTON_Y     200
#define HOME_BUTTON_WIDTH       240
#define HOME_BUTTON_HEIGHT      48
#define HOME_BUTTON_GAP         16
#define HOME_HINT_MARGIN        (TASKBAR_HEIGHT + 40)
#define HOME_BUTTON_COLOR       ((gfx_color_t){0x30, 0x30, 0x30})

#define HOME_LABEL_MAX          32
#define HOME_RUNNING_SUFFIX     " (running)"

// --- apps ---

static void start_terminal(void) {
    terminal_set_top_margin(TITLE_BAR_HEIGHT);
    terminal_set_bottom_margin(TASKBAR_HEIGHT);
    terminal_initialize();
    terminal_print("Terminal - '-' minimizes, 'X' closes\n");
    shell_init();
}

static const app_t apps[] = {
    { "Terminal", start_terminal, terminal_redraw, shell_handle_char, terminal_update_cursor },
};

#define APP_COUNT ((int) (sizeof(apps) / sizeof(apps[0])))

static button_t home_buttons[APP_COUNT];

// --- state ---

typedef enum {
    APP_STATE_STOPPED = 0,
    APP_STATE_FOREGROUND,
    APP_STATE_MINIMIZED,
} app_state_t;

static app_state_t app_state[APP_COUNT];

static const app_t* volatile running_app = 0;

static volatile int minimize_requested = 0;

static char home_labels[APP_COUNT][HOME_LABEL_MAX];

static int taskbar_slot_app[APP_COUNT];

// --- helpers ---

static int app_index(const app_t* app) {
    return (int) (app - apps);
}

static void build_home_label(int index) {
    char* out = home_labels[index];
    const char* name = apps[index].name;
    int pos = 0;

    for (int i = 0; name[i] != '\0' && pos < HOME_LABEL_MAX - 1; i++) {
        out[pos] = name[i];
        pos++;
    }

    if (app_state[index] != APP_STATE_STOPPED) {
        const char* suffix = HOME_RUNNING_SUFFIX;
        for (int i = 0; suffix[i] != '\0' && pos < HOME_LABEL_MAX - 1; i++) {
            out[pos] = suffix[i];
            pos++;
        }
    }
    out[pos] = '\0';
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
    taskbar_item_t items[APP_COUNT];
    int count = 0;

    for (int i = 0; i < APP_COUNT; i++) {
        if (app_state[i] == APP_STATE_STOPPED) {
            continue;
        }
        items[count].label = apps[i].name;
        items[count].highlighted = (app_state[i] == APP_STATE_FOREGROUND);
        taskbar_slot_app[count] = i;
        count++;
    }

    taskbar_draw(items, count);
}

static void draw_home_screen(void) {
    mouse_hide_cursor();
    gfx_clear(GFX_BLACK);

    draw_centered_string(HOME_TITLE_Y, HOME_TITLE, GFX_GREEN);
    draw_taskbar();

    for (int i = 0; i < APP_COUNT; i++) {
        build_home_label(i);
        button_draw(&home_buttons[i]);
    }

    draw_centered_string(gfx_screen_height() - HOME_HINT_MARGIN, HOME_HINT, GFX_WHITE);
}

static void show_home_screen(void) {
    running_app = 0;
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

// --- keyboard ---

static void launcher_on_key(char c) {
    const app_t* app = running_app;

    if (app == 0) {
        return;
    }

    if (c == KEY_ESCAPE) {
        minimize_requested = 1;
        return;
    }

    if (app->on_key != 0) {
        app->on_key(c);
    }
}

// --- public functions & stuff™ ---

void launcher_init(void) {
    uint32_t button_x = (gfx_screen_width() - HOME_BUTTON_WIDTH) / 2;

    for (int i = 0; i < APP_COUNT; i++) {
        home_buttons[i].x = button_x;
        home_buttons[i].y = HOME_FIRST_BUTTON_Y + (uint32_t) i * (HOME_BUTTON_HEIGHT + HOME_BUTTON_GAP);
        home_buttons[i].width = HOME_BUTTON_WIDTH;
        home_buttons[i].height = HOME_BUTTON_HEIGHT;
        home_buttons[i].label = home_labels[i];
        home_buttons[i].color = HOME_BUTTON_COLOR;
        home_buttons[i].on_click = 0;
    }

    keyboard_set_handler(launcher_on_key);
    show_home_screen();
}

void launcher_update(void) {
    if (minimize_requested) {
        minimize_requested = 0;
        minimize_foreground_app();
        return;
    }

    int clicked = mouse_left_clicked();
    int32_t mouse_x = mouse_get_x();
    int32_t mouse_y = mouse_get_y();

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
        }

        if (app->on_tick != 0) {
            app->on_tick();
        }
        return;
    }

    if (clicked) {
        int index = button_find_at(home_buttons, APP_COUNT, mouse_x, mouse_y);
        if (index >= 0) {
            launch_app(index);
        }
    }
}
