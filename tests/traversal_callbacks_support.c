#include "render/rp_pool.h"
#include "world/model.h"
#include "skills/world_video.h"
#include <stdlib.h>

struct Renderer *g_renderer;
struct Texture g_whiteTexture;

struct WorldVideoList *game_current_video_list(void) { abort(); }
struct WorldVideo *world_video_list_open(struct WorldVideoList *list, const char *name, int loop) { abort(); }
int world_video_add_position(struct WorldVideo *video, Vec3 position) { abort(); }
void error_msg(const char *message, int flags) { abort(); }

struct C3dNode;
struct C3dNodeV1;
struct Renderer;
struct TexDrawVertex;
struct RPQuad;
struct List;
struct ListNode;
struct String;
struct Tree;
struct TreeNode;
struct Vec;

void c3d_node_v1_render(struct C3dNode *node, const float *matrix, BYTE mirrored, BYTE clip, BYTE layer) { abort(); }
void c3d_node_v2_render(struct C3dNode *node, const float *matrix, BYTE mirrored, BYTE clip, BYTE layer) { abort(); }
void c3d_node_light(struct C3dNode * arg0, const float * arg1, const float * arg2, const float * arg3, const float * arg4, int arg5) { abort(); }
void renderer_add_prim(struct Renderer * arg0, struct RPQuad * arg1, int arg2) { abort(); }
struct TexDrawPrim * renderer_new_prim(struct Renderer * arg0) { abort(); }
void renderer_project_vertex(struct Renderer * arg0, const Vec3 * arg1, const float * arg2, struct TexDrawVertex * arg3) { abort(); }
void string_copy(struct String *arg0, const struct String *arg1) { abort(); }
void string_free(struct String *arg0) { abort(); }
int string_compare(const struct String *arg0, const struct String *arg1) { abort(); }
