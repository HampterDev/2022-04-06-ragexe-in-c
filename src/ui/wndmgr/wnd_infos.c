#include "session/state.h"
#include "ui/window_manager_state.h"
#include "ui/wndmgr/wndmgr.h"
#include "ui/wndmgr/make_window_internal.h"
#include "session/session_jobs.h"

int g_guildWndHeight = 317;

int g_guildWndWidth = 400;

static BYTE g_wndInfoDefaultsPending = 1u;

static int frame_inside(const struct WindowInfo *info)
{
    return info->x >= 0 && info->x <= g_renderer->width - info->width
           && info->y >= 0 && info->y <= g_renderer->height - info->height;
}

static int frame_exact(const struct WindowInfo *info, int width, int height)
{
    return info->width == width && info->height == height && frame_inside(info);
}

static int frame_at_least(const struct WindowInfo *info, int width, int height)
{
    return info->width >= width && info->height >= height && frame_inside(info);
}

static void frame_set(struct WindowInfo *info, int x, int y, int width, int height)
{
    info->x = x;
    info->y = y;
    info->width = width;
    info->height = height;
}

#define BASIC_INFO_WIDTH 0xdc
#define BASIC_INFO_SHORT 0x37
#define BASIC_INFO_TALL 0x86
#define BASIC_INFO_SHORT_ALT 0x47
#define BASIC_INFO_TALL_ALT 0x95

static int basic_info_alt_job(void)
{
    return session_is_fourth_job(&g_session1, session_get_drawn_job(&g_session1)) == 1;
}

int basic_info_wnd_info_is_on_screen(struct BasicInfoWndInfo *self)
{
    int height;

    if (self->rect.width != BASIC_INFO_WIDTH)
        return 0;
    height = self->rect.height;
    if (height == BASIC_INFO_SHORT || height == BASIC_INFO_SHORT_ALT)
        self->tall = 0;
    else if (height == BASIC_INFO_TALL || height == BASIC_INFO_TALL_ALT)
        self->tall = 1;
    else
        return 0;
    if (basic_info_alt_job()) {
        if (height != BASIC_INFO_SHORT_ALT && height != BASIC_INFO_TALL_ALT)
            return 0;
    } else if (height != BASIC_INFO_SHORT && height != BASIC_INFO_TALL) {
        return 0;
    }
    return frame_inside(&self->rect);
}

void basic_info_wnd_info_set_default(struct BasicInfoWndInfo *self)
{
    frame_set(&self->rect, 0, 0, BASIC_INFO_WIDTH, BASIC_INFO_TALL);
    if (basic_info_alt_job())
        self->rect.height = self->tall == 1 ? BASIC_INFO_TALL_ALT : BASIC_INFO_SHORT_ALT;
    else
        self->rect.height = self->tall == 1 ? BASIC_INFO_TALL : BASIC_INFO_SHORT;
    self->expanded = 0;
}

int ach_tracing_info_is_on_screen(POINT *self)
{
    return self->x > 0 && self->x <= g_renderer->width - 300 && self->y > 0 && self->y <= g_renderer->height - 150;
}

void ach_tracing_info_set_default(POINT *self)
{
    self->x = g_renderer->width - 350;
    self->y = (g_renderer->height - 150) / 2;
}

int chat_room_info_is_valid(struct WindowInfo *self)
{
    return frame_at_least(self, 0x118, 0x78);
}

void chat_room_info_set_default(struct WindowInfo *self)
{
    frame_set(self, 500, 180, 0x118, 0x78);
}

int npc_menu_info_is_valid(struct WindowInfo *self)
{
    return frame_exact(self, 0x118, 0x78);
}

void npc_menu_info_set_default(struct WindowInfo *self)
{
    frame_set(self, 200, 300, 0x118, 0x78);
}

int say_dialog_info_is_valid(struct WindowInfo *self)
{
    return frame_exact(self, 0x118, 0xb4);
}

