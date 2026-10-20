// apps/files/files.c

#include "app.h"
#include "api.h"
#include "font.h"
#include "title_bar.h"
#include "taskbar.h"

// --- api pointer ---

static const vigil_api_t* api = 0;

// --- layout (pixels) ---

#define FILES_TOP        TITLE_BAR_HEIGHT
#define FILES_BOTTOM     TASKBAR_HEIGHT

#define HEADER_HEIGHT    24
#define FOOTER_HEIGHT    24
#define ROW_HEIGHT       20

#define TEXT_X           12
#define SIZE_COLUMN_X    360

#define BAR_COLOR        ((gfx_color_t){0x20, 0x20, 0x20})
#define LIST_COLOR       GFX_BLACK
#define FILE_COLOR       GFX_WHITE
#define FOLDER_COLOR     ((gfx_color_t){0x60, 0xc0, 0xff})
#define MESSAGE_COLOR    ((gfx_color_t){0xff, 0xd0, 0x40})

#define HINT_TEXT        "Click to open  |  Right-click for more options"

#define MESSAGE_MAX      112

#define PARENT_ENTRY     (-1)

// --- the right-click menu ---

typedef enum {
    MENU_KIND_ITEM = 0,
    MENU_KIND_BACKGROUND
} menu_kind_t;

typedef enum {
    NEW_FOLDER = 0,
    NEW_FILE,
    NEW_ACTION_COUNT
} new_action_t;

static const char* const new_menu_labels[NEW_ACTION_COUNT] = {
    "New folder",
    "New file"
};

typedef enum {
    ACTION_OPEN = 0,
    ACTION_RENAME,
    ACTION_FORGET,
    ACTION_PROPERTIES,
    ACTION_COUNT
} menu_action_t;

static const char* const menu_labels[ACTION_COUNT] = {
    "Open",
    "Rename",
    "Forget",
    "Properties"
};

// --- state ---

static char current_path[FS_MAX_PATH];

static fs_info_t entries[FS_MAX_FILES];
static int entry_count = 0;

static char message[MESSAGE_MAX];

static char target_path[FS_MAX_PATH];
static char target_name[FS_MAX_NAME_LEN];
static int target_is_dir = 0;

typedef enum {
    PURPOSE_NONE = 0,
    PURPOSE_RENAME,
    PURPOSE_NEW_FOLDER,
    PURPOSE_NEW_FILE
} prompt_purpose_t;

static prompt_purpose_t prompt_purpose = PURPOSE_NONE;

static menu_kind_t active_menu = MENU_KIND_ITEM;

// --- small text helpers ---

static void append(char* dest, int* pos, int max, const char* piece) {
    for (int i = 0; piece[i] != '\0' && *pos < max - 1; i++) {
        dest[*pos] = piece[i];
        (*pos)++;
    }
    dest[*pos] = '\0';
}

static void set_message(const char* text) {
    int pos = 0;
    message[0] = '\0';
    append(message, &pos, MESSAGE_MAX, text);
}

static void clear_message(void) {
    message[0] = '\0';
}

// --- paths ---

static int child_path(const char* name, char* out) {
    uint32_t base = api->kstrlen(current_path);
    uint32_t needed = base + (base > 0 ? 1 : 0) + api->kstrlen(name);
    if (needed >= FS_MAX_PATH) {
        return 0;
    }

    int pos = 0;
    out[0] = '\0';
    if (base > 0) {
        append(out, &pos, FS_MAX_PATH, current_path);
        append(out, &pos, FS_MAX_PATH, "/");
    }
    append(out, &pos, FS_MAX_PATH, name);
    return 1;
}

static void path_go_up(void) {
    int last_slash = -1;
    for (int i = 0; current_path[i] != '\0'; i++) {
        if (current_path[i] == '/') {
            last_slash = i;
        }
    }

    if (last_slash < 0) {
        current_path[0] = '\0';
    } else {
        current_path[last_slash] = '\0';
    }
}

// --- rows ---

static int has_parent_row(void) {
    return current_path[0] != '\0';
}

static int row_count(void) {
    return entry_count + (has_parent_row() ? 1 : 0);
}

static uint32_t list_top(void) {
    return FILES_TOP + HEADER_HEIGHT;
}

