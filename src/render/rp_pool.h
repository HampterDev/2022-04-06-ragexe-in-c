#include "render/renderer.h"
#ifndef RENDER_RP_POOL_H
#define RENDER_RP_POOL_H

#include "render/primitive_types.h"

struct Renderer;

struct RPQuad *rp_quad_construct(struct RPQuad *self);
struct TexDrawPrim *rp_quad_batch_construct(struct TexDrawPrim *self);
/* custom: a quad whose instance, cleared, is drawn in place of its vertices (render/gfx.h,
   gfx_draw_quad) */
struct TexDrawPrim *rp_instance_construct(struct TexDrawPrim *self);
struct TexDrawVertex *tex_draw_vertex_construct(struct TexDrawVertex *self);

/* Called first by renderer_destroy_all_rp_list, before the render lists are cleared,
   so that the effect manager destroys the effects removed during the frame.
   client_install_hooks() (core/client_init.c) installs effect_mgr_destroy_removed_effects
   (effects/effect_render_hooks.c). NULL: nothing is called. Defined in render/rp_pool.c. */
extern void (*g_renderBeforeClearLists)(void);

/* The frame's next quad of four vertices. Its header is as the frame's quad of the same place in
   an earlier frame left it, whoever that was (sprites set alpha_mode, STR effects stageMode, most
   users srcBlend and destBlend), as in the original; only world is 0 and, custom, instance NULL. */
struct TexDrawPrim *renderer_new_prim(struct Renderer *self);
/* custom: the frame's next quad of the same pool, as renderer_new_prim, for an instance, cleared, in
   place of its vertices (render/gfx.h, gfx_draw_quad), with the rhw of its first two corners in
   base.rhw for the lists' order: the sprites and sprite particles whose vertices the device makes
   take the quads they took when the CPU made them, and with them the header those left. */
struct TexDrawPrim *renderer_new_prim_instance(struct Renderer *self);
/* custom: the frame's next quad of the multitexture list's own pool (the original's two-texture
   quads), for an instance, cleared */
struct TexDrawPrim *renderer_new_multi_tex_quad(struct Renderer *self);
struct RPQuad *renderer_new_quad(struct Renderer *self);
struct TexDrawVertex *renderer_new_tex_draw_vertices(struct Renderer *self, int count);
void renderer_add_prim(struct Renderer *self, struct RPQuad *prim, int flags);
/* custom: a quad of the multitexture list, its texture and texture1 read by the two stages */
void renderer_add_multi_tex_batch(struct Renderer *self, struct RPQuad *prim, struct Texture *texture1);
void renderer_add_triangle(struct Renderer *self, struct PrimTriangle *tri, int flags);
/* custom: queues a draw of a static mesh (render/gfx.h) with its two textures. In the queue that
   renderer_mesh_pass gives for the flags a model's quads would have, the draw takes their place,
   drawn among them with their states and, where that queue sorts by depth, by `key` (the rhw of
   its first vertex, as theirs); RENDER_GROUND_MESH is drawn where the ground's two-texture quads
   are, with their states, and casts nothing; RENDER_SHADOW_CASTERS only into the shadow maps, its
   first texture, where there is one, tested for alpha. */
int renderer_mesh_pass(int flags);
void renderer_add_mesh_draw(struct Renderer *self, int pass, const struct RPMeshDraw *draw, struct Texture *texture0,
                            struct Texture *texture1, float key);
void rp_quad_set_vertex_pos(struct RPQuad *prim, int index, const struct TexDrawVertex *vertex);
DWORD rp_fog_blend(DWORD argb);

void renderer_destroy_all_rp_list(struct Renderer *self);

#endif
