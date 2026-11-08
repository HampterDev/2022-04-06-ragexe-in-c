#include "render/renderer.h"
#ifndef RENDER_RP_POOL_H
#define RENDER_RP_POOL_H

#include "render/primitive_types.h"

struct Renderer;

struct RPQuad *rp_quad_construct(struct RPQuad *self);
struct RPTwoTexQuad *rp_two_tex_quad_construct(struct RPTwoTexQuad *self);
struct TexDrawPrim *rp_quad_batch_construct(struct TexDrawPrim *self);
struct TexDrawPrim *rp_quad_batch_copy_verts(struct TexDrawPrim *self, const struct TexDrawPrim *src);
struct TexDrawVertex *tex_draw_vertex_construct(struct TexDrawVertex *self);

/* Called first by renderer_destroy_all_rp_list, before the render lists are cleared,
   so that the effect manager destroys the effects removed during the frame.
   client_install_hooks() (core/client_init.c) installs effect_mgr_destroy_removed_effects
   (effects/effect_render_hooks.c). NULL: nothing is called. Defined in render/rp_pool.c. */
extern void (*g_renderBeforeClearLists)(void);

struct TexDrawPrim *renderer_new_prim(struct Renderer *self);
struct RPTwoTexQuad *renderer_new_two_tex_quad(struct Renderer *self);
struct RPQuad *renderer_new_quad(struct Renderer *self);
struct TexDrawVertex *renderer_new_tex_draw_vertices(struct Renderer *self, int count);
void renderer_add_prim(struct Renderer *self, struct RPQuad *prim, int flags);
void renderer_add_two_tex_prim(struct Renderer *self, struct RPTwoTex *prim, int which);
void renderer_add_multi_tex_batch(struct Renderer *self, struct RPTwoTex *prim);
void renderer_add_triangle(struct Renderer *self, struct PrimTriangle *tri, int flags);
/* custom: queues `count` indices from `first` of a static world mesh (render/gfx.h), drawn with the
   view matrix `view` and the two textures where the ground's two-texture quads are drawn */
void renderer_add_mesh_draw(struct Renderer *self, struct gfx_mesh *mesh, int first, int count,
                            const float *view, struct Texture *texture0, struct Texture *texture1);
void rp_quad_set_vertex_pos(struct RPQuad *prim, int index, const struct TexDrawVertex *vertex);
DWORD rp_fog_blend(DWORD argb);
int rp_vertex_fog_alpha(float rhw);

void renderer_destroy_all_rp_list(struct Renderer *self);

#endif
