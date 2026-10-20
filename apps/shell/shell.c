// apps/shell/shell.c

#include "shell.h"
#include "keyboard.h"
#include "terminal.h"
#include "kstring.h"
#include "kprintf.h"
#include "memory.h"
#include "fs.h"
#include "timer.h"
#include "heap.h"
#include "launcher.h"
#include "api.h"
#include "loader.h"

#define INPUT_BUFFER_SIZE 128
#define HISTORY_SIZE 8

static char input_buffer[INPUT_BUFFER_SIZE];
static int input_length = 0;

static char history[HISTORY_SIZE][INPUT_BUFFER_SIZE];
static char saved_line[INPUT_BUFFER_SIZE];

static int history_total = 0;
static int browse_offset = 0;

static void set_input_line(const char* new_text) {
    while (input_length > 0) {
        terminal_putchar('\b');
        input_length--;
    }

    int i = 0;
    while (new_text[i] != '\0' && i < INPUT_BUFFER_SIZE - 1) {
        input_buffer[i] = new_text[i];
        terminal_putchar(new_text[i]);
        i++;
    }
    input_length = i;
}

static void add_to_history(const char* line) {
    if (kstrcmp(line, "") == 0) {
        return;
    }

    int slot = history_total % HISTORY_SIZE;
    int i = 0;
    while (line[i] != '\0' && i < INPUT_BUFFER_SIZE - 1) {
        history[slot][i] = line[i];
        i++;
    }
    history[slot][i] = '\0';
    history_total++;
}

static void history_recall_previous(void) {
    int available = (history_total < HISTORY_SIZE) ? history_total : HISTORY_SIZE;
    if (browse_offset >= available) {
        // already as far back as possible (nothing older exists)
        return;
    }

    if (browse_offset == 0) {
        int i = 0;
        while (i < input_length) {
            saved_line[i] = input_buffer[i];
            i++;
        }
        saved_line[i] = '\0';
    }

    browse_offset++;
    int slot = (history_total - browse_offset) % HISTORY_SIZE;
    set_input_line(history[slot]);
}

