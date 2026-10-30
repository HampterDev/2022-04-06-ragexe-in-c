#include "contents/repute_mgr.h"

void repute_info_record_pair_destroy(void *value)
{
    struct ReputeInfoRecordPair *pair = value;

    string_free(&pair->record.visibility);
    string_free(&pair->record.name);
}

void repute_group_record_pair_destroy(void *value)
{
    struct ReputeGroupRecordPair *pair = value;

    vec_free(&pair->record.ids);
    string_free(&pair->record.text);
    string_free(&pair->record.id_text);
}
