// kernel/ui/taskbar.c

#include "taskbar.h"
#include "button.h"
#include "graphics.h"
#include "font.h"
#include "app.h"

#define TASKBAR_COLOR       ((gfx_color_t){0x20, 0x20, 0x20})
#define TASKBAR_LINE_COLOR  ((gfx_color_t){0x60, 0x60, 0x60})
#define FALLBACK_COLOR      ((gfx_color_t){0x40, 0x40, 0x40})
#define FOCUS_BAR_COLOR     ((gfx_color_t){0x30, 0xd0, 0x30})
#define OPEN_BAR_COLOR      ((gfx_color_t){0x90, 0x90, 0x90})

#define ITEM_WIDTH       48
#define ITEM_GAP         4
#define ITEM_MARGIN      4

#define ICON_PX          APP_ICON_SIZE
#define ICON_TOP         5

#define BAR_TOP          41
#define BAR_HEIGHT       3
#define FOCUS_BAR_WIDTH  28
#define OPEN_BAR_WIDTH   8

static button_t item_buttons[TASKBAR_MAX_ITEMS];
static int item_count = 0;

// --- drawing one item ---

static void draw_item(const button_t* slot, const taskbar_item_t* item) {
    uint32_t icon_x = slot->x + (slot->width - ICON_PX) / 2;
    uint32_t icon_y = slot->y + ICON_TOP;

    if (item->icon != 0) {
        gfx_draw_icon(icon_x, icon_y, item->icon, APP_ICON_SIZE, 1);
    } else {
        // no icon: grey square with the first letter
        gfx_fill_rect(icon_x, icon_y, ICON_PX, ICON_PX, FALLBACK_COLOR);
        gfx_draw_rect_outline(icon_x, icon_y, ICON_PX, ICON_PX, GFX_WHITE);
        gfx_draw_char(
            icon_x + (ICON_PX - FONT_WIDTH) / 2,
            icon_y + (ICON_PX - FONT_HEIGHT) / 2,
            item->label[0],
            GFX_WHITE
        );
    }

    uint32_t bar_width = item->highlighted ? FOCUS_BAR_WIDTH : OPEN_BAR_WIDTH;
    gfx_color_t bar_color = item->highlighted ? FOCUS_BAR_COLOR : OPEN_BAR_COLOR;
    uint32_t bar_x = slot->x + (slot->width - bar_width) / 2;

    gfx_fill_rect(bar_x, slot->y + BAR_TOP, bar_width, BAR_HEIGHT, bar_color);
}

// --- public funcs ---

void taskbar_draw(const taskbar_item_t* items, int count) {
    uint32_t screen_w = gfx_screen_width();
    uint32_t screen_h = gfx_screen_height();

    uint32_t bar_y = 0;
    if (screen_h > TASKBAR_HEIGHT) {
        bar_y = screen_h - TASKBAR_HEIGHT;
    }

    gfx_fill_rect(0, bar_y, screen_w, TASKBAR_HEIGHT, TASKBAR_COLOR);
    gfx_fill_rect(0, bar_y, screen_w, 1, TASKBAR_LINE_COLOR);

    if (count > TASKBAR_MAX_ITEMS) {
        count = TASKBAR_MAX_ITEMS;
    }
    if (count < 0) {
        count = 0;
    }

    item_count = 0;
    uint32_t x = ITEM_MARGIN;

    for (int i = 0; i < count; i++) {
        if (x + ITEM_WIDTH > screen_w) {
            break;
        }

        button_t* slot = &item_buttons[item_count];
        slot->x = x;
        slot->y = bar_y;
        slot->width = ITEM_WIDTH;
        slot->height = TASKBAR_HEIGHT;
        slot->label = items[i].label;
        slot->color = TASKBAR_COLOR;
        slot->on_click = 0;

        draw_item(slot, &items[i]);

        item_count++;
        x += ITEM_WIDTH + ITEM_GAP;
    }
}

int taskbar_hit_test(int32_t px, int32_t py) {
    return button_find_at(item_buttons, item_count, px, py);
}
