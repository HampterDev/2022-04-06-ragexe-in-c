#include <errno.h>
#include "core/fatal.h"
#include <stdlib.h>
#include <string.h>
#include "contents/repute_groups.h"
#include "contents/repute_mgr.h"
#include "core/text_util.h"

struct ReputeGroupCopier *repute_groups_copy(struct ReputeGroupCopier *out,
                                                     struct TreeNode *first,
                                                     struct TreeNode *last,
                                                     void *repute_mgr, struct Tree *groups)
{
    struct TreeNode *it;

    for (it = first; it != last; it = tree_next(it)) {
        const struct ReputeGroupRecordPair *pair = TREE_VALUE(it, struct ReputeGroupRecordPair);
        int group_id = pair->group_id;
        struct ReputeGroupEntry *entry;
        struct ReputeGroupRecord rec;
        struct ReputeGroup group;
        struct String converted;
        struct String *text;
        const int *id;
        long long *wide;
        unsigned long long key;

        string_copy(&rec.id_text, &pair->record.id_text);
        string_copy(&rec.text, &pair->record.text);
        vec_copy(&rec.ids, &pair->record.ids, sizeof(int));

        string_init(&group.text);
        vec_init(&group.ids);

        text = text_utf8_to_codepage(&converted, &rec.text, g_codePage);
        string_assign(&group.text, string_cstr(text), text->size);
        string_free(&converted);

        wide = vec_add(&group.ids, VEC_COUNT(&rec.ids, int), sizeof *wide);
        for (id = rec.ids.first; id != rec.ids.last; id++)
            *wide++ = *id;

        key = (unsigned long long)(long long)group_id;
        entry = tree_get(groups, &key, sizeof key, sizeof *entry, tree_less_uint64);
        repute_group_assign(&entry->group, &group);

        vec_free(&group.ids);
        string_free(&group.text);
        vec_free(&rec.ids);
        string_free(&rec.text);
        string_free(&rec.id_text);
    }
    out->repute_mgr = repute_mgr;
    out->groups = groups;
    return out;
}

#define TABLE_NAME "ReputeGroup"

#define MSG_NOT_AN_INT   "잘못된 데이터가 입력되었" \
                         "습니다. intValue 가 입력되어야 " \
                         "합니다. (keyName : %s)"
#define MSG_DUPLICATE    "중복키가 입력되었습니다. " \
                         "(name : %s)"

static void field_int_list(struct JsonCppLoader *loader, const char *key, const struct JsonTree *row,
                           struct Vec *out)
{
    const struct JsonTree *list = json_loader_read_array(loader, key, row);
    size_t i;

    if (!list)
        return;
    for (i = 0; i < list->count; i++) {
        const struct JsonTree *e = &list->u.items[i];
        int value;

        if (e->type != JSON_TREE_UINT64 && e->type != JSON_TREE_INT64) {
            json_loader_set_error(loader, MSG_NOT_AN_INT, key);
            break;
        }
        value = (int)e->u.i64;
        vec_push(out, &value, sizeof(int));
    }
}

unsigned char repute_groups_read(struct JsonCppLoader *loader,
                                            struct Tree *records)
{
    const struct JsonTree *table;
    size_t i;

    table = json_loader_table(loader, TABLE_NAME, sizeof(TABLE_NAME) - 1);
    if (!table)
        return 0;
    if (table->type != JSON_TREE_OBJECT)
        return 0;

    for (i = 0; i < table->count; i++) {
        const struct JsonTreeMember *row = &table->u.members[i];
        struct String name, id_text, text;
        struct Vec ids;
        struct ReputeGroupRecordPair pair;
        int added;

        string_init(&name);
        string_assign(&name, row->key, row->key_length);
        string_init(&id_text);
        string_init(&text);
        vec_init(&ids);

        json_loader_read_string(loader, "ID", &row->value, &id_text);
        json_loader_read_string(loader, "Name", &row->value, &text);
        field_int_list(loader, "ReputeList", &row->value, &ids);

        pair.group_id = string_to_int(&name);
        string_copy(&pair.record.id_text, &id_text);
        string_copy(&pair.record.text, &text);
        vec_copy(&pair.record.ids, &ids, sizeof(int));
        tree_add(records, &pair, sizeof pair, tree_less_int, &added);
        if (!added) {
            repute_group_record_pair_destroy(&pair);
            json_loader_set_error(loader, MSG_DUPLICATE, string_cstr(&name));
            vec_free(&ids);
            string_free(&text);
            string_free(&id_text);
            string_free(&name);
            return 0;
        }
        vec_free(&ids);
        string_free(&text);
        string_free(&id_text);
        string_free(&name);
    }
    return loader->ok;
}
