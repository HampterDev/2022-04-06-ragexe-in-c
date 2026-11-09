#include "render/rp_pool.h"
#include "world/model.h"
#include <stdlib.h>

struct Renderer *g_renderer;
struct Texture g_whiteTexture;
void fatal_error(const char *message);
void c3d_node_light(struct C3dNode *node, const float *matrix, const float *direction, const float *diffuse, const float *ambient, int shade) { abort(); }
void c3d_node_v1_render(struct C3dNode *node, const float *matrix, BYTE mirrored, BYTE layer) { abort(); }
void c3d_node_v2_render(struct C3dNode *node, const float *matrix, BYTE mirrored, BYTE layer) { abort(); }
void c3d_node_v1_cast(struct C3dNode *node, const float *matrix) { abort(); }
void c3d_node_v2_cast(struct C3dNode *node, const float *matrix) { abort(); }
void renderer_add_prim(struct Renderer *renderer, struct RPQuad *prim, int queue) { abort(); }
struct TexDrawPrim *renderer_new_prim(struct Renderer *renderer) { abort(); }
void renderer_project_vertex(struct Renderer *renderer, const Vec3 *point, const float *matrix, struct TexDrawVertex *out) { abort(); }
