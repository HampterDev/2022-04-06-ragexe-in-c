#include "render/rp_pool.h"
#include "session/state.h"
#include "effects/particle.h"
#include "effects/effect_math.h"
#include "render/render_api.h"
#include "actor/game_actor.h"
#include "session/session_state.h"
#include "effects/rag/rag_str_anim.h"

static void render_layer(struct Ez2StrParticle *self, struct StrLayer *layer, const struct StrKeyState *s,
                         const float *view, int layer_no)
{
    struct TexDrawPrim *batch = renderer_new_prim(g_renderer);
    struct TexDrawVertex *q = batch->base.verts;
    Vec3 pos;
    float screen_x, screen_y, rhw, rad, sin_a, cos_a, scale, u_scale, v_scale, depth;
    struct Texture *texture;
    int tex_frame, i;

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

    pos = self->base.pos;
    if (view) {
        renderer_project_point(g_renderer, &pos, view, &screen_x, &screen_y, &rhw);
        scale = renderer_screen_scale(g_renderer, rhw);
    } else {
        screen_x = self->screen_pos.x;
        screen_y = self->screen_pos.y;
        rhw = 1.0f;
        scale = 1.0f;
    }
    rad = (float)(s->angle / k_strAnglePerDegree) / g_halfTurnDegrees * g_pi;
    sin_a = float_sin(rad);
    rad = (float)(s->angle / k_strAnglePerDegree) / g_halfTurnDegrees * g_pi;
    cos_a = float_cos(rad);
    if (self->scale_by_actor == 1) {
        void *actor = effect_find_actor(self->actor_aid);
        if (actor)
            render_object_apply_shrink_scale(actor, &scale);
    }

    for (i = 0; i < 4; i++) {
        float x = q[i].x, y = q[i].y, turned_y, down;
        turned_y = y * cos_a + x * sin_a;
        q[i].x = ((s->x - STR_SCREEN_CX) + (x * cos_a - y * sin_a)) * scale + self->screen_pos.x;
        down = ((s->y - STR_SCREEN_CY) + turned_y) * scale;
        if (session_is_screen_flipped(&g_session1))
            q[i].y = (self->screen_pos.y - down) + (screen_y - self->screen_pos.y + (screen_y - self->screen_pos.y));
        else
            q[i].y = down + self->screen_pos.y;
    }

    for (i = 0; i < 3; i++) {
        q[i].x = (q[i].x - screen_x) * self->zoom + screen_x;
        q[i].y = (q[i].y - screen_y) * self->zoom + screen_y;
    }
    q[3].x = self->zoom * (q[3].x - screen_x) + screen_x;
    q[3].y = self->zoom * (q[3].y - screen_y) + screen_y;

    if (self->spin_degrees > 0.0f) {
        for (i = 0; i < 4; i++) {
            float spin[12], dx = q[i].x - screen_x, dy = q[i].y - screen_y, spun_dy;
            mat_set_rotate_z(spin, self->spin_degrees / g_halfTurnDegrees * g_pi);
            q[i].x = spin[0] * dx + spin[3] * dy + spin[6] * 0.0f + spin[9] + screen_x;
            spun_dy = spin[1] * dx + spin[4] * dy + spin[7] * 0.0f + spin[10];
            q[i].y = screen_y + spun_dy;
        }
    }

    for (i = 0; i < 4; i++) {
        q[i].x = q[i].x + self->screen_dx;
        q[i].y = self->screen_dy + q[i].y;
    }
    for (i = 0; i < 4; i++)
        q[i].y = q[i].y - (float)(double)self->lift;

    tex_frame = (int)s->aniframe;
    if (tex_frame <= 0)
        tex_frame = 0;
    if (self->texture_set.size == 0) {
        texture = str_layer_get_texture(layer, tex_frame);
    } else {
        texture = str_layer_get_set_texture(layer, tex_frame, string_cstr(&self->texture_set));
    }

    u_scale = (float)(unsigned)texture->reqWidth / (float)(unsigned)texture->base.width;
    v_scale = (float)(unsigned)texture->reqHeight / (float)(unsigned)texture->base.height;
    depth = (float)layer_no * 1e-5f;
    for (i = 0; i < 4; i++) {
        unsigned color;
        q[i].u = q[i].u * u_scale;
        q[i].v = q[i].v * v_scale;
        color = (unsigned)(int)(self->alpha_scale * s->color[3]);
        color = (color << 8) + (unsigned char)(int)s->color[0];
        color = (color << 8) + (unsigned char)(int)s->color[1];
        color = (color << 8) + (unsigned char)(int)s->color[2];
        q[i].specular = 0xffffffff;
        q[i].z = 3.102121991105378e-05f;
        q[i].diffuse = color;
        q[i].rhw = rhw - depth;
    }

    if (self->flip_v == 1) {
        float u = q[0].u, v = q[0].v;
        q[0].u = q[2].u; q[0].v = q[2].v; q[2].u = u; q[2].v = v;
        u = q[1].u; v = q[1].v;
        q[1].u = q[3].u; q[1].v = q[3].v; q[3].u = u; q[3].v = v;
    }
    if (self->flip_h == 1) {
        float u = q[0].u, v = q[0].v;
        q[0].u = q[1].u; q[0].v = q[1].v; q[1].u = u; q[1].v = v;
        u = q[2].u; v = q[2].v;
        q[2].u = q[3].u; q[2].v = q[3].v; q[3].u = u; q[3].v = v;
    }

    batch->base.texture = texture;
    batch->base.srcBlend = s->src_blend;
    batch->base.destBlend = s->dest_blend;
    batch->base.stageMode = s->mtpreset;
    self->base.list_flags |= 0x20;
    self->base.submit(batch, self->base.list_flags);
}

void ez2_str_particle_render(struct Ez2StrParticle *self, const float *view)
{
    int i;

    if (!self->anim || !self->visible || (!view && !ez2str_particle_is_ui(self)))
        return;
    for (i = 1; i < self->anim->layer_count; i++)
        if (self->drawn[i])
            render_layer(self, &self->anim->layers[i], &self->state[i], view, i);
}
