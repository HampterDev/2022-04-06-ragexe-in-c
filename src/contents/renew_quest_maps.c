#include "contents/renew_quest.h"

void recommended_quest_entry_destroy(void *entry)
{
    renew_quest_a_destruct(&((struct RecommendedQuestEntry *)entry)->quest);
}

void ongoing_quest_entry_destroy(void *entry)
{
    quest_hunt_destroy(&((struct OngoingQuestEntry *)entry)->quest);
}

struct RenewQuestA *renew_quest_a_at(struct Tree *map, unsigned key)
{
    struct RecommendedQuestEntry *entry = tree_find(map, &key, tree_less_uint);
    struct RecommendedQuestEntry made;

    if (!entry) {
        made.id = key;
        renew_quest_a_construct(&made.quest);
        entry = tree_add(map, &made, sizeof made, tree_less_uint, NULL);
    }
    return &entry->quest;
}
