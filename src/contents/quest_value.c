#include "std/std.h"
#include <string.h>
#include "contents/renew_quest.h"
#include "ui/ui_std.h"

struct RenewQuestBase *renew_quest_base_construct(struct RenewQuestBase *quest)
{
    int i;
    memset(quest, 0, sizeof *quest);
    quest->active = 1;
    for (i = 0; i < 7; ++i)
        string_init(&quest->text[i]);
    return quest;
}

void renew_quest_base_destruct(struct RenewQuestBase *quest)
{
    int i;
    for (i = 6; i >= 0; --i)
        string_free(&quest->text[i]);
}

static void quest_base_assign(struct RenewQuestBase *quest, const struct RenewQuestBase *from)
{
    int i;
    quest->active = from->active;
    for (i = 0; i < 7; ++i)
        string_assign(&quest->text[i], string_cstr(&from->text[i]), from->text[i].size);
    quest->npc_x = from->npc_x;
    quest->npc_y = from->npc_y;
}

struct RenewQuestBase *renew_quest_base_copy(struct RenewQuestBase *self,
                                                        const struct RenewQuestBase *src)
{
    int i;

    self->active = src->active;
    for (i = 0; i < 7; i++)
        string_copy(&self->text[i], &src->text[i]);
    self->npc_x = src->npc_x;
    self->npc_y = src->npc_y;
    return self;
}

static void copy_plain_fields(struct RenewQuestB *quest, const struct RenewQuestB *from)
{
    quest->plain = from->plain;
}

struct RenewQuestB *quest_hunt_construct(struct RenewQuestB *quest)
{
    int i;
    memset(quest, 0, sizeof *quest);
    renew_quest_base_construct(&quest->base);
    quest->shown = 1;
    string_init(&quest->reward_exp);
    string_init(&quest->reward_job_exp);
    for (i = 0; i < RENEW_QUEST_HUNT_MAX; ++i) {
        quest->plain.hunt_mob[i] = RENEW_QUEST_ANY_MOB;
        quest->plain.hunt_min_level[i] = -1;
        quest->plain.hunt_max_level[i] = -1;
    }
    return quest;
}

struct RenewQuestB *quest_hunt_copy(struct RenewQuestB *quest, const struct RenewQuestB *from)
{
    renew_quest_base_copy(&quest->base, &from->base);
    quest->shown = from->shown;
    string_vec_copy(&quest->description, &from->description);
    copy_plain_fields(quest, from);
    string_copy(&quest->reward_exp, &from->reward_exp);
    string_copy(&quest->reward_job_exp, &from->reward_job_exp);
    vec_copy(&quest->rewards, &from->rewards, sizeof(struct RenewQuestReward));
    quest->cool_time_quest = from->cool_time_quest;
    quest->cool_time_quest_high = from->cool_time_quest_high;
    return quest;
}

struct RenewQuestB *quest_hunt_assign(struct RenewQuestB *quest, const struct RenewQuestB *from)
{
    if (quest == from)
        return quest;
    quest_base_assign(&quest->base, &from->base);
    quest->shown = from->shown;
    if (&quest->description != &from->description)
        string_vec_assign(&quest->description, from->description.first, from->description.last);
    copy_plain_fields(quest, from);
    string_assign(&quest->reward_exp, string_cstr(&from->reward_exp), from->reward_exp.size);
    string_assign(&quest->reward_job_exp, string_cstr(&from->reward_job_exp), from->reward_job_exp.size);
    vec_assign(&quest->rewards, from->rewards.first, from->rewards.last, sizeof(struct RenewQuestReward));
    quest->cool_time_quest = from->cool_time_quest;
    quest->cool_time_quest_high = from->cool_time_quest_high;
    return quest;
}

void quest_hunt_destroy(struct RenewQuestB *quest)
{
    vec_free(&quest->rewards);
    string_free(&quest->reward_job_exp);
    string_free(&quest->reward_exp);
    string_vec_free(&quest->description);
    renew_quest_base_destruct(&quest->base);
}
