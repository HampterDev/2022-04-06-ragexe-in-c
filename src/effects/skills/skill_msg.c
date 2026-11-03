#include "game/cells.h"
#include "effects/rag/rag_effect.h"
#include "actor/object_ops.h"
#include "ui/window_access.h"
#include "ui/chat/balloon.h"
#include "ui/window.h"
#include "world/model_types.h"
#include "world/object_lookup.h"
#include "resources/attr_queries.h"
#include "contents/sound.h"
#include "session/state.h"
#include "ui/window_manager_state.h"
#include "world/actor_lookup.h"
#include "game/context.h"
#include "effects/effect_mgr.h"
#include "effects/skills/skill_view.h"
#include "effects/skills/skill.h"
#include "effects/skills/ez2str_effect.h"
#include <stdlib.h>
#include <string.h>
#include "render/render_api.h"
#include "actor/actor_common.h"
#include "session/session_names.h"
#include "session/session_party.h"
#include "actor/actor_msg.h"

#define MSG_BALLOON 0x07
#define MSG_SKILL_ATTACK 0x25
#define MSG_APPEAR 0x2d
#define MSG_SET_TYPE 0x2f
#define MSG_LEAVE 0x35
#define MSG_REVEAL 0x77
#define ACTOR_MSG_HIT 0x1b
#define EFFECT_MSG_START 0xe

#define NEW_FRAMES 36000
#define TRAP_GLOW 0x91

#define BALLOON_TICK(me) ((struct GameActor *)(me))->balloon_tick
#define BALLOON_W(me) ((struct GameActor *)(me))->balloon_width
#define BALLOON_H(me) ((struct GameActor *)(me))->balloon_height
#define BALLOON_ANCHOR_X(me) ((struct RenderObject *)(me))->screen_x
#define BALLOON_ANCHOR_Y(me) ((struct RenderObject *)(me))->screen_y

struct CaneOfEvilEyeEffect;

static Vec3 vec_zero(void)
{
    Vec3 v;
    v.x = 0.0f;
    v.y = 0.0f;
    v.z = 0.0f;
    return v;
}

static void play_at_unit(struct Skill *self, const char *wave)
{
    effect_play_wave_at(wave, &self->base.pos, 0.0f);
}

static struct Effect *spawn(struct Skill *self, int type, Vec3 pos, int level)
{
    return effect_mgr_spawn(g_effectMgr, self->base.aid, type, pos, 0, level, 0, 0);
}

static void show_trap_model(struct Skill *self)
{
    skill_load_model(self, skill_trap_model(self));
}

static void trap_glow(struct Skill *self)
{
    struct RenderObject *me = (struct RenderObject *)&self->base;

    if (!able_to_make_effect_is_effect_exist(me, TRAP_GLOW))
        game_actor_launch_effect((struct GameActor *)me, TRAP_GLOW, vec_zero(), 0);
}

static void show_balloon(struct Skill *self, const char *text)
{
    struct RenderObject *me = (struct RenderObject *)&self->base;
    struct UIWindow *wnd;
    struct UIBalloonText *balloon;

    if (strlen(text) == 0)
        return;
    BALLOON_TICK(me) = timeGetTime();
    if (!self->base.balloon_wnd) {
        void *p = mem_alloc(sizeof(struct UITransBalloonText));
        wnd = p ? ui_trans_balloon_text_construct(p) : 0;
        self->base.balloon_wnd = wnd;
        ui_window_create(wnd, 8, 8);
        ui_window_mgr_add_to_window_list(&g_windowMgr, self->base.balloon_wnd);
    }
    balloon = (struct UIBalloonText *)self->base.balloon_wnd;
    ui_balloon_text_set_text(balloon, text, 0x23);
    balloon = (struct UIBalloonText *)self->base.balloon_wnd;
    balloon->colour = 0xff8080;
    balloon->outline = 0;
    wnd = self->base.balloon_wnd;
    BALLOON_W(me) = wnd->width;
    BALLOON_H(me) = wnd->height;
    ui_window_move(wnd, BALLOON_ANCHOR_X(me) - wnd->width / 2,
             BALLOON_ANCHOR_Y(me) - g_renderer->height * 110 / 480);
}

