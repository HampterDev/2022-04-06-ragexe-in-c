#include "actor/object_ops.h"
#include "actor/render_ops.h"
#include "resources/sprite.h"
#include "resources/animation.h"
#include "fileio/resource_api.h"
#include <string.h>
#include "effects/rag/rag_effect.h"
#include "actor/game_actor.h"

struct GameObject;
struct RenderObject;

static const struct RenderObjectOps rag_effect_ops = {
    .base = {
        .destroy = rag_effect_delete,
        .on_process = rag_effect_on_process,
        .send_msg = render_object_send_msg,
        .render = rag_effect_render,
        .get_direction = game_object_dir_of,
    },
    .render_body = game_object_set_render_info,
    .set_action = render_object_set_action_base,
    .update_motion = render_object_update_motion_base,
    .get_effect_state = render_object_return_zero,
};

struct RagEffect *rag_effect_construct(struct RagEffect *self)
{
    int i;

    render_object_construct(&self->base);
    self->base.ops = &rag_effect_ops.base;
    memset(&self->offset, 0, sizeof(self->offset));
    memset(&self->offset2, 0, sizeof(self->offset2));
    self->head_offset.x = 0;
    self->head_offset.y = 0;
    self->head_offset.z = 0;
    list_init(&self->prims);
    for (i = 0; i < PRIM_TRIANGLES; i++)
        prim_triangle_construct(&self->triangles[i]);

    self->master = NULL;
    self->hit_count = 0;
    self->frame = 0;
    self->effect_id = 0;
    self->repeat_count = 1;
    self->id_state = 0;
    self->has_str_offset = 0;
    self->aux_flag = 0;
    memset(&self->offset, 0, sizeof(self->offset));
    memset(&self->offset2, 0, sizeof(self->offset2));
    self->head_offset.x = 0;
    self->head_offset.y = 0;
    self->head_offset.z = 0;
    self->str_res = NULL;
    self->str_anim = NULL;
    self->str_layers_done = 0;
    memset(self->str_key_index, 0, sizeof(self->str_key_index));
    memset(self->triangles, 0, sizeof(self->triangles));
    self->color_r = -1;
    self->color_g = -1;
    self->color_b = -1;
    self->dest_blend = -1;
    self->str_loops = 0;
    self->str_max_loops = 0;
    self->always_draw = 0;
    self->last_prim = NULL;
    self->show_id = 0;
    return self;
}

static void release_prims(struct RagEffect *self)
{
    struct ListNode *n;

    for (n = self->prims.head->next; n != self->prims.head; n = n->next)
        effect_prim_send_msg(*LIST_VALUE(n, struct EffectPrim *), PRIM_MSG_MASTER_GONE);
    list_clear(&self->prims, NULL);
    self->last_prim = NULL;
}

void rag_effect_destruct(struct RagEffect *self)
{
    self->base.ops = &rag_effect_ops.base;
    if (self->prims.size != 0)
        release_prims(self);
    if (self->master)
        render_object_send(self->master, &self->base, 0x17, 0, 0, 0, 0);
    rag_str_unload(self);
    list_free(&self->prims, NULL);
    render_object_dc_render_object(&self->base);
}

void rag_effect_delete(struct GameObject *object)
{
    struct RagEffect *self = (struct RagEffect *)object;

    rag_effect_destruct(self);
    mem_free(self);
}

int rag_effect_get_duration(struct RagEffect *self)
{
    return self->duration;
}

static void msg_2c(struct RagEffect *self, RenderMsgArg par1, RenderMsgArg par2, RenderMsgArg par3)
{
    Vec3 *v = &self->base.pos;

    if (par1 == 0xffff) {
        if (par2 == 0x82 || par2 == 0xfa) {
            v->x = 200.0f;
            v->y = 200.0f;
            v->z = 0.0f;
        } else if (par2 == 0x80) {
            v->x = 250.0f;
            v->y = 150.0f;
            v->z = 150.0f;
        } else if (par2 == 0xa9) {
            v->x = 180.0f;
            v->y = 180.0f;
            v->z = 250.0f;
        } else if (par2 == 0xf3) {
            v->x = 250.0f;
            v->y = 250.0f;
            v->z = 250.0f;
            self->duration = (int)par3;
        } else if (par2 == 0x318 || par2 == 0x39d || par2 == 0x338) {
            v->x = 250.0f;
            v->y = 250.0f;
            v->z = 250.0f;
        } else if (par2 == 0x422) {
            self->offset = *(const Vec3 *)MSG_PTR(par3);
            self->duration = 20;
        }
    } else if (par1 == 0xfffe) {
        if (par2 == 0x80) {
            v->x = -1.0f;
            v->y = -1.0f;
            v->z = -1.0f;
        } else if (par2 == 0x82) {
            v->x = -2.0f;
            v->y = -2.0f;
            v->z = -2.0f;
        }
    } else {
        self->hit_count = (int)par1;
        if (self->hit_count > 50)
            self->hit_count = 50;
    }
}

void rag_effect_send_msg(struct RagEffect *self, struct RenderObject *sender,
                                    RenderMsgArg msg, RenderMsgArg par1, RenderMsgArg par2, RenderMsgArg par3)
{
    struct ListNode *n;

    if (msg < 0x0e || msg > 0x8f)
        return;
    switch ((int)msg) {
    case RAG_MSG_SET_OFFSETS:
        if (par1 != 0)
            self->offset = *(const Vec3 *)MSG_PTR(par1);
        if (par2 != 0)
            self->offset2 = *(const Vec3 *)MSG_PTR(par2);
        break;
    case RAG_MSG_MASTER_GONE:
        self->master = NULL;
        break;
    case RAG_MSG_PRIM_GONE:
        if (par1 == 0) {
            if (!sender)
                return;
            for (n = self->prims.head->next; n != self->prims.head; n = n->next) {
                struct EffectPrim *prim = *LIST_VALUE(n, struct EffectPrim *);

                if (&prim->base == sender) {
                    if (self->last_prim == prim)
                        self->last_prim = NULL;
                    list_erase(&self->prims, n, NULL);
                    return;
                }
            }
        } else {
            release_prims(self);
        }
        break;
    case 0x2c:
        msg_2c(self, par1, par2, par3);
        break;
    case 0x2e:
        memcpy(self->msg_params, MSG_PTR(par1), sizeof(self->msg_params));
        break;
    case RAG_MSG_SET_DURATION:
        rag_effect_set_duration(self, (int)par1);
        break;
    case RAG_MSG_SET_COLOR:
        self->color_r = (int)par1;
        self->color_g = (int)par2;
        self->color_b = (int)par3;
        break;
    case 0x8f:
        if (self->last_prim && par1 >= 0x2df && par1 <= 0x2e2)
            self->last_prim->stage = (int)par2;
        break;
    default:
        break;
    }
}

void rag_effect_set_duration(struct RagEffect *self, int frame)
{
    self->duration = frame;
}
