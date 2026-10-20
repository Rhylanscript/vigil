// apps/hello/hello.c

#include "app.h"
#include "api.h"
#include "font.h"
#include "title_bar.h"
#include "taskbar.h"

#define TEXT_X 40

#define ICON_BG 0xff30a030
#define ICON_FG 0xffffffff

static const vigil_api_t* api = 0;

static int clicks = 0;
static button_t click_button;

static uint32_t icon_pixels[APP_ICON_SIZE * APP_ICON_SIZE];

static void paint(void) {
    uint32_t band = api->gfx_screen_height() - TITLE_BAR_HEIGHT - TASKBAR_HEIGHT;
    api->gfx_fill_rect(0, TITLE_BAR_HEIGHT, api->gfx_screen_width(), band, GFX_BLACK);

    api->gfx_draw_string(TEXT_X, TITLE_BAR_HEIGHT + 40, "Hello from a loaded app!", GFX_GREEN);
    api->gfx_draw_string(TEXT_X, TITLE_BAR_HEIGHT + 64, "I live in a file on the disk, not in the kernel.", GFX_WHITE);

    char number[12];
    api->kuitoa((uint32_t) clicks, number);
    api->gfx_draw_string(TEXT_X, TITLE_BAR_HEIGHT + 104, "Clicks: ", GFX_WHITE);
    api->gfx_draw_string(TEXT_X + 8 * FONT_WIDTH, TITLE_BAR_HEIGHT + 104, number, GFX_WHITE);

    api->button_draw(&click_button);
}

static void icon_box(int x, int y, int w, int h, uint32_t argb) {
    for (int row = y; row < y + h; row++) {
        for (int col = x; col < x + w; col++) {
            icon_pixels[row * APP_ICON_SIZE + col] = argb;
        }
    }
}

static void build_icon(void) {
    icon_box(2, 2, 28, 28, ICON_BG);
    icon_box(8, 8, 4, 16, ICON_FG);
    icon_box(20, 8, 4, 16, ICON_FG);
    icon_box(12, 14, 8, 4, ICON_FG);
}

static void hello_start(void) {
    clicks = 0;

    click_button.x = TEXT_X;
    click_button.y = TITLE_BAR_HEIGHT + 144;
    click_button.width = 200;
    click_button.height = 48;
    click_button.label = "Click me";
    click_button.color = ((gfx_color_t){0x30, 0x70, 0x30});
    click_button.on_click = 0;

    paint();
}

static void hello_redraw(void) {
    paint();
}

static void hello_on_key(char c) {
    if (c == 'r') {
        api->mouse_hide_cursor();
        clicks = 0;
        paint();
    }
}

static void hello_on_click(int32_t x, int32_t y) {
    if (api->button_find_at(&click_button, 1, x, y) >= 0) {
        clicks++;
        paint();
    }
}

int app_entry(const vigil_api_t* kernel_api, app_t* out_app) {
    if (kernel_api->version != VIGIL_API_VERSION || kernel_api->struct_size < sizeof(vigil_api_t)) {
        return 1;
    }
    api = kernel_api;

    build_icon();

    out_app->name = "Hello";
    out_app->start = hello_start;
    out_app->redraw = hello_redraw;
    out_app->on_key = hello_on_key;
    out_app->on_click = hello_on_click;
    out_app->icon = icon_pixels;
    return 0;
}
