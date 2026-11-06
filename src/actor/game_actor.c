#include "actor/actor_ops.h"
#include "actor/actor_spawns.h"
#include "effects/rag/rag_effect.h"
#include "render/screen_bounds.h"
#include "ui/window_access.h"
#include "std/tree.h"
#include "std/integer_map.h"
#include "ui/character/gauge.h"
#include "session/state.h"
#include "ui/window_manager_state.h"
#include "core/object_type.h"
#include "game/context.h"
#include "actor/actor.h"
#include "actor/game_actor.h"
#include "session/session_jobs.h"
#include "session/session_names.h"
#include "session/session_skills.h"
#include "session/session_talk.h"
#include <stdint.h>

#define ACTOR_SEND(self, sender, msg, par1) render_object_send((self), (sender), (msg), (par1), 0, 0, 0)

static struct Tree *actor_counters(struct GameActor *actor)
{
    return &actor->vars;
}

int game_actor_get_counter(struct GameActor *self, int key)
{
    struct IntInt64Pair *var = tree_find(actor_counters(self), &key, tree_less_int);

    if (!var)
        return 0;
    return (int)var->value;
}

void game_actor_set_counter(struct GameActor *self, int key, int64_t value)
{
    struct IntInt64Pair *var = tree_get(actor_counters(self), &key, sizeof key, sizeof *var, tree_less_int);

    var->value = value;
}

void game_actor_set_pet_accessory(struct GameActor *self, int accessory)
{
    const char *name = accessory ? session_get_pet_accessory_act(&g_session1, accessory) : NULL;

    if (name)
        render_object_load_act_res((struct RenderObject *)self, name);
    else
        actor_set_spr_job(self, game_actor_get_job(self));
}

void game_actor_show_level_aura(struct GameActor *self)
{
    int level;

    if (g_session1.auraOff)
        return;
    if (session_is_homunculus_job(&g_session1, game_actor_get_job(self))) {
        level = (short)self->level;
        if (g_session1.baseLevelHomun == level) {
            ACTOR_SEND(self, NULL, 0x93, 0);
            return;
        }
        if (g_session1.baseLevel != level) {
            self->aura_on = 0;
            return;
        }
    } else {
        level = (short)self->level;
        if (g_session1.baseLevel != level) {
            self->aura_on = 0;
            return;
        }
    }
    ACTOR_SEND(self, NULL, 0x7d, 0);
}

void game_actor_stop_aura_effects(struct GameActor *self)
{
    static const int own[] = { 0xc8, 0x3d0, 0x3d1, 0x3d2, 0x3d3, 0x3fe, 0x3ff, 0x4ca, 0x4cb };
    static const int anon[] = { 0x2f5, 0x370, 0xc8, 0x18d, 0x18e, 0x16a, 0x16a, 0xc9, 0xca, 0xca };
    int i;

    self->aura_level = 0;
    for (i = 0; i < (int)(sizeof own / sizeof own[0]); i++)
        ACTOR_SEND(self, self, 0x6d, own[i]);
    for (i = 0; i < (int)(sizeof anon / sizeof anon[0]); i++)
        ACTOR_SEND(self, NULL, 0x6d, anon[i]);
}

void game_actor_drop_siege_window(struct GameActor *self)
{
    if (session_is_siege_mode(&g_session1) && self->siege_wnd) {
        game_mode_remove_listed(game_current_mode(), self->siege_wnd);
        ui_window_mgr_post_quit2(&g_windowMgr, self->siege_wnd);
        self->siege_wnd = NULL;
    }
}

void game_actor_forget_handle(struct GameActor *self, struct GameObject *handle)
{
    if (self->cart == handle) { self->cart = 0; return; }
    if (self->falcon == handle) { self->falcon = 0; return; }
    if (self->emotion_mark == handle) { self->emotion_mark = 0; return; }
    if (self->frozen_mark == handle) { self->frozen_mark = 0; return; }
    if (self->curse_mark == handle) { self->curse_mark = 0; return; }
    if (self->sleep_mark == handle) { self->sleep_mark = 0; return; }
    if (self->stun_mark == handle) { self->stun_mark = 0; return; }
    if (self->msg_mark_1 == handle) { self->msg_mark_1 = 0; return; }
    if (self->msg_mark_2 == handle) { self->msg_mark_2 = 0; return; }
    if (self->emotion_mark_a == handle) { self->emotion_mark_a = 0; return; }
    if (self->emotion_mute == handle) { self->emotion_mute = 0; return; }
    if (self->cursor_mark == handle) { self->cursor_mark = 0; return; }
}

