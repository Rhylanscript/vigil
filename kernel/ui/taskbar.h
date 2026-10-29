// kernel/ui/taskbar.h

#ifndef TASKBAR_H
#define TASKBAR_H
#include <stdint.h>

#define TASKBAR_HEIGHT 48
#define TASKBAR_MAX_ITEMS 8

typedef struct {
    const char* label;
    int highlighted;
    const uint32_t* icon;
} taskbar_item_t;

void taskbar_draw(const taskbar_item_t* items, int count);
int taskbar_hit_test(int32_t px, int32_t py);

void taskbar_set_hover(int slot);
uint32_t taskbar_slot_center_x(int slot);

#endif
