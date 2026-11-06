#include "game/mode_callbacks.h"

/* Defined with their header, below their readers: the run loop (game/mode_run.c) sets
   them, the actors, effects and camera read whether this frame is skipped, and the follow
   helpers (game/mode_util.c) keep the follow times. */
unsigned g_followLastMoveTick;
unsigned g_followLostSinceTick;
int g_isSkipFrame = 0;