void say_dialog_info_set_default(struct WindowInfo *self)
{
    frame_set(self, 200, 100, 0x118, 0xb4);
}

int n_cash_info_is_on_screen(struct WindowInfo *self)
{
    return ui_info_on_screen(self);
}

void n_cash_info_set_default(struct WindowInfo *self)
{
    frame_set(self, 0, 0x1e, 0x280, 0x1fc);
}

int equip_wnd_info_is_valid(struct WndInfoTall *self)
{
    int swap = g_session1.equipSwapOpen == 1 ? 0x9e : 0;
    int height = self->rect.height;

    if (self->rect.width != 0x118)
        return 0;
    if (height != 0xa7 && height != 0x11 && height != 0xbb && height != 0xbb + swap)
        return 0;
    if (self->org_height != 0xa7 && self->org_height != 0xbb)
        return 0;
    return self->rect.x >= 0 && self->rect.x <= g_renderer->width - 0x118 && self->rect.y >= 0
           && self->rect.y <= g_renderer->height - height;
}

void equip_wnd_info_set_default(struct WndInfoTall *self)
{
    frame_set(&self->rect, 0xb9, 0x12c, 0x118, 0xa7);
    self->org_height = 0xa7;
    self->show = 0;
}

int guild_wnd_info_is_valid_info(struct GuildWndInfo *self)
{
    return frame_exact(&self->frame, g_guildWndWidth, g_guildWndHeight) && (unsigned)self->tab <= 6;
}

void guild_wnd_info_make_default(struct GuildWndInfo *self)
{
    frame_set(&self->frame, 0, 0x22, g_guildWndWidth, g_guildWndHeight);
    self->tab = 0;
}

int homun_info_wnd_info_is_on_screen(struct WindowInfo *self)
{
    return ui_info_on_screen(self);
}

void homun_info_wnd_info_set_default(struct WindowInfo *self)
{
    frame_set(self, 150, 100, 0x118, 0xb4);
}

int mer_info_wnd_info_is_on_screen(struct WindowInfo *self)
{
    return ui_info_on_screen(self);
}

void mer_info_wnd_info_set_default(struct WindowInfo *self)
{
    frame_set(self, 150, 100, 0x118, 0xb4);
}

int mini_map_info_is_on_screen(struct MiniMapWndInfo *self)
{
    return ui_info_on_screen((const struct WindowInfo *)&self->x);
}

void mini_map_info_set_default(struct MiniMapWndInfo *self)
{
    int x = g_renderer->width - 0x218;

    self->x = x;
    self->y = 0xbe;
    self->width = 0x204;
    self->height = 0x22f;
    self->show_quest = 1;
    self->show_npc = 1;
    self->show_member = 1;
}

int window_info_is_on_screen(struct WindowInfo *self)
{
    return ui_info_on_screen(self);
}

void window_info_set_default(struct WindowInfo *self)
{
    frame_set(self, 150, 100, 0x118, 0xc8);
}

int party_setting_wnd_info_is_valid(struct WindowInfo *self)
{
    return frame_exact(self, 0x118, 0xb4);
}

void party_setting_wnd_info_make_default(struct WindowInfo *self)
{
    frame_set(self, 0x118, 0xf6, 0x118, 0xb4);
}

int quik_slot_wnd_info_is_valid(struct WindowInfo *self)
{
    return ui_info_on_screen(self);
}

void quik_slot_wnd_info_make_default(struct WindowInfo *self)
{
    frame_set(self, g_renderer->width - 0x7c, 0, 0x4d, 0x22);
}

int short_cut_wnd_info_is_valid(struct WndInfoShow *self)
{
    return self->rect.width == 0x122 && self->rect.height <= 0x88 && frame_inside(&self->rect);
}

void short_cut_wnd_info_make_default(struct WndInfoShow *self)
{
    frame_set(&self->rect, 0xe6, 0, 0x122, 0x43);
    self->show = 1;
}

