#include "actor/object_ops.h"
#include "actor/render_ops.h"
#include "resources/sprite.h"
#include "resources/animation.h"
#include "effects/render_object.h"
#include "actor/game_actor.h"
#include "effects/skills/mandra_attack_effect.h"

#define MONSTER_DIR "몬스터\\"

#define MSG_START 0xe
#define JOB_HYDRA 0x42c

struct MandraAttackEffect {
    struct RenderObject base;
    float start_delay;
};

struct GameObject;

const struct RenderObjectOps mandra_attack_effect_ops = {
    .base = {
        .destroy = mandra_attack_effect_delete,
        .on_process = mandra_attack_effect_on_process,
        .send_msg = mandra_attack_effect_send_msg,
        .render = render_object_render,
        .get_direction = game_object_dir_of,
    },
    .render_body = game_object_set_render_info,
    .set_action = render_object_set_action_base,
    .update_motion = render_object_update_motion_base,
    .get_effect_state = render_object_return_zero,
};

static struct MandraAttackEffect *mandra_attack_effect_construct(struct MandraAttackEffect *self)
{
    render_object_construct(&self->base);
    self->base.ops = &mandra_attack_effect_ops.base;
    render_object_load_spr_res(&self->base, MONSTER_DIR "mandragora_atk.spr");
    render_object_load_act_res(&self->base, MONSTER_DIR "mandragora_atk.act");
    self->start_delay = 0.0f;
    return self;
}

struct MandraAttackEffect *mandra_attack_effect_new(void)
{
    struct MandraAttackEffect *self = mem_alloc(sizeof *self);

    return self ? mandra_attack_effect_construct(self) : NULL;
}

void mandra_attack_effect_destruct(struct MandraAttackEffect *self)
{
    self->base.ops = &mandra_attack_effect_ops.base;
    render_object_dc_render_object(&self->base);
}

void mandra_attack_effect_delete(struct GameObject *object)
{
    struct MandraAttackEffect *self = (struct MandraAttackEffect *)object;

    mandra_attack_effect_destruct(self);
    mem_free(self);
}

unsigned char mandra_attack_effect_on_process(struct GameObject *object)
{
    struct MandraAttackEffect *self = (struct MandraAttackEffect *)object;

    float delay = self->start_delay;
    float frames = render_object_frames_since(self->base.start_tick);

    if (delay > frames) {
        self->base.visible = 0;
        return 1;
    }
    self->base.visible = 1;
    if (!self->base.motion_finished) {
        int count = (int)actres_frame_count(render_object_get_act_res(&self->base), self->base.action);
        float t = (frames - delay) / self->base.motion_delay;
        int motion = (int)t;

        t /= (float)count;
        self->base.motion = motion % count;
        if (t >= 1.0f) {
            self->base.motion_finished = 1;
            self->base.motion = count - 1;
        }
    }
    return self->base.motion_finished == 0;
}

void mandra_attack_effect_send_msg(struct GameObject *object, struct GameObject *source,
    RenderMsgArg msg, RenderMsgArg par1, RenderMsgArg par2, RenderMsgArg par3, RenderMsgArg par4)
{
    struct MandraAttackEffect *self = (struct MandraAttackEffect *)object;

    int job;
    const char *spr, *act;
    Vec3 target;

    if (msg != MSG_START) {
        render_object_send_msg((struct GameObject *)&self->base, source, msg, par1, par2, par3, par4);
        return;
    }
    job = (int)par3;
    if (job > 0x75e) {
        if (job > 0xe94) {
            if (job == 0xecf) {
                spr = MONSTER_DIR "jp_2017evt_atk.spr";
                act = MONSTER_DIR "jp_2017evt_atk.act";
            } else if (job == 0x500b) {
                spr = MONSTER_DIR "JP_ABYSS_BOSS_1_atk.spr";
                act = MONSTER_DIR "JP_ABYSS_BOSS_1_atk.act";
            } else {
                spr = MONSTER_DIR "mandragora_atk.spr";
                act = MONSTER_DIR "mandragora_atk.act";
            }
        } else if (job == 0xe94) {
            spr = MONSTER_DIR "JP_ABYSS_BOSS_1_atk.spr";
            act = MONSTER_DIR "JP_ABYSS_BOSS_1_atk.act";
        } else if (job >= 0x7a5 && job <= 0x7a9) {
            spr = MONSTER_DIR "ENTWEIHEN_BULLET.spr";
            act = MONSTER_DIR "ENTWEIHEN_BULLET.act";
        } else if (job == 0x7c4) {
            spr = MONSTER_DIR "NEPENTHES_BULLET.spr";
            act = MONSTER_DIR "NEPENTHES_BULLET.act";
        } else {
            spr = MONSTER_DIR "mandragora_atk.spr";
            act = MONSTER_DIR "mandragora_atk.act";
        }
    } else if (job == 0x75e || job == 0x75c) {
        spr = MONSTER_DIR "MAVKA_BULLET.spr";
        act = MONSTER_DIR "MAVKA_BULLET.act";
    } else if (job == 0x6f5) {
        spr = MONSTER_DIR "drosera_bullet.spr";
        act = MONSTER_DIR "drosera_bullet.act";
    } else if (job == 0x6ad || job == 0x6a8) {
        spr = MONSTER_DIR "tha_odium_ef.spr";
        act = MONSTER_DIR "tha_odium_ef.act";
    } else if (job == JOB_HYDRA) {
        spr = MONSTER_DIR "hydra_atk.spr";
        act = MONSTER_DIR "hydra_atk.act";
    } else {
        spr = MONSTER_DIR "mandragora_atk.spr";
        act = MONSTER_DIR "mandragora_atk.act";
    }
    render_object_load_spr_res(&self->base, spr);
    render_object_load_act_res(&self->base, act);

    target.x = 0.0f;
    target.y = 0.0f;
    target.z = 0.0f;
    render_object_get_actor_pos(&self->base, (int)par1, &target);
    self->base.pos = target;
    self->base.visible = 1;
    self->base.start_tick = timeGetTime();
    render_object_set_motion_delay(&self->base, act_res_get_delay(render_object_get_act_res(&self->base), 0));
    if (job == JOB_HYDRA)
        render_object_set_motion_delay(&self->base, 2.0f);
    if (par2)
        self->start_delay = *(const float *)MSG_PTR(par2);
}
