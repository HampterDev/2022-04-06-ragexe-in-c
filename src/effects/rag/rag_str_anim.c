#include "actor/render_ops.h"
#include "actor/object_ops.h"
#include "render/projection.h"
#include "render/math3d.h"
#include "render/rp_pool.h"
#include "resources/ezeffect.h"
#include "fileio/resource_api.h"
#include "effects/durations.h"
#include "effects/effect_data.h"
#include "session/state.h"
#include <stdlib.h>
#include <string.h>
#include "effects/rag/rag_effect.h"
#include "render/render_api.h"
#include "session/session_state.h"
#include "effects/rag/rag_str_anim.h"

const double k_strAnglePerDegree = 2.8444444444444;

static int roll(int n)
{
    int r = rand() % n;
    return (r < 0 ? 0 : r) + 1;
}

void rag_effect_init(struct RagEffect *self, struct RenderObject *master, int effect_id,
                                 Vec3 offset)
{
    const struct RagStrBinding *binding;
    char file_name[64];
    const char *name = NULL;
    int variant = 0;

    self->effect_id = effect_id;
    self->master = master;
    self->base.pos = master->pos;
    self->master_roty = master->roty;
    rag_effect_set_duration(self, effect_duration(self->effect_id));
    self->offset = offset;
    sprintf(file_name, "");

    binding = effect_data_rag_str(self->effect_id);
    if (binding && binding->file) {
        if (binding->variants)
            variant = roll(binding->variants);
        name = binding->min_file && g_session1.map_rule_flags[0] != 0 ? binding->min_file : binding->file;
        if (binding->loops >= 0)
            self->str_max_loops = binding->loops;
    }
    if (name) {
        if (variant)
            snprintf(file_name, sizeof file_name, name, variant);
        else
            snprintf(file_name, sizeof file_name, "%s", name);
    }

    if (strlen(file_name) == 0)
        return;
    rag_str_load(self, file_name);
}

static void render_layer(struct RagEffect *self, struct StrLayer *layer, const struct StrKeyState *s, const float *view)
{
    struct TexDrawPrim *batch;
    struct TexDrawVertex *q;
    Vec3 pos = self->master->pos;
    float sx, sy, rhw, scale, rad, sin_a, cos_a, u_scale, v_scale;
    struct Texture *texture;
    unsigned color;
    int tex_frame, i;

    renderer_project_point(g_renderer, &pos, view, &sx, &sy, &rhw);
    scale = renderer_screen_scale(g_renderer, rhw);
    batch = renderer_new_prim(g_renderer);
    q = batch->base.verts;

    if (s->corner_x[1] > s->corner_x[0]) {
        q[0].x = s->corner_x[3]; q[1].x = s->corner_x[0]; q[2].x = s->corner_x[2]; q[3].x = s->corner_x[1];
        q[1].u = q[0].u = s->u;
        q[3].u = q[2].u = s->us + s->u;
    } else {
        q[0].x = s->corner_x[2]; q[1].x = s->corner_x[1]; q[2].x = s->corner_x[3]; q[3].x = s->corner_x[0];
        q[1].u = q[0].u = s->us + s->u;
        q[3].u = q[2].u = s->u;
    }
    if (s->corner_y[2] > s->corner_y[1]) {
        q[0].y = s->corner_y[3]; q[1].y = s->corner_y[0]; q[2].y = s->corner_y[2]; q[3].y = s->corner_y[1];
        q[3].v = q[1].v = s->v;
        q[2].v = q[0].v = s->vs + s->v;
    } else {
        q[0].y = s->corner_y[0]; q[1].y = s->corner_y[3]; q[2].y = s->corner_y[1]; q[3].y = s->corner_y[2];
        q[2].v = q[0].v = s->v;
        q[3].v = q[1].v = s->vs + s->v;
    }

    tex_frame = (int)s->aniframe;
    if (tex_frame <= 0)
        tex_frame = 0;
    texture = str_layer_get_texture(layer, tex_frame);
    u_scale = (float)(unsigned)texture->reqWidth / (float)(unsigned)texture->base.width;
    v_scale = (float)(unsigned)texture->reqHeight / (float)(unsigned)texture->base.height;
    for (i = 0; i < 4; i++) {
        q[i].u = q[i].u * u_scale;
        q[i].v = v_scale * q[i].v;
    }

    rad = (float)(s->angle / k_strAnglePerDegree) / g_halfTurnDegrees * g_pi;
    sin_a = float_sin(rad);
    rad = (float)(s->angle / k_strAnglePerDegree) / g_halfTurnDegrees * g_pi;
    cos_a = float_cos(rad);
    for (i = 0; i < 4; i++) {
        float x = q[i].x, y = q[i].y, turned_y, d;
        turned_y = y * cos_a + x * sin_a;
        q[i].x = ((x * cos_a - y * sin_a) + (s->x - STR_SCREEN_CX)) * scale + self->screen_pos.x;
        d = ((s->y - STR_SCREEN_CY) + turned_y) * scale;
        if (session_is_screen_flipped(&g_session1))
            q[i].y = (self->screen_pos.y - d) + (sy - self->screen_pos.y + (sy - self->screen_pos.y));
        else
            q[i].y = d + self->screen_pos.y;
    }

    color = (unsigned)(int)s->color[3];
    color = (color << 8) + (unsigned char)(int)s->color[0];
    color = (color << 8) + (unsigned char)(int)s->color[1];
    color = (color << 8) + (unsigned char)(int)s->color[2];
    for (i = 0; i < 4; i++) {
        q[i].diffuse = color;
        q[i].specular = 0xffffffff;
        q[i].z = 3.102121991105378e-05f;
        q[i].rhw = rhw;
    }

    batch->base.texture = texture;
    batch->base.srcBlend = s->src_blend;
    batch->base.destBlend = s->dest_blend;
    batch->base.stageMode = s->mtpreset;
    renderer_add_prim(g_renderer, &batch->base, 5);
}

void rag_effect_render(struct GameObject *object, const float *view)
{
    struct RagEffect *self = (struct RagEffect *)object;

    int i, depth = 0;

    if (g_session1.effectOn == 0 && self->always_draw == 0)
        return;
    if (!self->str_anim || !self->master)
        return;
    for (i = 1; i < self->str_anim->layer_count; i++) {
        if (self->str_drawn[i]) {
            render_layer(self, &self->str_anim->layers[i], &self->str_state[i], view);
            depth += 2;
        }
    }
}
