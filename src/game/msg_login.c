#include "core/app.h"
#include "game/mode_msg.h"
#include "world/mode_manager.h"
#include "login/loginmode.h"

int game_mode_to_login(void)
{
    mode_mgr_switch(&g_modeMgr, 0, "login.rsw");
    return 0;
}

int game_mode_msg_go_to_login(void)
{
    g_mapServerAidPending = 0;
    g_loginStartMode = 3;
    return game_mode_to_login();
}
