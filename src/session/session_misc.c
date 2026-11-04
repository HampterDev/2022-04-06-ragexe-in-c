#include "session/csession.h"
#include "session/session_jobs.h"
#include "session/session_names.h"
#include "session/session_state.h"

#include <mmsystem.h>

#include <ctype.h>
#include <stdio.h>
#include <string.h>

struct IntPair2 {
    int key, value;
};
void session_set_combo_delay(struct Session *self, int key, int value)
{
    struct Vec *v = &self->comboDelays;
    struct IntPair2 *first = (struct IntPair2 *)v->first;
    struct IntPair2 *last = (struct IntPair2 *)v->last;
    int count = (int)((BYTE *)last - (BYTE *)first) >> 3;
    struct IntPair2 pair;
    int i;

    for (i = 0; i < count; i++) {
        if (first[i].key == key) {
            ((struct IntPair2 *)v->first)[i].value = value;
            return;
        }
    }
    pair.key = key;
    pair.value = value;
    vec_push(v, &pair, sizeof pair);
}

int session_get_max_base_level(struct Session *self, int job)
{
    if (session_is_fourth_job(&g_session1, job) == 1)
        return g_session1.baseLevel4th;
    if (session_is_third_job(&g_session1, job))
        return g_session1.baseLevel3rd;
    if (session_is_expanded_super_novice_job(&g_session1, job))
        return g_session1.baseLevelExtend2;
    if (session_is_expanded_job(&g_session1, job))
        return g_session1.baseLevelUpperJob;
    return session_is_doram_job(&g_session1, job) == 1 ? g_session1.baseLevelDoram : g_session1.baseLevel;
}

int session_get_max_job_level(struct Session *self, int job)
{
    if (session_is_super_novice_job(&g_session1, job))
        return g_session1.jobLevelSuperNovice;
    if ((job >= 0xfa8 && job < 0xfb7) || job == 0x18 || job == 0x19 || job == 0x1084 || job == 0x107e)
        return g_session1.jobLevel2nd;
    if (job == 0 || job == 0xfb7 || job == 0xfa1)
        return g_session1.jobLevelNovice;
    if (session_is_fourth_job(&g_session1, job) == 1)
        return g_session1.jobLevel4th;
    if (session_is_third_job(&g_session1, job))
        return g_session1.jobLevel3rd;
    if (session_is_expanded_job(&g_session1, job))
        return g_session1.jobLevelUpperJob;
    if (session_is_doram_job(&g_session1, job) == 1)
        return g_session1.jobLevelDoram;
    if (session_is_expanded_super_novice_job(&g_session1, job))
        return g_session1.jobLevelUpperJob;
    return g_session1.jobLevelBase;
}

BYTE session_pet_talk_due(struct Session *self, __int64 interval)
{
    DWORD now = timeGetTime();

    if ((__int64)(int)now < (__int64)self->lastPetTalkTime + interval)
        return 0;
    self->lastPetTalkTime = (int)now;
    return 1;
}

BYTE session_reset_for_replay(struct Session *self)
{
    self->charListCounts[1] = 1;
    self->charListCounts[2] = 1;
    self->charListCounts[3] = 1;
    self->charListCounts[4] = 1;
    self->charListCounts[6] = 1;
    self->charListCounts[0] = 1;
    self->charCount = 1;
    self->petJob = -1;
    self->ownIllusion = 0;
    self->serverTimeOffset = 0;
    self->serverTimeLagCount = 0;
    return 1;
}

static int count_distinct_indexes(const struct ItemInfo *items)
{
    struct Vec seen;
    int count = 0;
    int i;

    seen.first = seen.last = seen.end = NULL;
    for (i = 0; i < 10; i++) {
        int index = items[i].index;
        int *p;

        if (index == 0)
            continue;
        for (p = seen.first; p != seen.last && *p != index; p++)
            ;
        if (p != seen.last)
            continue;
        vec_push(&seen, &index, sizeof(int));
        count++;
    }
    vec_free(&seen);
    return count;
}

int session_count_worn_costumes(struct Session *self)
{
    return count_distinct_indexes(self->costumeSlots);
}

int session_count_worn_equips(struct Session *self)
{
    return count_distinct_indexes(self->equipSlots);
}

const char *session_get_map_display_name(struct Session *self)
{
    struct String name;
    const char *found;

    string_init(&name);
    string_assign_cstr(&name, self->mapName);
    string_append_cstr(&name, ".rsw");
    found = session_map_display_name(string_cstr(&name));
    string_free(&name);
    return found;
}
