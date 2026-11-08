#include "world/mode_manager.h"
#include "game/context.h"
#include "world/mode_actions.h"

struct ModeMgr g_modeMgr = { 1 };

struct Mode *client_current_mode(void)
{
    return g_modeMgr.curMode;
}

struct Mode *game_current_base_mode(void)
{
    return g_modeMgr.curModeType == MODE_TYPE_GAME ? g_modeMgr.curMode : NULL;
}

void mode_nothing(struct Mode *self)
{
}

struct GameMode *game_current_mode(void)
{
    return (struct GameMode *)game_current_base_mode();
}
