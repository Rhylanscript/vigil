// boot/shell/shell.c

#include "shell.h"
#include "keyboard.h"
#include "terminal.h"
#include "kstring.h"

#define INPUT_BUFFER_SIZE 128

static char input_buffer[INPUT_BUFFER_SIZE];
static int input_length = 0;

static void print_prompt(void) {
    terminal_print("VIGIL > ");
}

static void execute_command(const char* command) {
    if (kstrcmp(command, "") == 0) {
        return;
    }

    if (kstrcmp(command, "help") == 0) {
        terminal_print("Commands: help, erase, recall <text>, status\n");
        return;
    }

    if (kstrcmp(command, "erase") == 0) {
        terminal_initialize();
        return;
    }

    if (kstrcmp(command, "status") == 0) {
        terminal_print("VIGIL - a small operating system.\n");
        return;
    }

    if (kstrncmp(command, "recall ", 7) == 0) {
        terminal_print(command + 7);
        terminal_print("\n");
        return;
    }

    terminal_print("Unknown command: ");
    terminal_print(command);
    terminal_print("\n");
}

static void shell_handle_char(char c) {
    if (c == '\n') {
        terminal_putchar('\n');
        input_buffer[input_length] = '\0';
        execute_command(input_buffer);
        input_length = 0;
        print_prompt();
        return;
    }

    if (c == '\b') {
        if (input_length > 0) {
            input_length--;
            terminal_putchar('\b');
        }
        return;
    }

    if (input_length < INPUT_BUFFER_SIZE - 1) {
        input_buffer[input_length] = c;
        input_length++;
        terminal_putchar(c);
    }
}

void shell_init(void) {
    input_length = 0;
    terminal_print("\n");
    print_prompt();
    keyboard_set_handler(shell_handle_char);
}