static uint32_t footer_y(void) {
    return api->gfx_screen_height() - FILES_BOTTOM - FOOTER_HEIGHT;
}

static uint32_t row_y(int row) {
    return list_top() + (uint32_t) row * ROW_HEIGHT;
}

static int rows_that_fit(void) {
    uint32_t used = FILES_TOP + FILES_BOTTOM + HEADER_HEIGHT + FOOTER_HEIGHT;
    if (api->gfx_screen_height() <= used) {
        return 0;
    }
    return (int) ((api->gfx_screen_height() - used) / ROW_HEIGHT);
}

static int visible_rows(void) {
    int total = row_count();
    int fit = rows_that_fit();
    return (total < fit) ? total : fit;
}

static int row_at(int32_t y) {
    int32_t top = (int32_t) list_top();
    if (y < top) {
        return -1;
    }

    int row = (int) ((y - top) / ROW_HEIGHT);
    if (row >= visible_rows()) {
        return -1;
    }
    return row;
}

static int entry_for_row(int row) {
    if (has_parent_row()) {
        if (row == 0) {
            return PARENT_ENTRY;
        }
        return row - 1;
    }
    return row;
}

// --- drawing ---

static void draw_header(void) {
    api->gfx_fill_rect(0, FILES_TOP, api->gfx_screen_width(), HEADER_HEIGHT, BAR_COLOR);

    char line[FS_MAX_PATH + 16];
    int pos = 0;
    line[0] = '\0';
    append(line, &pos, (int) sizeof(line), "Files - /");
    append(line, &pos, (int) sizeof(line), current_path);

    uint32_t text_y = FILES_TOP + (HEADER_HEIGHT - FONT_HEIGHT) / 2;
    api->gfx_draw_string(TEXT_X, text_y, line, GFX_WHITE);
}

static void draw_footer(void) {
    uint32_t y = footer_y();
    api->gfx_fill_rect(0, y, api->gfx_screen_width(), FOOTER_HEIGHT, BAR_COLOR);

    uint32_t text_y = y + (FOOTER_HEIGHT - FONT_HEIGHT) / 2;
    if (message[0] != '\0') {
        api->gfx_draw_string(TEXT_X, text_y, message, MESSAGE_COLOR);
    } else {
        api->gfx_draw_string(TEXT_X, text_y, HINT_TEXT, GFX_WHITE);
    }
}

static void draw_row(int row) {
    uint32_t text_y = row_y(row) + (ROW_HEIGHT - FONT_HEIGHT) / 2;
    int entry = entry_for_row(row);

    if (entry == PARENT_ENTRY) {
        api->gfx_draw_string(TEXT_X, text_y, "..", FOLDER_COLOR);
        return;
    }

    const fs_info_t* info = &entries[entry];

    char label[FS_MAX_NAME_LEN + 2];
    int pos = 0;
    label[0] = '\0';
    append(label, &pos, (int) sizeof(label), info->name);
    if (info->is_dir) {
        append(label, &pos, (int) sizeof(label), "/");
    }
    api->gfx_draw_string(TEXT_X, text_y, label, info->is_dir ? FOLDER_COLOR : FILE_COLOR);

    if (info->is_dir) {
        api->gfx_draw_string(SIZE_COLUMN_X, text_y, "folder", FOLDER_COLOR);
    } else {
        char number[12];
        char size_text[24];
        int size_pos = 0;
        size_text[0] = '\0';

        api->kuitoa(info->size_bytes, number);
        append(size_text, &size_pos, (int) sizeof(size_text), number);
        append(size_text, &size_pos, (int) sizeof(size_text), " bytes");
        api->gfx_draw_string(SIZE_COLUMN_X, text_y, size_text, FILE_COLOR);
    }
}

static void paint(void) {
    uint32_t band_height = 0;
    if (api->gfx_screen_height() > (uint32_t) (FILES_TOP + FILES_BOTTOM)) {
        band_height = api->gfx_screen_height() - FILES_TOP - FILES_BOTTOM;
    }
    api->gfx_fill_rect(0, FILES_TOP, api->gfx_screen_width(), band_height, LIST_COLOR);

    draw_header();

    int rows = visible_rows();
    for (int row = 0; row < rows; row++) {
        draw_row(row);
    }

    if (row_count() == 0) {
        api->gfx_draw_string(TEXT_X, list_top() + 6, "(this folder is empty - right-click here to add something)", GFX_WHITE);
    }

    draw_footer();
}

