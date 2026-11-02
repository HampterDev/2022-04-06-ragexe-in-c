#include "game/actor_names.h"
#include "ui/chat/balloon.h"
#include "ui/window.h"
#include "actor/actor_ops.h"
#include "ui/window_access.h"
#include "render/rp_pool.h"
#include "contents/guild_emblem_api.h"
#include "game/ground_item_state.h"
#include "world/object_lookup.h"
#include "session/state.h"
#include "core/object_type_tables.h"
#include "world/actor_lookup.h"
#include <string.h>
#include "game/name_balloon.h"
#include "platform/language.h"
#include "actor/game_actor.h"
#include "core/text_util.h"
#include "platform/clientinfo.h"
#include "session/session_jobs.h"
#include "ui/chat/chat.h"

struct Item;

struct ActorNameCard *game_mode_get_name_card(struct GameMode *self, unsigned aid)
{
    return name_card_holder_get(&self->actor_names, aid);
}

void game_mode_make_name_balloon(struct GameMode *self, struct UIWindow **slot)
{
    struct UIWindow *wnd;

    if (*slot != 0)
        return;
    switch ((unsigned)self->name_balloon_pos) {
    case 2:
    case 3:
        wnd = &vertical_name_balloon_construct(mem_alloc(sizeof(struct UIVerticalNameBalloonText)))->name.balloon.base;
        break;
    default:
        wnd = &ui_name_balloon_text_construct(mem_alloc(sizeof(struct UINameBalloonText)))->balloon.base;
        break;
    }
    *slot = wnd;
    ui_window_create(wnd, 8, 8);
    ui_window_mgr_add_to_window_list(&g_windowMgr, *slot);
}

#define BALLOON(w) ((struct UINameBalloonText *)(w))

#define GROUP_EMBLEM_FMT "유저인터페이스\\group\\group_%d.bmp"

static void card_set_name(struct GameMode *self, struct ActorNameCard *card, struct GameActor *actor)
{
    struct String name;
    struct String *got;

    if (actor->name_masked == 1 && (self->world->is_pvp == 1 || self->world->is_siege == 1)) {
        string_assign_cstr(&card->name, "??????");
        return;
    }
    got = name_card_holder_name(&self->actor_names, &name, actor->aid);
    string_assign(&card->name, string_cstr(got), got->size);
    string_free(&name);
}

static int siege_emblem_frame(const struct GameMode *mode)
{
    return mode->world && mode->world->is_siege == 1 && g_session1.emblemFrame == 1;
}