int game_actor_action_of(struct GameActor *self, unsigned motion)
{
    int mode;
    uint64_t weapon;

    switch (motion) {
    case 0x01:
        return 8;
    case 0x02: case 0x2b: case 0x2c: case 0x34:
        return 0x50;
    case 0x03:
        return 0x40;
    case 0x04:
        return 0x30;
    case 0x05: case 0x14: case 0x19:
        return 0x18;
    case 0x09: case 0x12: case 0x20: case 0x31: case 0x35: case 0x36: case 0x3b:
        return 0x58;
    case 0x0b:
        return 0x20;
    case 0x0c: case 0x13: case 0x1c: case 0x28: case 0x2a: case 0x32: case 0x33: case 0x3a:
        return 0x28;
    case 0x0d:
        mode = self->sex;
        weapon = ((uint64_t)actor_get_weapon(self));
        return session_job_accepts_weapon(&g_session1, game_actor_get_job(self), mode, weapon) ? 0x58 : 0x50;
    case 0x10: case 0x11: case 0x15: case 0x16: case 0x17: case 0x18: case 0x1b: case 0x1e:
    case 0x21: case 0x22: case 0x23: case 0x24: case 0x25: case 0x26: case 0x27: case 0x29:
    case 0x2d: case 0x2f: case 0x30: case 0x37: case 0x38: case 0x39:
        return 0x60;
    case 0x1a:
        switch (game_actor_get_job(self)) {
        case 0xfd1: case 0x1083:
            return 0x58;
        default:
            return 0x28;
        }
    default:
        return 0;
    }
}

int game_actor_get_height_offset(struct GameActor *self)
{
    int offset = 0;

    if (session_is_player_job(&g_session1, game_actor_get_job(self))
        || session_get_pet_acc_act_name(&g_session1, game_actor_get_job(self))
        || self->aid == g_session1.ownAid) {
        if (session_is_baby_job(&g_session1, game_actor_get_job(self)))
            return self->mounted ? 10 : -20;
        if (self->mounted)
            offset = 30;
    }
    return offset;
}

int game_actor_get_attack_motion(struct GameActor * self)
{
    void *act = self->act;
    int count = actres_frame_count(act, self->action);
    int i;

    for (i = 0; i < count; i++) {
        int event = act_res_get_motion(act, self->action, i)->eventId;

        if (event != -1 && strcmp(act_res_get_event_name(act, event), "atk") == 0)
            return i;
    }
    return count == 1 ? 0 : count - 2;
}

void render_object_apply_shrink_scale(struct GameActor *self, float *scale)
{
    unsigned step, deg;

    if (!(self->overlay_flags & 0x10))
        return;
    step = (unsigned char)self->swell_step;
    deg = step * 4 + 90;
    if (deg > 270) {
        *scale = *scale * 0.5f;
        return;
    }
    if (deg < 90)
        deg = 90;
    *scale = ((g_sinTable[deg] + 1.0f) * 0.25f + 0.5f) * *scale;
}

int game_actor_transform_action_of(int kind)
{
    switch (kind) {
    case 0: return 0;
    case 1: return 8;
    case 3: return 0x20;
    case 4: return 0x18;
    default: return 0x10;
    }
}

int game_actor_get_transform_job(struct GameActor *self)
{
    int r;

    if (self->transform_jobs[3] != -1)
        return self->transform_jobs[3];
    if (self->aid == g_session1.ownAid)
        return session_get_transform_job(&g_session1);
    r = -1;
    if (self->transform_jobs[0] != -1)
        r = self->transform_jobs[0];
    if (self->transform_jobs[1] != -1)
        r = self->transform_jobs[1];
    if (self->transform_jobs[2] != -1)
        r = self->transform_jobs[2];
    return r;
}

