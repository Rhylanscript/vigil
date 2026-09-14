// boot/fs/fs.h

#ifndef FS_H
#define FS_H
#include <stdint.h>

#define FS_MAX_NAME_LEN 16
#define FS_MAX_FILES 16

int fs_write_file(const char* name, const uint8_t* data, uint32_t size);
int fs_read_file(const char* name, uint8_t* buffer, uint32_t buffer_size, uint32_t* out_size);
int fs_delete_file(const char* name);
int fs_list_files(char names_out[FS_MAX_FILES][FS_MAX_NAME_LEN], uint32_t sizes_out[FS_MAX_FILES]);

#endif