static void history_recall_next(void) {
    if (browse_offset == 0) {
        return;
    }

    browse_offset--;

    if (browse_offset == 0) {
        set_input_line(saved_line);
    } else {
        int slot = (history_total - browse_offset) % HISTORY_SIZE;
        set_input_line(history[slot]);
    }
}

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
        terminal_print("    edit <path>:            Open a file in Notepad\n");
        terminal_print("    folder <path>:          Create a folder\n");
        terminal_print("    rename <path> <name>:   Rename a file or folder\n");
        terminal_print("    files [folder]:         Show what is in a folder\n");
        terminal_print("    api:                    Show info about the app API table\n");
        terminal_print("    loadapp <path>:         Load an app file from disk\n\n");

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
        kprintf("Boot cycles recorded: %d\n", vigil_memory_get_boot_count());
        return;
    }

    if (kstrcmp(command, "uptime") == 0) {
        uint32_t seconds = timer_get_ticks() / timer_get_frequency();
        kprintf("Uptime: %d seconds\n", seconds);
        return;
    }

    if (kstrcmp(command, "meminfo") == 0) {
        uint32_t total = 0;
        uint32_t free_bytes = 0;
        heap_get_stats(&total, &free_bytes);

        kprintf("Heap: %d / %d bytes free\n", free_bytes, total);
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
        if (result == FS_OK) {
            terminal_print("Saved\n");
        } else if (result == FS_ERR_FULL) {
            terminal_print("Write failed - file table full.\n");
        } else if (result == FS_ERR_IS_DIR) {
            terminal_print("Write failed - that name is a folder.\n");
        } else {
            terminal_print("Write failed - bad path, long name or missing folder.\n");
        }
        return;
    }

    if (kstrcmp(command, "read") == 0) {
        char* filename = args;
        static char buffer[512];
        uint32_t out_size = 0;
        int result = fs_read_file(filename, (uint8_t*) buffer, sizeof(buffer) - 1, &out_size);
        if (result == FS_ERR_NOT_FOUND) {
            terminal_print("File not found\n");
        } else if (result == FS_ERR_TOO_BIG) {
            terminal_print("File too large to read\n");
        } else if (result != FS_OK) {
            terminal_print("Cannot read that (is it a folder?)\n");
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
        if (result == FS_OK) {
            terminal_print("Forgotten\n");
        } else if (result == FS_ERR_NOT_EMPTY) {
            terminal_print("Folder is not empty\n");
        } else {
            terminal_print("File not found\n");
        }
        return;
    }

    if (kstrcmp(command, "edit") == 0) {
        if (kstrcmp(args, "") == 0) {
            terminal_print("Usage: edit <path>\n");
            return;
        }

        launcher_open_file(args);
        terminal_print("Opening in Notepad...\n");
        return;
    }

    if (kstrcmp(command, "folder") == 0) {
        if (kstrcmp(args, "") == 0) {
            terminal_print("Usage: folder <path>\n");
            return;
        }

        int result = fs_make_folder(args);
        if (result == FS_OK) {
            terminal_print("Folder created\n");
        } else if (result == FS_ERR_EXISTS) {
            terminal_print("Something with that name already exists\n");
        } else if (result == FS_ERR_FULL) {
            terminal_print("Cannot create folder - file table full.\n");
        } else {
            terminal_print("Cannot create folder - bad path, long name or missing folder.\n");
        }
        return;
    }

    if (kstrcmp(command, "rename") == 0) {
        int j = 0;
        while (args[j] != '\0' && args[j] != ' ') {
            j++;
        }

        if (args[j] != ' ') {
            terminal_print("Usage: rename <path> <name>\n");
            return;
        }
        args[j] = '\0';
        char* old_path = args;
        char* new_name = &args[j + 1];
        while (*new_name == ' ') {
            new_name++;
        }

        int result = fs_rename(old_path, new_name);
        if (result == FS_OK) {
            terminal_print("Renamed\n");
        } else if (result == FS_ERR_NOT_FOUND) {
            terminal_print("File not found\n");
        } else if (result == FS_ERR_EXISTS) {
            terminal_print("Something with that name already exists\n");
        } else {
            terminal_print("Bad name (1-15 characters, no '/')\n");
        }
        return;
    }

    if (kstrcmp(command, "files") == 0) {
        fs_info_t entries[FS_MAX_FILES];
        int count = fs_list_dir(args, entries);

        if (count < 0) {
            terminal_print("No such folder\n");
            return;
        }
        if (count == 0) {
            terminal_print("No files\n");
            return;
        }

        for (int i = 0; i < count; i++) {
            if (entries[i].is_dir) {
                kprintf("%s/ (folder)\n", entries[i].name);
            } else {
                kprintf("%s (%d bytes)\n", entries[i].name, entries[i].size_bytes);
            }
        }
        return;
    }
    
    if (kstrcmp(command, "api") == 0) {
        const vigil_api_t* api = api_get();

        kprintf("API version: %d, table size: %d bytes\n", api->version, api->struct_size);
        kprintf("kstrlen(\"vigil\") via table: %d\n", api->kstrlen("vigil"));
        kprintf("timer ticks via table: %d\n", api->timer_get_ticks());
        return;
    }

    if (kstrcmp(command, "loadapp") == 0) {
        if (kstrcmp(args, "") == 0) {
            terminal_print("Usage: loadapp <path>\n");
            return;
        }

        int result = loader_load(args);
        if (result == LOADER_OK) {
            terminal_print("App loaded - find it on the home screen\n");
        } else {
            terminal_print("Load failed: ");
            terminal_print(loader_error_text(result));
            terminal_print("\n");
        }
        return;
    }

    terminal_print("Unknown command: ");
    terminal_print(command);
    terminal_print("\n");
}

void shell_handle_char(char c) {
    if (c == KEY_ARROW_UP) {
        history_recall_previous();
        return;
    }

    if (c == KEY_ARROW_DOWN) {
        history_recall_next();
        return;
    }

    if (c == KEY_CTRL_S || c == KEY_ARROW_LEFT || c == KEY_ARROW_RIGHT || c == KEY_HOME || c == KEY_END || c == KEY_DELETE) {
        return;
    }

    browse_offset = 0;

    if (c == '\n') {
        terminal_putchar('\n');
        input_buffer[input_length] = '\0';
        add_to_history(input_buffer);
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
    browse_offset = 0;
    terminal_print("\n");
    print_prompt();
}
