#include "actor/object_ops.h"
#include "actor/render_ops.h"
#include "resources/sprite.h"
#include "resources/animation.h"
#include "effects/render_object.h"
#include "actor/game_actor.h"
#include "effects/skills/arrow_effect.h"

#define MONSTER_DIR "몬스터\\"
#define BULLET(name) { MONSTER_DIR name ".spr", MONSTER_DIR name ".act" }

#define MSG_START 0xe
#define MSG_START_AT_POINT 0x40

struct ArrowEffect {
    struct RenderObject base;
    Vec3 start;
    Vec3 step;
    Vec3 target_pos;
    float speed;
    float launch_delay;
    int target_aid;
    int frames_flown;
};

struct MissileSprite {
    const char *spr, *act;
};

struct GameObject;

const struct RenderObjectOps arrow_effect_ops = {
    .base = {
        .destroy = arrow_effect_delete,
        .on_process = arrow_effect_on_process,
        .send_msg = arrow_effect_send_msg,
        .render = render_object_render,
        .get_direction = game_object_dir_of,
    },
    .render_body = game_object_set_render_info,
    .set_action = render_object_set_action_base,
    .update_motion = render_object_update_motion_base,
    .get_effect_state = render_object_return_zero,
};

void arrow_effect_destruct(struct ArrowEffect *self)
{
    self->base.ops = &arrow_effect_ops.base;
    render_object_dc_render_object(&self->base);
}

void arrow_effect_delete(struct GameObject *object)
{
    struct ArrowEffect *self = (struct ArrowEffect *)object;

    arrow_effect_destruct(self);
    mem_free(self);
}

static struct ArrowEffect *arrow_effect_construct(struct ArrowEffect *self)
{
    render_object_construct(&self->base);
    self->base.ops = &arrow_effect_ops.base;
    self->start.x = 0.0f;
    self->start.y = 0.0f;
    self->start.z = 0.0f;
    self->step.x = 0.0f;
    self->step.y = 0.0f;
    self->step.z = 0.0f;
    self->target_pos.x = 0.0f;
    self->target_pos.y = 0.0f;
    self->target_pos.z = 0.0f;
    self->base.base_action = 0;
    self->start = self->base.pos;
    self->launch_delay = 0.0f;
    self->step.x = 0.0f;
    self->step.y = 0.0f;
    self->step.z = 0.0f;
    self->base.motion = 0;
    self->frames_flown = 0;
    return self;
}

struct ArrowEffect *arrow_effect_new(void)
{
    struct ArrowEffect *self = mem_alloc(sizeof *self);

    return self ? arrow_effect_construct(self) : NULL;
}

static void arrow_aim(struct ArrowEffect *self)
{
    Vec3 target, d;
    float dist, inv;

    self->base.visible = 1;
    target.x = 0.0f;
    target.y = 0.0f;
    target.z = 0.0f;
    if (!self->target_aid || !render_object_get_actor_pos(&self->base, self->target_aid, &target)) {
        target.x = self->target_pos.x;
        target.z = self->target_pos.z;
        target.y = self->target_pos.y;
    }
    target.y -= 10.0f;
    self->step.z = target.z - self->base.pos.z;
    self->step.y = target.y - self->base.pos.y;
    self->step.y = self->step.y * 0.125f;
    self->step.x = (target.x - self->base.pos.x) * 0.125f;
    self->step.z = self->step.z * 0.125f;
    dist = render_object_calc_dist(&self->base, target.x, target.y, target.z);
    if (40.0f > dist) {
        self->speed = 40.0f / dist;
        self->step.x = self->speed * self->step.x;
        self->step.y = self->step.y * self->speed;
        self->step.z = self->speed * self->step.z;
    }
    render_object_turn_toward(&self->base, target.x, target.z);
    d.y = target.y - self->base.pos.y;
    d.x = target.x - self->base.pos.x;
    d.z = target.z - self->base.pos.z;
    inv = 1.0f / float_sqrt(d.y * d.y + d.x * d.x + d.z * d.z);
    self->base.pos.x = inv * d.x * 2.5f + self->base.pos.x;
    self->base.pos.y = inv * d.y * 2.5f + self->base.pos.y;
    self->base.pos.z = inv * d.z * 2.5f + self->base.pos.z;
}

unsigned char arrow_effect_on_process(struct GameObject *object)
{
    struct ArrowEffect *self = (struct ArrowEffect *)object;

    float frames = render_object_frames_since(self->base.start_tick);
    float t;
    void *act;

    if (self->launch_delay > frames)
        return 1;
    if (!self->base.visible)
        arrow_aim(self);

    t = frames - self->launch_delay;
    self->frames_flown++;
    self->base.action = self->base.base_action;
    if (self->frames_flown > 8) {
        self->base.motion++;
        act = render_object_get_act_res(&self->base);
        if (self->base.motion >= (int)actres_frame_count(act, self->base.base_action))
            self->base.motion = (int)actres_frame_count(render_object_get_act_res(&self->base), self->base.action) - 1;
    }
    self->base.spr_angle = render_object_camera_angle(self->base.roty);
    self->base.pos.x = self->step.x * t + self->start.x;
    self->base.pos.y = self->step.y * t + self->start.y;
    self->base.pos.z = self->step.z * t + self->start.z;
    if (t > 8.0f / self->speed)
        return 0;
    return 1;
}

