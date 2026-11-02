#include "game/packet_handlers.h"
#include "actor/actor_ops.h"
#include "iteminfo/item_api.h"
#include "actor/render_object.h"
#include "session/state.h"
#include "ui/window_manager_state.h"
#include "ui/window_access.h"
#include <windows.h>
#include <string.h>
#include "game/packets.h"
#include "ui/items/pet_windows.h"
#include "ui/window_types.h"
#include "platform/language.h"
#include "actor/game_actor.h"
#include "core/text_util.h"
#include "session/session_items.h"
#include "session/session_jobs.h"
#include "session/session_state.h"
#include "session/session_talk.h"
#include "session/session_windows.h"
#include "ui/items/item_list.h"
#include "ui/items/item_collection_internal.h"

#define WND_PET_EGG_LIST        0x5a
#define PET_LEVEL_OTHER         10000

#pragma pack(push, 1)
struct PACKET_ZC_PROPERTY_PET {
    unsigned short packet_type;
    char name[24];
    unsigned char modified;
    short level;
    short fullness;
    short intimacy;
    short accessory;
    short job;
};
#pragma pack(pop)
SIZE_ASSERT(PACKET_ZC_PROPERTY_PET, 0x25);

void game_mode_zc_property_pet(struct GameMode *self, unsigned char *pkt)
{
    struct PACKET_ZC_PROPERTY_PET *packet = (struct PACKET_ZC_PROPERTY_PET *)pkt;
    const char *name = packet->name;
    struct UIWindow *wnd;

    (void)self;
    g_session1.petJob = packet->job;
    if (packet->level < PET_LEVEL_OTHER) {
        _snprintf_s(g_session1.petName, sizeof g_session1.petName, _TRUNCATE, "%.*s", (int)sizeof packet->name, name);
        g_session1.petPrevFullness = g_session1.petFullness;
        g_session1.petFullness = packet->fullness;
        g_session1.petIntimacy = packet->intimacy;
        g_session1.petAccessory = (unsigned short)packet->accessory;
        if (g_windowMgr.pet_info_wnd) {
            ui_pet_info_update_intimacy(g_windowMgr.pet_info_wnd);
            ui_pet_info_set_name(g_windowMgr.pet_info_wnd, name);
            ui_window_invalidate(g_windowMgr.pet_info_wnd);
        }
        g_session1.petModified = packet->modified;
        g_session1.petLevel = packet->level;
        return;
    }
    packet->level -= PET_LEVEL_OTHER;
    wnd = ui_window_open(WND_ITEM_COLLECTION);
    if (!wnd)
        return;
    {
        struct PetEggDetails pet;

        pet.level = packet->level;
        pet.hunger = packet->fullness;
        pet.intimacy = packet->intimacy;
        pet.accessory = packet->accessory;
        pet.renamed = packet->modified;
        pet.name = name;
        ui_item_collection_set_pet(wnd, &pet);
    }
    wnd_send(wnd, 0x1d, 0, 0, 0, 0);
}

#pragma pack(push, 1)
struct PACKET_ZC_PET_ACT {
    unsigned short packet_type;
    unsigned aid;
    int value;
};
#pragma pack(pop)
SIZE_ASSERT(PACKET_ZC_PET_ACT, 10);

void game_mode_zc_pet_act(struct GameMode *self, unsigned char *pkt)
{
    struct PACKET_ZC_PET_ACT *packet = (struct PACKET_ZC_PET_ACT *)pkt;
    unsigned aid = packet->aid;
    struct GameActor *pet = world_get_game_actor_by_aid(self->world, aid);
    int value, kind, rest, hundreds;
    char line[256], said[256];

    if (!pet)
        return;
    value = packet->value;
    kind = value % 10;
    rest = value / 10;
    if (kind == 2) {
        actor_send(pet, 0x15, rest, 0);
        return;
    }
    hundreds = rest / 10;
    packet->value = hundreds;
    memset(line, 0, sizeof line);
    session_get_pet_speak(&g_session1, hundreds / 10, rest % 10, hundreds % 10, line);
    if (line[0] == 0)
        return;
    session_format_chat_line(&g_session1, line, said, aid, 1 - kind);
    if (strcmp(said, "") == 0)
        return;
    actor_send(pet, 7, (int)said, 0);
    if (g_session1.hideChat == 1 || g_session1.hideChatAndGuild == 1)
        return;
    ui_window_mgr_send_msg_(&g_windowMgr, 1, (int)said, 0xfafafa, 0, 0);
}

#pragma pack(push, 1)
struct PACKET_ZC_PETEGG_LIST {
    unsigned short packet_type;
    short length;
    short index[];
};
#pragma pack(pop)
SIZE_ASSERT(PACKET_ZC_PETEGG_LIST, 4);

