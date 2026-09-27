// boot/drivers/font.h

#ifndef FONT_H
#define FONT_H
#include <stdint.h>

#define FONT_WIDTH  8
#define FONT_HEIGHT 16

#define FONT_FIRST_CHAR 32
#define FONT_LAST_CHAR  126
#define FONT_NUM_GLYPHS (FONT_LAST_CHAR - FONT_FIRST_CHAR + 1)

extern const uint8_t font8x16[FONT_NUM_GLYPHS][FONT_HEIGHT];

#endif
