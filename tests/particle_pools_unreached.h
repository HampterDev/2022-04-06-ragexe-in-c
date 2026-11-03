#include "render/device.h"
struct Renderer *g_renderer;
struct TexCache g_textureCache;
struct EffectMgr *g_effectMgr;
float g_fogNear, g_fogFar;
void gfx_addref(gfx_surface *surface) { abort(); }
void gfx_release(gfx_surface *surface) { abort(); }
struct C3dDevice g_3dDevice;
void abstract_method_called(void) { abort(); }
struct C3dDevice;
struct C3dGround15;
struct ActRes;
struct Ez2StrParticle;
struct EffectMgr;
struct Motion;
struct Renderer;
struct Res;
struct ResMgr;
struct SprClip;
struct Texture;
struct TexCache;
struct RPTwoTexVertex;
struct SprImg;
struct SpriteRegion;
struct TexDrawVertex;
void c3d_device_save_frame(struct C3dDevice *self) { abort(); }
BYTE c3d_ground15_get_attr_quad_half(struct C3dGround15 *self, int ax, int ay, float *quad) { abort(); }
struct Motion *act_res_get_motion(const struct ActRes *self, unsigned action, unsigned motion) { abort(); }
unsigned char ez2_str_particle_process(struct Ez2StrParticle *self, Vec3 offset) { abort(); }
struct SprClip *motion_get_clip(const struct Motion *self, unsigned index) { abort(); }
void *res_mgr_get(struct ResMgr *self, const char *name) { abort(); }
void res_lock(struct Res *self) { abort(); }
struct Texture *texture_cache_get(struct TexCache *cache, const char *name, int keyed, int full_size) { abort(); }
unsigned actres_frame_count(const struct ActRes *self, unsigned action) { abort(); }
__declspec(noreturn) void fatal_error(const char *message) { abort(); }
struct ResMgr *g_resMgr(void) { abort(); }
float game_camera_heading(void) { abort(); }
float game_camera_pitch(void) { abort(); }
int game_copy_view_matrix(float matrix[12]) { abort(); }
struct C3dGround15 *game_current_ground(void) { abort(); }
float renderer_project_depth(float rhw) { abort(); }
void renderer_project_multitex_vertex(struct Renderer *renderer, const Vec3 *point,
    const float *view, struct RPTwoTexVertex *vertex) { abort(); }
void renderer_project_point(struct Renderer *renderer, const Vec3 *point,
    const float *view, float *x, float *y, float *rhw) { abort(); }
void renderer_project_vertex(struct Renderer *renderer, const Vec3 *point,
    const float *view, struct TexDrawVertex *vertex) { abort(); }
float renderer_screen_scale(struct Renderer *renderer, float rhw) { abort(); }
void sprite_atlas_destroy(struct Renderer *renderer) { abort(); }
void sprite_atlas_finish_frame(struct Renderer *renderer) { abort(); }
void sprite_atlas_initialize(struct Renderer *renderer) { abort(); }
struct Texture *sprite_atlas_resolve(struct Renderer *renderer,
    const struct SprImg *image, const DWORD *palette, struct SpriteRegion *region) { abort(); }
