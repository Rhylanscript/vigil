// kernel/ui/dialog.h

#ifndef DIALOG_H
#define DIALOG_H
#include <stdint.h>

#define DIALOG_MAX_BUTTONS 3
#define DIALOG_NO_PICK (-1)

void dialog_open(const char* title, const char* message, const char* const* labels, int count);

int dialog_is_open(void);

int dialog_click(int32_t px, int32_t py);
int dialog_handle_key(char c);

void dialog_close(void);

#endif
