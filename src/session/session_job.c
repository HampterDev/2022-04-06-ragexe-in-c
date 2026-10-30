#include "session/character_name.h"
#include "session/state.h"
#include "ui/window_manager_state.h"
#include "session/csession.h"
#include "session/session_jobs.h"
#include "session/session_talk.h"
#include "session/session_windows.h"
#include "game/context.h"

#include <string.h>
#include "ui/character/gauge.h"

#define STATE_BIT(n) (((DWORD)g_session1.effectState >> (n)) & 1)

static int rune_knight_dragon_job(int job, int dragon_offset, int base)
{
    if (STATE_BIT(19))
        return job + dragon_offset;
    if (STATE_BIT(23))
        return base;
    if (STATE_BIT(24))
        return base + 2;
    if (STATE_BIT(25))
        return base + 4;
    if (STATE_BIT(26))
        return base + 6;
    return job;
}

int session_get_drawn_job(struct Session *self)
{
    int job = self->job;

    switch (job) {
    case 0x7:    return STATE_BIT(5) ? job + 0x6 : job;
    case 0xe:    return STATE_BIT(5) ? job + 0x7 : job;
    case 0xfa8:  return STATE_BIT(5) ? job + 0x6 : job;
    case 0xfbe:  return STATE_BIT(5) ? job + 0x6 : job;
    case 0xfaf:  return STATE_BIT(5) ? job + 0x7 : job;
    case 0xfc5:  return STATE_BIT(5) ? job + 0x7 : job;
    case 0xfe2:  return STATE_BIT(5) ? job + 0x10 : job;
    case 0xfe9:  return STATE_BIT(5) ? job + 0xa : job;
    case 0x1006: return STATE_BIT(5) ? job + 0x8 : job;
    case 0xfd6:  return rune_knight_dragon_job(job, 0x1a, 0xff8);
    case 0xfdc:  return rune_knight_dragon_job(job, 0x15, 0xff9);
    case 0x1000: return STATE_BIT(19) ? job + 0xd : job;
    case 0xfcf:  return STATE_BIT(15) ? job + 0x1 : job;
    case 0xfd8:  return STATE_BIT(21) ? job + 0x1c : job;
    case 0xfde:  return STATE_BIT(21) ? job + 0x17 : job;
    case 0x1002: return STATE_BIT(21) ? job + 0xd : job;
    case 0xfda:  return g_session1.madogear ? job + 0x1c : job;
    case 0xfe0:  return g_session1.madogear ? job + 0x17 : job;
    case 0x1004: return g_session1.madogear ? job + 0xc : job;
    case 0x109d: return g_session1.madogear ? job + 0x1a : job;
    case 0x1082: return STATE_BIT(15) ? job + 0xc : job;
    case 0x1091: return STATE_BIT(15) ? job + 0x3 : job;
    case 0x10a1: return STATE_BIT(21) ? job + 0x15 : job;
    case 0x109c: return STATE_BIT(19) ? job + 0x1c : job;
    case 0x10a2: return STATE_BIT(5) ? job + 0x17 : job;
    case 0x10ce: return STATE_BIT(15) ? job + 0xe : job;
    default:     return job;
    }
}

float session_get_job_float(struct Session *self,
                                         int job, int b, int c, int mode)
{
    if (mode == 0) {
        if (job == 5)
            return 5.85f;
        if (job == 6)
            return 5.75f;
        return 6.0f;
    }
    if (mode != 1)
        return 6.0f;
    switch (job) {
    case 0xc: case 0xfad: case 0xfc3: case 0xfdb: case 0xfe1: case 0x1005: case 0x109e:
        if (c == 0x10 || (unsigned)(c - 0x19) <= 5)
            return 3.0f;
        return 6.0f;
    case 0: case 0x17: case 0xfcd: case 0x105b: case 0x105c:
        if (b == 1)
            return 5.85f;
        return 6.0f;
    default:
        return 6.0f;
    }
}

void session_set_job(struct Session *self, int job)
{
    int old = self->job;
    BYTE before;

    self->job = job;
    before = session_is_fourth_job(self, old);
    if (before != session_is_fourth_job(self, job)) {
        if (g_windowMgr.basic_info_wnd)
            ui_basic_info_wnd_apply_job_layout((struct UIBasicInfoWnd *)g_windowMgr.basic_info_wnd);
        game_send_message(0x144, 0, 0, 0, 0);
    }
    session_invalidate_windows(self);
    session_refresh_skill_tree(self);
}
