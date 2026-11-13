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

/* custom: the cell's half quad as an instance whose corners the device projects as
   renderer_project_vertex did them through the view (render/gfx.h, GFX_QUAD_WORLD), z nudged
   towards the camera, with each stage's texture coordinates; the CPU finds the corners' rhw alone,
   which say whether the quad lies flat on the screen. */
void multi_tex_particle_render_on_cell(struct MultiTexParticle *self, int cell_x, int cell_y)
{
    struct C3dGround15 *ground = game_current_ground();
    float corners[12] = {0};
    float view[12], rhw[4];
    struct TexDrawPrim *quad;
    struct GfxQuad *q;
    int i;

    if (!ground || !game_copy_view_matrix(view) || !c3d_ground15_get_attr_quad_half(ground, cell_x, cell_y, corners))
        return;
    quad = renderer_new_multi_tex_quad(g_renderer);
    quad->base.texture = self->texture0;
    q = &quad->instance;
    q->kind = GFX_QUAD_WORLD;
    for (i = 0; i < 4; i++) {
        Vec3 corner = {corners[i * 3], corners[i * 3 + 1], corners[i * 3 + 2]};

        q->world.point[i][0] = corner.x;
        q->world.point[i][1] = corner.y;
        q->world.point[i][2] = corner.z;
        rhw[i] = renderer_project_rhw(&corner, view);
        q->world.u[i] = self->u_scroll + self->uv0[i].u;
        q->world.v[i] = self->v_scroll + self->uv0[i].v;
        q->world.u1[i] = self->uv1[i].u;
        q->world.v1[i] = self->uv1[i].v;
    }
    memcpy(q->world.view, view, sizeof q->world.view);
    q->world.proj[0] = g_renderer->proj[0];
    q->world.proj[1] = g_renderer->proj[1];
    q->world.centre[0] = (float)g_renderer->centreX;
    q->world.centre[1] = (float)g_renderer->centreY;
    q->world.rhw = GFX_RHW_PROJECTED;
    q->world.depth = GFX_DEPTH_ADD;
    q->world.depthValue = -k_multitexDepthNudge;
    q->world.colour = 0xffffffff;
    q->world.stage2 = 1;
    q->world.flat = quad_rhw_flat(rhw, 4);
    renderer_add_multi_tex_batch(g_renderer, &quad->base, self->texture1);
}
