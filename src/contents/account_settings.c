#include "contents/account_settings_internal.h"
#include "contents/settings_file.h"
#include "contents/settings_json.h"
#include "session/state.h"
#include "platform/client_config.h"
#include "platform/system.h"
#include "platform/language.h"
#include "core/app.h"
#include <stdlib.h>
#include <string.h>

struct AccountUserDataMgr *g_accountUserDataMgr;

int account_user_data_create(void)
{
    if (g_accountUserDataMgr) return 0;
    g_accountUserDataMgr = calloc(1, sizeof *g_accountUserDataMgr);
    return g_accountUserDataMgr != NULL;
}

void account_user_data_destroy(void)
{
    struct AccountUserDataMgr *self = g_accountUserDataMgr;
    g_accountUserDataMgr = NULL;
    if (!self) return;
    json_tree_free(&self->defaults);
    free(self);
}

int account_user_data_is_loaded(const struct AccountUserDataMgr *self)
{
    return self && self->loaded;
}

static int restore_defaults(struct AccountUserDataMgr *self)
{
    if (!self->have_defaults) {
        if (!account_settings_capture(&self->defaults)) return 0;
        self->have_defaults = 1;
    }
    return account_settings_apply(&self->defaults);
}

void account_user_data_reset(struct AccountUserDataMgr *self)
{
    if (!self) return;
    if (self->loaded) account_user_data_save(self);
    self->loaded = 0;
    self->path[0] = 0;
    restore_defaults(self);
}

void account_user_data_load(struct AccountUserDataMgr *self, int aid, const char *world)
{
    char path[MAX_PATH];
    struct OwnedText text = {0};
    struct JsonTree data;
    int result;
    if (!self) return;
    if (!settings_file_path(path, g_clientDir, g_serverAddress, g_serverPort, world, (unsigned)aid, 0)) {
        account_user_data_reset(self);
        return;
    }
    if (self->loaded && !strcmp(self->path, path)) return;
    if (self->loaded) account_user_data_save(self);
    self->loaded = 0;
    strcpy(self->path, path);
    if (!restore_defaults(self)) return;
    result = settings_file_read(path, &text);
    if (result == 2) {
        self->loaded = 1;
    } else if (result == 1) {
        json_tree_init(&data);
        if (json_parse_text(&data, text.data, text.size) &&
            settings_json_convert(&data, CODE_PAGE_UTF8, g_codePage))
            self->loaded = account_settings_apply(&data);
        json_tree_free(&data);
    }
    owned_text_clear(&text);
    if (!self->loaded)
        platform_debug_output("Account settings could not be loaded; using initial preferences and preserving the file.\n");
}

int account_user_data_save(struct AccountUserDataMgr *self)
{
    struct JsonTree data;
    struct OwnedText text = {0};
    int ok = 0;
    if (!self || !self->loaded || !self->path[0]) return 0;
    json_tree_init(&data);
    if (account_settings_capture(&data) && settings_json_convert(&data, g_codePage, CODE_PAGE_UTF8) &&
        json_write_request(&data, &text))
        ok = settings_file_write(self->path, owned_text_view(&text));
    owned_text_clear(&text);
    json_tree_free(&data);
    if (!ok) platform_debug_output("Account settings could not be saved; previous file preserved.\n");
    return ok;
}
