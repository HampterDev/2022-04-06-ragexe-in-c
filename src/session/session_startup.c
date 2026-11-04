#include "session/csession.h"
#include "session/session_names.h"
#include "session/session_options.h"
#include "session/session_state.h"

#include <stdio.h>
#include <stdlib.h>
#include "platform/win_types.h"
#include "platform/language.h"
#include "ui/render/graphic_setting.h"
#include "platform/window.h"
#include "platform/clock.h"

void session_startup(struct Session *self)
{
    if (!item_info_load())
        platform_message_box(NULL, "ItemInfo file Init", "Error", 0);
    srand((unsigned)platform_time_now());
    session_init_table(self);
    map_info_load(g_mapInfo);
    session_load_options(self);
}

#define NUMBERS_ACT "이팩트\\숫자.act"
#define NUMBERS_SPR "이팩트\\숫자.spr"

static void hold_pair(const char *act, const char *spr)
{
    void *a = res_mgr_get(g_resMgr(), act);
    void *s = res_mgr_get(g_resMgr(), spr);
    res_lock(a);
    res_lock(s);
}

void session_pre_load_always_needed_res(struct Session *self)
{
    int saved_half_width, saved_half_height;

    hold_pair("shadow.act", "shadow.spr");
    saved_half_height = g_sprHalfHeight;
    saved_half_width = g_sprHalfWidth;
    g_sprHalfWidth = 0;
    g_sprHalfHeight = 0;
    hold_pair("cursors.act", "cursors.spr");
    hold_pair(NUMBERS_ACT, NUMBERS_SPR);
    g_sprHalfWidth = saved_half_width;
    g_sprHalfHeight = saved_half_height;
}