void game_mode_zc_pet_egg_list(struct GameMode *self, const unsigned char *pkt)
{
    const struct PACKET_ZC_PETEGG_LIST *packet = (const struct PACKET_ZC_PETEGG_LIST *)pkt;
    unsigned count = (unsigned)(packet->length - 4) >> 1;
    const short *index = packet->index;
    void *wnd = ui_window_open(WND_PET_EGG_LIST);

    (void)self;
    wnd_send(wnd, 0x4b, 0, 0, 0, 0);
    for (; count != 0; count--, index++) {
        struct ItemInfo egg;

        session_get_inventory_item(&g_session1, &egg, *index);
        if (egg.index != 0)
            wnd_send(wnd, 0x1f, (int)string_cstr(&egg.id_text), egg.index, 0, 0);
        item_info_destruct(&egg);
    }
    wnd_send(wnd, 0x3c, 0, 0, 0, 0);
}

void game_mode_zc_pet_evolution_result(struct GameMode *self, unsigned result)
{
    (void)self;
    if (result <= 5)
        ui_window_mgr_send_msg_(&g_windowMgr, 1, (int)msg_str(0xa0b + result), 0xff, 0, 0);
    else if (result == 7)
        session_reset_pet(&g_session1);
}

static void homun_redraw(void)
{
    session_invalidate_homun_info_wnd(&g_session1);
}

#define WND_INVENTORY         8
#define WND_PET_INFO                  0x58
#define ACTOR_MSG_HP            0x22
#define JOB_GANGSI                 0xfd2
#define JOB_DARK_COLLECTOR                 0xfd4

enum {
    PET_STATE_ID = 0,
    PET_STATE_INTIMACY = 1,
    PET_STATE_FULLNESS = 2,
    PET_STATE_ACCESSORY = 3,
    PET_STATE_PERFORMANCE = 4,
    PET_STATE_EGG = 6,
};

static void redraw_window(int id)
{
    void *wnd = ui_window_find(id);

    if (wnd)
        ui_window_invalidate(wnd);
}

#pragma pack(push, 1)
struct PACKET_ZC_CHANGESTATE_PET {
    unsigned short packet_type;
    signed char type;
    unsigned aid;
    int value;
};
#pragma pack(pop)
SIZE_ASSERT(PACKET_ZC_CHANGESTATE_PET, 11);

void game_mode_zc_change_state_pet(struct GameMode *self, const unsigned char *pkt)
{
    const struct PACKET_ZC_CHANGESTATE_PET *packet = (const struct PACKET_ZC_CHANGESTATE_PET *)pkt;
    unsigned aid = packet->aid;
    int value = packet->value;
    struct GameActor *pet = world_get_game_actor_by_aid(self->world, aid);
    struct ItemInfo egg;

    switch (packet->type) {
    case PET_STATE_ID:
        g_session1.aid = (int)(aid);
        if (!pet)
            return;
        g_session1.petJob = game_actor_get_job(pet);
        actor_send(pet, ACTOR_MSG_HP, 0, 0);
        pet->object_type = 7;
        return;
    case PET_STATE_INTIMACY:
        g_session1.petIntimacy = value;
        if (pet)
            g_session1.petJob = game_actor_get_job(pet);
        if (g_windowMgr.pet_info_wnd)
            ui_pet_info_update_intimacy(g_windowMgr.pet_info_wnd);
        if (session_get_drawn_job(&g_session1) >= JOB_GANGSI && session_get_drawn_job(&g_session1) <= JOB_DARK_COLLECTOR
            && value < 100 && g_session1.petFullness < 30)
            ui_window_mgr_send_msg_(&g_windowMgr, 1, (int)msg_str(0x4a4), 0x3232ff, 0, 0);
        return;
    case PET_STATE_FULLNESS:
        g_session1.petPrevFullness = g_session1.petFullness;
        g_session1.petFullness = value;
        if (pet) {
            g_session1.petJob = game_actor_get_job(pet);
            actor_send(pet, ACTOR_MSG_HP, 0, 0);
        }
        break;
    case PET_STATE_ACCESSORY:
        if (!pet)
            return;
        if (value != 0) {
            render_object_load_act_res((struct RenderObject *)pet, session_get_pet_accessory_act(&g_session1, value));
            g_session1.petAccessory = value;
        } else {
            g_session1.petAccessory = 0;
            actor_set_spr_job(pet, game_actor_get_job(pet));
        }
        if (aid != ((unsigned)g_session1.aid))
            return;
        break;
    case PET_STATE_PERFORMANCE:
        if (!pet)
            return;
        game_actor_set_pk_mode(pet, value);
        actor_set_state(pet, 0, 1);
        return;
    case PET_STATE_EGG:
        if (value == 0) {
            session_get_inventory_item(&g_session1, &egg, g_session1.hatchedEggIndex);
            egg.damaged = 0;
            session_reset_pet(&g_session1);
            redraw_window(WND_INVENTORY);
            ui_window_mgr_post_quit1(&g_windowMgr, WND_PET_INFO);
        } else if (value == 1) {
            void *wnd = ui_window_find(WND_PET_EGG_LIST);
            int picked = 0;

            if (!wnd)
                return;
            ui_item_identify_wnd_get_selected_item(wnd, &egg, &picked);
            if (picked == 1) {
                egg.damaged = 1;
                session_inventory_update(&g_session1, &egg, 1);
                g_session1.hatchedEggIndex = egg.index;
                redraw_window(WND_INVENTORY);
            }
            ui_window_mgr_post_quit1(&g_windowMgr, WND_PET_EGG_LIST);
        } else {
            return;
        }
        item_info_destruct(&egg);
        return;
    default:
        return;
    }
    if (g_windowMgr.pet_info_wnd)
        ui_window_invalidate(g_windowMgr.pet_info_wnd);
}

