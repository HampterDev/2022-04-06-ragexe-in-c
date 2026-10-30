#include "session/csession.h"
#include "session/session_names.h"
#include "game/context.h"

#define STATUS_BERSERK  0x6b

int session_get_battlefield_state(struct Session *self)
{
    (void)self;
    return game_battleground_state();
}

int session_has_berserk(struct Session *self)
{
    (void)self;
    return own_states_find(STATUS_BERSERK) != NULL;
}
