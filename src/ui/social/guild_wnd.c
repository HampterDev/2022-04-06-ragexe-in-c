#include "core/abstract_method.h"
#include "contents/guild_info.h"
#include "ui/window_info.h"
#include "world/mode_manager.h"
#include "ui/social/social.h"
#include "ui/social/guild_wnd.h"
#include "core/object_type_tables.h"
#include "core/msg_str.h"
#include "diag/perf.h"
#include "ui/wndmgr/make_window_internal.h"

const struct GuildLayout g_guildLayout = {
    {82, 292}, 19,
    {151, 82, 70, 70}, 19,
    {98, 65, 53, 39, 39, 76, 50, 100, 60, 60, 60, 60}, 19
};

#define GUILD_WND_ID 0x3b
#define GUILD_TAB_WND_ID_FIRST 0x3c
#define GUILD_TAB_COUNT 6
#define GUILD_TAB_LAST 6
#define GUILD_TAB_WIDTH 0x42

#define MSG_GUILD_SWITCH_TAB 0x45
#define UI_MSG_INVALIDATE_CHILDREN 0x3c

static const unsigned guild_tab_rights[7] = { 0, 1, 2, 4, 0x10, 0x80, 0x40 };

static int guild_tab(struct UIGuildWnd *self)
{
    const struct UIGuildWndOps *ops = (const struct UIGuildWndOps *)self->frame.base.ops;
    if (ops->tab < 0)
        abstract_method_called();
    return ops->tab;
}

struct UIGuildWnd *ui_guild_wnd_construct(struct UIGuildWnd *self)
{
    int i;

    ui_frame_wnd_construct(&self->frame, 0);
    self->frame.base.ops = &ui_guild_wnd_ops.base;
    self->info = NULL;
    for (i = 0; i < 6; i++)
        self->tab_labels[i] = 0;
    return self;
}

void ui_guild_wnd_destruct(struct UIGuildWnd *self)
{
    self->frame.base.ops = &ui_guild_wnd_ops.base;
    ui_frame_wnd_destruct(&self->frame);
}

void ui_guild_wnd_destroy(struct UIWindow *window)
{
    struct UIGuildWnd *self = (struct UIGuildWnd *)window;
    ui_guild_wnd_destruct(self);
    mem_free(self);
}

static DWORD skin_rgb(int x, int y)
{
    int r, g, b;

    ui_skin_color_chip(&g_skinMgr, x, y, &r, &g, &b);
    return RGB(r, g, b);
}

void ui_guild_wnd_draw_tabs(struct UIGuildWnd *self, int tab)
{
    struct UIWindow *wnd = &self->frame.base;
    int i;

    ui_window_fill_rect(wnd, 0, 0x11, wnd->width, 0x18, skin_rgb(0xa, 2));
    for (i = 0; i < GUILD_TAB_COUNT; i++) {
        DWORD colour = i == tab ? skin_rgb(2, 2) : skin_rgb(0xe, 6);

        ui_window_fill_rect(wnd, 1 + GUILD_TAB_WIDTH * i, 0x12, 0x40, 0x17, colour);
    }
}

void ui_guild_wnd_on_draw(struct UIWindow *window)
{
    struct UIGuildWnd *self = (struct UIGuildWnd *)window;
    struct UIWindow *wnd = &self->frame.base;

    ui_guild_wnd_draw_tabs(self, guild_tab(self));
    ui_window_draw_title_bar(wnd, 0, "", 0);
    ui_window_fill_rect(wnd, 0, 0x29, wnd->width, wnd->height - 0x29, skin_rgb(0xe, 6));
    ui_window_fill_rect(wnd, 1, 0x29, wnd->width - 2, wnd->height - 0x29, skin_rgb(2, 2));
    ui_window_draw_button_bar(wnd, 2, 0, 1);
}

