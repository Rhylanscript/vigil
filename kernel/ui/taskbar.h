// kernel/ui/taskbar.h

#ifndef TASKBAR_H
#define TASKBAR_H
#include <stdint.h>

#define TASKBAR_HEIGHT 32
#define TASKBAR_MAX_ITEMS 8

typedef struct {
    const char* label;
    int highlighted;
} taskbar_item_t;

void taskbar_draw(const taskbar_item_t* items, int count);
int taskbar_hit_test(int32_t px, int32_t py);

#endif
