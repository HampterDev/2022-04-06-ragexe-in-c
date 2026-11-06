#include "game/actor_names.h"
#include "game/gamemode.h"
#include "web/name_card_api.h"

/* The name the game mode knows for an actor. Apart from game/actor_names.c, whose name
   packets reach the pet windows, so that the actors, the popup menu and the session asking
   for a name do not. */

struct String *game_mode_get_actor_name(struct GameMode *self, struct String *out, int aid)
{
    name_card_holder_name(&self->actor_names, out, aid);
    return out;
}
