#include "actor/attr_state.h"

/* Set while the player is blinded (actor/attr_state.c); the ground, the models and the
   screen sprites draw dark then. Apart from attr_state.c, which reaches the player's
   status effects, so that its readers do not. */
int g_playerBlind = 0;
