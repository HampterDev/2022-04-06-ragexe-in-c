#include "actor/actor_ops.h"
#include "contents/client_lua_calls.h"
#include "session/state.h"
#include "actor/actor.h"
#include "actor/pc.h"
#include "actor/sprite_render.h"
#include "std/std.h"
#include "core/msg_str.h"
#include "platform/clientinfo.h"
#include "session/session_jobs.h"
#include "session/session_lua.h"
#include "session/session_names.h"
#include "session/session_talk.h"
#include "ui/ui_std.h"
#include "session/session_skills.h"
#include <stdint.h>

void pc_set_imf_name(struct GameActor * base_actor)
{
    struct Pc * self = (struct Pc *)base_actor;

    struct String name;
    int job = game_actor_get_job((struct GameActor *)self);

    session_get_imf_name(&name, job, self->sex);
    string_free(&self->imf_name);
    string_move(&self->imf_name, &name);
}

void pc_set_head_pal_name(struct GameActor * base_actor)
{
    struct Pc * self = (struct Pc *)base_actor;

    struct String name;

    if (!self->head_palette)
        return;
    session_get_head_pal_name(&name, self->head, self->look_job, self->sex, self->head_palette);
    string_free(&self->head_pal_name);
    string_move(&self->head_pal_name, &name);
}

void pc_set_body_pal_name(struct GameActor * base_actor)
{
    struct Pc * self = (struct Pc *)base_actor;

    struct String name;
    int job;

    if (!self->body_palette)
        return;
    job = game_actor_get_job((struct GameActor *)self);
    if (self->mounted) {
        if (self->body_style > 0)
            job = session_get_mounted_job(game_actor_get_job((struct GameActor *)self));
    }
    session_get_body_pal_name(&name, job, self->sex, self->body_palette, self->body_style,
                            actor_get_sprite_variant(self));
    string_free(&self->body_pal_name);
    string_move(&self->body_pal_name, &name);
}

void pc_set_body_palette(struct GameActor * base_actor, int palette)
{
    struct Pc * self = (struct Pc *)base_actor;

    self->body_palette = palette;
    if (palette < 2 && !self->body_style) {
        if (game_actor_get_job((struct GameActor *)self) > 0xfa0
            && game_actor_get_job((struct GameActor *)self) < 0xfb7) {
            self->body_palette = 0;
            if (game_actor_get_job((struct GameActor *)self) > 0xfa0
                && game_actor_get_job((struct GameActor *)self) <= 0xfa7)
                self->body_palette = 1;
        } else {
            int job = session_remap_job(&g_session1, game_actor_get_job((struct GameActor *)self));

            self->body_palette = (unsigned)(job - 0xfa1) <= 6;
        }
    }
    actor_set_body_pal_name(self);
}

void pc_set_head_palette(struct GameActor * base_actor, int palette)
{
    struct Pc * self = (struct Pc *)base_actor;

    self->head_palette = palette;
    actor_set_head_pal_name(self);
}

void pc_set_delay_factor(struct GameActor * base_actor, int ms)
{
    struct Pc * self = (struct Pc *)base_actor;

    int kind = (int)self->weapon;

    if (kind > 0x69)
        kind = session_get_weapon_view_id(kind);
    if (kind != 11 && ms > g_maxAttackDelayMs)
        ms = g_maxAttackDelayMs;
    self->delay_factor = (float)ms / 432.0f;
}

void pc_set_honor(struct GameActor * base_actor, int value)
{
    struct Pc * self = (struct Pc *)base_actor;

    if (value < 0)
        render_object_send(self, NULL, 0x15, 0x22, 0, 0, 0);
    else
        render_object_send(self, NULL, 0x76, 0, 0, 0, 0);
    self->honor = value;
}

void pc_set_honor_virtue(struct GameActor * base_actor, int value, unsigned int flags)
{
    struct Pc * self = (struct Pc *)base_actor;

    actor_set_honor(self, value);
    self->virtue = flags;
}

