#include "world/mode_actions.h"
#include "game/packet_handlers.h"
#include "game/connection.h"
#include "actor/actor_ops.h"
#include "ui/quests/say_dialog.h"
#include "platform/code_page.h"
#include "std/string.h"
#include "game/own_states.h"
#include "iteminfo/item_table.h"
#include "contents/sound.h"
#include "session/state.h"
#include "ui/window_manager_state.h"
#include "core/object_type.h"
#include "core/text_util.h"
#include "world/mode_manager.h"
#include "game/context.h"
#include "ui/window_access.h"
#include <windows.h>
#include <mmsystem.h>
#include <stdlib.h>
#include <string.h>
#include "game/packets.h"
#include "game/map_entry.h"
#include "platform/language.h"
#include "game/chat.h"
#include "game/store_search_items.h"
#include "session/session_items.h"
#include "session/session_state.h"
#include "session/session_talk.h"
#include "ui/character/character.h"
#include "ui/shops/shops.h"

#pragma pack(push, 1)
struct PACKET_ZC_NPCSPRITE_CHANGE {
    unsigned short packet_type;
    unsigned aid;
    unsigned char type;
    int value;
};
struct PACKET_ZC_NPC_CHAT {
    unsigned short packet_type;
    unsigned short length;
    unsigned aid;
    int color;
    char text[];
};
#pragma pack(pop)
SIZE_ASSERT(PACKET_ZC_NPCSPRITE_CHANGE, 11);
SIZE_ASSERT(PACKET_ZC_NPC_CHAT, 12);

void game_mode_zc_npc_sprite_change(struct GameMode *self, const unsigned char *pkt)
{
    const struct PACKET_ZC_NPCSPRITE_CHANGE *packet = (const struct PACKET_ZC_NPCSPRITE_CHANGE *)pkt;
    struct GameActor *actor = world_get_game_actor_by_aid(self->world, packet->aid);

    if (actor)
        actor_set_job_model(actor, packet->value, 0);
}

void game_mode_zc_npc_chat(struct GameMode *self, const unsigned char *pkt)
{
    const struct PACKET_ZC_NPC_CHAT *packet = (const struct PACKET_ZC_NPC_CHAT *)pkt;
    unsigned aid = packet->aid;
    int color = packet->color;
    char text[256];
    struct GameActor *npc;

    memcpy(text, packet->text, packet->length - 12);
    ui_window_mgr_send_msg_(&g_windowMgr, 1, (int)text, color, 0, 0);
    npc = world_get_game_actor_by_aid(self->world, aid);
    if (npc)
        actor_send(npc, 7, (int)text, color);
    if (g_session1.autoSaveChat != 0)
        game_mode_log_whisper(self, text);
}

#pragma pack(push, 1)
struct MarketEntry2 {
    int item_id;
    unsigned char type;
    int price;
    int count;
    unsigned short weight;
    int location;
};
struct MarketEntry {
    int item_id;
    unsigned char type;
    int price;
    int count;
    unsigned short weight;
};
struct MarketBought {
    int item_id;
    short amount;
    int price;
};
struct PACKET_ZC_NPC_MARKET_OPEN2 {
    unsigned short packet_type;
    short length;
    struct MarketEntry2 entries[];
};
struct PACKET_ZC_NPC_MARKET_OPEN {
    unsigned short packet_type;
    short length;
    struct MarketEntry entries[];
};
struct PACKET_ZC_NPC_MARKET_PURCHASE_RESULT_V2 {
    unsigned short packet_type;
    short length;
    short result;
    struct MarketBought items[];
};
#pragma pack(pop)
SIZE_ASSERT(PACKET_ZC_NPC_MARKET_OPEN2, 4);
SIZE_ASSERT(PACKET_ZC_NPC_MARKET_OPEN, 4);
SIZE_ASSERT(PACKET_ZC_NPC_MARKET_PURCHASE_RESULT_V2, 6);
SIZE_ASSERT(MarketEntry2, 19);
SIZE_ASSERT(MarketEntry, 15);
SIZE_ASSERT(MarketBought, 10);

