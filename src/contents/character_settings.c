#include "contents/character_settings_internal.h"
#include "contents/settings_file.h"
#include "platform/client_config.h"
#include "platform/system.h"
#include "core/app.h"
#include <stdlib.h>
#include <string.h>

struct CharUserDataMgr *g_charUserDataMgr;

int char_user_data_create(void)
{
    if (g_charUserDataMgr) return 0;
    g_charUserDataMgr = calloc(1, sizeof *g_charUserDataMgr);
    return g_charUserDataMgr != NULL;
}

void char_user_data_destroy(void)
{
    struct CharUserDataMgr *self = g_charUserDataMgr;
    g_charUserDataMgr = NULL;
    free(self);
}

void char_user_data_reset(struct CharUserDataMgr *self)
{
    if (!self) return;
    if (self->loaded) char_user_data_save(self);
    self->loaded = 0;
    self->path[0] = 0;
    character_settings_clear();
}

void char_user_data_load(struct CharUserDataMgr *self, int aid, int gid, const char *world)
{
    char path[MAX_PATH];
    struct OwnedText text = {0};
    struct JsonTree data;
    int result;
    if (!self) return;
    if (!gid || !settings_file_path(path, g_clientDir, g_serverAddress, g_serverPort,
                                    world, (unsigned)aid, (unsigned)gid)) {
        char_user_data_reset(self);
        return;
    }
    if (self->loaded && !strcmp(self->path, path)) return;
    if (self->loaded) char_user_data_save(self);
    self->loaded = 0;
    strcpy(self->path, path);
    if (!character_settings_clear()) return;
    character_settings_seed();
    result = settings_file_read(path, &text);
    if (result == 2) {
        self->loaded = 1;
    } else if (result == 1) {
        json_tree_init(&data);
        if (json_parse_text(&data, text.data, text.size))
            self->loaded = character_settings_apply(&data);
        json_tree_free(&data);
    }
    owned_text_clear(&text);
    if (!self->loaded)
        platform_debug_output("Character settings could not be loaded; using initial preferences and preserving the file.\n");
}

int char_user_data_save(struct CharUserDataMgr *self)
{
    struct JsonTree data;
    struct OwnedText text = {0};
    int ok = 0;
    if (!self || !self->loaded || !self->path[0]) return 0;
    json_tree_init(&data);
    if (character_settings_capture(&data) && json_write_request(&data, &text))
        ok = settings_file_write(self->path, owned_text_view(&text));
    owned_text_clear(&text);
    json_tree_free(&data);
    if (!ok) platform_debug_output("Character settings could not be saved; previous file preserved.\n");
    return ok;
}