void game_mode_show_name_balloon(struct GameMode *self, struct GameActor *actor)
{
    struct UIWindow **slot;
    struct ActorNameCard card;
    struct String text;
    unsigned char flag;
    unsigned aid;
    int job;

    if (actor == 0)
        return;
    slot = &self->name_balloon;
    game_mode_make_name_balloon(self, slot);
    name_card_copy_init(&card, name_card_holder_get(&self->actor_names, actor->aid));
    if (!card.complete) {
        name_card_destroy(&card);
        return;
    }
    card_set_name(self, &card, actor);

    if (session_is_monster_actor(&g_session1, actor) == 1) {
        flag = game_actor_get_job(actor) == 0x4f2d ? 0 : actor->is_boss;
        job = game_actor_get_job(actor);
        ui_name_balloon_text_set_name_text(BALLOON(self->name_balloon), &card, game_actor_is_pc_job(job) == 0, flag);
    } else if (actor->object_type != 7 && actor->object_type != 8) {
        flag = 1;
        job = game_actor_get_job(actor);
        if (game_actor_is_pc_job(job) == 1) {
            aid = actor->aid;
            job = game_actor_get_job(actor);
            flag &= session_is_npc(&g_session1, aid, job) != 0 ? 0xff : 0;
        }
        ui_name_balloon_text_set_name_text(BALLOON(*slot), &card, flag, 0);
    } else {
        job = game_actor_get_job(actor);
        ui_name_balloon_text_set_name_text(BALLOON(self->name_balloon), &card, game_actor_is_pc_job(job) == 0, 0);
    }

    if ((session_get_pet_acc_act_name(&g_session1, game_actor_get_job(actor)) != 0 ||
         session_is_monster_actor(&g_session1, actor) != 0) && card.group_id != 0) {
        string_init(&text);
        string_format(&text, GROUP_EMBLEM_FMT, card.group_id);
        ui_name_balloon_text_set_emblem(BALLOON(*slot), string_cstr(&text), 0);
        string_free(&text);
    } else if ((aid = actor->aid, session_is_npc(&g_session1, aid, game_actor_get_job(actor)) != 0) &&
               card.group_id != 0) {
        string_init(&text);
        string_format(&text, GROUP_EMBLEM_FMT, card.group_id);
        ui_name_balloon_text_set_emblem(BALLOON(*slot), string_cstr(&text), 0);
        string_free(&text);
    } else if (game_actor_get_guild_id(actor) != 0 &&
               g_session1.showGuildEmblems != 0) {
        void *emblems = g_guildMgr;
        int id = game_actor_get_guild_id(actor);

        guild_mgr_emblem_file_name(emblems, &text, id);
        ui_name_balloon_text_set_emblem(BALLOON(*slot), string_cstr(&text), siege_emblem_frame(self));
        string_free(&text);
    } else {
        ui_name_balloon_text_set_emblem(BALLOON(*slot), "", 0);
    }

    self->name_balloon_width = (*slot)->width;
    self->name_balloon_height = (*slot)->height;

    aid = actor->aid;
    if (g_serviceType == 0xc && aid - 0x189111 <= 9 && aid - 0x189111 != 4 && aid - 0x189111 != 7) {
        BALLOON(*slot)->balloon.colour = 0xff;
        BALLOON(*slot)->balloon.outline = 0x646464;
    } else {
        BALLOON(*slot)->balloon.colour = game_mode_name_color(self, game_actor_get_job(actor), aid);
        BALLOON(*slot)->balloon.outline = 0;
    }
    job = game_actor_get_job(actor);
    if (game_actor_is_pc_job(job) == 0 && (short)actor->level > 0 && session_is_monster_actor(&g_session1, actor) != 0) {
        short diff = (short)(((unsigned short)g_session1.charBaseLevel) - actor->level);

        if (diff < -10) {
            BALLOON(*slot)->balloon.colour = 0xff;
            BALLOON(*slot)->balloon.outline = 0;
        } else if (diff > 15) {
            BALLOON(*slot)->balloon.colour = 0x9b9b9b;
            BALLOON(*slot)->balloon.outline = 0;
        }
    }
    if (g_session1.ownAid == actor->owner_aid) {
        BALLOON(*slot)->balloon.colour = 0xc8c8c8;
        BALLOON(*slot)->balloon.outline = 0;
    }
    game_mode_move_balloon_text_window(self, *slot, actor);
    name_card_destroy(&card);
}

void game_mode_show_name_balloon2(struct GameMode *self, struct GameActor *actor)
{
    struct UIWindow **slot;
    struct ActorNameCard card;
    struct String text;
    unsigned char flag;
    unsigned aid;
    int job, player_job;

    if (actor == 0)
        return;
    slot = &self->name_balloon2;
    game_mode_make_name_balloon(self, slot);
    name_card_copy_init(&card, name_card_holder_get(&self->actor_names, actor->aid));
    if (!card.complete) {
        name_card_destroy(&card);
        return;
    }
    card_set_name(self, &card, actor);

    if (session_is_monster_actor(&g_session1, actor) == 1) {
        flag = game_actor_get_job(actor) == 0x4f2d ? 0 : actor->is_boss;
        job = game_actor_get_job(actor);
        player_job = game_actor_is_pc_job(job);
    } else {
        job = game_actor_get_job(actor);
        player_job = game_actor_is_pc_job(job);
        flag = 0;
    }
    ui_name_balloon_text_set_name_text(BALLOON(*slot), &card, player_job == 0, flag);

    if ((session_get_pet_acc_act_name(&g_session1, game_actor_get_job(actor)) != 0 ||
         session_is_monster_actor(&g_session1, actor) != 0) && card.group_id != 0) {
        string_init(&text);
        string_format(&text, GROUP_EMBLEM_FMT, card.group_id);
        ui_name_balloon_text_set_emblem(BALLOON(*slot), string_cstr(&text), 0);
        string_free(&text);
    } else if ((aid = actor->aid, session_is_npc(&g_session1, aid, game_actor_get_job(actor)) != 0) &&
               card.group_id != 0) {
        string_init(&text);
        string_format(&text, GROUP_EMBLEM_FMT, card.group_id);
        ui_name_balloon_text_set_emblem(BALLOON(self->name_balloon), string_cstr(&text), 0);
        string_free(&text);
    } else if (game_actor_get_guild_id(actor) != 0 &&
               g_session1.showGuildEmblems != 0) {
        void *emblems = g_guildMgr;
        int id = game_actor_get_guild_id(actor);
        int frame;

        guild_mgr_emblem_file_name(emblems, &text, id);
        frame = siege_emblem_frame(self);
        ui_name_balloon_text_set_emblem(BALLOON(*slot), string_cstr(&text), frame);
        string_free(&text);
    } else {
        ui_name_balloon_text_set_emblem(BALLOON(*slot), "", 0);
    }

    aid = actor->aid;
    BALLOON(*slot)->balloon.colour = game_mode_name_color(self, game_actor_get_job(actor), aid);
    BALLOON(*slot)->balloon.outline = 0;
    game_mode_move_balloon_text_window(self, *slot, actor);
    name_card_destroy(&card);
}