// --- the folder contents ---

static void refresh_list(void) {
    entry_count = api->fs_list_dir(current_path, entries);

    if (entry_count < 0) {
        current_path[0] = '\0';
        entry_count = api->fs_list_dir(current_path, entries);
        set_message("that folder no longer exists - back at the top");
    }
    if (entry_count < 0) {
        entry_count = 0;
    }
}

// --- actions ---

static void show_folder(const char* path) {
    int pos = 0;
    current_path[0] = '\0';
    append(current_path, &pos, FS_MAX_PATH, path);

    clear_message();
    refresh_list();
    paint();
}

static void go_up(void) {
    path_go_up();

    clear_message();
    refresh_list();
    paint();
}

static void open_path(const char* path, int is_dir) {
    if (is_dir) {
        show_folder(path);
    } else {
        api->launcher_open_file(path);
    }
}

static void forget_target(void) {
    int result = api->fs_delete_file(target_path);

    if (result == FS_OK) {
        set_message("forgotten");
    } else if (result == FS_ERR_NOT_EMPTY) {
        set_message("folder is not empty - forget what is inside it first");
    } else {
        set_message("could not forget it - it may already be gone");
    }

    refresh_list();
    paint();
}

static void start_new_item(prompt_purpose_t purpose) {
    prompt_purpose = purpose;

    const char* title = (purpose == PURPOSE_NEW_FOLDER) ? "New folder name:" : "New file name:";
    api->prompt_open(title, "", FS_MAX_NAME_LEN - 1, "/");
}

static void finish_new_item(const char* name, int is_folder) {
    char path[FS_MAX_PATH];

    if (!child_path(name, path)) {
        set_message("that path is too long to create");
    } else {
        int result;

        if (is_folder) {
            result = api->fs_make_folder(path);
        } else {
            fs_info_t existing;
            if (api->fs_stat(path, &existing) == FS_OK) {
                result = FS_ERR_EXISTS;
            } else {
                result = api->fs_write_file(path, (const uint8_t*) "", 0);
            }
        }

        if (result == FS_OK) {
            set_message(is_folder ? "folder created" : "file created");
        } else if (result == FS_ERR_EXISTS) {
            set_message("something with that name already exists in this folder");
        } else if (result == FS_ERR_FULL) {
            set_message("cannot create - the file table is full (32 items)");
        } else if (result == FS_ERR_BAD_PATH) {
            set_message("that is not a valid name");
        } else {
            set_message("could not create it");
        }
    }

    refresh_list();
    paint();
}

static void start_rename(void) {
    char title[48];
    int pos = 0;
    title[0] = '\0';
    append(title, &pos, (int) sizeof(title), "Rename ");
    append(title, &pos, (int) sizeof(title), target_name);
    append(title, &pos, (int) sizeof(title), " to:");

    prompt_purpose = PURPOSE_RENAME;

    api->prompt_open(title, target_name, FS_MAX_NAME_LEN - 1, "/");
}

static void finish_rename(const char* new_name) {
    if (api->kstrcmp(new_name, target_name) == 0) {
        set_message("name unchanged");
    } else {
        int result = api->fs_rename(target_path, new_name);

        if (result == FS_OK) {
            set_message("renamed");
        } else if (result == FS_ERR_EXISTS) {
            set_message("something with that name already exists in this folder");
        } else if (result == FS_ERR_BAD_PATH) {
            set_message("that is not a valid name");
        } else if (result == FS_ERR_NOT_FOUND) {
            set_message("it no longer exists");
        } else {
            set_message("could not rename it");
        }
    }

    refresh_list();
    paint();
}