#define WND_NPC_MARKET          0xfe
#define WND_NPC_MARKET_CART     0xff
#define WND_PARA_RESULT         0x100
#define SHOP_MSG_SET_MODE         0x24
#define WND_MSG_SHOW            0x17

static void market_refresh(void *wnd)
{
    wnd_send(wnd, SHOP_MSG_SET_MODE, 0, 0, 0, 0);
    wnd_send(wnd, WND_MSG_SHOW, 0, 0, 0, 0);
}

void game_mode_zc_npc_market_open2(struct GameMode *self, const unsigned char *pkt)
{
    const struct PACKET_ZC_NPC_MARKET_OPEN2 *packet = (const struct PACKET_ZC_NPC_MARKET_OPEN2 *)pkt;
    const struct MarketEntry2 *entries = packet->entries;
    int count = (packet->length - 4) / (int)sizeof *entries;
    int i;
    void *wnd;

    (void)self;
    session_clear_lists(&g_session1);
    for (i = 0; i < count; i++) {
        struct MarketEntry2 entry = entries[i];
        struct ItemRecordA item;

        item_record_a_clear(&item);
        _itoa(entry.item_id, item.id_text, 10);
        item.price = entry.price;
        item.charged_price = entry.price;
        item.type = entry.type;
        item.equip_location = entry.location;
        item.index = -1;
        item.identified = 1;
        item.view = item_table_get(atoi(item.id_text))->class_num;
        session_append_market_goods(&g_session1, &item, entry.count, entry.weight / 10);
    }
    wnd = ui_window_open(WND_NPC_MARKET);
    if (wnd)
        market_refresh(wnd);
    wnd = ui_window_open(WND_NPC_MARKET_CART);
    if (wnd)
        market_refresh(wnd);
}

void game_mode_zc_npc_market_open(struct GameMode *self, const unsigned char *pkt)
{
    const struct PACKET_ZC_NPC_MARKET_OPEN *packet = (const struct PACKET_ZC_NPC_MARKET_OPEN *)pkt;
    const struct MarketEntry *entry = packet->entries;
    int count = (int)((unsigned)(packet->length - 4) / sizeof *entry);

    (void)self;
    session_clear_lists(&g_session1);
    for (; count > 0; count--, entry++) {
        struct MarketEntry copy = *entry;
        struct ItemRecordA item;

        item_record_a_clear(&item);
        _itoa(copy.item_id, item.id_text, 10);
        item.price = copy.price;
        item.charged_price = copy.price;
        item.type = copy.type;
        item.index = -1;
        item.identified = 1;
        session_append_market_goods(&g_session1, &item, copy.count, copy.weight / 10);
    }
    market_refresh(ui_window_open(WND_NPC_MARKET));
    market_refresh(ui_window_open(WND_NPC_MARKET_CART));
}

void game_mode_zc_npc_market_purchase_result_v2(struct GameMode *self, const unsigned char *pkt)
{
    const struct PACKET_ZC_NPC_MARKET_PURCHASE_RESULT_V2 *packet =
        (const struct PACKET_ZC_NPC_MARKET_PURCHASE_RESULT_V2 *)pkt;
    int count = (int)((unsigned)(packet->length - 6) / sizeof(struct MarketBought));
    int msg = 0, color = 0xff;
    struct GameMode *mode;

    (void)self;
    switch (packet->result) {
    case -1: msg = 0xbe3; break;
    case 0: msg = 0x36; color = 0xffff00; break;
    case 1: msg = 0x37; break;
    case 2: msg = 0x38; break;
    case 3: msg = 0xdd; break;
    case 9: msg = 0x794; break;
    }
    if (msg) {
        ui_window_mgr_send_msg_(&g_windowMgr, 3, 0, 0, 0, 0);
        ui_window_mgr_send_msg_(&g_windowMgr, 1, (int)msg_str(msg), color, 0, 0);
    }
    if (count <= 0) {
        mode = game_current_mode();
        if (mode)
            mode_send_msg(mode, 0x124, 0, 0, 0, 0);
        return;
    }
    {
        void *wnd = object_as_type(ui_window_open(WND_PARA_RESULT), OBJECT_PARA_RESULT_WINDOW);
        const struct MarketBought *bought = packet->items;
        int left = count, rows;

        if (!wnd)
            return;
        for (; left != 0; left--, bought++) {
            struct MarketBought copy = *bought;
            struct ItemRecordA item;

            item_record_a_clear(&item);
            _itoa(copy.item_id, item.id_text, 10);
            item.price = copy.price;
            item.charged_price = copy.price;
            item.index = -1;
            item.identified = 1;
            ui_para_result_wnd_add_bought_record(wnd, &item, copy.amount);
        }
        rows = count - 2;
        if (rows <= 0)
            rows = 0;
        wnd_send(wnd, SHOP_MSG_SET_MODE, rows * 32 + 0x78, 0, 0, 0);
        wnd_send(wnd, WND_MSG_SHOW, 0, 0, 0, 0);
    }
}

