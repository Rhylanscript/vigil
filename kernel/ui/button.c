// kernel/ui/button.c

#include "button.h"
#include "kstring.h"
#include "font.h"

#define BUTTON_BORDER_COLOR GFX_WHITE
#define BUTTON_TEXT_COLOR   GFX_WHITE

void button_draw(const button_t* button) {
    gfx_fill_rect(button->x, button->y, button->width, button->height, button->color);
    gfx_draw_rect_outline(button->x, button->y, button->width, button->height, BUTTON_BORDER_COLOR);

    uint32_t text_width = kstrlen(button->label) * FONT_WIDTH;
    uint32_t text_x = button->x;
    if (text_width < button->width) {
        text_x += (button->width - text_width) / 2;
    }

    uint32_t text_y = button->y;
    if ((uint32_t) FONT_HEIGHT < button->height) {
        text_y += (button->height - FONT_HEIGHT) / 2;
    }

    gfx_draw_string(text_x, text_y, button->label, BUTTON_TEXT_COLOR);
}

int button_contains(const button_t* button, int32_t px, int32_t py) {
    int32_t left    = (int32_t) button->x;
    int32_t top     = (int32_t) button->y;
    int32_t right   = left + (int32_t) button->width;
    int32_t bottom  = top + (int32_t) button->height;

    return px >= left && px < right && py >= top && py < bottom;
}

int button_dispatch_click(const button_t* buttons, int count, int32_t px, int32_t py) {
    for (int i = 0; i < count; i++) {
        if (button_contains(&buttons[i], px, py)) {
            if (buttons[i].on_click != 0) {
                buttons[i].on_click();
            }
            return 1;
        }
    }
    return 0;
}
