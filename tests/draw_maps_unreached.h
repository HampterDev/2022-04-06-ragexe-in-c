struct ActRes;
struct GameActor;
struct ImfRes;
struct Motion;
struct Pc;
struct RenderObject;
struct Renderer;
struct ResMgr;
struct Session;
struct SprClip;
struct SprRes;
struct ClientLua;
struct ItemInfo;
struct ScreenBounds;
struct SqQuad;
struct List;
struct ListNode;
struct String;
struct Vec;
void bounds_set(struct ScreenBounds *arg0, float arg1, float arg2, float arg3, float arg4,
                float arg5)
{
    abort();
}
void renderer_project_point(struct Renderer *arg0, const Vec3 *arg1, const float *arg2,
                            float *arg3, float *arg4, float *arg5)
{
    abort();
}
float renderer_screen_scale(struct Renderer *arg0, float arg1)
{
    abort();
}
float renderer_scale_x(struct Renderer *arg0)
{
    abort();
}
struct Renderer *g_renderer;
int client_lua_call(struct ClientLua *arg0, const char *arg1, const char *arg2, ...)
{
    abort();
}
struct ItemInfo *item_info_copy(struct ItemInfo *arg0, const struct ItemInfo *arg1)
{
    abort();
}
struct Session g_session1;
int crt_snprintf_s(char *arg0, size_t arg1, size_t arg2, const char *arg3, ...)
{
    abort();
}
char *std_copy_memmove(const char *arg0, const char *arg1, char *arg2)
{
    abort();
}
size_t animation_action_count(const struct ActRes *arg0)
{
    abort();
}
struct Motion *act_res_get_motion(const struct ActRes *arg0, unsigned int arg1, unsigned int arg2)
{
    abort();
}
struct SprClip *motion_get_clip(const struct Motion *arg0, unsigned int arg1)
{
    abort();
}
const char *act_res_get_event_name(const struct ActRes *arg0, unsigned int arg1)
{
    abort();
}
unsigned int actres_frame_count(const struct ActRes *arg0, unsigned int arg1)
{
    abort();
}
int game_actor_get_job(struct GameActor *arg0)
{
    abort();
}
struct ResMgr *g_resMgr(void)
{
    abort();
}
void *res_mgr_get(struct ResMgr *arg0, const char *arg1)
{
    abort();
}
void *res_mgr_get_string(struct ResMgr *arg0, struct String arg1)
{
    abort();
}
BYTE session_is_doram_job(struct Session *arg0, int arg1)
{
    abort();
}
int game_actor_get_transform_job(struct GameActor *arg0)
{
    abort();
}
BYTE session_is_screen_flipped(struct Session *arg0)
{
    abort();
}
void render_object_submit_spr_quad(struct RenderObject *arg0, struct SqQuad *arg1, const void *arg2,
                                  int arg3, float arg4, float arg5)
{
    abort();
}
int imf_layer_at(const struct ImfRes *arg0, int arg1, int arg2, int arg3, int arg4)
{
    abort();
}
void bounds_add_quad(struct ScreenBounds *arg0, const struct SqQuad *arg1)
{
    abort();
}
void render_object_submit_flipped_quad(struct RenderObject *arg0, struct SqQuad *arg1, void *arg2,
                                     int arg3, float arg4, float arg5)
{
    abort();
}
void render_object_apply_pulse_scale(struct RenderObject *arg0, float *arg1)
{
    abort();
}
void render_object_swing_point(struct RenderObject *arg0, int arg1, int *arg2, int *arg3, int *arg4,
                              int *arg5)
{
    abort();
}
void render_object_draw_flashes(struct RenderObject *arg0, struct SqQuad arg1, void *arg2)
{
    abort();
}
struct SprRes *pc_layer_spr(struct Pc *arg0, int arg1)
{
    abort();
}
struct ActRes *pc_layer_act(struct Pc *arg0, int arg1)
{
    abort();
}
int layer_clip_index(int arg0)
{
    abort();
}
char action_is_ready_or_attack(int arg0)
{
    abort();
}
char layer_is_ignored_riding_state(int arg0, int arg1, int arg2, char arg3)
{
    abort();
}
int layer_body_attach_dy(struct Motion *arg0, int arg1, int arg2, int arg3, int arg4, int arg5)
{
    abort();
}
void pc_render_robe(struct Pc *arg0, int arg1, int *arg2, const float *arg3, float arg4,
                    float arg5, float arg6, float arg7)
{
    abort();
}
void pc_render_body_alone(struct Pc *arg0, int arg1, const float *arg2, float arg3, float arg4,
                          float arg5, float arg6, float arg7)
{
    abort();
}
struct Motion *pc_get_layer_motion(struct Pc *arg0, int arg1, struct ActRes *arg2, int *arg3,
                                   int *arg4, char *arg5)
{
    abort();
}
void pc_layer_depth_bias(int arg0, struct SqQuad *arg1, float arg2, float arg3)
{
    abort();
}
void pc_layer_depth_bias_by_view(struct Pc *arg0, int arg1, struct SqQuad *arg2, float arg3,
                              float arg4)
{
    abort();
}
void pc_draw_layer_overlays(struct Pc *arg0, struct SqQuad *arg1, struct SprClip *arg2, int arg3,
                           const int *arg4, float arg5, float arg6, float arg7, float arg8,
                           float arg9, float arg10, void *arg11, void *arg12, int *arg13, int arg14,
                           int arg15)
{
    abort();
}
float g_spriteScaleDivisor;
BYTE session_is_head_replacing_view(struct Session *arg0, int arg1)
{
    abort();
}
float session_get_baby_job_scale(struct Session *arg0, int arg1)
{
    abort();
}
void *session_get_stepped_motion(void *arg0, void *arg1, int arg2,
                                         int arg3, DWORD arg4, int *arg5)
{
    abort();
}
BYTE effect_state_bit11(DWORD arg0)
{
    abort();
}
void store_search_items_destroy(void *arg0, void *arg1)
{
    abort();
}
void store_search_item_copy(void *arg0, const void *arg1)
{
    abort();
}
unsigned char render_object_clip_to_quad(struct RenderObject *arg0, int arg1, int arg2, float arg3,
    const struct SprClip *arg4, float arg5, float arg6, const float *arg7, float arg8, float *arg9,
    float *arg10, const struct SprImg *arg11, struct SqQuad *arg12, int *arg13, unsigned *arg14,
    int arg15, int arg16, unsigned char arg17)
{
    abort();
}
void render_object_set_screen_position(struct RenderObject *arg0, float arg1, float arg2)
{
    abort();
}
void game_actor_set_render_info(struct RenderObject *arg0, const struct ScreenBounds *arg1, float arg2, float arg3)
{
    abort();
}
