// apps/launcher/app.h

#ifndef APP_H
#define APP_H
#include <stdint.h>

typedef struct {
    const char* name;

    void (*start)(void);
    void (*redraw)(void);
    
    void (*on_key)(char c);
    void (*on_tick)(void);

    void (*on_click)(int32_t x, int32_t y);
    void (*on_right_click)(int32_t x, int32_t y);
    
    void (*on_menu_pick)(int index);
} app_t;

#endif
