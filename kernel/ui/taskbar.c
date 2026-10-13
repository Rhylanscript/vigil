// kernel/ui/taskbar.c

#include "taskbar.h"
#include "button.h"
#include "graphics.h"

#define TASKBAR_COLOR       ((gfx_color_t){0x20, 0x20, 0x20})
#define TASKBAR_LINE_COLOR  ((gfx_color_t){0x60, 0x60, 0x60})
#define ITEM_COLOR          ((gfx_color_t){0x40, 0x40, 0x40})
#define ITEM_ACTIVE_COLOR   ((gfx_color_t){0x30, 0x70, 0x30})

#define ITEM_WIDTH   140
#define ITEM_GAP     4
#define ITEM_MARGIN  4
#define ITEM_PAD_Y   3

static button_t item_buttons[TASKBAR_MAX_ITEMS];
static int item_count = 0;

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

        button_t* button = &item_buttons[item_count];
        button->x = x;
        button->y = bar_y + ITEM_PAD_Y;
        button->width = ITEM_WIDTH;
        button->height = TASKBAR_HEIGHT - 2 * ITEM_PAD_Y;
        button->label = items[i].label;
        button->color = items[i].highlighted ? ITEM_ACTIVE_COLOR : ITEM_COLOR;
        button->on_click = 0;

        button_draw(button);

        item_count++;
        x += ITEM_WIDTH + ITEM_GAP;
    }
}

int taskbar_hit_test(int32_t px, int32_t py) {
    return button_find_at(item_buttons, item_count, px, py);
}