int game_actor_get_mount_height(struct GameActor *self)
{
    int job = game_actor_get_job(self);

    if (job == 0xd || job == 0x15 || job == 0xfae || job == 0xfb6 || job == 0xfc4 || job == 0xfcc)
        return 3;
    return 0;
}

int * game_actor_get_request(struct GameActor * self, int which)
{
    if (which == 0)
        return &self->requests[0];
    if (which == 1)
        return &self->requests[12];
    return NULL;
}

int * game_actor_get_request2(struct GameActor * self, int which)
{
    if (which == 0)
        return &self->requests[6];
    if (which == 1)
        return &self->requests[18];
    return NULL;
}

int game_actor_state_job(int job, int state)
{
    if (job == 0x7)    return effect_state_bit5(state) ? job + 6 : job;
    if (job == 0xe)    return effect_state_bit5(state) ? job + 7 : job;
    if (job == 0xfa8)  return effect_state_bit5(state) ? job + 6 : job;
    if (job == 0xfbe)  return effect_state_bit5(state) ? job + 6 : job;
    if (job == 0xfaf)  return effect_state_bit5(state) ? job + 7 : job;
    if (job == 0xfc5)  return effect_state_bit5(state) ? job + 7 : job;
    if (job == 0xfcf)  return effect_state_bit15(state) ? job + 1 : job;
    if (job == 0x1082) return effect_state_bit15(state) ? job + 0xc : job;
    return job;
}

float game_actor_get_mount_lift(struct GameActor *self)
{
    int job = game_actor_get_job(self);

    if (job == 0xfb6 || job == 0xd || job == 0x15 || job == 0xfae)
        return 6.0f;
    if (job == 0xfc4 || job == 0xfcc)
        return 4.0f;
    if (session_is_player_job(&g_session1, game_actor_get_job(self))
        || session_get_pet_acc_act_name(&g_session1, game_actor_get_job(self))
        || self->aid == g_session1.ownAid) {
        if (session_is_baby_job(&g_session1, game_actor_get_job(self)))
            return self->mounted ? 4.0f : -1.0f;
        if (self->mounted)
            return 6.0f;
    }
    return 0.0f;
}

unsigned char job_is_monster(int job)
{
    if ((job > 0x3e8 && job < 0xf9f) || (unsigned)(job - 0x4e35) <= 0x2eca)
        return 1;
    return 0;
}

unsigned char game_actor_has_pending_hits(struct GameActor *self)
{
    return self->hit_queue.size > 0;
}

int game_actor_is_pc_job(int job)
{
    return session_is_pc_job(&g_session1, job);
}

void game_actor_launch_ranged_attack(struct GameActor *self, int id, float height)
{
    struct GameMode *mode = game_current_mode();

    switch (game_actor_get_job(self)) {
    case 0x3fc: case 0x42c: case 0x6a8: case 0x6ad: case 0x6f5: case 0x75c: case 0x75e:
    case 0x7a5: case 0x7a6: case 0x7a7: case 0x7a8: case 0x7a9: case 0x7c4: case 0xe94:
    case 0xecf: case 0x500b:
        actor_spawn_plant_attack(mode, id, game_actor_get_job(self), 0.0f);
        break;
    case 0x3f8: case 0x4a5: case 0x4e5: case 0x4ea: case 0x4fc: case 0x502: case 0x505:
    case 0x582: case 0x584: case 0x58c: case 0x5ad: case 0x5af: case 0x5c1: case 0x5d7:
    case 0x5da: case 0x5dc: case 0x5fb: case 0x60e: case 0x613: case 0x680: case 0x681:
    case 0x682: case 0x683: case 0x698: case 0x749: case 0x7e5: case 0x93c: case 0x9ab:
    case 0xf2a: case 0x1781: case 0x1782: case 0x1783: case 0x1784: case 0x1785: case 0x1786:
    case 0x1787: case 0x1788: case 0x1789: case 0x178a: case 0x4f32: case 0x4f33: case 0x4f37:
    case 0x4f90: case 0x505f: case 0x5062: case 0x5069: case 0x5410: case 0x5455:
        actor_spawn_arrow(mode, &self->pos, id, height, game_actor_get_job(self));
        break;
    }
}

