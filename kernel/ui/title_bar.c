// kernel/ui/title_bar.c

#include "title_bar.h"
#include "button.h"
#include "graphics.h"
#include "font.h"

#define TITLE_BAR_COLOR         ((gfx_color_t){0x20, 0x20, 0x20})
#define TITLE_BAR_LINE_COLOR    ((gfx_color_t){0x60, 0x60, 0x60})
#define TITLE_TEXT_COLOR        GFX_WHITE
#define TITLE_TEXT_X            12

#define CLOSE_BUTTON_COLOR      ((gfx_color_t){0xa0, 0x20, 0x20})
#define CLOSE_BUTTON_WIDTH      TITLE_BAR_HEIGHT

static button_t close_button;

static void layout_buttons(void) {
    uint32_t screen_w = gfx_screen_width();

    uint32_t x = 0;
    if (screen_w > CLOSE_BUTTON_WIDTH) {
        x = screen_w - CLOSE_BUTTON_WIDTH;
    }

    close_button.x = x;
    close_button.y = 0;
    close_button.width = CLOSE_BUTTON_WIDTH;
    close_button.height = TITLE_BAR_HEIGHT - 1;
    close_button.label = "X";
    close_button.color = CLOSE_BUTTON_COLOR;
    close_button.on_click = 0;
}

void title_bar_draw(const char* title) {
    uint32_t screen_w = gfx_screen_width();

    gfx_fill_rect(0, 0, screen_w, TITLE_BAR_HEIGHT, TITLE_BAR_COLOR);
    gfx_fill_rect(0, TITLE_BAR_HEIGHT - 1, screen_w, 1, TITLE_BAR_LINE_COLOR);

    uint32_t text_y = (TITLE_BAR_HEIGHT - FONT_HEIGHT) / 2;
    gfx_draw_string(TITLE_TEXT_X, text_y, title, TITLE_TEXT_COLOR);

    layout_buttons();
    button_draw(&close_button);
}

title_bar_hit_t title_bar_hit_test(int32_t px, int32_t py) {
    if (button_contains(&close_button, px, py)) {
        return TITLE_BAR_HIT_CLOSE;
    }
    return TITLE_BAR_HIT_NONE;
}
