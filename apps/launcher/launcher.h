// apps/launcher/launcher.h

#ifndef LAUNCHER_H
#define LAUNCHER_H

#include "app.h"

void launcher_init(void);

void launcher_update(void);

void launcher_open_file(const char* path);

int launcher_register_app(const app_t* app);

#endif