int status_wnd_info_is_valid(struct StatusWndInfo *self)
{
    return self->rect.width == 0x118 && frame_inside(&self->rect);
}

void status_wnd_info_make_default(struct StatusWndInfo *self)
{
    self->rect.x = 0xb9;
    self->rect.y = 0x12c;
    self->rect.width = 0x118;
    self->show = 0;
    self->rect.height = 0x8d;
    self->org_height = 0x8d;
    self->expanded = 1;
}

int graphic_setting_wnd_info_is_valid(struct WindowInfo *self)
{
    return frame_exact(self, 0x190, 0x15e);
}

void graphic_setting_wnd_info_make_default(struct WindowInfo *self)
{
    frame_set(self, 200, 200, 0x190, 0x15e);
}

BYTE preview_equip_info_is_on_screen(POINT *self)
{
    return self->x >= 0 && self->x <= g_renderer->width - 0x118 && self->y >= 0 && self->y <= g_renderer->height - 0x104;
}

void preview_equip_info_set_default(POINT *self)
{
    self->x = (int)((float)(g_renderer->width - 0x118) * 0.5f);
    self->y = (int)((float)(g_renderer->height - 0x104) * 0.5f);
}

void item_store_wnd_info_set_default(struct ItemStoreWndInfo *self)
{
    frame_set(&self->rect, 0x168, 0xc8, 0x118, 0x1ca);
    self->sub_wnd = 0;
}

static const POINT item_store_sub_corners[ITEM_STORE_SUB_WNDS] = {
    {0xd2, 0x32}, {0x3c, 0x32}, {0xd2, 0xcd}, {0x3c, 0xcd},
    {0xd2, 0x168}, {0x3c, 0x168}, {0xd2, 0x203}, {0x3c, 0x203},
};

void item_store_sub_info_set_default(struct WindowInfo *self, int index)
{
    self->width = 0x8c;
    self->height = 0x91;
    self->x = item_store_sub_corners[index].x;
    self->y = item_store_sub_corners[index].y;
}

void navi_window_info_clamp(struct NavigationWndInfo *self)
{
    int width;

    self->rect.height = 0x152;
    width = (BYTE)self->is_screen ? 0x110 : 0xe4;
    self->rect.width = width;
    if (self->rect.x < 0)
        self->rect.x = 0;
    if (self->rect.x > g_renderer->width - width)
        self->rect.x = g_renderer->width - width;
    if (self->rect.y < 0)
        self->rect.y = 0;
    if (self->rect.y > g_renderer->height - 0x152)
        self->rect.y = g_renderer->height - 0x152;
}

void detail_level_wnd_info_make_default(struct WindowInfo *self)
{
    frame_set(self, 200, 200, 0x104, 0xaf);
}

void exchange_accept_wnd_info_make_default(struct WindowInfo *self)
{
    frame_set(self, 200, 200, 0x118, 0x78);
}

void equip_wnd_info_make_default(struct MessengerGroupWndInfo *self)
{
    frame_set(&self->rect, 0x12c, 0xc8, 0x111, 0x146);
    self->show = 0;
    self->radio = 1;
    self->size_mode = 0;
}

void navigation_wnd_info_make_default(struct NavigationWndInfo *self)
{
    self->rect.x = (int)((double)g_renderer->width * 0.5 - 136.0);
    self->rect.width = 0x110;
    self->rect.height = 0x152;
    self->is_screen = 1;
    self->rect.y = (int)((double)g_renderer->height * 0.5 - 169.0);
}

void quest_wnd_info_make_default(struct QuestWndInfo *self)
{
    frame_set(&self->rect, 0, 0x78, 0x118, 0xc8);
    self->org_height = 0xc8;
    self->cur_tab = 0;
    self->show = 0;
}

void shorten_item_wnd_info_make_default(struct WndInfoShow *self)
{
    frame_set(&self->rect, 0xb9, 0x12c, 0x8c, 0x1e);
    self->show = 0;
}

