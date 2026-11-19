/* Which skills leave their user as it is: the action session_get_use_effect gives a skill's
   user, with 0 for "no pose". The server holds a skill's pose in the user's place
   (ring-buffer consumers/eos-combat/skill-pose.js) and has a copy of this list, so a change
   here has to reach it. A skill with a begin effect in the client's Lua data gets the skill
   pose whatever this list says; that data is not in the test, which reads the code alone. */
#include "session/csession.h"
#include "session/session_skills.h"
#include <assert.h>
#include <stdio.h>

int session_get_begin_effect_id(int id)
{
    (void)id;
    return -1;
}

int main(void)
{
    static const int none[] = {
        0x1d, 0x97, 0xc4, 0xd1, 0xf9, 0xfc, 0x100, 0x101, 0x105, 0x10c, 0x10d, 0x130, 0x15d, 0x15e, 0x164, 0x171, 0x17c,
        0x180, 0x183, 0x191, 0x19b, 0x1bc, 0x2b2, 0x2da, 0x3f7,
    };
    int skill, effect, action, found = 0, wrong = 0;
    size_t i;

    for (skill = 0; skill < 0x1000; skill++) {
        int listed = 0;

        effect = 0;
        action = -1;
        session_get_use_effect(NULL, skill, &effect, &action);
        for (i = 0; i < sizeof none / sizeof none[0]; i++)
            if (none[i] == skill)
                listed = 1;
        if ((action == 0) != listed) {
            fprintf(stderr, "skill 0x%x: action %d, %s\n", skill, action, listed ? "listed as none" : "not listed");
            wrong++;
        }
        found += action == 0;
    }
    assert(!wrong && found == (int)(sizeof none / sizeof none[0]));
    /* Bash keeps the attack's action, a spell the skill pose */
    session_get_use_effect(NULL, 5, &effect, &action);
    assert(action == 2);
    session_get_use_effect(NULL, 19, &effect, &action);
    assert(action == 7);
    puts("skill_poses: ok");
    return 0;
}
