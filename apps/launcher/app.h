// apps/launcher/app.h

#ifndef APP_H
#define APP_H
#include <stdint.h>

#define APP_ICON_SIZE 32

typedef struct {
    const char* name;

    void (*start)(void);
    void (*redraw)(void);
    
    void (*on_key)(char c);
    void (*on_tick)(void);

    void (*on_click)(int32_t x, int32_t y);
    void (*on_right_click)(int32_t x, int32_t y);
    
    void (*on_menu_pick)(int index);
    void (*on_prompt_done)(const char* text);

    void (*on_open_file)(const char* path);

    int (*on_close_request)(void);
    void (*on_dialog_done)(int index);

    const uint32_t* icon;
} app_t;

#endif