static int skill_hit_effect(int skill_id)
{
    int none = skill_id < 0x1ed ? 0 : -1;

    switch (skill_id) {
    case 0x12: return 0x31;
    case 0x4f: return 0x98;
    case 0x55: return 0x34;
    case 0x59: case 0x2d3: return 0x33;
    case 0x1ac: case 0x1ad: case 0x1ae: return 0x247;
    case 0x1f6c:
        return session_get_skill_hit_effect_id(&g_session1, skill_id);
    }
    return none;
}

static void skill_attack(struct Skill *self, const struct ActorSkillUse *attack)
{
    struct RenderObject *me = (struct RenderObject *)&self->base;
    int count = attack->count ? attack->count : 1;
    int per_hit = attack->damage / count;
    int so_far, i;
    struct RenderObject *target = effect_find_actor(attack->target_aid);
    struct ActorHit hit;
    hit.effect = skill_hit_effect(attack->skill_id);

    if (target) {
        hit.attacker_aid = self->unit_aid;
        hit.target_aid = self->base.attack_target_aid;
        hit.tick = timeGetTime();
        hit.cell_x = attack->cell_x;
        hit.cell_y = attack->cell_y;
        hit.value = attack->level;
        hit.skill_id = attack->skill_id;
        hit.kind = 0x1a;
        hit.damage = per_hit;
        hit.hit_effect = -1;
        hit.is_sp_damage = 0;
        hit.motion = (float)attack->attacked_mt / 288.0f;
        so_far = per_hit;
        render_object_send(target, me, ACTOR_MSG_HIT, MSG_ARG_PTR(&hit), so_far, 0, 0);
        for (i = 1; i < count; i++) {
            target = effect_find_actor(attack->target_aid);
            if (!target)
                break;
            hit.tick += g_multiHitInterval;
            so_far += per_hit;
            render_object_send(target, me, ACTOR_MSG_HIT, MSG_ARG_PTR(&hit), so_far, 0, 0);
        }
    }
    if (self->model)
        self->model->animType = 1;
}

static unsigned char laid_by_friend(int creator_aid)
{
    struct PartyMember member;
    unsigned char found;

    if (creator_aid == g_session1.ownAid)
        return 1;
    session_get_party_member(&g_session1, &member, creator_aid);
    found = member.found != 0;
    string_free(&member.map_name);
    string_free(&member.name);
    return found;
}

static void appear_by_type(struct Skill *self, const struct SkillUnitEntry *entry)
{
    struct RenderObject *me = (struct RenderObject *)&self->base;
    int type = game_actor_get_job((struct GameActor *)me);

    switch (type) {
    case 0x7f: case 0xed:
        play_at_unit(self, "effect\\EF_FireWall.wav");
        me->pickable = 0;
        break;
    case 0x85:
        spawn(self, 0x8d, me->pos, -1);
        break;

    case 0x8a: case 0x90: case 0x91: case 0x93: case 0x99:
        me->pickable = 1;
        show_trap_model(self);
        break;
    case 0x8d:
        me->pickable = 1;
        break;
    case 0x8f: me->pickable = 1; self->effect_id = 0x6a; show_trap_model(self); break;
    case 0x95: me->pickable = 1; self->effect_id = 0x8b; show_trap_model(self); break;
    case 0x96: me->pickable = 1; self->effect_id = 0x63; show_trap_model(self); break;
    case 0x97: me->pickable = 1; self->effect_id = 0x6c; show_trap_model(self); break;
    case 0x98: me->pickable = 1; self->effect_id = 0x6b; show_trap_model(self); break;
    case 0x94:
        me->pickable = 1;
        show_trap_model(self);
        if (me->revealed)
            trap_glow(self);
        break;
    case 0xbc: case 0xdc: case 0xdd:
        me->pickable = 0;
        break;
    case 0xbd:
        {
            int r = rand() % 8;
            if (r < 0)
                r = 0;
            if (r == 0)
                play_at_unit(self, "effect\\화염진.wav");
        }
        me->pickable = 0;
        break;
    case 0xbe: self->effect_id = 0x6b; me->pickable = 0; break;
    case 0xbf: case 0xc2: self->effect_id = 0x63; me->pickable = 0; break;
    case 0xc0: self->effect_id = 0x8b; me->pickable = 0; break;
    case 0xc1: self->effect_id = 0x6c; me->pickable = 0; break;
    case 0xda: me->pickable = 1; self->effect_id = 0x1d0; break;
    case 0x101: play_at_unit(self, "effect\\NC_MAGMA_ERUPTION.wav"); me->pickable = 0; break;
    case 0x10c:
        if (entry->level >= 1 && entry->level <= 5)
            self->effect_id = 0x641 + entry->level;
        break;
    case 0x11d:
        spawn(self, 0x903, vec_zero(), -1);
        break;
    case 0x532c:
        spawn(self, 0x925, vec_zero(), -1);
        break;
    }
}