#pragma pack(push, 1)
struct PACKET_ZC_NPCACK_MAPMOVE {
    unsigned short packet_type;
    char map[16];
    short x;
    short y;
};
struct PACKET_ZC_NPCACK_SERVERMOVE {
    unsigned short packet_type;
    char map[16];
    short x;
    short y;
    unsigned ip;
    short port;
};
struct PACKET_ZC_NPCACK_SERVERMOVE_DOMAIN {
    unsigned short packet_type;
    char map[16];
    short x;
    short y;
    unsigned ip;
    short port;
    char domain[128];
};
#pragma pack(pop)
SIZE_ASSERT(PACKET_ZC_NPCACK_MAPMOVE, 0x16);
SIZE_ASSERT(PACKET_ZC_NPCACK_SERVERMOVE, 0x1c);
SIZE_ASSERT(PACKET_ZC_NPCACK_SERVERMOVE_DOMAIN, 0x9c);

void game_mode_zc_npcack_mapmove(struct GameMode *self, const unsigned char *pkt)
{
    const struct PACKET_ZC_NPCACK_MAPMOVE *packet = (const struct PACKET_ZC_NPCACK_MAPMOVE *)pkt;
    char name_buf[17];
    struct String map;
    size_t state_index;

    string_init(&map);
    g_session1.playerPosX = packet->x;
    g_session1.playerPosY = packet->y;
    memcpy(name_buf, packet->map, 16);
    name_buf[16] = 0;
    string_assign_cstr(&map, name_buf);
    text_drop_map_extension(&map);

    if (strcmp(string_cstr(&map), g_session1.mapName) != 0) {
        void *loading;

        strcpy(g_session1.mapName, string_cstr(&map));
        string_append_cstr(&map, ".rsw");
        mode_mgr_switch(&g_modeMgr, 1, string_cstr(&map));
        loading = ui_window_open(6);
        ui_wait_wnd_set_text(loading, msg_str(0x79));
        g_windowMgr.modal_result = 0xb1;
        session_reset_store_search(&g_session1);
    } else {
        if (!game_mode_reset_for_warp(self)) {
            string_free(&map);
            return;
        }
        game_mode_initialize(self);
        mode_run_fade_out(&self->base, 0);
    }

    for (state_index = 0; state_index < own_states_count(); ++state_index) {
        int index = own_states_at(state_index)->index;
        if (index == 0xc8 || index == 0xc9 || index == 4) {
            own_states_erase_at(state_index);
            break;
        }
    }
    session_reset_mail_items(&g_session1);
    session_clear_mail_item_counts(&g_session1);
    string_free(&map);
}

void game_mode_zc_npcack_servermove(struct GameMode *self, const unsigned char *pkt)
{
    const struct PACKET_ZC_NPCACK_SERVERMOVE *packet = (const struct PACKET_ZC_NPCACK_SERVERMOVE *)pkt;
    struct String extra, name, map;

    string_init(&extra);
    string_init(&name);
    string_assign(&name, packet->map, 16);
    string_init(&map);
    string_assign_cstr(&map, string_cstr(&name));
    game_mode_move_to_server(self, map, packet->x, packet->y, packet->ip, packet->port, extra);
    string_free(&name);
}