unsigned game_mode_name_color(struct GameMode *self, int job, unsigned aid)
{
    if (clientinfo_is_yellow_aid(aid))
        return 0xffff;
    if (session_is_npc(&g_session1, aid, job) == 1)
        return 0xf7b895;
    if ((job > 1000 && job < 0xf9f) || (unsigned)(job - 0x4e35) <= 0x2eca)
        return 0xc3c3ff;
    return 0xffffff;
}

void game_mode_move_balloon_text_window(struct GameMode *self, struct UIWindow *balloon,
                                                struct GameActor *actor)
{
    int w = balloon->width, h = balloon->height;
    int pos = self->name_balloon_pos;
    int saved = pos;
    int dx, dy, x, y, max_x, max_y;

    if (actor->is_pc == 0) {
        self->name_balloon_pos = 0;
        pos = 0;
    }
    switch ((unsigned)pos) {
    case 0:
        dx = -(w / 2);
        dy = (int)((float)(g_renderer->height * 20 / 480) * actor->scale);
        break;
    case 1:
        dx = -(w / 2);
        dy = (int)((float)(g_renderer->height * -100 / 480) * actor->scale);
        break;
    case 2:
        dx = (int)((float)(-30 * g_renderer->width / 640) * actor->scale - (float)(w / 2));
        dy = (int)((float)(-25 * g_renderer->height / 480) * actor->scale - (float)(h / 2));
        break;
    default:
        dx = (int)((float)(30 * g_renderer->width / 640) * actor->scale - (float)(w / 2));
        dy = (int)((float)(-25 * g_renderer->height / 480) * actor->scale - (float)(h / 2));
        break;
    }
    self->name_balloon_pos = saved;
    x = actor->screen_x + dx;
    y = actor->screen_y + dy;
    if (x <= -3)
        x = -3;
    max_x = g_renderer->width - balloon->width + 3;
    if (y <= -3)
        y = -3;
    max_y = g_renderer->height - balloon->height + 3;
    ui_window_move_safe(balloon, x < max_x ? x : max_x, y < max_y ? y : max_y);
}

void game_mode_process_monster_pick(struct GameMode *self, unsigned aid)
{
    struct GameActor *actor;
    struct Item *item;
    struct String name;
    struct UIWindow **slot;

    if (aid == 0)
        return;
    if (self->picked_aid != aid && self->picked_aid != 0) {
        self->name_balloon_stale = 1;
        self->picked_aid = aid;
        return;
    }
    self->picked_aid = aid;
    actor = world_get_game_actor_by_aid(self->world, aid);
    if (actor != 0) {
        if (game_actor_get_job(actor) == 0x2d)
            return;
        self->name_balloon_stale = 0;
        game_mode_show_name_balloon(self, actor);
        if (self->copy_name && session_is_player_job(&g_session1, game_actor_get_job(actor)) != 0 &&
            clientinfo_is_admin_aid(g_session1.ownAid)) {
            game_mode_get_actor_name(self, &name, aid);
            ui_window_mgr_set_whisper_name(&g_windowMgr, string_cstr(&name));
            string_free(&name);
        }
        return;
    }
    if (aid == (unsigned)g_session1.ownAid) {
        self->name_balloon_stale = 0;
        game_mode_show_name_balloon(self, (struct GameActor *)self->world->player);
        return;
    }
    item = world_get_item_by_aid(self->world, aid);
    if (item == 0)
        return;
    string_init(&name);
    string_assign_cstr(&name, string_cstr(&item->name));
    slot = &self->name_balloon;
    self->name_balloon_stale = 0;
    game_mode_make_name_balloon(self, slot);
    ((const struct UINameBalloonTextOps *)(*slot)->ops)->set_text(
        BALLOON(*slot), string_cstr(&name));
    ui_name_balloon_text_set_emblem(BALLOON(*slot), "", 0);
    BALLOON(*slot)->balloon.colour = 0x95efff;
    BALLOON(*slot)->balloon.outline = 0;
    self->name_balloon_width = (*slot)->width;
    self->name_balloon_height = (*slot)->height;
    game_mode_move_balloon_text_window(self, *slot, (struct GameActor *)item);
    string_free(&name);
}
