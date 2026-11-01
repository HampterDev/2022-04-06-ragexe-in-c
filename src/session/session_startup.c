#include "session/csession.h"
#include "session/session_names.h"
#include "session/session_options.h"
#include "session/session_state.h"

#include "platform/embedded_fonts.h"
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <windows.h>
#include "platform/language.h"
#include "ui/render/graphic_setting.h"

void session_startup(struct Session *self)
{
    const char *missing_font;

    if (!item_info_load())
        MessageBoxA(NULL, "ItemInfo file Init", "Error", 0);
    srand((unsigned)_time32(NULL));
    session_init_table(self);
    map_info_load(g_mapInfo);
    session_load_options(self);
    if (g_serviceType != 0)
        return;
    embedded_fonts_load(&missing_font);
    if (missing_font) {
        char message[128];
        snprintf(message, sizeof message, "System\\Font\\%s 파일 없음!", missing_font);
        error_msg(message, 0);
        exit(EXIT_FAILURE);
    }
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
