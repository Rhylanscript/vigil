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

// --- home screen layout (px) ---

#define HOME_TITLE              "VIGIL"
#define HOME_HINT               "Press ESC inside an app to return here"
#define HOME_TITLE_Y            120
#define HOME_FIRST_BUTTON_Y     200
#define HOME_BUTTON_WIDTH       240
#define HOME_BUTTON_HEIGHT      48
#define HOME_BUTTON_GAP         16
#define HOME_HINT_MARGIN        40
#define HOME_BUTTON_COLOR       ((gfx_color_t){0x30, 0x30, 0x30})

// --- apps ---

static void start_terminal(void) {
    terminal_initialize();
    terminal_print("Terminal - press ESC to return to home screen\n");
    shell_init();
}

static const app_t apps[] = {
    { "Terminal", start_terminal, shell_handle_char, terminal_update_cursor },
};

#define APP_COUNT ((int) (sizeof(apps) / sizeof(apps[0])))

static button_t home_buttons[APP_COUNT];

// --- state ---

static const app_t* volatile running_app = 0;

static volatile int home_requested = 0;

// --- drawing ---

static void draw_centered_string(uint32_t y, const char* text, gfx_color_t color) {
    uint32_t text_width = kstrlen(text) * FONT_WIDTH;
    uint32_t x = 0;
    if (text_width < gfx_screen_width()) {
        x = (gfx_screen_width() - text_width) / 2;
    }
    gfx_draw_string(x, y, text, color);
}

static void draw_home_screen(void) {
    mouse_hide_cursor();
    gfx_clear(GFX_BLACK);

    draw_centered_string(HOME_TITLE_Y, HOME_TITLE, GFX_GREEN);

    for (int i = 0; i < APP_COUNT; i++) {
        button_draw(&home_buttons[i]);
    }

    draw_centered_string(gfx_screen_height() - HOME_HINT_MARGIN, HOME_HINT, GFX_WHITE);
}

static void show_home_screen(void) {
    running_app = 0;
    mouse_left_clicked();
    draw_home_screen();
}

static void launch_app(int index) {
    mouse_hide_cursor();
    apps[index].start();
    running_app = &apps[index];
}

// --- keyboard ---

static void launcher_on_key(char c) {
    const app_t* app = running_app;

    if (app == 0) {
        return;
    }

    if (c == KEY_ESCAPE) {
        home_requested = 1;
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
        home_buttons[i].label = apps[i].name;
        home_buttons[i].color = HOME_BUTTON_COLOR;
        home_buttons[i].on_click = 0;
    }

    keyboard_set_handler(launcher_on_key);
    show_home_screen();
}

void launcher_update(void) {
    if (home_requested) {
        home_requested = 0;
        show_home_screen();
        return;
    }

    const app_t* app = running_app;

    if (app != 0) {
        if (app->on_tick != 0) {
            app->on_tick();
        }
        return;
    }

    if (mouse_left_clicked()) {
        int index = button_find_at(home_buttons, APP_COUNT, mouse_get_x(), mouse_get_y());
        if (index >= 0) {
            launch_app(index);
        }
    }
}
