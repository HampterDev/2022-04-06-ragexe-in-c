#include "ui/window_access.h"
#include "render/renderer.h"
#include "ui/controls/agit.h"
#include "game/context.h"
#include "ui/controls/controls.h"
#include "ui/window_mgr.h"
#include "ui/game.h"
#include "ui/ui_items.h"
#include "ui/runtime.h"
#include "core/object_type_tables.h"
#include "core/msg_str.h"

#define MSG_AGIT_BOX_TITLE 0xe38
#define AGIT_BOX_YES 0xb3
#define AGIT_BOX_NO 0xb4
#define MODE_REQ_MOVE_GUILD_AGIT 0x13b
#define AGIT_BOX_WIDTH 0x136
#define AGIT_BOX_HEIGHT 0x82

void agit_message_box_open(WORD agit_id, const char *text)
{
    struct UIAgitMessageBox *box = mem_alloc(sizeof *box);

    if (box) {
        ui_frame_wnd_construct(&box->frame, 0);
        box->frame.base.ops = &ui_agit_message_box_ops;
        box->agit_id = 0;
    }
    ui_agit_message_box_create(box, text, AGIT_BOX_WIDTH, AGIT_BOX_HEIGHT);
    ui_frame_wnd_move(&box->frame.base, g_renderer->centreX - box->frame.base.width / 2,
             g_renderer->centreY - box->frame.base.height / 2);
    box->agit_id = agit_id;
    ui_window_mgr_add_window(&g_windowMgr, &box->frame.base);
}

void ui_agit_message_box_on_draw(struct UIWindow *window)
{
    struct UIAgitMessageBox *self = (struct UIAgitMessageBox *)window;
    struct UIWindow *wnd = &self->frame.base;

    ui_window_draw_title_bar(wnd, 1, msg_str(MSG_AGIT_BOX_TITLE), 0);
    ui_window_draw_body(wnd, 0x11, 0, "");
    ui_window_draw_button_bar(wnd, 2, 0, 1);
}

int ui_agit_message_box_send_msg(struct UIWindow *window, struct UIWindow *sender, int msg, int val1,
                                        int val2, int val3, int val4)
{
    struct UIAgitMessageBox *self = (struct UIAgitMessageBox *)window;
    if (msg != UI_MSG_COMMAND)
        return AGIT_BOX_NO;
    ui_window_record_command(&self->frame.base, val1, val2, val3, val4);
    if (val1 == AGIT_BOX_YES) {
        game_send_message(MODE_REQ_MOVE_GUILD_AGIT, self->agit_id, 0, 0, 0);
        ui_window_mgr_post_quit2(&g_windowMgr, &self->frame.base);
    } else if (val1 == AGIT_BOX_NO) {
        ui_window_mgr_post_quit2(&g_windowMgr, &self->frame.base);
    }
    return AGIT_BOX_NO;
}
