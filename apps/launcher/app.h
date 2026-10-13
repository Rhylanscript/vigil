// apps/launcher/app.h

#ifndef APP_H
#define APP_H

typedef struct {
    const char* name;

    void (*start)(void);
    void (*redraw)(void);
    
    void (*on_key)(char c);
    void (*on_tick)(void);
} app_t;

#endif