void game_mode_zc_npcack_servermove_domain(struct GameMode *self, const unsigned char *pkt)
{
    const struct PACKET_ZC_NPCACK_SERVERMOVE_DOMAIN *packet = (const struct PACKET_ZC_NPCACK_SERVERMOVE_DOMAIN *)pkt;
    struct String domain, domain_arg, name, map;

    string_init(&domain);
    string_assign(&domain, packet->domain, 0x80);
    string_init(&domain_arg);
    string_assign_cstr(&domain_arg, string_cstr(&domain));
    string_init(&name);
    string_assign(&name, packet->map, 16);
    string_init(&map);
    string_assign_cstr(&map, string_cstr(&name));
    game_mode_move_to_server(self, map, packet->x, packet->y, packet->ip, packet->port, domain_arg);
    string_free(&name);
    string_free(&domain);
}

#define WND_NPC_INPUT_STR       0x64

#pragma pack(push, 1)
struct PACKET_ZC_OPEN_EDITDLGSTR {
    unsigned short packet_type;
    unsigned aid;
};
#pragma pack(pop)
SIZE_ASSERT(PACKET_ZC_OPEN_EDITDLGSTR, 6);

void game_mode_zc_open_edit_dlg_str(struct GameMode *self, const unsigned char *pkt)
{
    const struct PACKET_ZC_OPEN_EDITDLGSTR *packet = (const struct PACKET_ZC_OPEN_EDITDLGSTR *)pkt;
    unsigned aid = packet->aid;

    if (world_get_game_actor_by_aid(self->world, aid) || aid == 0) {
        void *wnd = ui_window_open(WND_NPC_INPUT_STR);

        wnd_send(wnd, 0x1c, aid, 0, 0, 0);
    }
}

#define WND_NPC_DIALOG          0x10
#define WND_NPC_DEAL_TYPE       0x19
#define WND_SHOP_BUY            0x16
#define WND_SHOP_BUY_CART       0x17
#define WND_SHOP_SELL           0x18
#define NPC_DIALOG_TEXT_MAX     0x803
#define NPC_DIALOG_TEXT_LEN_MAX 0x7fd

#pragma pack(push, 1)
struct PACKET_ZC_SAY_DIALOG {
    unsigned short packet_type;
    unsigned short length;
    unsigned aid;
    char text[];
};
struct PACKET_ZC_WAIT_DIALOG {
    unsigned short packet_type;
    unsigned aid;
};
struct PACKET_ZC_SELECT_DEALTYPE {
    unsigned short packet_type;
    int aid;
};
#pragma pack(pop)
SIZE_ASSERT(PACKET_ZC_SAY_DIALOG, 8);
SIZE_ASSERT(PACKET_ZC_WAIT_DIALOG, 6);
SIZE_ASSERT(PACKET_ZC_SELECT_DEALTYPE, 6);

void game_mode_zc_say_dialog(struct GameMode *self, const unsigned char *pkt)
{
    const struct PACKET_ZC_SAY_DIALOG *packet = (const struct PACKET_ZC_SAY_DIALOG *)pkt;
    char text[NPC_DIALOG_TEXT_MAX];
    int length = packet->length - 8;
    void *wnd;

    if (length < 0)
        return;
    if (length > NPC_DIALOG_TEXT_LEN_MAX)
        length = NPC_DIALOG_TEXT_LEN_MAX;
    self->npc_dialog_open = 1;
    self->npc_dialog_id = packet->aid;
    memcpy(text, packet->text, length);
    text[length] = 0;
    wnd = ui_window_open(WND_NPC_DIALOG);
    wnd_send(wnd, 0x1a, (int)text, (int)packet->aid, 0, 0);
}

void game_mode_zc_select_dealtype(struct GameMode *self, const unsigned char *pkt)
{
    const struct PACKET_ZC_SELECT_DEALTYPE *packet = (const struct PACKET_ZC_SELECT_DEALTYPE *)pkt;
    void *wnd;

    self->npc_dialog_open = 1;
    wnd = ui_window_open(WND_NPC_DEAL_TYPE);
    wnd_send(wnd, 0x1c, packet->aid, 0, 0, 0);
    ui_window_mgr_post_quit1(&g_windowMgr, WND_SHOP_BUY);
    ui_window_mgr_post_quit1(&g_windowMgr, WND_SHOP_BUY_CART);
    ui_window_mgr_post_quit1(&g_windowMgr, WND_SHOP_SELL);
}

