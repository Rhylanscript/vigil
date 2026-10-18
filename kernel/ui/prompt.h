// kernel/ui/prompt.h

#ifndef PROMPT_H
#define PROMPT_H

#define PROMPT_BUFFER_SIZE 40

typedef enum {
    PROMPT_CONTINUE = 0,
    PROMPT_ACCEPTED,
    PROMPT_CANCELLED
} prompt_result_t;

void prompt_open(const char* title, const char* initial_text, int max_length, const char* blocked_chars);

int prompt_is_open(void);

prompt_result_t prompt_handle_key(char c);

const char* prompt_text(void);

void prompt_close(void);

#endif