static void appear(struct Skill *self, const struct SkillUnitEntry *entry)
{
    struct RenderObject *me = (struct RenderObject *)&self->base;

    self->range = entry->range;
    self->unit_aid = entry->aid;
    me->aid = entry->aid;
    game_actor_set_job((struct GameActor *)me, entry->job);
    if (entry->visible)
        me->revealed = 1;
    else
        me->revealed = laid_by_friend(entry->creator_aid);
    self->effect_id = -1;
    {
        void *attr = game_current_map_attr();
        if (!attr || !game_cell_to_world(entry->x, entry->y, &me->pos.x, &me->pos.z)) {
            me->revealed = 0;
            return;
        }
        me->pos.y = c3d_attr_get_height(attr, me->pos.x, me->pos.z);
    }
    appear_by_type(self, entry);
}

static void set_type(struct Skill *self, int type)
{
    struct RenderObject *me = (struct RenderObject *)&self->base;

    game_actor_set_job((struct GameActor *)me, type);
    switch (game_actor_get_job((struct GameActor *)me)) {
    case 0x80:
        play_at_unit(self, "effect\\EF_Portal.wav");
        break;
    case 0x8c:
        if (self->effect_id != -1 && !effect_mgr_spawn(g_effectMgr, 0, self->effect_id, me->pos, 0, -1, 0, 0)) {
            struct RagEffect *effect = game_actor_launch_effect((struct GameActor *)me, self->effect_id, vec_zero(), 0);
            if (effect)
                rag_effect_send_msg(effect, 0, EFFECT_MSG_START, MSG_ARG_PTR(&me->pos), 0, 0);
        }
        break;
    }
    self->frames = NEW_FRAMES;
    show_trap_model(self);
    if (self->model)
        self->model->animType = 1;
}

static void leave(struct Skill *self)
{
    struct RenderObject *me = (struct RenderObject *)&self->base;

    if (game_actor_get_job((struct GameActor *)me) == 0x11d) {
        struct Effect *effect = effect_mgr_find_effect(g_effectMgr, me->aid, 0x903);
        if (effect)
            cane_of_evil_eye_effect_end((struct CaneOfEvilEyeEffect *)effect);
    }
}

static void reveal(struct Skill *self)
{
    struct RenderObject *me = (struct RenderObject *)&self->base;

    if (me->revealed)
        return;
    me->revealed = 1;
    show_trap_model(self);
    if (game_actor_get_job((struct GameActor *)me) == 0x94)
        trap_glow(self);
}

void skill_send_msg(struct GameObject *object, struct GameObject *source,
    RenderMsgArg msg, RenderMsgArg par1, RenderMsgArg par2, RenderMsgArg par3, RenderMsgArg par4)
{
    struct Skill *self = (struct Skill *)object;

    switch (msg) {
    case MSG_BALLOON:
        show_balloon(self, MSG_PTR(par1));
        return;
    case MSG_SKILL_ATTACK:
        skill_attack(self, MSG_PTR(par1));
        return;
    case MSG_APPEAR:
        appear(self, MSG_PTR(par1));
        return;
    case MSG_SET_TYPE:
        set_type(self, (int)par1);
        return;
    case MSG_LEAVE:
        leave(self);
        return;
    case MSG_REVEAL:
        reveal(self);
        return;
    }
    game_actor_send_msg((struct GameObject *)&self->base, source, msg, par1, par2, par3, par4);
}