void game_mode_zc_clear_dialog(struct GameMode *self, const unsigned char *pkt)
{
    void *wnd = ui_window_find(WND_NPC_DIALOG);

    (void)self;
    (void)pkt;
    if (wnd != 0)
        wnd_send(wnd, 0x4b, 0, 0, 0, 0);
}

void game_mode_zc_dialog_text_align(struct GameMode *self, unsigned char align)
{
    struct UIWindow *wnd = ui_window_find(WND_NPC_DIALOG);

    (void)self;
    if (wnd != 0)
        ui_say_dialog_wnd_set_text_align((struct UISayDialogWnd *)wnd, align);
}

#define WND_NPC_MENU            0x11
#define WND_NPC_ILLUST          0xe2
#define NPC_MENU_TEXT_MAX       0x800
enum {
    NPC_WND_WAIT = 0x19,
    NPC_WND_MENU_ITEM = 0x1b,
    NPC_WND_MENU_NPC = 0x1c,
    NPC_HELPER_DIALOG_EVENT = 0x20,
    NPC_HELPER_CLOSE = 0x3f,
};

void game_mode_zc_wait_dialog(struct GameMode *self, const unsigned char *pkt)
{
    const struct PACKET_ZC_WAIT_DIALOG *packet = (const struct PACKET_ZC_WAIT_DIALOG *)pkt;
    void *wnd;

    self->npc_dialog_open = 1;
    self->npc_dialog_id = packet->aid;
    wnd = ui_window_open(WND_NPC_DIALOG);
    wnd_send(wnd, NPC_WND_WAIT, 0, 0, 0, 0);
}

void game_mode_zc_close_dialog(struct GameMode *self, const unsigned char *pkt)
{
    const struct PACKET_ZC_WAIT_DIALOG *packet = (const struct PACKET_ZC_WAIT_DIALOG *)pkt;
    void *helper = g_windowMgr.say_dialog_wnd;

    if (helper) {
        wnd_send(helper, NPC_HELPER_DIALOG_EVENT, 0, 0, 0, 0);
        wnd_send(helper, NPC_HELPER_CLOSE, 0, 0, 0, 0);
        return;
    }
    self->npc_dialog_open = 0;
    self->npc_dialog_id = packet->aid;
    ui_window_mgr_post_quit1(&g_windowMgr, WND_NPC_DIALOG);
    ui_window_mgr_post_quit1(&g_windowMgr, WND_NPC_MENU);
    ui_window_mgr_post_quit1(&g_windowMgr, WND_NPC_ILLUST);
}

#pragma pack(push, 1)
struct PACKET_ZC_MENU_LIST {
    unsigned short packet_type;
    unsigned short length;
    int aid;
    char text[];
};
#pragma pack(pop)
SIZE_ASSERT(PACKET_ZC_MENU_LIST, 8);

void game_mode_zc_menu_list(struct GameMode *self, const unsigned char *pkt)
{
    const struct PACKET_ZC_MENU_LIST *packet = (const struct PACKET_ZC_MENU_LIST *)pkt;
    char text[NPC_MENU_TEXT_MAX];
    size_t length = packet->length;
    void *wnd;
    char *choice;

    if (length < 8 || length - 8 >= sizeof text)
        return;
    length -= 8;
    self->npc_dialog_open = 1;
    memcpy(text, packet->text, length);
    text[length] = 0;
    ui_window_mgr_post_quit1(&g_windowMgr, WND_NPC_MENU);
    wnd = ui_window_open(WND_NPC_MENU);
    for (choice = strtok(text, ":"); choice; choice = strtok(0, ":"))
        wnd_send(wnd, NPC_WND_MENU_ITEM, (int)choice, 0, 0, 0);
    wnd_send(wnd, NPC_WND_MENU_NPC, packet->aid, 0, 0, 0);
    if (g_windowMgr.say_dialog_wnd)
        wnd_send(g_windowMgr.say_dialog_wnd, NPC_HELPER_DIALOG_EVENT, 0, 0, 0, 0);
}