#define PACKET_ZC_FEED_PET      0x1a3
#define PACKET_ZC_FEED_MER      0x22f
#define FEED_COLOR_OK           0xffff64
#define FEED_COLOR_FAIL         0x1e1ef5
#define UIM_LOG_LINE        0x13
#define MODE_MSG_PET_ACT        0x94

static void feed_line(struct ItemInfo *food, int food_id, int msg)
{
    struct ItemInfo carried;
    struct String line;
    const char *name;
    int left = session_get_inventory_item_by_id(&g_session1, &carried, food_id, 0)->count;

    item_info_destruct(&carried);

    item_info_construct(food);
    item_info_set_item_id(food, food_id);
    name = item_info_get_name(food);
    string_init(&line);
    string_format(&line, msg_str(msg), name, left);
    ui_window_mgr_send_msg_(&g_windowMgr, 1, (int)string_cstr(&line), FEED_COLOR_OK, 0, 0);
    string_free(&line);
}

static void feed_failed_line(struct ItemInfo *food, int food_id, struct String *line)
{
    struct String name = {0};

    item_info_construct(food);
    item_info_set_item_id(food, food_id);
    item_info_get_display_name(food, &name);
    string_init(line);
    string_format(line, msg_str(0x24f), string_cstr(&name));
    string_free(&name);
    ui_window_mgr_send_msg_(&g_windowMgr, 1, (int)string_cstr(line), FEED_COLOR_FAIL, 0, 0);
    ui_window_mgr_send_msg_(&g_windowMgr, UIM_LOG_LINE, (int)string_cstr(line), FEED_COLOR_FAIL, 0, 0);
}

#pragma pack(push, 1)
struct PACKET_ZC_FEED {
    unsigned short packet_type;
    unsigned char fed;
    int food_id;
};
#pragma pack(pop)
SIZE_ASSERT(PACKET_ZC_FEED, 7);

void game_mode_zc_feed(struct GameMode *self, const unsigned char *pkt)
{
    const struct PACKET_ZC_FEED *packet = (const struct PACKET_ZC_FEED *)pkt;
    unsigned short id = packet->packet_type;
    int food_id = packet->food_id;
    struct ItemInfo food;
    struct String line;

    if (id == PACKET_ZC_FEED_PET) {
        if (packet->fed != 0) {
            int intimacy = session_pet_intimacy_level(g_session1.petIntimacy);
            int hunger = session_pet_hunger_level(g_session1.petPrevFullness);
            int act = session_make_pet_talk_id(g_session1.petJob, 0, hunger);
            int emotion = session_get_pet_emotion(&g_session1, hunger, intimacy, 0);

            mode_send_msg(self, MODE_MSG_PET_ACT, act, 0, 0, 0);
            mode_send_msg(self, MODE_MSG_PET_ACT, emotion, 0, 0, 0);
            feed_line(&food, food_id, 0xa12);
        } else {
            feed_failed_line(&food, food_id, &line);
            string_free(&line);
        }
    } else if (id == PACKET_ZC_FEED_MER) {
        if (packet->fed != 0) {
            g_session1.homun_ai_state[0] = 0x1d;
            feed_line(&food, food_id, 0xcd1);
        } else {
            struct GameActor *homun;

            feed_failed_line(&food, food_id, &line);
            homun = world_get_game_actor_by_aid(self->world, ((unsigned)g_session1.homunAid));
            if (homun)
                actor_send(homun, 0x15, 4, 0);
            string_free(&line);
        }
    } else {
        return;
    }
    item_info_destruct(&food);
}