static void pc_swap_res_cstr(struct Pc *self, int layer, const char *name)
{
    struct Res **res = (struct Res **)self->layer_sprs.first;

    if (res[layer])
        res_unlock(res[layer]);
    res[layer] = res_mgr_get(g_resMgr(), name);
    if (res[layer])
        res_lock(res[layer]);
}

static void pc_swap_res(struct Pc *self, int layer, struct String *name)
{
    pc_swap_res_cstr(self, layer, string_cstr(name));
}

static void pc_set_accessory_layer(struct Pc *self, int layer, int *field, int item)
{
    struct String name;

    *field = item;
    session_get_res_name0(&name, game_actor_get_job((struct GameActor *)self),
                         self->sex, self->head, item);
    pc_set_layer_act(self, layer, string_cstr(&name));
    string_free(&name);
    session_get_res_name1(&name, game_actor_get_job((struct GameActor *)self),
                         self->sex, self->head, *field);
    pc_swap_res(self, layer, &name);
    string_free(&name);
}

void pc_set_accessory(struct GameActor * base_actor, int item)
{
    struct Pc * self = (struct Pc *)base_actor;

    pc_set_accessory_layer(self, 2, &self->accessory[0], item);
}

void pc_set_accessory2(struct GameActor * base_actor, int item)
{
    struct Pc * self = (struct Pc *)base_actor;

    pc_set_accessory_layer(self, 3, &self->accessory[1], item);
    if (able_to_make_effect_is_effect_exist(self, 0x2a6)) {
        render_object_send(self, NULL, 0x6d, 0x2a6, 0, 0, 0);
        render_object_send(self, NULL, 0x6d, 0x2a7, 0, 0, 0);
        render_object_send(self, NULL, 0x6d, 0x2a8, 0, 0, 0);
    }
}

void pc_set_accessory3(struct GameActor * base_actor, int item)
{
    struct Pc * self = (struct Pc *)base_actor;

    pc_set_accessory_layer(self, 4, &self->accessory[2], item);
}

void pc_set_head(struct GameActor * base_actor, int head)
{
    struct Pc * self = (struct Pc *)base_actor;

    struct String name;

    self->head = head;
    session_get_head_spr_name0(&name, self->look_job, self->sex, &self->head);
    pc_set_layer_act(self, 1, string_cstr(&name));
    string_free(&name);
    session_get_head_spr_name1(&name, self->look_job, self->sex, &self->head);
    pc_swap_res(self, 1, &name);
    string_free(&name);
}

void pc_refresh_accessory3(struct GameActor * base_actor)
{
    struct Pc * self = (struct Pc *)base_actor;

    if (self->accessory[2] == 0xb9)
        actor_set_accessory3(self, 0xb9);
}

void pc_set_shield(struct GameActor * base_actor, int shield)
{
    struct Pc * self = (struct Pc *)base_actor;

    struct String name;
    int job;

    self->shield = shield;
    if (self->mounted) {
        pc_set_layer_act(self, 7, "");
        pc_swap_res_cstr(self, 7, "");
        return;
    }
    job = game_actor_get_job((struct GameActor *)self);
    session_get_shield_name0(&name, job, self->sex, self->shield);
    pc_set_layer_act(self, 7, string_cstr(&name));
    string_free(&name);
    session_get_shield_name1(&name, job, self->sex, self->shield);
    pc_swap_res(self, 7, &name);
    string_free(&name);
}

void pc_set_robe(struct GameActor * base_actor, int robe, unsigned char keep)
{
    struct Pc * self = (struct Pc *)base_actor;

    struct String name;
    int item;

    if (self->robe == robe || keep)
        return;
    self->robe = robe;

    client_lua_call_table(g_session1.luaState, "TB_Layer_Priority", "IsHelmRobe", "d>b", robe, &self->helm_robe);
    item = self->helm_robe == 1 ? self->robe : 0;
    session_get_res_name0(&name, game_actor_get_job((struct GameActor *)self),
                         self->sex, self->head, item);
    pc_set_layer_act(self, 8, string_cstr(&name));
    string_free(&name);
    item = self->helm_robe == 1 ? self->robe : 0;
    session_get_res_name1(&name, game_actor_get_job((struct GameActor *)self),
                         self->sex, self->head, item);
    pc_swap_res(self, 8, &name);
    string_free(&name);
}

