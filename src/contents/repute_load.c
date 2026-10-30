#include <string.h>
#include <lua.h>
#include "contents/repute_mgr.h"
#include "contents/ui_window_mgr.h"
#include "core/text_util.h"
#include "ui/ui_std.h"

#define REPUTE_INFO_FILE  "ContentData\\Repute\\ReputeInfoData.bson"
#define REPUTE_GROUP_FILE "ContentData\\Repute\\ReputeGroupData.bson"

char repute_mgr_load(struct ReputeMgr *self)
{
    if (repute_mgr_load_info_bson(self, &self->infos)
        && repute_mgr_load_group_bson(self, &self->groups))
        return 1;
    error_msg("Failed : LoadReputeInfoData", 0);
    return 0;
}

static char parse_file(struct ReputeMgr *self, struct File *file, struct JsonCppLoader *loader)
{
    struct Vec bytes;
    char ok;

    vec_init(&bytes);
    vec_assign(&bytes, file->buffer, (const char *)file->buffer + file->size, 1);
    ok = json_loader_load_bson(loader, &bytes);
    vec_free(&bytes);
    return ok;
}

char repute_mgr_load_group_bson(struct ReputeMgr *self, struct Tree *groups)
{
    struct String path;
    struct File file;
    char ok;

    string_init(&path);
    string_assign(&path, REPUTE_GROUP_FILE, sizeof(REPUTE_GROUP_FILE) - 1);
    file_construct(&file);
    if (!file_open(&file, string_cstr(&path), 0, 1, 0)) {
        ok = 0;
    } else {
        struct JsonCppLoader loader;

        json_loader_init(&loader);
        ok = parse_file(self, &file, &loader);
        if (ok) {
            struct Tree records;

            tree_init(&records);
            if (!repute_groups_read(&loader, &records)) {
                ok = 0;
            } else {
                struct ReputeGroupCopier copier;

                tree_clear(groups, repute_group_entry_destroy);
                repute_groups_copy(&copier, tree_first(&records), records.head, self, groups);
                ok = 1;
            }
            tree_free(&records, repute_group_record_pair_destroy);
        }
        json_loader_free(&loader);
    }
    file_destruct(&file);
    string_free(&path);
    return ok;
}

char repute_mgr_load_info_bson(struct ReputeMgr *self, struct Tree *infos)
{
    struct String path;
    struct File file;
    char ok;

    string_init(&path);
    string_assign(&path, REPUTE_INFO_FILE, sizeof(REPUTE_INFO_FILE) - 1);
    file_construct(&file);
    if (!file_open(&file, string_cstr(&path), 0, 1, 0)) {
        ok = 0;
    } else {
        struct JsonCppLoader loader;

        json_loader_init(&loader);
        ok = parse_file(self, &file, &loader);
        if (ok) {
            struct Tree records;

            tree_init(&records);
            if (!repute_info_read(&loader, &records)) {
                ok = 0;
            } else {
                struct ReputeInfoStore adder;
                struct TreeNode *it;

                tree_clear(infos, repute_info_entry_destroy);
                adder.repute_mgr = self;
                adder.infos = infos;
                for (it = tree_first(&records); it != records.head; it = tree_next(it)) {
                    const struct ReputeInfoRecordPair *stored = TREE_VALUE(it, struct ReputeInfoRecordPair);
                    struct ReputeInfoRecordPair pair;

                    pair.id = stored->id;
                    string_copy(&pair.record.name, &stored->record.name);
                    pair.record.max_point_positive = stored->record.max_point_positive;
                    pair.record.max_point_negative = stored->record.max_point_negative;
                    string_copy(&pair.record.visibility, &stored->record.visibility);
                    repute_info_store(&adder, &pair);
                    string_free(&pair.record.visibility);
                    string_free(&pair.record.name);
                }
                ok = 1;
            }
            tree_free(&records, repute_info_record_pair_destroy);
        }
        json_loader_free(&loader);
    }
    file_destruct(&file);
    string_free(&path);
    return ok;
}
