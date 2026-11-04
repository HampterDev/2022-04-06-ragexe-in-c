#include "session/csession.h"
#include "session/session_jobs.h"
#include "session/session_names.h"
#include "session/session_state.h"


#include <ctype.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include "platform/clock.h"

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
    return g_session1.baseLevel;
}

int session_get_max_job_level(struct Session *self, int job)
{
    if (session_is_super_novice_job(&g_session1, job))
        return g_session1.jobLevelSuperNovice;
    if ((job >= 0xfa8 && job < 0xfb7) || job == 0x18 || job == 0x19 || job == 0x1084 || job == 0x107e)
        return g_session1.jobLevelBase;
    if (job == 0 || job == 0xfb7 || job == 0xfa1)
        return g_session1.jobLevelNovice;
    return g_session1.jobLevel2nd;
}

BYTE session_pet_emotion_due(struct Session *self, int64_t interval)
{
    DWORD now = platform_ticks_ms();

    if ((int64_t)(int)now < (int64_t)self->lastPetEmotionTime + interval)
        return 0;
    self->lastPetEmotionTime = (int)now;
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
