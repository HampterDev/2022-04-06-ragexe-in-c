#ifndef CORE_SKIN_PATH_H
#define CORE_SKIN_PATH_H

#include "ui/skin_manager.h"

#define UI_SKIN_PREFIX_LEN 15

extern struct String g_skinPath;
extern struct Once g_skinPathGuard;
extern struct String g_uiFolderPath;
extern struct Once g_uiFolderPathGuard;
void skin_path_atexit(void);
void ui_folder_path_atexit(void);

const char *ui_skin_path(const char *path);

#endif