static struct MissileSprite missile_for_job(__int64 job64)
{
    static const struct MissileSprite arrow = BULLET("skel_archer_arrow");
    static const struct MissileSprite stone_shooter = BULLET("STONE_SHOOTER_BULLET");
    static const struct MissileSprite wootan_shooter = BULLET("WOOTAN_SHOOTER_BULLET");
    static const struct MissileSprite parasite = BULLET("PARASITE_BULLET");
    static const struct MissileSprite peach_tree = BULLET("PEACH_TREE_BULLET");
    static const struct MissileSprite ginseng = BULLET("GINSENG_BULLET");
    static const struct MissileSprite canon[4] = {
        BULLET("CANON_BULLET"), BULLET("CANON_BULLET_1"), BULLET("CANON_BULLET_2"), BULLET("CANON_BULLET_3"),
    };
    static const struct MissileSprite lady_tanee = BULLET("LADY_TANEE_BULLET");
    static const struct MissileSprite ragged_zombie = BULLET("RAGGED_ZOMBIE_BULLET");
    static const struct MissileSprite petal = BULLET("PETAL_BULLET");
    static const struct MissileSprite corruption_root = BULLET("MG_CORRUPTION_ROOT_BULLET");
    static const struct MissileSprite nyailo = BULLET("e_nyailo_ammo");
    static const struct MissileSprite soul_attack = BULLET("su_soulattack");
    static const struct MissileSprite corruption_root_2 = BULLET("mg_corruption_root_bullet");
    static const struct MissileSprite helltree_fruit = BULLET("SUMMON_HELLTREE_FRUIT");
    static const struct MissileSprite limacina = BULLET("EP19_LIMACINA_BULLET");
    int job;

    if (job64 < 0x582 || job64 > 0x5455)
        return arrow;
    job = (int)job64;
    switch (job) {
    case 0x582: return peach_tree;
    case 0x584: return ginseng;
    case 0x5d7: return stone_shooter;
    case 0x5da: return wootan_shooter;
    case 0x5dc: return parasite;
    case 0x5fb: return ginseng;
    case 0x60e: return peach_tree;
    case 0x613: return parasite;
    case 0x680: case 0x681: case 0x682: case 0x683: return canon[job - 0x680];
    case 0x698: return lady_tanee;
    case 0x749: return ragged_zombie;
    case 0x7e5: return lady_tanee;
    case 0x93c: return petal;
    case 0x9ab: return corruption_root;
    case 0xf2a: return nyailo;
    case 0x1079: case 0x107a: case 0x107b: case 0x107c: case 0x107d: return soul_attack;
    case 0x10d4: return soul_attack;
    case 0x10db: return soul_attack;
    case 0x4f32: return stone_shooter;
    case 0x4f33: return wootan_shooter;
    case 0x4f37: return wootan_shooter;
    case 0x505f: return corruption_root_2;
    case 0x5173: return helltree_fruit;
    case 0x5410: return limacina;
    case 0x5455: return limacina;
    }
    return arrow;
}

void arrow_effect_send_msg(struct GameObject *object, struct GameObject *source,
    RenderMsgArg msg, RenderMsgArg par1, RenderMsgArg par2, RenderMsgArg par3, RenderMsgArg par4)
{
    struct ArrowEffect *self = (struct ArrowEffect *)object;

    const Vec3 *from = (const Vec3 *)MSG_PTR(par1);

    if (msg == MSG_START) {
        struct MissileSprite sprite = missile_for_job(par4);

        render_object_load_spr_res(&self->base, sprite.spr);
        render_object_load_act_res(&self->base, sprite.act);
        self->speed = 1.0f;
        self->base.pos = *from;
        self->base.pos.y -= 10.0f;
        self->start = self->base.pos;
        self->target_aid = (int)par2;
        self->base.visible = 0;
        render_object_get_actor_pos(&self->base, self->target_aid, &self->target_pos);
    } else if (msg == MSG_START_AT_POINT) {
        render_object_load_spr_res(&self->base, MONSTER_DIR "skel_archer_arrow.spr");
        render_object_load_act_res(&self->base, MONSTER_DIR "skel_archer_arrow.act");
        self->speed = 1.0f;
        self->base.pos = *from;
        self->base.pos.y -= 10.0f;
        self->start = self->base.pos;
        self->target_pos = *(const Vec3 *)MSG_PTR(par2);
        self->base.visible = 0;
        self->target_aid = 0;
        self->target_pos.y -= 10.0f;
    } else {
        render_object_send_msg((struct GameObject *)&self->base, source, msg, par1, par2, par3, par4);
        return;
    }
    self->base.start_tick = timeGetTime();
    if (par3)
        self->launch_delay = *(const float *)MSG_PTR(par3);
}