void wnd_info_make_default140x30(struct WindowInfo *self)
{
    frame_set(self, 0xb9, 0x12c, 0x8c, 0x1e);
}

void swap_equipment_wnd_info_make_default(struct WindowInfo *self)
{
    self->width = 0x118;
    self->height = 0xa7;
    self->x = (g_renderer->width - 0x118) / 2;
    self->y = (g_renderer->height - 0xb1) / 2;
}

void item_sell_wnd_info_make_default(struct WindowInfo *self)
{
    frame_set(self, 0x118, 0x118, 0x118, 0x98);
}

void merchant_mirror_item_wnd_info_make_default(struct WindowInfo *self)
{
    frame_set(self, 0, 0x78, 0x118, 0x78);
}

void option_wnd_info_make_default(struct WndInfoTall *self)
{
    frame_set(&self->rect, 0xb9, 0x12c, 0xfa, 0x3c);
    self->org_height = 0x3c;
    self->show = 0;
}

void adven_party_board_wnd_info_make_default(struct WindowInfo *self)
{
    frame_set(self, 200, 200, 0x1e0, 0x154);
}

void adven_register_wnd_info_make_default(struct WindowInfo *self)
{
    frame_set(self, 200, 200, 0xdc, 0xc5);
}

static void frame_set_scaled(struct WindowInfo *info)
{
    info->x = (g_renderer->width - 0x280) / 2 + 0xc8;
    info->y = g_renderer->height * 0xc8 / 0x1e0;
    info->width = 0xc8;
    info->height = 0xc8;
}

