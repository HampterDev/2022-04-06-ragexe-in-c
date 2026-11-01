#include "session/state.h"
#include "session/session_jobs.h"
#include "session/session_state.h"
#include "session/session_talk.h"
#include "attack_records_session.h"
#include <assert.h>

int attack_test_is_attack = -1, attack_test_monster_actor;

int session_is_attack(struct Session *self, int state)
{
    assert(self == &g_session1);
    return state == attack_test_is_attack;
}

BYTE session_pet_emotion_due(struct Session *self, __int64 interval)
{
    (void)self; (void)interval;
    assert(!"no pet in sight");
    return 0;
}

int session_pet_hunger_level(int fullness) { return fullness; }
int session_pet_intimacy_level(int intimacy) { return intimacy; }
int session_get_pet_emotion(struct Session *self, int hungerLevel, int intimacyLevel, int action)
{ (void)self; (void)hungerLevel; (void)intimacyLevel; (void)action; return 0; }

BYTE session_get_pet_acc_act_name(struct Session *self, int job) { (void)self; (void)job; return 0; }
