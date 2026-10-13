// kernel/ui/title_bar.h

#ifndef TITLE_BAR_H
#define TITLE_BAR_H
#include <stdint.h>

#define TITLE_BAR_HEIGHT 32

typedef enum {
    TITLE_BAR_HIT_NONE = 0,
    TITLE_BAR_HIT_CLOSE,
    TITLE_BAR_HIT_MINIMIZE,
} title_bar_hit_t;

void title_bar_draw(const char* title);

title_bar_hit_t title_bar_hit_test(int32_t px, int32_t py);

#endif