#pragma pack(push, 1)
struct PACKET_ZC_PLAY_NPC_BGM {
    unsigned short packet_type;
    char name[];
};
#pragma pack(pop)
SIZE_ASSERT(PACKET_ZC_PLAY_NPC_BGM, 2);

void game_mode_zc_play_npc_bgm(struct GameMode *self, const unsigned char *pkt)
{
    const struct PACKET_ZC_PLAY_NPC_BGM *packet = (const struct PACKET_ZC_PLAY_NPC_BGM *)pkt;
    struct String path;

    (void)self;
    if (g_sound_settings.bgm_on == 0)
        return;
    string_init(&path);
    string_assign_cstr(&path, "bgm\\");
    string_append_cstr(&path, packet->name);
    string_append_cstr(&path, ".mp3");
    sound_play_stream(string_cstr(&path), 1);
    g_sound_settings.bgm_overridden = 1;
    string_free(&path);
}

enum { SOUND_ONCE = 0, SOUND_REPEAT = 1, SOUND_STOP = 2 };

#pragma pack(push, 1)
struct PACKET_ZC_SOUND {
    unsigned short packet_type;
    char name[24];
    unsigned char action;
    DWORD interval;
    unsigned npc_aid;
};
#pragma pack(pop)
SIZE_ASSERT(PACKET_ZC_SOUND, 0x23);

void game_mode_zc_sound(struct GameMode *self, const unsigned char *pkt)
{
    const struct PACKET_ZC_SOUND *packet = (const struct PACKET_ZC_SOUND *)pkt;
    const char *name = packet->name;
    unsigned char action = packet->action;
    DWORD interval = packet->interval;
    unsigned npc_aid = packet->npc_aid;

    if (action == SOUND_ONCE) {
        struct GameActor *npc = world_get_game_actor_by_aid(self->world, npc_aid);

        if (npc) {
            struct Player *player = self->world->player;

            sound_play_wave(name, player->pos.x - npc->pos.x, 0.0f, player->pos.z - npc->pos.z,
                     0xfa, 0x28, 1.0f);
        } else {
            sound_play_wave(name, 0.0f, 0.0f, 0.0f, 0xfa, 0x28, 1.0f);
        }
    } else if (action == SOUND_REPEAT) {
        struct LoopSound sound, copy;

        string_init(&sound.name);
        string_assign_cstr(&sound.name, name);
        sound.interval = interval;
        sound.next_tick = timeGetTime() + interval;
        sound.npc_aid = npc_aid;
        string_copy(&copy.name, &sound.name);
        copy.npc_aid = sound.npc_aid;
        copy.interval = sound.interval;
        copy.next_tick = sound.next_tick;
        vec_push(&self->loop_sounds, &copy, sizeof copy);
        sound_play_wave(name, 0.0f, 0.0f, 0.0f, 0xfa, 0x28, 1.0f);
        string_free(&sound.name);
    } else if (action == SOUND_STOP) {
        struct LoopSound *sound = (struct LoopSound *)self->loop_sounds.first;

        for (; sound != (struct LoopSound *)self->loop_sounds.last; sound++) {
            if (strcmp(string_cstr(&sound->name), name) == 0) {
                string_free(&sound->name);
                vec_erase(&self->loop_sounds, sound, sound + 1);
                return;
            }
        }
    }
}

#define WND_NPC_NUMBER_INPUT    0x38
#define NUMBER_INPUT_WND_NPC    0x1c

#pragma pack(push, 1)
struct PACKET_ZC_OPEN_EDITDLG {
    unsigned short packet_type;
    unsigned aid;
};
#pragma pack(pop)
SIZE_ASSERT(PACKET_ZC_OPEN_EDITDLG, 6);

void game_mode_zc_open_edit_dlg(struct GameMode *self, const unsigned char *pkt)
{
    const struct PACKET_ZC_OPEN_EDITDLG *packet = (const struct PACKET_ZC_OPEN_EDITDLG *)pkt;
    void *wnd;

    if (!world_get_game_actor_by_aid(self->world, packet->aid))
        return;
    wnd = ui_window_open(WND_NPC_NUMBER_INPUT);
    wnd_send(wnd, NUMBER_INPUT_WND_NPC, (int)packet->aid, 0, 0, 0);
}
