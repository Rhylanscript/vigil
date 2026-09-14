// boot/shell/shell.c

#include "shell.h"
#include "keyboard.h"
#include "terminal.h"
#include "kstring.h"
#include "memory.h"
#include "fs.h"
#include "timer.h"
#include "heap.h"

#define INPUT_BUFFER_SIZE 128

static char input_buffer[INPUT_BUFFER_SIZE];
static int input_length = 0;

static void print_prompt(void) {
    terminal_print("VIGIL > ");
}

static void execute_command(char* line) {
    if (kstrcmp(line, "") == 0) {
        return;
    }

    int i = 0;
    while (line[i] != '\0' && line[i] != ' ') {
        i++;
    }

    char had_space = (line[i] == ' ');
    line[i] = '\0';

    char* command = line;
    char* args = had_space ? &line[i + 1] : &line[i];

    while (*args == ' ') {
        args++;
    }

    if (kstrcmp(command, "help") == 0) {

        terminal_print("Available Commands:\n");
        terminal_print("    help:                   See available commands\n");
        terminal_print("    erase:                  Clear shell screen\n");
        terminal_print("    recall <text>:          Prints <text> to shell\n");
        terminal_print("    status:                 Show a brief description of project\n");
        terminal_print("    cycles:                 Show the total boot counts in memory\n");
        terminal_print("    uptime:                 Show elapsed time since boot\n");
        terminal_print("    meminfo:                Show heap usage\n");
        terminal_print("    write <name> <text>:    Write <text> text to a specified file\n");
        terminal_print("        under <name>\n");
        terminal_print("    read <name>:            Show contents of file <name>\n");
        terminal_print("    forget <name>:          Removes file <name>\n");
        terminal_print("    files:                  Show files in memory\n\n");

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

    if (kstrcmp(command, "recall") == 0) {
        terminal_print(args);
        terminal_print("\n");
        return;
    }

    if (kstrcmp(command, "cycles") == 0) {
        char count_str[11];
        kuitoa(vigil_memory_get_boot_count(), count_str);
        terminal_print("Boot cycles recorded: ");
        terminal_print(count_str);
        terminal_print("\n");
        return;
    }

    if (kstrcmp(command, "uptime") == 0) {
        uint32_t seconds = timer_get_ticks() / timer_get_frequency();
        char seconds_str[11];
        kuitoa(seconds, seconds_str);
        terminal_print("Uptime: ");
        terminal_print(seconds_str);
        terminal_print(" seconds\n");
        return;
    }

    if (kstrcmp(command, "meminfo") == 0) {
        uint32_t total = 0;
        uint32_t free_bytes = 0;
        heap_get_stats(&total, &free_bytes);

        char total_str[11];
        char free_str[11];
        kuitoa(total, total_str);
        kuitoa(free_bytes, free_str);

        terminal_print("Heap: ");
        terminal_print(free_str);
        terminal_print(" / ");
        terminal_print(total_str);
        terminal_print(" bytes free\n");
        return;
    }

    if (kstrcmp(command, "write") == 0) {
        int j = 0;
        while (args[j] != '\0' && args[j] != ' ') {
            j++;
        }

        if (args[j] != ' ') {
            terminal_print("Usage: write <name> <text>\n");
            return;
        }
        args[j] = '\0';
        char* filename = args;
        char* filedata = &args[j + 1];

        int len = 0;
        while (filedata[len] != '\0') {
            len++;
        }

        int result = fs_write_file(filename, (const uint8_t*) filedata, (uint32_t) len);
        if (result == 0) {
            terminal_print("Saved\n");
        } else {
            terminal_print("Write failed - file table full.\n");
        }
        return;
    }

    if (kstrcmp(command, "read") == 0) {
        char* filename = args;
        static char buffer[512];
        uint32_t out_size = 0;
        int result = fs_read_file(filename, (uint8_t*) buffer, sizeof(buffer) - 1, &out_size);
        if (result == -1) {
            terminal_print("File not found\n");
        } else if (result == -2) {
            terminal_print("File too large to read\n");
        } else {
            buffer[out_size] = '\0';
            terminal_print(buffer);
            terminal_print("\n");
        }
        return;
    }

    if (kstrcmp(command, "forget") == 0) {
        if (kstrcmp(args, "") == 0) {
            terminal_print("Usage: forget <name>\n");
            return;
        }

        int result = fs_delete_file(args);
        if (result == 0) {
            terminal_print("Forgotten\n");
        } else {
            terminal_print("File not found\n");
        }
        return;
    }

    if (kstrcmp(command, "files") == 0) {
        char names[FS_MAX_FILES][FS_MAX_NAME_LEN];
        uint32_t sizes[FS_MAX_FILES];
        int count = fs_list_files(names, sizes);

        if (count == 0) {
            terminal_print("No files\n");
            return;
        }

        for (int i = 0; i < count; i++) {
            terminal_print(names[i]);
            terminal_print(" (");
            char size_str[11];
            kuitoa(sizes[i], size_str);
            terminal_print(size_str);
            terminal_print(" bytes)\n");
        }
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
