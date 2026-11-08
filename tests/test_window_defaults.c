#include "session/state.h"
#include "render/renderer.h"
#include "ui/window_manager_state.h"
#include "ui/wndmgr/wndmgr.h"
#include "session/session_jobs.h"

#include <assert.h>
#include <stdio.h>
#include <string.h>

struct Session g_session1;
static struct Renderer renderer;
struct Renderer *g_renderer = &renderer;
static struct UIWindowMgr mgr;

int session_get_drawn_job(struct Session *self)
{
    assert(self == &g_session1);
    return 0;
}

BYTE session_is_fourth_job(struct Session *self, int job)
{
    assert(self == &g_session1 && job == 0);
    return 0;
}

int main(void)
{
    renderer.width = 1024;
    renderer.height = 768;
    memset(&mgr, 0, sizeof mgr);
    mgr.plate_width = 0x97;
    mgr.plate_height = 0x25;
    mgr.preview_equip_pos.x = mgr.preview_equip_pos.y = -1;
    mgr.swap_equipment_pos.x = mgr.swap_equipment_pos.y = -1;

    ui_window_mgr_set_default_rect_info(&mgr);

    assert(mgr.swap_equipment_pos.x == (1024 - 0x118) / 2);
    assert(mgr.swap_equipment_pos.y == (768 - 0xb1) / 2);
    assert(mgr.plate_width == 0x97 && mgr.plate_height == 0x25);
    assert(mgr.preview_equip_pos.x == (int)((1024 - 0x118) * 0.5f));
    assert(mgr.preview_equip_pos.y == (int)((768 - 0x104) * 0.5f));
    puts("Window defaults: swap-equipment position set; friend-list plate size preserved.");
    return 0;
}
