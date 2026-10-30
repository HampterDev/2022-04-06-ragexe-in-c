#ifndef FILEIO_FILE_MANAGER_H
#define FILEIO_FILE_MANAGER_H
#include <stddef.h>

struct GrfArchive;
struct GameFileMgr {
    struct GrfArchive **archives;
    size_t archive_count;
};
extern struct GameFileMgr g_fileMgr;
extern unsigned char g_readFolderFirst;

int game_file_manager_add_archive(struct GameFileMgr *manager, const char *name);
void game_file_manager_clear(struct GameFileMgr *manager);
unsigned char *game_file_mgr_get_data(struct GameFileMgr *manager, const char *name, int *size, unsigned char disk_only);
void game_file_mgr_release_data(struct GameFileMgr *manager, void *data);
unsigned char game_file_mgr_is_file_exist(struct GameFileMgr *manager, const char *name, unsigned char disk_only);
#endif
