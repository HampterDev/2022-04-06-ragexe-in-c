#include "render/projection.h"
#include "render/rp_pool.h"
#include "world/ground_api.h"
#include "core/memory.h"
#include <string.h>
#include "effects/particle.h"
#include "effects/effect_math.h"
#include "game/context.h"

static const float k_multitexDepthNudge = 3.0517578e-05f;

struct MultiTexParticle *multi_tex_particle_construct(struct MultiTexParticle *self, Vec3 pos, int list_flags)
{
    particle_construct(&self->base, pos, list_flags);
    self->base.pool_kind = PARTICLE_KIND_MULTITEX;
    particle_reset(&self->base);
    self->texture0 = 0;
    self->texture1 = 0;
    self->u_scroll = 0;
    self->v_scroll = 0;
    return self;
}

void multi_tex_particle_reset(struct MultiTexParticle *self)
{
    particle_reset(&self->base);
    self->texture0 = 0;
    self->texture1 = 0;
    self->u_scroll = 0;
    self->v_scroll = 0;
}

void multi_tex_particle_render_on_cell(struct MultiTexParticle *self, int cell_x, int cell_y)
{
    struct C3dGround15 *ground = game_current_ground();
    float corners[12] = {0};
    float view[12];
    struct RPTwoTexQuad *quad;
    int i;

    if (!ground || !game_copy_view_matrix(view) || !c3d_ground15_get_attr_quad_half(ground, cell_x, cell_y, corners))
        return;
    quad = renderer_new_two_tex_quad(g_renderer);
    quad->prim.tex0 = self->texture0;
    quad->prim.tex1 = self->texture1;
    for (i = 0; i < 4; i++) {
        struct RPTwoTexVertex *v = &quad->quad[i];
        Vec3 corner = {corners[i * 3], corners[i * 3 + 1], corners[i * 3 + 2]};

        renderer_project_multitex_vertex(g_renderer, &corner, view, v);
        v->z -= k_multitexDepthNudge;
        v->diffuse = 0xffffffff;
        v->u0 = self->u_scroll + self->uv0[i].u;
        v->v0 = self->v_scroll + self->uv0[i].v;
        v->u1 = self->uv1[i].u;
        v->v1 = self->uv1[i].v;
    }
    renderer_add_multi_tex_batch(g_renderer, &quad->prim);
}