static void pc_body_names(struct Pc *self, struct String *name, int job, int a, int b)
{
    session_get_body_spr_name_variant0(name, job, self->sex, a, b);
    pc_set_layer_act(self, 0, string_cstr(name));
    string_free(name);
    session_get_body_spr_name_variant1(name, job, self->sex, a, b);
}

void pc_set_spr_job(struct GameActor * base_actor, int job)
{
    struct Pc * self = (struct Pc *)base_actor;

    struct String name;
    const char *res_name;
    int st, other;

    switch (job) {
    case 0x13: case 0xfb4: case 0xfca:
        if (self->sex == 0)
            self->sex = 1;
        break;
    case 0x14: case 0xfb5: case 0xfcb:
        if (self->sex == 1)
            self->sex = 0;
        break;
    }
    self->shadow_scale = session_get_npc_location_radius(&g_session1, job, self->mounted == 1);

    st = self->effect_state;
    if (game_actor_get_transform_job((struct GameActor *)self) > 0) {
        session_get_monster_act_name(&g_session1, &name, game_actor_get_transform_job((struct GameActor *)self));
        pc_set_layer_act(self, 0, string_cstr(&name));
        string_free(&name);
        session_get_monster_sprite_name(&g_session1, &name, game_actor_get_transform_job((struct GameActor *)self));
    } else if (effect_state_bit12(st) == 1 || job == 0x16) {
        session_get_body_spr_name(&name, 0, self->look_job, 0x16, self->sex, 0);
        pc_set_layer_act(self, 0, string_cstr(&name));
        string_free(&name);
        session_get_body_spr_name(&name, 1, self->look_job, 0x16, self->sex, 0);
    } else if (effect_state_bit16(self->effect_state) && !effect_state_bit15(self->effect_state)) {
        pc_body_names(self, &name, 0x1a, 0, 0);
    } else if (effect_state_bit18(self->effect_state) && !effect_state_bit15(self->effect_state)) {
        pc_body_names(self, &name, 0x1b, 0, 0);
    } else if (effect_state_bit27(self->effect_state) && !effect_state_bit15(self->effect_state)) {
        pc_body_names(self, &name, 0x1c, 0, 0);
    } else if (effect_state_bit28(self->effect_state) && !effect_state_bit15(self->effect_state)) {
        pc_body_names(self, &name, 0x1d, 0, 0);
    } else if (self->mount_look == 1 && !effect_state_bit15(self->effect_state)) {
        pc_body_names(self, &name, 0x1e, 0, 0);
    } else if (self->mounted) {
        session_get_body_spr_name_variant0(&name, session_get_mounted_job(job), self->sex,
                                        self->body_style, 0);
        pc_set_layer_act(self, 0, string_cstr(&name));
        string_free(&name);
        session_get_body_spr_name_variant1(&name, session_get_mounted_job(job), self->sex,
                                        self->body_style, 0);
    } else {
        switch (job) {
        }
        other = actor_get_sprite_variant(self);
        session_get_body_spr_name_variant0(&name, job, self->sex, self->body_style, other);
        pc_set_layer_act(self, 0, string_cstr(&name));
        string_free(&name);
        other = actor_get_sprite_variant(self);
        session_get_body_spr_name_variant1(&name, job, self->sex, self->body_style, other);
    }
    pc_set_layer_spr(self, 0, string_cstr(&name));
    string_free(&name);

    res_name = ((struct Res **)self->layer_acts.first)[0] ? res_get_name(((struct Res **)self->layer_acts.first)[0])
                                                         : "";
    render_object_load_act_res((struct RenderObject *)self, res_name);
    res_name = ((struct Res **)self->layer_sprs.first)[0] ? res_get_name(((struct Res **)self->layer_sprs.first)[0])
                                                         : "";
    render_object_load_spr_res((struct RenderObject *)self, res_name);
    actor_set_imf_name(self);
}

