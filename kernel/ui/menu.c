// kernel/ui/menu.c

#include "menu.h"
#include "button.h"
#include "graphics.h"
#include "taskbar.h"
#include "title_bar.h"

#define MENU_WIDTH        160
#define MENU_ITEM_HEIGHT  28
#define MENU_COLOR        ((gfx_color_t){0x30, 0x30, 0x30})

static button_t items[MENU_MAX_ITEMS];
static int item_count = 0;

static int menu_active = 0;

static void draw_menu(void) {
    for (int i = 0; i < item_count; i++) {
        button_draw(&items[i]);
    }
}

void menu_open(int32_t x, int32_t y, const char* const* labels, int count) {
    if (count > MENU_MAX_ITEMS) {
        count = MENU_MAX_ITEMS;
    }
    if (count < 1) {
        return;
    }

    int32_t width = MENU_WIDTH;
    int32_t height = count * MENU_ITEM_HEIGHT;

    int32_t right_limit  = (int32_t) gfx_screen_width();
    int32_t top_limit    = TITLE_BAR_HEIGHT;
    int32_t bottom_limit = (int32_t) gfx_screen_height() - TASKBAR_HEIGHT;

    if (x + width > right_limit) {
        x = right_limit - width;
    }
    if (x < 0) {
        x = 0;
    }
    if (y + height > bottom_limit) {
        y = bottom_limit - height;
    }
    if (y < top_limit) {
        y = top_limit;
    }

    for (int i = 0; i < count; i++) {
        items[i].x = (uint32_t) x;
        items[i].y = (uint32_t) (y + i * MENU_ITEM_HEIGHT);
        items[i].width = MENU_WIDTH;
        items[i].height = MENU_ITEM_HEIGHT;
        items[i].label = labels[i];
        items[i].color = MENU_COLOR;
        items[i].on_click = 0;
    }

    item_count = count;
    menu_active = 1;
    draw_menu();
}

int menu_is_open(void) {
    return menu_active;
}

int menu_click(int32_t px, int32_t py) {
    int pick = button_find_at(items, item_count, px, py);
    menu_active = 0;
    return pick;
}

void menu_close(void) {
    menu_active = 0;
}
