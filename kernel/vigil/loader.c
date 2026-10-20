// kernel/vigil/loader.c

#include "loader.h"
#include "app_format.h"
#include "api.h"
#include "app.h"
#include "fs.h"
#include "heap.h"
#include "kstring.h"
#include "launcher.h"

typedef int (*app_entry_fn)(const vigil_api_t* api, app_t* out_app);
static int slot_used[LOADER_SLOT_COUNT];

// --- slots ---

static uint32_t slot_address(int slot) {
    return LOADER_SLOTS_BASE + (uint32_t) slot * LOADER_SLOT_SIZE;
}

static int slot_for_address(uint32_t address) {
    for (int i = 0; i < LOADER_SLOT_COUNT; i++) {
        if (slot_address(i) == address) {
            return i;
        }
    }
    return -1;
}

// --- checking the file ---

static int check_header(const app_header_t* header, uint32_t file_size, int* slot_out) {
    if (header->magic != VAPP_MAGIC) {
        return LOADER_ERR_BAD_MAGIC;
    }
    if (header->format_version != VAPP_FORMAT_VERSION) {
        return LOADER_ERR_BAD_VERSION;
    }

    int slot = slot_for_address(header->load_address);
    if (slot < 0) {
        return LOADER_ERR_BAD_LAYOUT;
    }
    if (header->file_size != file_size) {
        return LOADER_ERR_BAD_LAYOUT;
    }
    if (header->memory_size < header->file_size || header->memory_size > LOADER_SLOT_SIZE) {
        return LOADER_ERR_BAD_LAYOUT;
    }
    if (header->entry_offset < sizeof(app_header_t) || header->entry_offset >= header->file_size) {
        return LOADER_ERR_BAD_LAYOUT;
    }
    if (slot_used[slot]) {
        return LOADER_ERR_SLOT_BUSY;
    }

    *slot_out = slot;
    return LOADER_OK;
}

// --- placing app in memory ---

static void copy_into_slot(uint32_t address, const uint8_t* file, uint32_t file_size, uint32_t memory_size) {
    uint8_t* dest = (uint8_t*) address;

    for (uint32_t i = 0; i < file_size; i++) {
        dest[i] = file[i];
    }
    for (uint32_t i = file_size; i < memory_size; i++) {
        dest[i] = 0;
    }
}

static void clear_app(app_t* app) {
    uint8_t* bytes = (uint8_t*) app;
    for (uint32_t i = 0; i < sizeof(app_t); i++) {
        bytes[i] = 0;
    }
}

// --- public funcs ---

int loader_load(const char* path) {
    fs_info_t info;
    if (fs_stat(path, &info) != FS_OK || info.is_dir) {
        return LOADER_ERR_READ;
    }

    uint32_t size = info.size_bytes;
    if (size < sizeof(app_header_t)) {
        return LOADER_ERR_BAD_MAGIC;
    }
    if (size > LOADER_SLOT_SIZE) {
        return LOADER_ERR_TOO_BIG;
    }

    uint8_t* temp = (uint8_t*) kmalloc(size);
    if (temp == 0) {
        return LOADER_ERR_NO_MEMORY;
    }

    uint32_t read_size = 0;
    int result = fs_read_file(path, temp, size, &read_size);
    if (result != FS_OK || read_size != size) {
        kfree(temp);
        return LOADER_ERR_READ;
    }

    int slot = 0;
    const app_header_t* header = (const app_header_t*) temp;
    result = check_header(header, size, &slot);
    if (result != LOADER_OK) {
        kfree(temp);
        return result;
    }

    uint32_t address = slot_address(slot);
    uint32_t memory_size = header->memory_size;
    uint32_t entry_offset = header->entry_offset;

    copy_into_slot(address, temp, size, memory_size);
    kfree(temp);

    app_t app;
    clear_app(&app);
    slot_used[slot] = 1;

    app_entry_fn entry = (app_entry_fn) (address + entry_offset);
    if (entry(api_get(), &app) != 0 || app.name == 0 || app.start == 0) {
        slot_used[slot] = 0;
        return LOADER_ERR_ENTRY_FAILED;
    }

    if (launcher_register_app(&app) < 0) {
        slot_used[slot] = 0;
        return LOADER_ERR_LAUNCHER_FULL;
    }

    return LOADER_OK;
}

static int join_path(const char* folder, const char* name, char* out) {
    uint32_t needed = kstrlen(folder) + 1 + kstrlen(name);
    if (needed >= FS_MAX_PATH) return 0;

    int pos = 0;
    for (int i = 0; folder[i] != '\0'; i++) {
        out[pos++] = folder[i];
    }
    out[pos++] = '/';
    for (int i = 0; name[i] != '\0'; i++) {
        out[pos++] = name[i];
    }
    out[pos] = '\0';
    return 1;
}

int loader_load_folder(const char* folder) {
    fs_info_t entries[FS_MAX_FILES];

    int count = fs_list_dir(folder, entries);
    if (count < 0) return 0;

    int loaded = 0;
    for (int i = 0; i < count; i++) {
        if (entries[i].is_dir) continue;
        char path[FS_MAX_PATH];
        if (!join_path(folder, entries[i].name, path)) continue;
        if (loader_load(path) == LOADER_OK) loaded++;
    }
    return loaded;
}

const char* loader_error_text(int code) {
    switch (code) {
        case LOADER_OK:                 return "ok";
        case LOADER_ERR_READ:           return "cannot find or read that file";
        case LOADER_ERR_TOO_BIG:        return "file is bigger than one app slot (256 KB)";
        case LOADER_ERR_BAD_MAGIC:      return "not a VIGIL app file";
        case LOADER_ERR_BAD_VERSION:    return "app was built for a different loader version";
        case LOADER_ERR_BAD_LAYOUT:     return "app header is inconsistent (wrong slot address or sizes)";
        case LOADER_ERR_SLOT_BUSY:      return "that app slot is already in use";
        case LOADER_ERR_NO_MEMORY:      return "out of heap memory";
        case LOADER_ERR_ENTRY_FAILED:   return "the app refused to start";
        case LOADER_ERR_LAUNCHER_FULL:  return "the launcher has no free app slots";
        default:                        return "unknown error";
    }
}
