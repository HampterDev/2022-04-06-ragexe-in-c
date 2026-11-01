#include "contents/mcstore_backup.h"
#include "contents/settings_file.h"
#include "contents/settings_json.h"
#include "core/app.h"
#include "platform/client_config.h"
#include "platform/language.h"
#include "session/state.h"
#include "json/service_json.h"
#include <stdio.h>
#include <string.h>

static int file_path(char out[MAX_PATH], int aid, int gid, const char *world, int type)
{
    char base[MAX_PATH], *extension;
    int count;
    if (!gid || (type != 0 && type != 1) ||
        !settings_file_path(base, g_clientDir, g_serverAddress, g_serverPort, world,
                            (unsigned)aid, (unsigned)gid))
        return 0;
    extension = strrchr(base, '.');
    *extension = 0;
    count = snprintf(out, MAX_PATH, "%s.%s.json", base, type ? "buying" : "vending");
    return count > 0 && count < MAX_PATH;
}

static int read_file(struct McStoreBackup *self, const char *path, int type)
{
    struct OwnedText text = {0};
    struct JsonTree root;
    const struct JsonTree *version;
    int result = settings_file_read(path, &text);
    if (result != 1) return result;
    json_tree_init(&root);
    result = 0;
    if (json_parse_text(&root, text.data, text.size) && root.type == JSON_TREE_OBJECT) {
        version = json_tree_find(&root, "version", 7);
        if (version && (version->type == JSON_TREE_INT64 || version->type == JSON_TREE_UINT64) &&
            version->u.u64 == 1 && settings_json_convert(&root, CP_UTF8, g_codePage) &&
            mcstore_read_items(self, json_tree_find(&root, "data", 4)) &&
            self->item_count <= (type ? 5u : 13u))
            result = 1;
    }
    json_tree_free(&root);
    owned_text_clear(&text);
    return result;
}

int mcstore_request_load(struct McStoreBackup *self, int aid, int gid, const char *world, int type)
{
    struct McStoreBackup next = {0};
    char path[MAX_PATH];
    int result = 0;
    if (!self) return 0;
    if (file_path(path, aid, gid, world, type)) result = read_file(&next, path, type);
    if (result == 1) {
        mcstore_destroy(self);
        *self = next;
    } else {
        mcstore_destroy(&next);
    }
    return result;
}

int mcstore_request_save(struct McStoreBackup *self, int type)
{
    struct McStoreBackup previous = {0}, checked = {0};
    struct JsonTree root, data, version;
    struct OwnedText text = {0};
    char path[MAX_PATH];
    int result, ok = 0;
    if (!self || !file_path(path, g_session1.ownAid, g_session1.ownGid,
                            string_cstr(&g_session1.worldName), type)) return 0;
    result = read_file(&previous, path, type);
    mcstore_destroy(&previous);
    if (!result) return 0;
    json_tree_init(&root);
    json_tree_init(&data);
    if (mcstore_write_items(self, &data) && mcstore_read_items(&checked, &data) &&
        checked.item_count <= (type ? 5u : 13u) && settings_json_convert(&data, g_codePage, CP_UTF8)) {
        json_value_from_int(&version, 1);
        json_value_assign(json_value_member(&root, "version"), &version);
        json_value_assign(json_value_member(&root, "data"), &data);
        if (json_write_request(&root, &text))
            ok = settings_file_write(path, owned_text_view(&text));
    }
    mcstore_destroy(&checked);
    owned_text_clear(&text);
    json_tree_free(&data);
    json_tree_free(&root);
    return ok;
}