static void show_properties(void) {
    int pos = 0;
    char number[12];

    message[0] = '\0';
    append(message, &pos, MESSAGE_MAX, target_path);

    if (target_is_dir) {
        fs_info_t inside[FS_MAX_FILES];
        int count = api->fs_list_dir(target_path, inside);
        if (count < 0) {
            count = 0;
        }

        api->kuitoa((uint32_t) count, number);
        append(message, &pos, MESSAGE_MAX, ": folder, ");
        append(message, &pos, MESSAGE_MAX, number);
        append(message, &pos, MESSAGE_MAX, (count == 1) ? " item" : " items");
    } else {
        fs_info_t info;
        if (api->fs_stat(target_path, &info) == FS_OK) {
            api->kuitoa(info.size_bytes, number);
            append(message, &pos, MESSAGE_MAX, ": file, ");
            append(message, &pos, MESSAGE_MAX, number);
            append(message, &pos, MESSAGE_MAX, " bytes");
        } else {
            append(message, &pos, MESSAGE_MAX, ": file");
        }
    }

    draw_footer();
}

// --- the app_t slots ---

static void files_start(void) {
    current_path[0] = '\0';
    clear_message();
    refresh_list();
    paint();
}

static void files_redraw(void) {
    clear_message();
    refresh_list();
    paint();
}

static void files_handle_char(char c) {
    if (c == '\b') {
        if (has_parent_row()) {
            go_up();
        }
    }
}

static void files_handle_click(int32_t x, int32_t y) {
    (void) x;

    int row = row_at(y);
    if (row < 0) {
        return;
    }

    int entry = entry_for_row(row);
    if (entry == PARENT_ENTRY) {
        go_up();
        return;
    }

    char path[FS_MAX_PATH];
    if (!child_path(entries[entry].name, path)) {
        set_message("that path is too long to open");
        draw_footer();
        return;
    }

    open_path(path, entries[entry].is_dir);
}

static void files_handle_right_click(int32_t x, int32_t y) {
    int row = row_at(y);
    if (row < 0) {
        if (y >= (int32_t) list_top() && y < (int32_t) footer_y()) {
            active_menu = MENU_KIND_BACKGROUND;
            api->menu_open(x, y, new_menu_labels, NEW_ACTION_COUNT);
        }
        return;
    }

    int entry = entry_for_row(row);
    if (entry == PARENT_ENTRY) {
        return;
    }

    if (!child_path(entries[entry].name, target_path)) {
        set_message("that path is too long to use");
        draw_footer();
        return;
    }
    target_is_dir = entries[entry].is_dir;

    int name_pos = 0;
    target_name[0] = '\0';
    append(target_name, &name_pos, FS_MAX_NAME_LEN, entries[entry].name);

    active_menu = MENU_KIND_ITEM;
    api->menu_open(x, y, menu_labels, ACTION_COUNT);
}

static void files_handle_menu_pick(int index) {
    if (active_menu == MENU_KIND_BACKGROUND) {
        if (index == NEW_FOLDER) {
            start_new_item(PURPOSE_NEW_FOLDER);
        } else if (index == NEW_FILE) {
            start_new_item(PURPOSE_NEW_FILE);
        }
        return;
    }

    if (index == ACTION_OPEN) {
        open_path(target_path, target_is_dir);
    } else if (index == ACTION_RENAME) {
        start_rename();
    } else if (index == ACTION_FORGET) {
        forget_target();
    } else if (index == ACTION_PROPERTIES) {
        show_properties();
    }
}

static void files_handle_prompt_done(const char* text) {
    if (prompt_purpose == PURPOSE_RENAME) {
        finish_rename(text);
    } else if (prompt_purpose == PURPOSE_NEW_FOLDER) {
        finish_new_item(text, 1);
    } else if (prompt_purpose == PURPOSE_NEW_FILE) {
        finish_new_item(text, 0);
    }

    prompt_purpose = PURPOSE_NONE;
}

// --- loader entry point ---

int app_entry(const vigil_api_t* kernel_api, app_t* out_app) {
    if (kernel_api->version != VIGIL_API_VERSION || kernel_api->struct_size < sizeof(vigil_api_t)) {
        return 1;
    }
    api = kernel_api;

    out_app->name = "Files";
    out_app->start = files_start;
    out_app->redraw = files_redraw;
    out_app->on_key = files_handle_char;
    out_app->on_click = files_handle_click;
    out_app->on_right_click = files_handle_right_click;
    out_app->on_menu_pick = files_handle_menu_pick;
    out_app->on_prompt_done = files_handle_prompt_done;
    return 0;
}
