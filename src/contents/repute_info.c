#include "contents/repute_mgr.h"

#define TABLE_NAME     "reputeInfo"
#define TABLE_NAME_LEN 10

unsigned char repute_info_read(struct JsonCppLoader *loader, struct Tree *records)
{
    const struct JsonTree *table;
    size_t i;
    int max_point_positive = 0, max_point_negative = 0;

    table = json_loader_table(loader, TABLE_NAME, TABLE_NAME_LEN);
    if (!table)
        return 0;
    if (table->type != JSON_TREE_OBJECT)
        return 0;

    for (i = 0; i < table->count; i++) {
        const struct JsonTreeMember *row = &table->u.members[i];
        struct String key_text, name, invisible;
        struct ReputeInfoRecordPair pair;
        int id, added;

        string_init(&key_text);
        string_assign(&key_text, row->key, row->key_length);
        string_init(&name);
        string_init(&invisible);

        json_loader_read_string(loader, "Name", &row->value, &name);
        json_loader_read_int(loader, "MaxPoint_Positive", &row->value, &max_point_positive);
        json_loader_read_int(loader, "MaxPoint_Negative", &row->value, &max_point_negative);
        json_loader_read_string(loader, "Invisible", &row->value, &invisible);

        id = string_to_int(&key_text);
        pair.id = id;
        string_copy(&pair.record.name, &name);
        pair.record.max_point_positive = max_point_positive;
        pair.record.max_point_negative = max_point_negative;
        string_copy(&pair.record.visibility, &invisible);
        tree_add(records, &pair, sizeof pair, tree_less_int, &added);
        if (!added) {
            repute_info_record_pair_destroy(&pair);
            json_loader_set_error(loader, JSON_MSG_DUPLICATE, string_cstr(&key_text));
            string_free(&invisible);
            string_free(&name);
            string_free(&key_text);
            return 0;
        }
        string_free(&invisible);
        string_free(&name);
        string_free(&key_text);
    }
    return loader->ok;
}
