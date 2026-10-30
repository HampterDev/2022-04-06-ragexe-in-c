#include "game/cells.h"
#include "skills/skill_action.h"
#include "effects/rag/rag_effect.h"
#include "actor/object_ops.h"
#include "ui/window_access.h"
#include "ui/chat/balloon.h"
#include "ui/window.h"
#include "world/model_types.h"
#include "world/object_lookup.h"
#include "resources/attr_queries.h"
#include "contents/sound.h"
#include "skills/skill_action_registry.h"
#include "session/state.h"
#include "ui/window_manager_state.h"
#include "world/actor_lookup.h"
#include "actor/hallucination.h"
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

static void remove_if_absent(struct Skill *self, int type)
{
    int aid = self->base.aid;

    if (!effect_mgr_has_effect(g_effectMgr, aid, type))
        effect_mgr_remove_type(g_effectMgr, aid, type);
}

static void spawn_delayed(struct Skill *self, int type)
{
    struct Ez2StrEffect *e = (struct Ez2StrEffect *)spawn(self, type, vec_zero(), -1);

    if (e)
        e->base.ops->set_start_delay(&e->base, 10);
}

static void spawn_four(struct Skill *self, int first)
{
    spawn(self, first, vec_zero(), -1);
    spawn(self, first + 1, vec_zero(), -1);
    spawn_delayed(self, first + 2);
    spawn_delayed(self, first + 3);
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
    case 0x55: case 0xa1b: return 0x34;
    case 0x59: case 0x2d3: return 0x33;
    case 0x1ac: case 0x1ad: case 0x1ae: return 0x247;
    case 0x1461: return 0x6ef;
    case 0x1463: case 0x1465: return 0x764;
    case 0x1467: case 0x1468: return 0x707;
    case 0x146b: return 0x6f4;
    case 0x146d: return 0x70f;
    case 0x14d3: return 0x753;
    case 0x14d4: return 0x75b;
    case 0x14d5: return 0x757;
    case 0x14d7: return 0x782;
    case 0x14c9: case 0x14fa: case 0x14fb: case 0x14fc: case 0x1f6c:
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

    case 0x8a: case 0x90: case 0x91: case 0x93: case 0x99: case 0xe5:
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
    case 0x5174:
    case 0x5175:
    case 0x5176:
    case 0x5177:
        me->pickable = 1;
        show_trap_model(self);
        if (me->revealed) {
            static const int ids[4] = { 0x752, 0x75a, 0x756, 0x77f };
            spawn(self, ids[type - 0x5174], me->pos, -1);
        }
        break;

    case 0xbc: case 0xdc: case 0xdd: case 0xeb:
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
    case 0xbf: case 0xc2: case 0x108: self->effect_id = 0x63; me->pickable = 0; break;
    case 0xc0: self->effect_id = 0x8b; me->pickable = 0; break;
    case 0xc1: self->effect_id = 0x6c; me->pickable = 0; break;
    case 0xe4: self->effect_id = 0x6a; me->pickable = 0; break;
    case 0xe6: self->effect_id = 0x6a; me->pickable = 1; break;
    case 0xda: case 0xde: me->pickable = 1; self->effect_id = 0x1d0; break;
    case 0xd2: case 0xd3: case 0xd4: case 0xd5:
        self->effect_id = 0x8b; me->pickable = 0; show_trap_model(self); break;
    case 0xd6: case 0xd9:
        self->effect_id = 0x6b; me->pickable = 0; show_trap_model(self); break;
    case 0xd7:
        self->effect_id = 0x6c; me->pickable = 0; show_trap_model(self); break;
    case 0xd8:
        self->effect_id = 0x6b;
        me->pickable = 0;
        show_trap_model(self);
        trap_glow(self);
        break;
    case 0x101: play_at_unit(self, "effect\\NC_MAGMA_ERUPTION.wav"); me->pickable = 0; break;
    case 0x102: play_at_unit(self, "effect\\lg_kings_grace.wav"); me->pickable = 0; break;

    case 0xd1:
        {
            const struct GameActor *actor;
            const float position[3] = {me->pos.x, me->pos.y, me->pos.z};

            if (entry->creator_aid == g_session1.ownAid)
                actor = (const struct GameActor *)game_current_player();
            else
                actor = game_find_actor(self->creator_aid);
            actor_spawn_hallu_copy(actor, position, 0xb4);
        }
        break;

    case 0x104:
        switch (entry->level) {
        case 1: spawn(self, 0x42d, me->pos, -1); break;
        case 2: case 3: spawn(self, 0x42e, me->pos, -1); break;
        case 4: case 5: spawn(self, 0x42f, me->pos, -1); break;
        }
        break;
    case 0x106:
        spawn(self, 0x450, me->pos, entry->level);
        break;
    case 0x107:
        play_at_unit(self, "effect\\su_nyanggrass.wav");
        spawn(self, 0x465, me->pos, entry->level);
        break;
    case 0x109:
        spawn(self, 0x4b0, me->pos, -1);
        play_at_unit(self, "effect\\g카트토네이도.wav");
        break;
    case 0x10a:
        remove_if_absent(self, 0x6d8);
        spawn(self, 0x6d8, me->pos, entry->level);
        break;
    case 0x10b:
        remove_if_absent(self, 0x6ec);
        spawn(self, 0x6ec, me->pos, entry->level);
        break;
    case 0x10c:
        if (entry->level >= 1 && entry->level <= 5)
            self->effect_id = 0x641 + entry->level;
        break;
    case 0x10d:
        {
            int t;

            for (t = 0x765; t <= 0x766; t++) {
                struct Ez2StrEffect *e;

                remove_if_absent(self, t);
                e = (struct Ez2StrEffect *)spawn(self, t, me->pos, entry->level);
                if (e) {
                    ez2_str_effect_set_spin(&e->base, 90.0f);
                    e->turn_with_camera = 1;
                }
            }
        }
        break;
    case 0x10e:
        effect_mgr_remove_type(g_effectMgr, me->aid, 0x701);
        spawn(self, 0x701, me->pos, entry->level);
        break;
    case 0x10f:
        effect_mgr_remove_type(g_effectMgr, me->aid, 0x702);
        spawn(self, 0x702, me->pos, entry->level);
        effect_mgr_remove_type(g_effectMgr, me->aid, 0x703);
        spawn(self, 0x703, me->pos, entry->level);
        break;
    case 0x110:
        remove_if_absent(self, 0x6f0);
        spawn(self, 0x6f0, me->pos, entry->level);
        remove_if_absent(self, 0x6f1);
        spawn(self, 0x6f1, me->pos, entry->level);
        break;
    case 0x111:
        effect_mgr_remove_type(g_effectMgr, me->aid, 0x70c);
        spawn(self, 0x70c, me->pos, entry->level);
        break;
    case 0x112:
        remove_if_absent(self, 0x6e8);
        spawn(self, 0x6e8, me->pos, entry->level);
        break;
    case 0x113:
        {
            const struct RenderObject *player;

            spawn(self, 0x876, vec_zero(), -1);
            spawn(self, 0x875, me->pos, entry->level);
            player = (struct RenderObject *)game_current_player();
            if (player)
                sound_play_wave("effect\\IG_CROSS_RAIN.wav", player->pos.x - me->pos.x, 0.0f,
                                player->pos.z - me->pos.z, 250, 40, 1.0f);
        }
        break;
    case 0x114:
        spawn(self, 0x7a5, me->pos, entry->level);
        break;
    case 0x115:
        spawn(self, 0x852, vec_zero(), -1);
        spawn(self, 0x851, vec_zero(), -1);
        break;
    case 0x116: spawn_four(self, 0x8a8); break;
    case 0x117: spawn_four(self, 0x8b8); break;
    case 0x118: spawn_four(self, 0x8b0); break;
    case 0x119: spawn_four(self, 0x8a0); break;
    case 0x11a:
        remove_if_absent(self, 0x7fa);
        spawn(self, 0x7fa, vec_zero(), -1);
        remove_if_absent(self, 0x7f9);
        spawn(self, 0x7f9, vec_zero(), -1);
        break;
    case 0x11b:
        remove_if_absent(self, 0x7ef);
        spawn(self, 0x7ef, vec_zero(), -1);
        break;
    case 0x11c:
        remove_if_absent(self, 0x808);
        spawn(self, 0x808, vec_zero(), -1);
        remove_if_absent(self, 0x807);
        spawn(self, 0x807, vec_zero(), -1);
        break;
    case 0x11d:
        spawn(self, 0x903, vec_zero(), -1);
        break;
    case 0x532c:
        spawn(self, 0x925, vec_zero(), -1);
        break;

    default:
        skill_action_mgr_on_object_appear(skill_action_mgr_instance(), type, entry->level, (SkillActor *)(self), (SkillActor *)(self->creator_aid == g_session1.ownAid ? (void *)game_current_player() : (void *)game_find_actor(self->creator_aid)));
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
    } else {
        skill_action_mgr_on_object_leave(skill_action_mgr_instance(), game_actor_get_job((struct GameActor *)me), (SkillActor *)(self));
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
    switch (game_actor_get_job((struct GameActor *)me)) {
    case 0x5174: spawn(self, 0x752, vec_zero(), -1); break;
    case 0x5175: spawn(self, 0x75a, vec_zero(), -1); break;
    case 0x5176: spawn(self, 0x756, vec_zero(), -1); break;
    case 0x5177: spawn(self, 0x77f, vec_zero(), -1); break;
    }
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