void ui_window_mgr_set_default_rect_info(struct UIWindowMgr *self)
{
    int i;

    if (!g_wndInfoDefaultsPending)
        return;
    g_wndInfoDefaultsPending = 0;

    basic_info_wnd_info_set_default(&self->basic_info_info);

    frame_set(&self->item_info.rect, 0, 0x78, 0x118, 0x78);
    self->item_info.org_height = 0x78;
    self->item_info.cur_tab = 0;
    self->item_info.cur_radio[0] = 3;
    self->item_info.cur_radio[1] = 3;
    self->item_info.cur_radio[2] = 4;
    self->item_info.show = 0;

    quest_wnd_info_make_default(&self->quest_info);

    frame_set(&self->status_info.rect, 0xb9, 0x12c, 0x118, 0x8d);
    self->status_info.org_height = 0x8d;
    self->status_info.show = 0;
    self->status_info.expanded = 1;

    equip_wnd_info_set_default(&self->equip_info);
    option_wnd_info_make_default(&self->option_info);
    shorten_item_wnd_info_make_default(&self->shorten_item_info);
    wnd_info_make_default140x30(&self->item_drop_cnt_info);
    say_dialog_info_set_default(&self->say_dialog_info);
    npc_menu_info_set_default(&self->choose_info);
    chat_room_info_set_default(&self->chat_room_info);

    item_store_wnd_info_set_default(&self->item_store_info);
    for (i = 0; i < ITEM_STORE_SUB_WNDS; i++)
        item_store_sub_info_set_default(&self->item_store_info.sub_info[i], i);
    self->item_store_wnd_num = ITEM_STORE_SUB_WNDS;
    frame_set(&self->item_store_find_info, 0x1fe, 0x168, 0x118, 0x138);

    frame_set(&self->booking_list_info, 0x1fe, 0x168, 0x226, 0x15e);
    frame_set(&self->booking_mb_info, 0x1fe, 0x168, 0x168, 0x15e);
    frame_set(&self->booking_info, 0x1fe, 0x168, 0x98, 0xc7);
    frame_set(&self->booking_mb_check_info, 0x1fe, 0x168, 0x98, 0xc7);

    frame_set(&self->item_shop_info, 0, 0x78, 0x118, 0x138);
    item_sell_wnd_info_make_default(&self->item_sell_info);
    item_sell_wnd_info_make_default(&self->item_purchase_info);
    equip_wnd_info_make_default(&self->messenger_group_info);
    short_cut_wnd_info_make_default(&self->shortcut_info);
    exchange_accept_wnd_info_make_default(&self->exchange_accept_info);
    frame_set(&self->skill_list_info.rect, 0x168, 0xc8, 0x118, 0xf0);
    frame_set(&self->merchant_item_info.rect, 0xc8, 0x78, 0x118, 0x78);
    merchant_mirror_item_wnd_info_make_default(&self->merchant_mirror_item_info);
    frame_set(&self->merchant_shop_make_info, 0, 0xf0, 0x118, 0xdc);
    frame_set(&self->merchant_item_shop_info, 0, 0x78, 0x118, 0x138);
    frame_set(&self->merchant_item_my_shop_info, 0xc8, 0x78, 0x118, 0x78);
    frame_set(&self->merchant_item_purchase_info, 0x118, 0x118, 0x118, 0x98);
    frame_set(&self->item_collection_info, 0x118, 0x7e, 0x118, 0x78);
    frame_set(&self->combined_card_item_collection_info, 0x118, 0x116, 0x118, 0x78);
    party_setting_wnd_info_make_default(&self->party_setting_info);
    detail_level_wnd_info_make_default(&self->detail_level_info);
    guild_wnd_info_make_default(&self->guild_info);
    frame_set(&self->whisper_info, 0, 0x15b, 0x118, 0x78);

    frame_set_scaled(&self->item_composition_info);
    frame_set_scaled(&self->item_identify_info);
    frame_set(&self->emotion_info, 0xc8, 0x96, 0x177, 0x118);
    frame_set(&self->emotion_list_info, 0xc8, 0xc8, 0xf6, 0x14a);
    window_info_set_default(&self->pet_info_info);
    homun_info_wnd_info_set_default(&self->homun_info_info);
    mer_info_wnd_info_set_default(&self->mer_info_info);
    frame_set(&self->sub_chat_st_info, 0, g_renderer->height - 0x77, 0x12c, 0x5f);
    frame_set(&self->sub_chat_bt_info, 0x12c, g_renderer->height - 0x77, 0x12c, 0x5f);
    quik_slot_wnd_info_make_default(&self->quick_slot_info);
    frame_set(&self->buying_store_make_info, 0, 0xf0, 0x118, 0xdc);
    frame_set(&self->buying_store_mirror_item_info, 0, 0x78, 0x118, 0x78);
    frame_set(&self->buying_store_item_my_shop_info, 0xc8, 0x78, 0x118, 0x78);
    frame_set(&self->buying_store_item_shop_info, 0, 0x78, 0x15e, 0x138);
    frame_set(&self->buying_store_item_sell_info, 0x118, 0x118, 0x118, 0x98);
    frame_set(&self->buying_store_mirror_sell_list_info, 0, 0x78, 0x118, 0x78);
    n_cash_info_set_default(&self->n_cash_info);

    navigation_wnd_info_make_default(&self->navigation_info);
    mini_map_info_set_default(&self->minimap_info);
    ach_tracing_info_set_default(&self->ach_tracing_pos);
    self->macro_reporter_info.width = 0xd1;
    self->macro_reporter_info.height = 0x134;
    self->macro_reporter_info.x = g_renderer->width - 0x103;
    self->macro_reporter_info.y = (g_renderer->height - 0x134) / 2;
    preview_equip_info_set_default(&self->preview_equip_pos);
    swap_equipment_wnd_info_make_default((struct WindowInfo *)&self->swap_equipment_pos);

    graphic_setting_wnd_info_make_default(&self->graphic_setting_info);
    self->quest_display = 1;
    adven_register_wnd_info_make_default(&self->adven_register_info);
    frame_set(&self->adven_join_info, 0xc8, 0xc8, 0xdc, 0xbe);
    adven_party_board_wnd_info_make_default(&self->adven_party_board_info);
}
