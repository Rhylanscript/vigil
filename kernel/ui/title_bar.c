// kernel/ui/title_bar.c

#include "title_bar.h"
#include "button.h"
#include "graphics.h"
#include "font.h"

#define TITLE_BAR_COLOR       ((gfx_color_t){0x20, 0x20, 0x20})
#define TITLE_BAR_LINE_COLOR  ((gfx_color_t){0x60, 0x60, 0x60})
#define TITLE_TEXT_COLOR      GFX_WHITE
#define TITLE_TEXT_X          12

#define BAR_BUTTON_WIDTH      TITLE_BAR_HEIGHT

#define CLOSE_BUTTON_COLOR    ((gfx_color_t){0xa0, 0x20, 0x20})
#define MINIMIZE_BUTTON_COLOR ((gfx_color_t){0x40, 0x40, 0x40})
#define CLOSE_HOVER_COLOR     ((gfx_color_t){0xd0, 0x40, 0x40})
#define MINIMIZE_HOVER_COLOR  ((gfx_color_t){0x60, 0x60, 0x60})

#define CLOSE_SLOT            0
#define MINIMIZE_SLOT         1

static button_t close_button;
static button_t minimize_button;

static title_bar_hit_t hovered_hit = TITLE_BAR_HIT_NONE;

static void place_button(button_t* button, uint32_t slot, const char* label, gfx_color_t color) {
    uint32_t screen_w = gfx_screen_width();
    uint32_t needed = (slot + 1) * BAR_BUTTON_WIDTH;

    uint32_t x = 0;
    if (screen_w >= needed) {
        x = screen_w - needed;
    }

    button->x = x;
    button->y = 0;
    button->width = BAR_BUTTON_WIDTH;
    button->height = TITLE_BAR_HEIGHT - 1;
    button->label = label;
    button->color = color;
    button->on_click = 0;
}

void title_bar_draw(const char* title) {
    uint32_t screen_w = gfx_screen_width();

    gfx_fill_rect(0, 0, screen_w, TITLE_BAR_HEIGHT, TITLE_BAR_COLOR);
    gfx_fill_rect(0, TITLE_BAR_HEIGHT - 1, screen_w, 1, TITLE_BAR_LINE_COLOR);

    uint32_t text_y = (TITLE_BAR_HEIGHT - FONT_HEIGHT) / 2;
    gfx_draw_string(TITLE_TEXT_X, text_y, title, TITLE_TEXT_COLOR);

    place_button(&close_button, CLOSE_SLOT, "X", (hovered_hit == TITLE_BAR_HIT_CLOSE) ? CLOSE_HOVER_COLOR : CLOSE_BUTTON_COLOR);
    place_button(&minimize_button, MINIMIZE_SLOT, "-", (hovered_hit == TITLE_BAR_HIT_MINIMIZE) ? MINIMIZE_HOVER_COLOR : MINIMIZE_BUTTON_COLOR);

    button_draw(&close_button);
    button_draw(&minimize_button);
}

void title_bar_set_hover(title_bar_hit_t hit) {
    hovered_hit = hit;
}

title_bar_hit_t title_bar_hit_test(int32_t px, int32_t py) {
    if (button_contains(&close_button, px, py)) {
        return TITLE_BAR_HIT_CLOSE;
    }
    if (button_contains(&minimize_button, px, py)) {
        return TITLE_BAR_HIT_MINIMIZE;
    }
    return TITLE_BAR_HIT_NONE;
}
