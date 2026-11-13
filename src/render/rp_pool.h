#include "render/renderer.h"
#ifndef RENDER_RP_POOL_H
#define RENDER_RP_POOL_H

#include "render/primitive_types.h"

struct Renderer;
struct SkinPose;

struct RPQuad *rp_quad_construct(struct RPQuad *self);
struct TexDrawPrim *rp_quad_batch_construct(struct TexDrawPrim *self);
/* custom: a quad whose instance, cleared, is drawn in place of its vertices (render/gfx.h,
   gfx_draw_quad) */
struct TexDrawPrim *rp_instance_construct(struct TexDrawPrim *self);

/* Called first by renderer_destroy_all_rp_list, before the render lists are cleared,
   so that the effect manager destroys the effects removed during the frame.
   client_install_hooks() (core/client_init.c) installs effect_mgr_destroy_removed_effects
   (effects/effect_render_hooks.c). NULL: nothing is called. Defined in render/rp_pool.c. */
extern void (*g_renderBeforeClearLists)(void);

/* The frame's next quad of four vertices. Its header is as the frame's quad of the same place in
   an earlier frame left it, whoever that was (sprites set alpha_mode, STR effects stageMode, most
   users srcBlend and destBlend), as in the original; custom: only instance is NULL. */
struct TexDrawPrim *renderer_new_prim(struct Renderer *self);
/* custom: the frame's next quad of the same pool, as renderer_new_prim, for an instance, cleared, in
   place of its vertices (render/gfx.h, gfx_draw_quad), with the rhw of its first two corners in
   base.rhw for the lists' order: the sprites and sprite particles whose vertices the device makes
   take the quads they took when the CPU made them, and with them the header those left. */
struct TexDrawPrim *renderer_new_prim_instance(struct Renderer *self);
/* custom: the frame's next quad of the multitexture list's own pool (the original's two-texture
   quads), for an instance, cleared */
struct TexDrawPrim *renderer_new_multi_tex_quad(struct Renderer *self);
/* custom: the frame's next triangle of the world as an instance, cleared, from a pool of its own
   (effects/rag/effect_prim_draw.c): the fields of its header the triangle does not set are
   rp_instance_construct's, as the triangles of vertices found them in renderer_new_quad's pool; and
   the frame's next set of corners its triangles share (render/quads.h), cleared, of no row yet */
struct TexDrawPrim *renderer_new_triangle_instance(struct Renderer *self);
struct GfxCornerSet *renderer_new_corner_set(struct Renderer *self);
struct RPQuad *renderer_new_quad(struct Renderer *self);
struct TexDrawVertex *renderer_new_tex_draw_vertices(struct Renderer *self, int count);
void renderer_add_prim(struct Renderer *self, struct RPQuad *prim, int flags);
/* custom: a quad of the multitexture list, its texture and texture1 read by the two stages */
void renderer_add_multi_tex_batch(struct Renderer *self, struct RPQuad *prim, struct Texture *texture1);
void renderer_add_triangle(struct Renderer *self, struct PrimTriangle *tri, int flags);
/* custom: queues a draw of the static world (render/gfx.h, struct RPWorldDraw): a record or a
   cell of water into an alpha queue (RENDER_ALPHA_CLAMP, RENDER_ALPHA_WRAP, RENDER_ALPHA_WRITE_DEPTH),
   drawn among its quads with their states, sorted by `key` (the rhw of its first corner, as
   theirs), the water with `texture`; a caster into RENDER_SHADOW_CASTERS, drawn into the shadow
   maps alone. */
void renderer_add_world_draw(struct Renderer *self, int queue, const struct RPWorldDraw *draw, struct Texture *texture,
                             float key);
/* custom: a Granny actor's pose for the frame (render/skin_layout.h), packed into the frame's
   storage with the renderer's projection (its proj and centre, RENDER_NEAR_Z and
   RENDER_DEPTH_SCALE, renderer_project_vertex's) in place of the pose's; NULL for a pose of more
   bones than the device takes. It lasts until renderer_destroy_all_rp_list. */
struct RPSkinPose *renderer_skin_pose(struct Renderer *self, const struct SkinPose *pose);
/* custom: `count` of a Granny model's indices from `first` (one of its meshes) drawn in a pose with
   `texture`, queued where Granny's triangles went before, the opaque list with clamped texture
   coordinates (renderer_add_prim's flags 0), sorted among its quads by the texture; the queue holds
   the model until it is cleared. Nothing without a pose, model or texture. */
void renderer_add_skin(struct Renderer *self, struct RPSkinPose *pose, gfx_skin *model, uint32_t first,
                       uint32_t count, struct Texture *texture);
/* custom: room for `count` cells of water in the renderer's buffer of a run of them
   (Renderer.waterCells), NULL without memory */
uint32_t *renderer_water_cells(struct Renderer *self, size_t count);
DWORD rp_fog_blend(DWORD argb);

void renderer_destroy_all_rp_list(struct Renderer *self);

#endif