static struct Res *pc_layer_res(struct Pc *self, int layer, const char *name, const char *const names[2][2])
{
    if (clientinfo_is_yellow_aid(self->aid)) {
        if (layer == 0 || layer == 5)
            return res_mgr_get(g_resMgr(), names[layer == 5][self->sex != 0]);
        if (layer == 6)
            return NULL;
    }
    return res_mgr_get(g_resMgr(), name);
}

void pc_set_layer_spr(struct Pc *self, int layer, const char *name)
{
    static const char *const names[2][2] = {
        { "인간족\\몸통\\여\\운영자_여.Spr"  ,
          "인간족\\몸통\\남\\운영자_남.Spr"   },
        { "인간족\\운영자\\운영자_여_검.Spr"  ,
          "인간족\\운영자\\운영자_남_검.Spr"   },
    };
    struct Res **res = (struct Res **)self->layer_sprs.first;

    if (res[layer])
        res_unlock(res[layer]);
    ((struct Res **)self->layer_sprs.first)[layer] = pc_layer_res(self, layer, name, names);
    res = (struct Res **)self->layer_sprs.first;
    if (res[layer])
        res_lock(res[layer]);
}

void pc_set_layer_act(struct Pc *self, int layer, const char *name)
{
    static const char *const names[2][2] = {
        { "인간족\\몸통\\여\\운영자_여.Act"  ,
          "인간족\\몸통\\남\\운영자_남.Act"   },
        { "인간족\\운영자\\운영자_여_검.Act"  ,
          "인간족\\운영자\\운영자_남_검.Act"   },
    };
    struct Res **res = (struct Res **)self->layer_acts.first;

    if (res[layer])
        res_unlock(res[layer]);
    ((struct Res **)self->layer_acts.first)[layer] = pc_layer_res(self, layer, name, names);
    res = (struct Res **)self->layer_acts.first;
    if (res[layer])
        res_lock(res[layer]);
    pc_place_head((struct Pc *)self);
}

void pc_set_weapon(struct GameActor * base_actor, int64_t weapon)
{
    struct Pc * self = (struct Pc *)base_actor;

    struct String act, spr, glow_act, glow_spr;
    int glow;

    self->weapon = weapon;
    if (!session_job_accepts_value(&g_session1, game_actor_get_job((struct GameActor *)self),
                                  (uint64_t)weapon)) {
        pc_set_layer_act(self, 5, "");
        pc_set_layer_spr(self, 5, "");
        pc_set_layer_act(self, 6, "");
        pc_set_layer_spr(self, 6, "");
        return;
    }
    session_get_weapon_act_name(&act, game_actor_get_job((struct GameActor *)self),
                             self->sex, (int)self->weapon, -1);
    pc_set_layer_act(self, 5, string_cstr(&act));
    session_get_weapon_spr_name(&spr, game_actor_get_job((struct GameActor *)self),
                             self->sex, (int)self->weapon, -1);
    pc_set_layer_spr(self, 5, string_cstr(&spr));

    glow = 0;
    if (self->weapon >= 1 && self->weapon <= 0x1e) {
        int kind = (int)self->weapon;

        glow = (kind <= 7) || (kind >= 16 && kind <= 18) || kind >= 25;
    }
    if (glow) {
        session_get_glow_weapon_act_name(&glow_act, game_actor_get_job((struct GameActor *)self),
                                     self->sex, (int)self->weapon, -1, 0);
        pc_set_layer_act(self, 6, string_cstr(&glow_act));
        session_get_glow_weapon_spr_name(&glow_spr, game_actor_get_job((struct GameActor *)self),
                                     self->sex, (int)self->weapon, -1, 0);
        pc_set_layer_spr(self, 6, string_cstr(&glow_spr));
        string_free(&glow_spr);
        string_free(&glow_act);
    } else {
        pc_set_layer_act(self, 6, "");
        pc_set_layer_spr(self, 6, "");
    }
    string_free(&spr);
    string_free(&act);
}

