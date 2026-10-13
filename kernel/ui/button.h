// kernel/ui/button.h

#ifndef BUTTON_H
#define BUTTON_H
#include <stdint.h>
#include "graphics.h"

typedef void (*button_action_t)(void);

typedef struct {
    uint32_t x;
    uint32_t y;
    uint32_t width;
    uint32_t height;
    const char* label;
    gfx_color_t color;
    button_action_t on_click;
} button_t;

void button_draw(const button_t* button);

int button_contains(const button_t* button, int32_t px, int32_t py);
int button_dispatch_click(const button_t* buttons, int count, int32_t px, int32_t py);
int button_find_at(const button_t* buttons, int count, int32_t px, int32_t py);

#endif