static void actor_rejob(struct GameActor *self, int job)
{
    actor_set_spr_job(self, job > 0 ? job : game_actor_get_job(self));
    actor_set_state(self, self->state, 1);
}

void game_actor_raise_body_quarter_height(struct GameActor *self, int value)
{
    if (value <= 0 || self->sprite_height < value)
        self->sprite_height = value;
}

void game_actor_set_transform_job(struct GameActor *self, int value, int index)
{
    self->sprite_height = 0;
    if (self->aid == g_session1.ownAid)
        session_set_transform_job(&g_session1, value, index);
    else if (index == 0)
        self->transform_jobs[0] = value;
    else if (index == 1)
        self->transform_jobs[1] = value;
    else if (index == 2)
        self->transform_jobs[2] = value;
    actor_rejob(self, value);
}

void game_actor_set_transform_job_override(struct GameActor *self, int value)
{
    self->sprite_height = 0;
    self->transform_jobs[3] = value;
    actor_rejob(self, value);
}

void game_actor_set_mark_listed(struct GameActor *self, int value)
{
    self->mark_listed = value;
}

void game_actor_set_guild(struct GameActor * self, int guild, int version)
{
    struct Session *session = (struct Session *)(&g_session1);

    if (guild <= 0)
        return;
    self->guild_id = guild;
    if (game_actor_get_job(self) != 0x4f2d) {
        if (game_actor_get_job(self) != 0x2d2 && version <= 0)
            return;
        if (!session_get_pet_acc_act_name(session, game_actor_get_job(self))
            && !session_is_player_job(&g_session1, game_actor_get_job(self))
            && !session_is_guardian(session, game_actor_get_job(self))
            && game_actor_get_job(self) != 0x2d2
            && game_actor_get_job(self) != 0x508)
            return;
    }
    guild_mgr_set_emblem_version(g_guildMgr, guild, version);
}

void bounds_set(struct ScreenBounds *self, float a, float b, float c, float d, float e)
{
    self->left = a;
    self->right = c;
    self->top = b;
    self->bottom = d;
    self->z = e;
}

void game_actor_set_job(struct GameActor *self, int job)
{
    self->job = job;
    switch (job) {
    case 0xd: self->job = 7; break;
    case 0x15: self->job = 0xe; break;
    case 0xfae: self->job = 0xfa8; break;
    case 0xfb6: self->job = 0xfaf; break;
    case 0xfc4: self->job = 0xfbe; break;
    case 0xfcc: self->job = 0xfc5; break;
    case 0xfd0: self->job = 0xfcf; break;
    case 0x108e: self->job = 0x1082; break;
    }
    actor_set_spr_job(self, game_actor_get_job(self));
    game_actor_stop_aura_effects(self);
    game_actor_show_level_aura(self);
}

void game_actor_set_request(struct GameActor * self, const int * record, int which)
{
    if (which == 0)
        memcpy(&self->requests[0], record, 0x18);
    else if (which == 1)
        memcpy(&self->requests[12], record, 0x18);
}

void game_actor_set_pk_mode(struct GameActor *self, int value)
{
    self->pk_mode = value;
}

void game_object_set_render_info(struct RenderObject *self, const struct ScreenBounds *rect, float a, float b)
{
    (void)self; (void)rect; (void)a; (void)b;
}

void game_actor_set_request2(struct GameActor * self, const int * record, int which)
{
    if (which == 0)
        memcpy(&self->requests[6], record, 0x18);
    else if (which == 1)
        memcpy(&self->requests[18], record, 0x18);
}

void game_actor_set_sex(struct GameActor *self, int value)
{
    self->sex = value & 1;
}

void render_object_load_spr_res(struct RenderObject *self, const char *name)
{
    if (self->spr)
        res_unlock((struct Res *)self->spr);
    self->spr = res_mgr_get(g_resMgr(), name);
    if (self->spr)
        res_lock((struct Res *)self->spr);
}

void game_actor_clear_unit_cell(struct GameActor * self)
{
    game_send_message(0xd, self->aid, 0, 0, 0);
}
