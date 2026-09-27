// boot/drivers/graphics.h

#ifndef GRAPHICS_H
#define GRAPHICS_H
#include <stdint.h>

typedef struct {
    uint8_t r;
    uint8_t g;
    uint8_t b;
} gfx_color_t;

#define GFX_BLACK ((gfx_color_t){0x00, 0x00, 0x00})
#define GFX_WHITE ((gfx_color_t){0xff, 0xff, 0xff})
#define GFX_GREEN ((gfx_color_t){0x00, 0xff, 0x00})

void gfx_init(void);

void gfx_put_pixel(uint32_t x, uint32_t y, gfx_color_t color);

void gfx_clear(gfx_color_t color);

void gfx_draw_char(uint32_t x, uint32_t y, char c, gfx_color_t color);

void gfx_fill_rect(uint32_t x, uint32_t y, uint32_t width, uint32_t height, gfx_color_t color);

uint32_t gfx_screen_width(void);
uint32_t gfx_screen_height(void);

void gfx_scroll_up(uint32_t pixel_rows, gfx_color_t fill_color);

#endif
