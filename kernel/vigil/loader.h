// kernel/vigil/loader.h

#ifndef VIGIL_LOADER_H
#define VIGIL_LOADER_H

// folder (on the VIGIL disk) that is scanned for apps at boot
#define LOADER_APP_FOLDER "apps"

// --- result codes (0 = success, everything else is a failure) ---

#define LOADER_OK                 0
#define LOADER_ERR_READ          -1   // could not find or read the file
#define LOADER_ERR_TOO_BIG       -2   // larger than one slot
#define LOADER_ERR_BAD_MAGIC     -3   // not a VAPP file
#define LOADER_ERR_BAD_VERSION   -4   // made for a different loader version
#define LOADER_ERR_BAD_LAYOUT    -5   // header numbers dont add up
#define LOADER_ERR_SLOT_BUSY     -6   // another app already sits in that slot
#define LOADER_ERR_NO_MEMORY     -7   // heap could not provide a temp buffer
#define LOADER_ERR_ENTRY_FAILED  -8   // the apps app_entry() refused to start
#define LOADER_ERR_LAUNCHER_FULL -9   // no free place in the launchers list

int loader_load(const char* path);

int loader_load_folder(const char* folder);

const char* loader_error_text(int code);

#endif
