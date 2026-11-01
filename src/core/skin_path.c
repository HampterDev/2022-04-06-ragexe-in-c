#include "game/context.h"
#include <stdlib.h>
#include <string.h>
#include "core/skin_path.h"
#include "core/client.h"
#include "std/std.h"
#include "platform/language.h"

struct String g_skinPath;
INIT_ONCE g_skinPathGuard;
struct String g_uiFolderPath;
INIT_ONCE g_uiFolderPathGuard;

static void skin_path_tail(struct String *out, struct String *path)
{
    int count = (int)path->size - UI_SKIN_PREFIX_LEN, i;

    string_clear(out);
    for (i = 0; i < count; i++)
        string_append_char(out, string_cstr(path)[i + UI_SKIN_PREFIX_LEN]);
}

const char *ui_skin_path(const char *path)
{
    struct String skin, full, local;
    const char *result;

    if (strlen(path) == 0)
        return path;

    if (VEC_COUNT(&g_skinMgr.paths, struct String) != 0 && g_skinMgr.current != -1
        && game_current_mode()) {
        string_init(&skin);
        string_assign_cstr(&skin, skin_mgr_get_current_path(&g_skinMgr));
        if (once_begin(&g_skinPathGuard)) {
            string_init(&g_skinPath);
            atexit(skin_path_atexit);
            once_complete(&g_skinPathGuard);
        }
        string_init(&full);
        string_assign_cstr(&full, path);
        skin_path_tail(&g_skinPath, &full);
        string_insert(&g_skinPath, 0, string_cstr(&skin), skin.size);
        string_assign(&full, string_cstr(&g_skinPath), g_skinPath.size);
        if (res_mgr_is_available(g_resMgr(), string_cstr(&full))) {
            result = string_cstr(&g_skinPath);
            string_free(&full);
            string_free(&skin);
            return result;
        }
        string_free(&full);
        string_free(&skin);
    }

    if (g_serviceType == 0)
        return path;
    if (res_mgr_is_available(g_resMgr(), language_language_file(g_language, path)))
        return language_language_file(g_language, path);

    if (once_begin(&g_uiFolderPathGuard)) {
        string_init(&g_uiFolderPath);
        atexit(ui_folder_path_atexit);
        once_complete(&g_uiFolderPathGuard);
    }
    string_init(&local);
    string_assign_cstr(&local, path);
    skin_path_tail(&g_uiFolderPath, &local);
    string_insert(&g_uiFolderPath, 0, "UI\\", 3);
    string_assign(&local, string_cstr(&g_uiFolderPath), g_uiFolderPath.size);
    result = path;
    if (res_mgr_is_available(g_resMgr(), string_cstr(&local)))
        result = string_cstr(&g_uiFolderPath);
    string_free(&local);
    return result;
}

void skin_path_atexit(void)
{
    string_free(&g_skinPath);
}

void ui_folder_path_atexit(void)
{
    string_free(&g_uiFolderPath);
}
