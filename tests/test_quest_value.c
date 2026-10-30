#include "contents/renew_quest.h"
#include "world_state_memory.h"
#include <stdio.h>

struct GuardedQuest {
    unsigned before[4];
    struct RenewQuestB value;
    unsigned after[4];
};

static void guard(const struct GuardedQuest *q)
{
    int i;
    for (i = 0; i < 4; ++i)
        assert(q->before[i] == 0xa5a5a5a5 && q->after[i] == 0xa5a5a5a5);
}

static void fill(struct RenewQuestB *q, size_t lines)
{
    size_t i;
    struct String text;
    struct RenewQuestReward reward;
    for (i = 0; i < 7; ++i)
        string_assign(&q->base.text[i], "A quest title longer than the inline string buffer", 49);
    string_assign_cstr(&q->reward_exp, "123456789012345678901234567890");
    string_assign_cstr(&q->reward_job_exp, "42");
    for (i = 0; i < lines; ++i) {
        string_init(&text);
        string_assign_cstr(&text, "Owned quest description with heap storage");
        string_vec_push_move(&q->description, &text);
        string_free(&text);
        reward.item_id = 100 + (unsigned)i;
        reward.count = 10 + (unsigned)i;
        vec_push(&q->rewards, &reward, sizeof reward);
    }
    q->plain.hunt_count = 3;
    q->plain.hunt_id[2] = 0x123456789abcdefLL;
    q->plain.deadline.year = 2037;
    q->base.npc_x = 21;
    q->base.npc_y = 54;
    q->cool_time_quest = -3;
    q->cool_time_quest_high = -1;
}

static void equal_values(const struct RenewQuestB *a, const struct RenewQuestB *b)
{
    const struct String *x, *y;
    size_t i, count = vec_count(&a->description, sizeof *x);
    assert(count == vec_count(&b->description, sizeof *x));
    assert(!memcmp(&a->plain, &b->plain, sizeof a->plain));
    assert(a->base.npc_x == b->base.npc_x && a->base.npc_y == b->base.npc_y);
    assert(a->cool_time_quest == b->cool_time_quest && a->cool_time_quest_high == b->cool_time_quest_high);
    for (i = 0; i < 7; ++i) {
        assert(!strcmp(string_cstr(&a->base.text[i]), string_cstr(&b->base.text[i])));
        assert(string_cstr(&a->base.text[i]) != string_cstr(&b->base.text[i]));
    }
    x = a->description.first; y = b->description.first;
    for (i = 0; i < count; ++i) {
        assert(!strcmp(string_cstr(x + i), string_cstr(y + i)));
        assert(string_cstr(x + i) != string_cstr(y + i));
    }
    count = vec_count(&a->rewards, sizeof(struct RenewQuestReward));
    assert(count == vec_count(&b->rewards, sizeof(struct RenewQuestReward)));
    if (count) {
        assert(a->rewards.first != b->rewards.first);
        assert(!memcmp(a->rewards.first, b->rewards.first, count * sizeof(struct RenewQuestReward)));
    }
    assert(!strcmp(string_cstr(&a->reward_exp), string_cstr(&b->reward_exp)));
    assert(!strcmp(string_cstr(&a->reward_job_exp), string_cstr(&b->reward_job_exp)));
}

int main(void)
{
    struct GuardedQuest a, b;
    struct RenewQuestB small;
    unsigned before;
    int i;
    memset(&a, 0xa5, sizeof a); memset(&b, 0xa5, sizeof b);
    quest_hunt_construct(&a.value);
    assert(a.value.base.active == 1 && a.value.shown == 1);
    assert(a.value.plain.hunt_count == 0 && !a.value.cool_time_quest && !a.value.cool_time_quest_high);
    assert(!a.value.description.first && !a.value.rewards.first);
    assert(!a.value.plain.deadline.year && !a.value.plain.deadline.minute);
    for (i = 0; i < 3; ++i) {
        assert(a.value.plain.hunt_mob[i] == RENEW_QUEST_ANY_MOB);
        assert(a.value.plain.hunt_min_level[i] == -1 && a.value.plain.hunt_max_level[i] == -1);
        assert(!a.value.plain.hunt_name[i][0] && !a.value.plain.hunt_id[i]);
    }
    fill(&a.value, 180);
    quest_hunt_copy(&b.value, &a.value); equal_values(&a.value, &b.value);
    before = live_blocks;
    quest_hunt_assign(&a.value, &a.value); assert(before == live_blocks);
    quest_hunt_construct(&small); fill(&small, 2);
    quest_hunt_assign(&b.value, &small); equal_values(&small, &b.value);
    quest_hunt_assign(&b.value, &a.value); equal_values(&a.value, &b.value);
    quest_hunt_destroy(&a.value); quest_hunt_destroy(&b.value); quest_hunt_destroy(&small);
    quest_hunt_destroy(&a.value); quest_hunt_destroy(&b.value);
    guard(&a); guard(&b); assert(!live_blocks);
    puts("Quest values: complete guarded records, initialized hunts, deep copies, growth/shrink, self assignment and repeat cleanup passed");
    return 0;
}
