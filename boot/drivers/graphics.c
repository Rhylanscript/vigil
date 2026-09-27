// boot/drivers/graphics.c

#include "graphics.h"
#include "bootinfo.h"
#include "font.h"

static uint8_t* framebuffer;
static uint32_t pitch;
static uint32_t screen_width;
static uint32_t screen_height;

void gfx_init(void) {
    boot_info_t* info = boot_info();
    framebuffer = (uint8_t*) info->framebuffer_addr;
    pitch = info->pitch;
    screen_width = info->width;
    screen_height = info->height;
}

void gfx_put_pixel(uint32_t x, uint32_t y, gfx_color_t color) {
    if (x >= screen_width || y >= screen_height) {
        return;
    }

    uint8_t* row = framebuffer + (y * pitch);

    uint8_t* pixel = row + (x * 3);
    pixel[0] = color.b;
    pixel[1] = color.g;
    pixel[2] = color.r;
}

void gfx_clear(gfx_color_t color) {
    for (uint32_t y = 0; y < screen_height; y++) {
        for (uint32_t x = 0; x < screen_width; x++) {
            gfx_put_pixel(x, y, color);
        }
    }
}

void gfx_draw_char(uint32_t x, uint32_t y, char c, gfx_color_t color) {
    if (c < FONT_FIRST_CHAR || c > FONT_LAST_CHAR) {
        return;
    }

    const uint8_t* glyph = font8x16[c - FONT_FIRST_CHAR];

    for (int row = 0; row < FONT_HEIGHT; row++) {
        uint8_t row_bits = glyph[row];
        for (int col = 0; col < FONT_WIDTH; col++) {
            uint8_t mask = 0x80 >> col;
            if (row_bits & mask) {
                gfx_put_pixel(x + col, y + row, color);
            }
        }
    }
}

void gfx_fill_rect(uint32_t x, uint32_t y, uint32_t width, uint32_t height, gfx_color_t color) {
    for (uint32_t row = 0; row < height; row++) {
        for (uint32_t col = 0; col < width; col++) {
            gfx_put_pixel(x + col, y + row, color);
        }
    }
}

void gfx_scroll_up(uint32_t pixel_rows, gfx_color_t fill_color) {
    if (pixel_rows >= screen_height) {
        gfx_clear(fill_color);
        return;
    }

    for (uint32_t y = 0; y < screen_height - pixel_rows; y++) {
        uint8_t* dst = framebuffer + (y * pitch);
        uint8_t* src = framebuffer + ((y + pixel_rows) * pitch);
        for (uint32_t b = 0; b < pitch; b++) {
            dst[b] = src[b];
        }
    }

    for (uint32_t y = screen_height - pixel_rows; y < screen_height; y++) {
        for (uint32_t x = 0; x < screen_width; x++) {
            gfx_put_pixel(x, y, fill_color);
        }
    }
}

uint32_t gfx_screen_width(void) {
    return screen_width;
}

uint32_t gfx_screen_height(void) {
    return screen_height;
}