int pc_weapon_pair_kind(int64_t weapon);
int pc_job_weapon_kind(struct Pc *self, int kind);

static int pc_res_available(const struct String *name)
{
    return res_mgr_is_available(g_resMgr(), string_cstr(name));
}

static void pc_set_glow(struct Pc *self, int job, int kind, int item, int flag)
{
    struct String act, spr;

    session_get_glow_weapon_act_name(&act, job, self->sex, kind, item, flag);
    session_get_glow_weapon_spr_name(&spr, job, self->sex, kind, item, flag);
    pc_set_layer_act(self, 6, string_cstr(&act));
    pc_set_layer_spr(self, 6, string_cstr(&spr));
    string_free(&spr);
    string_free(&act);
}

static void pc_set_weapon5(struct Pc *self, int job, int kind, int item)
{
    struct String act, spr;

    session_get_weapon_act_name(&act, job, self->sex, kind, item);
    pc_set_layer_act(self, 5, string_cstr(&act));
    session_get_weapon_spr_name(&spr, job, self->sex, kind, item);
    pc_set_layer_spr(self, 5, string_cstr(&spr));
    string_free(&spr);
    string_free(&act);
}

void pc_set_weapon_look(struct GameActor * base_actor, int64_t weapon)
{
    struct Pc * self = (struct Pc *)base_actor;

    struct String act, name, glow_act, glow_spr;
    int job, kind, item;
    int st = self->effect_state;

    self->weapon = weapon;
    job = game_actor_get_job((struct GameActor *)self);
    if (job > 0xf6e)
        job -= 0xf6e;
    if (self->mounted || effect_state_bit12(st) || effect_state_bit16(st) || effect_state_bit18(st)
        || effect_state_bit27(st) || effect_state_bit28(st) || self->mount_look == 1)
        return;
    pc_set_layer_act(self, 5, "");
    pc_set_layer_spr(self, 5, "");
    pc_set_layer_act(self, 6, "");
    pc_set_layer_spr(self, 6, "");

    switch (game_actor_get_job((struct GameActor *)self)) {
    case 0xfce: case 0xfcf: case 0xfd0: case 0x1081: case 0x1082: case 0x108e:
        pc_set_glow(self, job, 0, -1, 1);
        return;
    }

    if (!self->weapon)
        return;
    if (session_get_weapon_view_id((int)(self->weapon >> 32)) > 0) {
        kind = pc_weapon_pair_kind(self->weapon);
        switch (game_actor_get_job((struct GameActor *)self)) {
        case 0xc: case 0xfad: case 0xfc3:
            if (kind == 0 || kind == 7 || kind == 8)
                kind = 2;
            break;
        }
        pc_set_weapon5(self, job, kind, -1);
        if ((kind >= 1 && kind <= 7) || (kind >= 16 && kind <= 18) || (kind >= 25 && kind <= 30))
            pc_set_glow(self, job, kind, -1, 0);
        return;
    }

    item = (int)self->weapon;
    kind = session_get_weapon_view_id(item);
    session_get_weapon_act_name(&act, job, self->sex, kind, item);
    if (pc_res_available(&act)) {
        pc_set_layer_act(self, 5, string_cstr(&act));
        session_get_weapon_spr_name(&name, job, self->sex, kind, item);
        pc_set_layer_spr(self, 5, string_cstr(&name));
        kind = pc_job_weapon_kind(self, kind);
    } else {
        kind = session_get_weapon_view_id(item);
        if (kind == -1 || kind == 0 || item == 0x63f) {
            string_free(&act);
            return;
        }
        if (kind == 0xc) {
            switch (game_actor_get_job((struct GameActor *)self)) {
            case 0x8: case 0xfa9: case 0xfbf:
                string_free(&act);
                return;
            }
        }
        kind = pc_job_weapon_kind(self, kind);
        session_get_weapon_act_name(&name, job, self->sex, kind, -1);
        if (pc_res_available(&name)) {
            struct String spr;

            pc_set_layer_act(self, 5, string_cstr(&name));
            session_get_weapon_spr_name(&spr, job, self->sex, kind, -1);
            pc_set_layer_spr(self, 5, string_cstr(&spr));
            string_free(&spr);
        }
    }
    string_free(&name);

    session_get_glow_weapon_act_name(&glow_act, job, self->sex, kind, item, 0);
    session_get_glow_weapon_spr_name(&glow_spr, job, self->sex, kind, item, 0);
    if (pc_res_available(&glow_act)) {
        pc_set_layer_act(self, 6, string_cstr(&glow_act));
        pc_set_layer_spr(self, 6, string_cstr(&glow_spr));
    } else {
        struct String tmp;

        session_get_glow_weapon_act_name(&tmp, job, self->sex, kind, -1, 0);
        string_assign(&glow_act, string_cstr(&tmp), tmp.size);
        string_free(&tmp);
        session_get_glow_weapon_spr_name(&tmp, job, self->sex, kind, -1, 0);
        string_assign(&glow_spr, string_cstr(&tmp), tmp.size);
        string_free(&tmp);
        if (pc_res_available(&glow_act)) {
            pc_set_layer_act(self, 6, string_cstr(&glow_act));
            pc_set_layer_spr(self, 6, string_cstr(&glow_spr));
        }
    }
    string_free(&glow_spr);
    string_free(&glow_act);
    string_free(&act);
}