enum {
    HOMUN_STATE_ID = 0,
    HOMUN_STATE_INTIMACY = 1,
    HOMUN_STATE_FULLNESS = 2,
    HOMUN_STATE_ITEM = 3,
};

static void homun_window_redraw(void)
{
    if (g_windowMgr.homun_info_wnd)
        ui_window_invalidate(g_windowMgr.homun_info_wnd);
}

static void homun_value_line(const char *fmt, int value, unsigned color)
{
    char line[0x104];

    if (!session_is_admin_option(&g_session1, 3))
        return;
    sprintf(line, fmt, value);
    ui_window_mgr_send_msg_(&g_windowMgr, 1, (int)line, color, 0, 0);
}

#pragma pack(push, 1)
struct PACKET_ZC_CHANGESTATE_MER {
    unsigned short packet_type;
    unsigned char type;
    signed char state;
    unsigned aid;
    int value;
};
#pragma pack(pop)
SIZE_ASSERT(PACKET_ZC_CHANGESTATE_MER, 12);

void game_mode_zc_change_state_homun(struct GameMode *self, const unsigned char *pkt)
{
    const struct PACKET_ZC_CHANGESTATE_MER *packet = (const struct PACKET_ZC_CHANGESTATE_MER *)pkt;
    unsigned aid = packet->aid;
    int value = packet->value;
    struct GameActor *homun = world_get_game_actor_by_aid(self->world, aid);

    switch (packet->state) {
    case HOMUN_STATE_ID:
        g_session1.homunAid = (int)(aid);
        if (!homun)
            return;
        g_session1.homun_ai_flags[0] = 1;
        g_session1.homunJob = game_actor_get_job(homun);
        return;
    case HOMUN_STATE_INTIMACY:
        g_session1.homunIntimacy = value;
        if (homun)
            g_session1.homunJob = game_actor_get_job(homun);
        homun_window_redraw();
        homun_value_line("%d", value, 0xf5f5f5);
        return;
    case HOMUN_STATE_FULLNESS:
        g_session1.homunFullness = value;
        homun_redraw();
        if (homun)
            g_session1.homunJob = game_actor_get_job(homun);
        homun_window_redraw();
        homun_value_line("Full : %d", value, 0x1ef5f5);
        return;
    case HOMUN_STATE_ITEM:
        if (!homun)
            return;
        if (value != 0) {
            g_session1.homunItem = value;
        } else {
            g_session1.homunItem = 0;
            actor_set_spr_job(homun, game_actor_get_job(homun));
        }
        if (session_is_homunculus_aid(&g_session1, aid))
            homun_window_redraw();
        return;
    }
}

enum {
    HOMUN_PAR_EXP = 1,
    HOMUN_PAR_HP = 5,
    HOMUN_PAR_SP = 7,
};

#pragma pack(push, 1)
struct PACKET_ZC_HO_PAR_CHANGE2 {
    unsigned short packet_type;
    unsigned short var;
    long long value;
};
struct PACKET_ZC_HO_PAR_CHANGE {
    unsigned short packet_type;
    unsigned short var;
    int value;
};
#pragma pack(pop)
SIZE_ASSERT(PACKET_ZC_HO_PAR_CHANGE2, 12);
SIZE_ASSERT(PACKET_ZC_HO_PAR_CHANGE, 8);

void game_mode_zc_ho_par_change2(const unsigned char *pkt)
{
    const struct PACKET_ZC_HO_PAR_CHANGE2 *packet = (const struct PACKET_ZC_HO_PAR_CHANGE2 *)pkt;

    if (!pkt)
        return;
    switch (packet->var) {
    case HOMUN_PAR_EXP:
        g_session1.homunExp = packet->value;
        break;
    case HOMUN_PAR_HP:
        g_session1.homunHp = (int)packet->value;
        break;
    case HOMUN_PAR_SP:
        g_session1.homunSp = (int)packet->value;
        break;
    default:
        homun_redraw();
        return;
    }
    homun_redraw();
    homun_redraw();
}

void game_mode_zc_ho_par_change(const unsigned char *pkt)
{
    const struct PACKET_ZC_HO_PAR_CHANGE *packet = (const struct PACKET_ZC_HO_PAR_CHANGE *)pkt;

    switch (packet->var) {
    case HOMUN_PAR_HP:
        g_session1.homunHp = packet->value;
        break;
    case HOMUN_PAR_SP:
        g_session1.homunSp = packet->value;
        break;
    case HOMUN_PAR_EXP:
        g_session1.homunExp = packet->value;
        break;
    default:
        homun_redraw();
        return;
    }
    homun_redraw();
    homun_redraw();
}
