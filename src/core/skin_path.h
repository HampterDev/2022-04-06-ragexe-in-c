#ifndef CORE_SKIN_PATH_H
#define CORE_SKIN_PATH_H

#include "base.h"

#define UI_SKIN_PREFIX_LEN 15

extern struct String g_skinPath;
extern struct Once g_skinPathGuard;
extern struct String g_uiFolderPath;
extern struct Once g_uiFolderPathGuard;
void skin_path_atexit(void);
void ui_folder_path_atexit(void);

/* The folder prefix of the selected UI skin, as skin_mgr_scan builds it from the skin
   folder's name, or NULL while no skin is selected. The skin manager
   (ui/misc/skin_mgr.c) sets it whenever its folder list or selection changes; the
   string stays the skin manager's. */
void ui_skin_path_set_skin(const char *folder);

/* `path` with its UI_SKIN prefix replaced by the selected skin's folder when a game
   mode is running and the skin has the file; otherwise `path` if it exists, otherwise
   the same name in the UI folder if that exists, otherwise `path`. */
const char *ui_skin_path(const char *path);

#endif