static int weapon_group_a(int id)
{
    return (id >= 0x44c && id < 0x47e) || (id >= 0x3458 && id < 0x34bc) || (unsigned)(id - 0x7a120) <= 0x270f;
}

static int weapon_group_b(int id)
{
    return (id >= 0x4b0 && id < 0x4e2) || (id >= 0x32c8 && id < 0x332c) || (id >= 0x701c && id < 0x70e4)
           || (unsigned)(id - 0x7c830) <= 0x270f;
}

static int weapon_group_c(int id)
{
    return (id >= 0x514 && id < 0x546) || (unsigned)(id - 0x7ef40) <= 0x270f;
}

int pc_weapon_pair_kind(int64_t weapon)
{
    int right = (int)weapon, left = (int)(weapon >> 32);

    if (!right) {
        right = left;
        if (!right)
            return 0;
        left = 0;
    }
    if (weapon_group_a(right)) {
        if (weapon_group_a(left))
            return 0x1a;
        if (weapon_group_b(left))
            return 0x1c;
        if (weapon_group_c(left))
            return 0x1e;
        return 2;
    }
    if (weapon_group_b(right)) {
        if (weapon_group_a(left))
            return 0x1c;
        if (weapon_group_b(left))
            return 0x19;
        if (weapon_group_c(left))
            return 0x1d;
        return 1;
    }
    if (weapon_group_c(right))
        return weapon_group_c(left) ? 0x1b : 6;
    return 0;
}

int pc_job_weapon_kind(struct Pc *self, int kind)
{
    switch (game_actor_get_job((struct GameActor *)self)) {
    case 2: case 9: case 16: case 0xfa3: case 0xfaa: case 0xfb1: case 0xfb9: case 0xfc0: case 0xfc7:
        if (kind == 5)
            kind = 0xa;
        break;
    case 6: case 0xfa7: case 0xfbd:
        if (kind == 8 || kind == 6 || kind == 7)
            kind = 2;
        break;
    case 12: case 17: case 0xfad: case 0xfb2: case 0xfc3: case 0xfc8:
        if (kind == 8 || kind == 7)
            kind = 2;
        else if (kind == 6
                 && (game_actor_get_job((struct GameActor *)self) == 0x11
                     || game_actor_get_job((struct GameActor *)self) == 0xfb2
                     || game_actor_get_job((struct GameActor *)self) == 0xfc8))
            kind = 1;
        break;
    }
    return kind;
}

void pc_set_shoes(struct GameActor *pc, int value)
{
    ((struct Pc *)pc)->shoes = value;
}
