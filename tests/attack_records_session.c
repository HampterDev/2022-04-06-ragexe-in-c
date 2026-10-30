#include "session/state.h"
#include "session/session_jobs.h"
#include "session/session_state.h"
#include "session/session_talk.h"
#include "attack_records_session.h"
#include <assert.h>
#include <string.h>

int attack_test_talk_line, attack_test_is_attack = -1, attack_test_monster_actor;

int session_is_attack(struct Session *self, int state)
{
    assert(self == &g_session1);
    return state == attack_test_is_attack;
}

BYTE session_pet_talk_due(struct Session *self, __int64 interval)
{
    (void)self; (void)interval;
    assert(!"no pet in sight");
    return 0;
}

int session_pet_hunger_level(int fullness) { return fullness; }
int session_pet_intimacy_level(int intimacy) { return intimacy; }
int session_get_pet_emotion(struct Session *self, int hungerLevel, int intimacyLevel, int action)
{ (void)self; (void)hungerLevel; (void)intimacyLevel; (void)action; return 0; }
int session_make_pet_talk_id(int job, int action, int hunger) { (void)job; (void)action; (void)hunger; return 0; }

void session_get_monster_talk_info(struct Session *self, int job, int state, int value, char *out, int aid)
{
    assert(self == &g_session1 && job == 1002 && state == 0 && value == 0 && aid == 100);
    strcpy(out, attack_test_talk_line ? "Poring!" : "");
}

BYTE session_get_pet_acc_act_name(struct Session *self, int job) { (void)self; (void)job; return 0; }
