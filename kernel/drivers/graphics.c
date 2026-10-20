// kernel/drivers/graphics.c

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

gfx_color_t gfx_get_pixel(uint32_t x, uint32_t y) {
    if (x >= screen_width || y >= screen_height) {
        return GFX_BLACK;
    }

    uint8_t* row = framebuffer + (y * pitch);
    uint8_t* pixel = row + (x * 3);

    gfx_color_t color;
    color.b = pixel[0];
    color.g = pixel[1];
    color.r = pixel[2];
    return color;
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
void gfx_draw_char_bg(uint32_t x, uint32_t y, char c, gfx_color_t fg, gfx_color_t bg) {
    const uint8_t* glyph = 0;
    if (c >= FONT_FIRST_CHAR && c <= FONT_LAST_CHAR) {
        glyph = font8x16[c - FONT_FIRST_CHAR];
    }

    for (int row = 0; row < FONT_HEIGHT; row++) {
        uint8_t row_bits = (glyph != 0) ? glyph[row] : 0;

        for (int col = 0; col < FONT_WIDTH; col++) {
            uint8_t mask = 0x80 >> col;
            gfx_put_pixel(x + col, y + row, (row_bits & mask) ? fg : bg);
        }
    }
}

void gfx_draw_string(uint32_t x, uint32_t y, const char* str, gfx_color_t color) {
    for (int i = 0; str[i] != '\0'; i++) {
        gfx_draw_char(x + (uint32_t) i * FONT_WIDTH, y, str[i], color);
    }
}

void gfx_fill_rect(uint32_t x, uint32_t y, uint32_t width, uint32_t height, gfx_color_t color) {
    for (uint32_t row = 0; row < height; row++) {
        for (uint32_t col = 0; col < width; col++) {
            gfx_put_pixel(x + col, y + row, color);
        }
    }
}

void gfx_draw_rect_outline(uint32_t x, uint32_t y, uint32_t width, uint32_t height, gfx_color_t color) {
    if (width == 0 || height == 0) {
        return;
    }

    gfx_fill_rect(x, y, width, 1, color);
    gfx_fill_rect(x, y + height - 1, width, 1, color);
    gfx_fill_rect(x, y, 1, height, color);
    gfx_fill_rect(x + width - 1, y, 1, height, color);
}

void gfx_draw_icon(uint32_t x, uint32_t y, const uint32_t* pixels, uint32_t size, uint32_t scale) {
    for (uint32_t row = 0; row < size; row++) {
        for (uint32_t col = 0; col < size; col++) {
            uint32_t argb = pixels[row * size + col];
            if ((argb >> 24) < 128) continue;

            gfx_color_t colour = { (uint8_t) (argb >> 16), (uint8_t) (argb >> 8), (uint8_t) argb };
            gfx_fill_rect(x + col * scale, y + row * scale, scale, scale, colour);
        }
    }
}

void gfx_scroll_region_up(uint32_t top_y, uint32_t region_height, uint32_t pixel_rows, gfx_color_t fill_color) {
    if (top_y >= screen_height) {
        return;
    }

    if (region_height > screen_height - top_y) {
        region_height = screen_height - top_y;
    }

    if (pixel_rows >= region_height) {
        gfx_fill_rect(0, top_y, screen_width, region_height, fill_color);
        return;
    }

    uint32_t bottom_y = top_y + region_height;

    for (uint32_t y = top_y; y < bottom_y - pixel_rows; y++) {
        uint8_t* dst = framebuffer + (y * pitch);
        uint8_t* src = framebuffer + ((y + pixel_rows) * pitch);
        for (uint32_t b = 0; b < pitch; b++) {
            dst[b] = src[b];
        }
    }

    for (uint32_t y = bottom_y - pixel_rows; y < bottom_y; y++) {
        uint8_t* row = framebuffer + (y * pitch);
        for (uint32_t x = 0; x < screen_width; x++) {
            uint8_t* pixel = row + (x * 3);
            pixel[0] = fill_color.b;
            pixel[1] = fill_color.g;
            pixel[2] = fill_color.r;
        }
    }
}

void gfx_scroll_up(uint32_t pixel_rows, gfx_color_t fill_color) {
    gfx_scroll_region_up(0, screen_height, pixel_rows, fill_color);
}

uint32_t gfx_screen_width(void) {
    return screen_width;
}

uint32_t gfx_screen_height(void) {
    return screen_height;
}