void ui_guild_wnd_on_l_btn_down(struct UIWindow *window, int x, int y)
{
    struct UIGuildWnd *self = (struct UIGuildWnd *)window;
    int tab, clicked;

    if (y < 0x11) {
        ui_frame_wnd_on_l_btn_down((struct UIWindow *)(&self->frame), x, y);
        return;
    }
    if (y > 0x29)
        return;
    tab = guild_tab(self);
    guild_tab(self);
    clicked = x / GUILD_TAB_WIDTH;
    if (clicked < GUILD_TAB_COUNT && tab != clicked)
        ui_window_send_msg(&self->frame.base, NULL, MSG_GUILD_SWITCH_TAB, clicked, 0, 0, 0);
}

void ui_guild_wnd_store_info(struct UIWindow *window)
{
    struct UIGuildWnd *self = (struct UIGuildWnd *)window;
    struct UIWindow *wnd = &self->frame.base;

    if (!self->info)
        return;
    self->info->frame.x = wnd->x;
    self->info->frame.y = wnd->y;
    self->info->frame.width = wnd->width;
    self->info->frame.height = wnd->height;
    self->info->tab = guild_tab(self);
}

intptr_t ui_guild_wnd_send_msg(struct UIWindow *window, struct UIWindow *sender, int msg, intptr_t val1, intptr_t val2,
                                  intptr_t val3, intptr_t val4)
{
    struct UIGuildWnd *self = (struct UIGuildWnd *)window;
    switch (msg) {
    case MSG_GUILD_SWITCH_TAB: {
        int tab = val1;

        if (tab >= 1 && tab <= 6) {
            if (tab == 6)
                return 0;
            if (tab != 3 && !(g_guildInfo.menu_flags & guild_tab_rights[tab]))
                return 0;
        }
        ui_window_mgr_post_quit1(&g_windowMgr, GUILD_WND_ID);
        if ((unsigned)tab <= GUILD_TAB_LAST)
            ui_make_window(GUILD_TAB_WND_ID_FIRST + tab);
        return 0;
    }
    case UI_MSG_INVALIDATE_CHILDREN:
        ui_window_invalidate_children(&self->frame.base);
        return 0;
    case UI_MSG_SET_INFO:
        self->info = (struct GuildWndInfo *)val1;
        if (!guild_wnd_info_is_valid_info(self->info))
            guild_wnd_info_make_default(self->info);
        ui_frame_wnd_move(&self->frame.base, self->info->frame.x, self->info->frame.y);
        return 0;
    }
    return ui_frame_wnd_send_msg((struct UIWindow *)(&self->frame), sender, msg, val1, val2, val3, val4);
}

#define MODE_REQ_GUILD_RIGHTS 0x69
#define MSG_GUILD_TAB_FIRST 0x154
#define GUILD_TAB_LABEL_X 3
#define GUILD_TAB_LABEL_Y 0x16

void ui_guild_wnd_init_tab_text(struct UIGuildWnd *self)
{
    struct UIWindow **labels = (struct UIWindow **)self->tab_labels;
    const char *texts[GUILD_TAB_COUNT];
    int i, x;

    if (labels[0])
        return;
    if (g_guildInfo.menu_flags == 0xffffffff)
        mode_send_message(client_current_mode(), MODE_REQ_GUILD_RIGHTS, 0, 0, 0, 0);
    for (i = 0; i < GUILD_TAB_COUNT; i++)
        texts[i] = msg_str(MSG_GUILD_TAB_FIRST + i);
    for (i = 0, x = GUILD_TAB_LABEL_X; x < 0x18f; i++, x += GUILD_TAB_WIDTH) {
        struct UIWindow *label = mem_alloc(sizeof(struct UIStaticText));

        label = label ? ui_static_text_construct(label) : NULL;
        labels[i] = label;
        ui_window_create(label, 0x3e, 0xf);
        ui_static_text_set_text_ellipsis(labels[i], texts[i]);
        ui_window_move(labels[i], x, GUILD_TAB_LABEL_Y);
        ui_window_add_child(&self->frame.base, labels[i]);
    }
    ui_window_add_close_button(&self->frame.base, "Alt+G", 0);
}
