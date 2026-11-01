#include "contents/renew_quest.h"

void ongoing_quest_entry_destroy(void *entry)
{
    quest_hunt_destroy(&((struct OngoingQuestEntry *)entry)->quest);
}
