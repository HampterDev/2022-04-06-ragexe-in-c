#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#include <string.h>
#include <math.h>
#include <windows.h>
#include <intrin.h>
#include "oracle_image.h"
#include "session/state.h"
#include "effects/rag/rag_effect.h"
#include "effects/rag/rag_prim_setup.h"
#include "effects/durations.h"
#include "effects/effect_mgr.h"
#include "effects/effect.h"
#include "actor/game_actor.h"
#include "actor/actor_common.h"
#include "actor/render_object.h"
#include "core/object_type.h"
#include "contents/sound.h"
#include "game/context.h"
#include "game/mode_callbacks.h"
#include "game/gamemode.h"
#include "world/view_types.h"
#include "world/actor_lookup.h"
#include "render/texture_cache.h"
#include "render/render_api.h"
#include "render/projection.h"
#include "platform/error_message.h"
#include "session/session_jobs.h"
#include "session/session_skills.h"
#include "core/memory.h"
#include "core/fatal.h"
#include "actor/actor_ops.h"
#include "actor/job.h"
#include "game/own_states.h"
#include "skills/skill_info.h"
#include "contents/client_lua_calls.h"
#include "resources/animation.h"
#include "world/mode_dispatch.h"
#include "world/mode_manager.h"
#include "std/vec.h"
#include "actor/render_ops.h"
#include "actor/object_ops.h"
#include "actor/attr_state.h"
#include "resources/state_icon_sheet.h"
#include "replay/replay.h"
#include "render/renderer.h"
#include "world/ground_api.h"
#include "world/world_imports.h"
#include "session/party_queries.h"

#define ORIG_ON_PROCESS      0x6f85c0u
#define ORIG_RAG_VTABLE      0xd2a7f8u
#define ORIG_RAG_VTABLE_SLOTS 19
#define ORIG_PRIM_VTABLE     0xd2a590u
#define ORIG_PRIM_VTABLE_SLOTS 15
#define ORIG_LAUNCH_PRIM     0x6ea870u
#define ORIG_PLAY_WAVE       0x90e8a0u
#define ORIG_GET_GAME_MODE   0x6617e0u
#define ORIG_ATTR_HEIGHT     0x9f2780u
#define ORIG_LOAD_ACT        0x75cc50u
#define ORIG_LOAD_SPR        0x75fd60u
#define ORIG_TEXTURE_GET     0x4eb3e0u
#define ORIG_RES_MGR         0x5032b0u
#define ORIG_RES_GET         0x500d00u
#define ORIG_MESSAGE_BOX     0x661710u
#define ORIG_SET_ACTION_BASE 0x75ce00u
#define ORIG_PLAY_MOTION     0x75f0c0u
#define ORIG_SPAWN           0x96aca0u
#define ORIG_ACTOR_BY_AID    0x65c580u
#define ORIG_ACTOR_OR_PLAYER 0x65c540u
#define ORIG_PROJECT         0x4eb970u
#define ORIG_PROJECT_VERTEX  0x4ebb40u
#define ORIG_GET_JOB         0x74aea0u
#define ORIG_STR_REWIND      0x6ea670u
#define ORIG_RAG_VT_SEND_MSG (0x40 / 4)
#define ORIG_SKIP_FRAME      0x113bd44u
#define ORIG_CURRENT_MODE    0xe8e304u
#define ORIG_LUA_CALL        0x93dfc0u
#define ORIG_LAUNCH_EFFECT   0x74bf80u
#define ORIG_NEW             0xb461beu
#define ORIG_NEW_2           0xb46099u
#define ORIG_NEW_3           0xb46508u
#define ORIG_EFFECT_OPTION   0x1138cbcu
#define ORIG_DURATIONS       0xe907a0u
#define ORIG_RADIAN_TABLE    0xee39f0u
#define ORIG_NEG_RADIAN_TABLE 0xee3f94u
#define ORIG_SIN_TABLE       0xee4538u
#define ORIG_NEG_SIN_TABLE   0xee4adcu
#define ORIG_COS_TABLE       0xee5080u
#define ORIG_NEG_COS_TABLE   0xee5624u
#define ORIG_PRIM_SIZE       0xfa18u
#define ORIG_ACT_RES         0x108
#define ORIG_ACTRES_ACTIONS  0x110
#define ORIG_MOTION_SIZE     0x44
#define FAKE_ACTIONS         32
#define ORIG_MODE_WORLD      0xcc
#define ORIG_MODE_VIEW       0xd0
#define ORIG_VIEW_LATITUDE   0x2c
#define ORIG_VIEW_LONGITUDE  0x30
#define ORIG_WORLD_PLAYER    0x2c
#define ORIG_WORLD_ATTR      0x30
#define ORIG_MODE_VT_SEND_MSG (0x18 / 4)
#define ORIG_ACTOR_VT_SEND_MSG (0x08 / 4)
#define ORIG_ACTOR_VT_SET_ACTION (0x1c / 4)
#define ORIG_ACTOR_VT_EFFECT_STATE (0x34 / 4)
#define ORIG_ACTOR_VT_SET_STATE (0x3c / 4)

#define ID_LIMIT EFFECT_DURATION_COUNT
#define FRAMES 64
#define PRIMS 1024
#define PRIM_STEPS 32
#define MAX_REPORTED 40
#define EVENTS 64
#define EVENT_LEN 128
#define CAMERA_HEADING 37.0f
#define CAMERA_PITCH 33.0f

struct Session g_session1;
int g_isSkipFrame;
struct EffectMgr *g_effectMgr;
struct TexCache g_textureCache;
struct Renderer *g_renderer;

static char events[EVENTS][EVENT_LEN];
static int event_count, overflow;
static unsigned fault_code, fault_va;

static int trace;
static FILE *record_dump;

static void event(const char *fmt, ...)
{
    va_list ap;
    if (event_count >= EVENTS) {
        overflow = 1;
        return;
    }
    va_start(ap, fmt);
    vsnprintf(events[event_count++], EVENT_LEN, fmt, ap);
    va_end(ap);
    if (trace) {
        fprintf(stderr, "    %s\n", events[event_count - 1]);
        fflush(stderr);
    }
}

static const char *mode_msg_text(int message, int a, int b, int c, int d)
{
    static char text[96];
    if (message == 0x87 && a && b && c)
        snprintf(text, sizeof text, "mode msg 87 %.2f %.2f %.2f %d", *(const float *)a, *(const float *)b, *(const float *)c, d);
    else
        snprintf(text, sizeof text, "mode msg %x %d %d %d %d", message, a, b, c, d);
    return text;
}

static unsigned char orig_effect[0x12000], orig_master[0x200], orig_mode[0x200], orig_world[0x100], orig_view[0x200];
static unsigned char orig_player[0x200], orig_attr[0x100], orig_res_mgr[0x100], orig_texture[0x200], orig_actres[0x200], lift_actres[0x10];
static int orig_actions[FAKE_ACTIONS][3];
static unsigned char *orig_prims;
static void *orig_rag_vt[ORIG_RAG_VTABLE_SLOTS], *orig_master_vt[65], *orig_mode_vt[64], *orig_prim_vt[ORIG_PRIM_VTABLE_SLOTS];
static int orig_prim_count, lift_prim_count;

static struct RagEffect lift_effect;
static struct RenderObject lift_master, lift_player;
static struct RenderObjectOps lift_master_ops, lift_prim_ops;
static struct ModeOps lift_mode_ops;
static struct GameMode lift_mode;
static struct View lift_view;
static unsigned char lift_world[0x100], lift_attr[0x100], lift_texture[0x200];
static struct EffectPrim *lift_prims;

static unsigned fake_motions(unsigned action) { return action < FAKE_ACTIONS ? 2 + action % 5 : 1; }

static const char *who(const void *p)
{
    static char other[24];
    if (!p) return "0";
    if (p == orig_master || p == &lift_master) return "M";
    if (p == orig_effect || p == &lift_effect) return "E";
    if (p == orig_player || p == &lift_player) return "P";
    if (orig_prims && (unsigned char *)p >= orig_prims && (unsigned char *)p < orig_prims + (size_t)PRIMS * ORIG_PRIM_SIZE) {
        snprintf(other, sizeof other, "p%u", (unsigned)(((unsigned char *)p - orig_prims) / ORIG_PRIM_SIZE));
        return other;
    }
    if (lift_prims && (struct EffectPrim *)p >= lift_prims && (struct EffectPrim *)p < lift_prims + PRIMS) {
        snprintf(other, sizeof other, "p%u", (unsigned)((struct EffectPrim *)p - lift_prims));
        return other;
    }
    snprintf(other, sizeof other, "?%p", p);
    return other;
}

static int makes_game_objects(int effect_id)
{
    switch (effect_id) {
    case 166: case 617: case 752: case 792: case 824: case 925: case 1004: case 1081:
    case 1602: case 1603: case 1604: case 1605: case 1606:
        return 1;
    }
    return 0;
}

static float fake_height(float x, float z)
{
    return (float)floor(x / 5.0) * 0.5f - (float)floor(z / 5.0) * 0.25f;
}

static void mirror_prim(unsigned char *orig, const struct EffectPrim *lift);

#define ZERO_AS_INT(field) (*(int *)&(field) = 0)

static void prim_defaults(struct EffectPrim *self, struct RagEffect *master, int kind, Vec3 delta)
{
    int i;

    memset(self, 0, sizeof *self);
    self->base.zoom = 1.0f;
    self->base.shadow_scale = 1.0f;
    self->base.motion_delay = 1.0f;
    self->base.visible = 1;
    self->base.revealed = 1;
    self->base.argb = 0xffffffffu;
    self->base.hit_argb = 0xffff0000u;
    self->base.hit_tick = 0x10000000u;
    self->base.scale = 1.0f;
    self->base.plays = 1.0f;
    self->base.delay_factor = 1.0f;
    self->base.script_step_frames = 10;
    self->base.drawn = 1;
    self->base.cell_lit = 1;
    for (i = 0; i < PRIM_TRIANGLES; i++) {
        self->triangles[i].v[0].specular = 0xff000000u;
        self->triangles[i].v[1].specular = 0xff000000u;
        self->triangles[i].v[2].specular = 0xff000000u;
    }
    self->list_flags = 1;
    self->arc_degrees = 36.0f;
    self->spread_degrees = 180;
    self->scale = 1.0f;
    self->size2_max = 100.0f;
    self->r = 255;
    self->g = 255;
    self->b = 255;
    self->change_frame = -1;
    self->next_size1_speed = PRIM_UNSET;
    self->next_size2_speed = PRIM_UNSET;
    self->next_speed = PRIM_UNSET;
    self->next_size1_accel = PRIM_UNSET;
    self->next_size2_accel = PRIM_UNSET;
    self->next_accel = PRIM_UNSET;
    self->duration = 100;
    self->start_frame = -1;
    self->fade_frame = -1;
    self->argb = 0xffffffffu;
    self->alpha = 254.0f;
    self->fade_scale = 1.0f;
    self->alpha_max = 254.0f;
    self->loop_motion = 1;
    self->particle_count = 1;
    self->motion_period = 4;
    self->motion_count = 1;
    self->tex_v_bottom = 1.0f;
    self->tex_u_right = 1.0f;
    memcpy(self->matrix, g_identityMatrix.m, sizeof self->matrix);
    memcpy(self->matrix2, g_identityMatrix.m, sizeof self->matrix2);

    self->master = master;
    self->delta.x += delta.x;
    self->delta.y += delta.y;
    self->delta.z += delta.z;
    self->kind = kind;
    self->master_pos = master->base.pos;
    self->base.pos = self->master_pos;
    self->stage = 0;
}

/* ---- the lift's side ---- */

struct EffectPrim *rag_launch_prim(struct RagEffect *self, int kind, float dx, float dy, float dz)
{
    struct EffectPrim *p;
    Vec3 delta = { dx, dy, dz };
    event("prim %d at %.0f %.0f %.0f", kind, dx, dy, dz);
    if (lift_prim_count >= PRIMS)
        return NULL;
    p = &lift_prims[lift_prim_count++];
    prim_defaults(p, self, kind, delta);
    p->base.ops = &lift_prim_ops.base;
    return p;
}
void sound_play_wave(const char *name, float x, float y, float z, int max_distance, int full_distance, float volume)
{ event("wave %s %.0f %.0f %.0f %d %d %.1f", name, x, y, z, max_distance, full_distance, volume); }
int game_send_message(int message, int a, int b, int c, int d) { event("%s", mode_msg_text(message, a, b, c, d)); return 0; }
struct Player *game_current_player(void) { return (struct Player *)&lift_player; }
struct World *game_current_world(void) { return (struct World *)lift_world; }
struct C3dAttr *game_current_map_attr(void) { return (struct C3dAttr *)lift_attr; }
struct GameMode *game_current_mode(void) { return &lift_mode; }
struct View *game_current_view(void) { return &lift_view; }
struct List *game_current_object_list(void) { return NULL; }
float game_camera_heading(void) { return CAMERA_HEADING; }
int game_copy_view_matrix(float matrix[12]) { memset(matrix, 0, 12 * sizeof *matrix); return 1; }
void renderer_project_point(struct Renderer *renderer, const Vec3 *point, const float *view, float *x, float *y, float *rhw)
{ (void)renderer; (void)view; *x = point->x * 2.0f + 1.0f; *y = point->z * 3.0f + 2.0f; *rhw = 1.0f; }
float c3d_attr_get_height(struct C3dAttr *self, float x, float z) { (void)self; return fake_height(x, z); }
struct Texture *texture_cache_get(struct TexCache *cache, const char *name, int keyed, int full_size)
{ (void)cache; event("tex %s %d %d", name, keyed, full_size); return (struct Texture *)lift_texture; }
void render_object_load_act_res(struct RenderObject *self, const char *name) { self->act = lift_actres; event("act %s %s", who(self), name); }
void render_object_load_spr_res(struct RenderObject *self, const char *name) { event("spr %s %s", who(self), name); }
void render_object_set_action_base(struct RenderObject *object, int action, int type) { event("setaction %s %d %d", who(object), action, type); }
void render_object_play_motion_range(struct RenderObject *self, unsigned char action, unsigned char motion, unsigned char last,
                                     unsigned char close_action, unsigned char close_motion, int timed, int duration, int loop)
{ (void)loop; event("motion %s %d %d %d %d %d %d %d", who(self), action, motion, last, close_action, close_motion, timed, duration); }
void *object_as_type(void *object, enum ObjectType type) { (void)object; (void)type; return NULL; }
struct GameActor *game_find_actor(int aid) { (void)aid; return NULL; }
struct Effect *effect_mgr_spawn(struct EffectMgr *self, int aid, int type, Vec3 pos, int target_aid, int level, unsigned char reuse, int key)
{ (void)self; event("spawn %d %x at %.0f %.0f %.0f %d %d %d %d", aid, type, pos.x, pos.y, pos.z, target_aid, level, reuse, key); return NULL; }
void game_actor_set_job(struct GameActor *self, int job) { event("job %s %d", who(self), job); }
struct RenderObject *hallu_pc_construct(void *mem, struct RenderObject *actor) { (void)mem; (void)actor; return NULL; }
int effect_add_world_object(struct World *expected_world, struct RenderObject *object) { (void)expected_world; event("add %s", who(object)); return 1; }
int session_is_pc_job(struct Session *self, int job) { (void)self; (void)job; return 1; }
const char *session_get_efst_image(struct Session *self, int efst, int pass) { (void)self; (void)efst; (void)pass; return ""; }
const char *game_current_map_name(void) { return ""; }
int game_actor_get_height_offset(struct GameActor *self) { (void)self; return 0; }
void *render_object_get_act_res(struct RenderObject *self) { return self->act; }
unsigned char rag_str_step(struct RagEffect *self, float dx, float dy, float dz) { (void)self; (void)dx; (void)dy; (void)dz; return 0; }
void rag_str_rewind(struct RagEffect *self) { (void)self; event("rewind"); }
void error_msg(const char *text, int flags) { (void)flags; event("error %s", text); }
void *mem_alloc(size_t bytes) { return malloc(bytes); }
void mem_free(void *memory) { free(memory); }
void fatal_error(const char *message) { fprintf(stderr, "fatal: %s\n", message); ExitProcess(5); }
struct BlurPC *blur_pc_construct(struct BlurPC *self, struct Pc *master) { (void)self; (void)master; return NULL; }
struct HalluNPC *hallu_npc_construct(struct HalluNPC *self, struct GameActor *src) { (void)self; (void)src; return NULL; }
struct HalluPC2 *hallu_pc2_construct(struct HalluPC2 *self, struct Pc *master) { (void)self; (void)master; return NULL; }
struct HalluPlant *hallu_plant_construct(struct HalluPlant *self) { (void)self; return NULL; }
int game_actor_get_job(struct GameActor *actor) { (void)actor; return 0; }
int mode_send_message(struct Mode *mode, int message, int a, int b, int c, int d)
{ (void)mode; event("%s", mode_msg_text(message, a, b, c, d)); return 0; }
struct Mode *client_current_mode(void) { return &lift_mode.base; }
size_t own_states_count(void) { return 0; }
const struct OwnStateEntry *own_states_at(size_t position) { (void)position; return NULL; }
struct Motion *act_res_get_motion(const struct ActRes *self, unsigned action, unsigned motion) { (void)self; (void)action; (void)motion; return NULL; }
struct SprClip *motion_get_clip(const struct Motion *self, unsigned index) { (void)self; (void)index; return NULL; }
int client_lua_call(struct ClientLua *lua, const char *function, const char *format, ...) { (void)lua; (void)function; (void)format; return 0; }
void rag_effect_send_msg(struct RagEffect *self, struct RenderObject *sender, RenderMsgArg msg, RenderMsgArg par1, RenderMsgArg par2, RenderMsgArg par3)
{ event("ragmsg %s from %s %llx %llx %llx %llx", who(self), who(sender), msg, par1, par2, par3); }
static void project_into(struct TexDrawVertex *out, const Vec3 *pos)
{
    out->x = pos->x * 2.0f + 1.0f;
    out->y = pos->z * 3.0f + 2.0f;
    out->z = 0.0f;
    out->rhw = 1.0f;
}
void renderer_project_vertex(struct Renderer *renderer, const Vec3 *point, const float *view, struct TexDrawVertex *vertex)
{ (void)renderer; (void)view; project_into(vertex, point); }
static void lift_master_send(struct GameObject *self, struct GameObject *sender, RenderMsgArg msg, RenderMsgArg a, RenderMsgArg b,
                             RenderMsgArg c, RenderMsgArg d)
{ event("msg %s from %s %llx %llx %llx %llx %llx", who(self), who(sender), msg, a, b, c, d); }
static void lift_master_set_action(struct RenderObject *self, int action, int type) { event("setaction %s %d %d", who(self), action, type); }
static int lift_master_effect_state(struct RenderObject *self) { (void)self; return 0; }
int g_playerBlind, g_replayPlaying;
float g_depthPerPixelCot, g_spriteRowDepthStep;
struct StateIconSheet *g_stateIconSheet;
static struct ReplayRecorder lift_replay;
static struct Renderer lift_renderer;
unsigned actres_frame_count(const struct ActRes *self, unsigned action) { (void)self; return fake_motions(action); }
void c3d_ground15_draw_attr_quad(struct C3dGround15 *self, const float *view, int ax, int ay, DWORD colour, struct Texture *texture, const float *uv)
{ (void)self; (void)view; (void)texture; (void)uv; event("attrquad %d %d %lx", ax, ay, colour); }
float game_actor_get_mount_lift(struct GameActor *self) { (void)self; return 0.0f; }
struct RagEffect *game_actor_launch_effect(struct GameActor *self, int effect_id, Vec3 offset, float rotation)
{ event("launch %s %d %.0f %.0f %.0f %.1f", who(self), effect_id, offset.x, offset.y, offset.z, rotation); return NULL; }
float game_camera_pitch(void) { return lift_view.latitude; }
struct C3dGround15 *game_current_ground(void) { return NULL; }
int game_map_preserves_party_effects(void) { return 0; }
void game_object_set_render_info(struct RenderObject *object, const struct ScreenBounds *bounds, float x, float y)
{ (void)object; (void)bounds; (void)x; (void)y; }
int game_world_to_cell(float x, float z, int *cell_x, int *cell_y) { (void)x; (void)z; *cell_x = *cell_y = 0; return 0; }
void icon_sheet_clear(struct StateIconSheet *sheet) { (void)sheet; event("icons clear"); }
void icon_sheet_flush(struct StateIconSheet *sheet) { (void)sheet; event("icons flush"); }
void icon_sheet_queue_state(struct StateIconSheet *sheet, unsigned state, int x, int y) { (void)sheet; event("icon %u %d %d", state, x, y); }
BYTE is_effect_state_cloaking(DWORD state) { (void)state; return 0; }
const struct OwnStateEntry *own_states_find(int index) { (void)index; return NULL; }
void render_object_apply_shrink_scale(struct GameActor *self, float *scale) { (void)self; (void)scale; }
struct RenderObject *render_object_construct(struct RenderObject *self) { return self; }
void render_object_dc_render_object(struct RenderObject *self) { (void)self; }
void *render_object_get_spr_res(struct RenderObject *self) { return self->spr; }
int render_object_return_zero(struct RenderObject *object) { (void)object; return 0; }
void render_object_send_msg(struct GameObject *object, struct GameObject *source, RenderMsgArg msg, RenderMsgArg a, RenderMsgArg b,
                            RenderMsgArg c, RenderMsgArg d)
{ event("msg %s from %s %llx %llx %llx %llx %llx", who(object), who(source), msg, a, b, c, d); }
void render_object_submit_spr_quad(struct RenderObject *self, struct SqQuad *quad, const void *palette, int flags, float x, float y)
{ (void)quad; (void)palette; event("sprquad %s %d %.1f %.1f", who(self), flags, x, y); }
void render_object_update_motion_base(struct RenderObject *object) { (void)object; }
void renderer_add_triangle(struct Renderer *self, struct PrimTriangle *tri, int flags) { (void)self; (void)tri; event("tri %d", flags); }
struct ReplayRecorder *replay_recorder_instance(void) { return &lift_replay; }
int session_is_party_member_aid(struct Session *self, int aid, unsigned char only_if_clear) { (void)self; (void)aid; (void)only_if_clear; return 0; }

#include "rag_process.inc"

/* ---- the original's side ---- */

static void *__fastcall o_launch_prim(void *self, void *edx, int kind, float dx, float dy, float dz)
{
    unsigned char *p;
    static struct EffectPrim model;
    Vec3 delta = { dx, dy, dz };
    (void)self; (void)edx;
    if (trace)
        fprintf(stderr, "    (launch from original 0x%x)\n",
                (unsigned)((unsigned char *)_ReturnAddress() - (unsigned char *)oracle_addr(0x400000u)) + 0x400000u);
    event("prim %d at %.0f %.0f %.0f", kind, dx, dy, dz);
    if (orig_prim_count >= PRIMS)
        return NULL;
    p = orig_prims + (size_t)orig_prim_count++ * ORIG_PRIM_SIZE;
    memset(p, 0, ORIG_PRIM_SIZE);
    prim_defaults(&model, &lift_effect, kind, delta);
    mirror_prim(p, &model);
    *(void **)p = orig_prim_vt;
    *(void **)(p + 0x140) = orig_effect;
    memcpy(p + 0x37c, orig_effect + 0x10, 12);
    memcpy(p + 0x10, orig_effect + 0x10, 12);
    return p;
}
static void __stdcall o_play_wave(const char *name, float x, float y, float z, int max_distance, int full_distance, float volume)
{ event("wave %s %.0f %.0f %.0f %d %d %.1f", name, x, y, z, max_distance, full_distance, volume); }
static void *__fastcall o_get_mode(void *self, void *edx) { (void)self; (void)edx; return orig_mode; }
static double __fastcall o_height(void *self, void *edx, float x, float z) { (void)self; (void)edx; return (double)fake_height(x, z); }
static void __fastcall o_load_act(void *self, void *edx, const char *name)
{ (void)edx; *(void **)((unsigned char *)self + ORIG_ACT_RES) = orig_actres; event("act %s %s", who(self), name); }
static void __fastcall o_load_spr(void *self, void *edx, const char *name) { (void)edx; event("spr %s %s", who(self), name); }
static void *__fastcall o_texture_get(void *self, void *edx, const char *name, char keyed, int full_size)
{ (void)self; (void)edx; event("tex %s %d %d", name, keyed, full_size); return orig_texture; }
static void *__cdecl o_res_mgr(void) { return orig_res_mgr; }
static void *__fastcall o_res_get(void *self, void *edx, const char *name) { (void)self; (void)edx; event("res %s", name); return NULL; }
static int __cdecl o_message_box(const char *text, int flags) { (void)flags; event("error %s", text); return 0; }
static void __fastcall o_set_action_base(void *self, void *edx, int action, int unused, int type)
{ (void)edx; (void)unused; event("setaction %s %d %d", who(self), action, type); }
static int __fastcall o_motion(void *self, void *edx, char action, char motion, char last, char close_action, char close_motion, char timed, int duration)
{ (void)edx; event("motion %s %d %d %d %d %d %d %d", who(self), action, motion, last, close_action, close_motion, timed, duration); return 0; }
static void *__fastcall o_spawn(void *self, void *edx, int aid, int type, float x, float y, float z, int target, int level, int reuse, int key)
{ (void)self; (void)edx; event("spawn %d %x at %.0f %.0f %.0f %d %d %d %d", aid, type, x, y, z, target, level, reuse, key); return NULL; }
static void *__fastcall o_actor_by_aid(void *self, void *edx, int aid) { (void)self; (void)edx; (void)aid; return NULL; }
static void __fastcall o_project(void *self, void *edx, const float *point, const float *view, float *x, float *y, float *rhw)
{ (void)self; (void)edx; (void)view; *x = point[0] * 2.0f + 1.0f; *y = point[2] * 3.0f + 2.0f; *rhw = 1.0f; }
static float *__fastcall o_project_vertex(void *self, void *edx, const float *point, const float *view, float *out)
{
    (void)self; (void)edx; (void)view;
    out[0] = point[0] * 2.0f + 1.0f;
    out[1] = point[2] * 3.0f + 2.0f;
    out[2] = 0.0f;
    out[3] = 1.0f;
    return out;
}
static int __fastcall o_mode_send_msg(void *self, void *edx, int msg, int a, int b, int c, int d)
{ (void)self; (void)edx; event("%s", mode_msg_text(msg, a, b, c, d)); return 0; }
static void __fastcall o_actor_send_msg(void *self, void *edx, void *sender, long long msg, long long a, long long b, long long c, long long d)
{ (void)edx; event("msg %s from %s %llx %llx %llx %llx %llx", who(self), who(sender), msg, a, b, c, d); }
static void __fastcall o_set_action(void *self, void *edx, int action, int unused, int type) { (void)edx; (void)unused; event("setaction %s %d %d", who(self), action, type); }
static void *__cdecl o_new(size_t size) { return size >= 0x3c0 ? NULL : malloc(size); }
static int __cdecl o_lua_call(void *lua, const char *function, const char *format, ...) { (void)lua; (void)function; (void)format; return 0; }
static int __fastcall o_get_effect_state(void *self, void *edx) { (void)self; (void)edx; return 0; }
static void __fastcall o_set_state(void *self, void *edx, int state, int snap) { (void)edx; event("state %s %d %d", who(self), state, snap); }
static void __fastcall o_rag_send_msg(void *self, void *edx, void *sender, long long msg, long long a, long long b, long long c)
{ (void)edx; event("ragmsg %s from %s %llx %llx %llx %llx", who(self), who(sender), msg, a, b, c); }
static int __fastcall o_get_job(void *self, void *edx) { (void)self; (void)edx; return 0; }
static void *__fastcall o_launch_effect(void *self, void *edx, int effect_id, float x, float y, float z, float rotation)
{ (void)edx; event("launch %s %d %.0f %.0f %.0f %.1f", who(self), effect_id, x, y, z, rotation); return NULL; }
static int __fastcall o_str_rewind(void *self, void *edx) { (void)self; (void)edx; event("rewind"); return 0; }
static DWORD WINAPI o_time(void) { return 0x10000000u; }
void *oracle_time_slot = (void *)o_time;
#pragma comment(linker, "/alternatename:__imp__timeGetTime@0=_oracle_time_slot")

static int trap_slot;
static void *trap_self;
static void __cdecl trap_hit(void)
{
    fprintf(stderr, "  the original called vtable slot %d (offset 0x%x) on %s, which the test does not provide\n", trap_slot,
            trap_slot * 4, who(trap_self));
    fflush(stderr);
    ExitProcess(3);
}
#define TRAP(n) static void __cdecl trap_##n(void) { void *self; __asm { mov self, ecx } trap_slot = n; trap_self = self; trap_hit(); }
TRAP(0) TRAP(1) TRAP(2) TRAP(3) TRAP(4) TRAP(5) TRAP(6) TRAP(7) TRAP(8) TRAP(9) TRAP(10) TRAP(11) TRAP(12) TRAP(13) TRAP(14) TRAP(15)
TRAP(16) TRAP(17) TRAP(18) TRAP(19) TRAP(20) TRAP(21) TRAP(22) TRAP(23) TRAP(24) TRAP(25) TRAP(26) TRAP(27) TRAP(28) TRAP(29) TRAP(30) TRAP(31)
TRAP(32) TRAP(33) TRAP(34) TRAP(35) TRAP(36) TRAP(37) TRAP(38) TRAP(39) TRAP(40) TRAP(41) TRAP(42) TRAP(43) TRAP(44) TRAP(45) TRAP(46) TRAP(47)
TRAP(48) TRAP(49) TRAP(50) TRAP(51) TRAP(52) TRAP(53) TRAP(54) TRAP(55) TRAP(56) TRAP(57) TRAP(58) TRAP(59) TRAP(60) TRAP(61) TRAP(62) TRAP(63)
static void *const traps[64] = {
    trap_0, trap_1, trap_2, trap_3, trap_4, trap_5, trap_6, trap_7, trap_8, trap_9, trap_10, trap_11, trap_12, trap_13, trap_14, trap_15,
    trap_16, trap_17, trap_18, trap_19, trap_20, trap_21, trap_22, trap_23, trap_24, trap_25, trap_26, trap_27, trap_28, trap_29, trap_30, trap_31,
    trap_32, trap_33, trap_34, trap_35, trap_36, trap_37, trap_38, trap_39, trap_40, trap_41, trap_42, trap_43, trap_44, trap_45, trap_46, trap_47,
    trap_48, trap_49, trap_50, trap_51, trap_52, trap_53, trap_54, trap_55, trap_56, trap_57, trap_58, trap_59, trap_60, trap_61, trap_62, trap_63,
};

typedef unsigned char (__fastcall *process_fn)(void *self, void *edx);

static LONG WINAPI first_look(EXCEPTION_POINTERS *ep)
{
    unsigned char *at = (unsigned char *)ep->ExceptionRecord->ExceptionAddress;
    unsigned char *image = (unsigned char *)oracle_addr(0x400000u);
    const unsigned *sp = (const unsigned *)ep->ContextRecord->Esp;
    int i;
    fprintf(stderr, "  fault %08lx at %p (original 0x%x) eax=%08lx ecx=%08lx edx=%08lx ebx=%08lx esi=%08lx edi=%08lx ebp=%08lx esp=%08lx\n",
            ep->ExceptionRecord->ExceptionCode, at, (unsigned)(at - image) + 0x400000u, ep->ContextRecord->Eax, ep->ContextRecord->Ecx,
            ep->ContextRecord->Edx, ep->ContextRecord->Ebx, ep->ContextRecord->Esi, ep->ContextRecord->Edi, ep->ContextRecord->Ebp,
            ep->ContextRecord->Esp);
    for (i = 0; i < 96; i++) {
        unsigned char *v = (unsigned char *)sp[i];
        if (v >= image + 0x1000 && v < image + 0x8da000)
            fprintf(stderr, "    stack[%d] -> original 0x%x\n", i, (unsigned)(v - image) + 0x400000u);
    }
    fflush(stderr);
    ExitProcess(4);
    return EXCEPTION_CONTINUE_SEARCH;
}

static int note_fault(EXCEPTION_POINTERS *ep)
{
    fault_code = ep->ExceptionRecord->ExceptionCode;
    fault_va = (unsigned)((unsigned char *)ep->ExceptionRecord->ExceptionAddress - (unsigned char *)oracle_addr(0x400000u)) + 0x400000u;
    return EXCEPTION_EXECUTE_HANDLER;
}

static int call_original(process_fn process, unsigned char *result)
{
    __try { *result = process(orig_effect, NULL); return 1; }
    __except (note_fault(GetExceptionInformation())) { return 0; }
}

static int step_original(unsigned char *prim, unsigned char *result)
{
    process_fn process = (process_fn)orig_prim_vt[1];
    __try { *result = process(prim, NULL); return 1; }
    __except (note_fault(GetExceptionInformation())) { return 0; }
}

static void joined(char *out, size_t size)
{
    int i;
    out[0] = 0;
    for (i = 0; i < event_count; i++) {
        strncat(out, events[i], size - strlen(out) - 1);
        strncat(out, "; ", size - strlen(out) - 1);
    }
    if (overflow)
        strncat(out, "...", size - strlen(out) - 1);
}

/* ---- field maps ---- */

static unsigned base_orig_offset(unsigned lift_offset)
{
    if (lift_offset < 0x1c)
        return lift_offset;
    if (lift_offset < 0x10c)
        return lift_offset + 0xc;
    return lift_offset + 0x10;
}

struct FieldMap { const char *name; unsigned lift_offset, orig_offset, size; char kind; };
#define RO(field) #field, offsetof(struct RenderObject, field), 0, sizeof(((struct RenderObject *)0)->field)
#define PF(field, orig, kind) { #field, offsetof(struct EffectPrim, field), orig, sizeof(((struct EffectPrim *)0)->field), kind }
#define EF(field, orig, kind) { #field, offsetof(struct RagEffect, field), orig, sizeof(((struct RagEffect *)0)->field), kind }

static const struct FieldMap prim_map[] = {
    PF(change_reached, 0x138, 'v'), PF(stage, 0x13c, 'v'), PF(master, 0x140, 'p'), PF(textures, 0x144, 'n'),
    PF(texture_frame, 0x148, 'v'), PF(arrived, 0x14c, 'v'), PF(alpha_keys_set_speed, 0x150, 'v'), PF(alpha_keys_set_alpha, 0x154, 'v'),
    PF(alpha_keys_set_size, 0x158, 'v'), PF(fading_out, 0x15c, 'v'), PF(matrix, 0x160, 'v'), PF(matrix2, 0x190, 'v'),
    PF(argb, 0x1c0, 'v'), PF(list_flags, 0x1c4, 'v'), PF(use_list_flags, 0x1c8, 'v'), PF(size2_flip_period, 0x1cc, 'v'),
    PF(kind, 0x1d0, 'v'), PF(pattern_arg, 0x1d4, 'v'), PF(alpha_key, 0x1d8, 'v'), PF(pattern, 0x1dc, 'v'), PF(frame, 0x1e0, 'v'),
    PF(duration, 0x1e4, 'v'), PF(motion_period, 0x1e8, 'v'), PF(start_frame, 0x1ec, 'v'), PF(fade_frame, 0x1f0, 'v'),
    PF(particle_count, 0x1f4, 'v'), PF(spread_degrees, 0x1f8, 'v'), PF(color_hold_count, 0x1fc, 'v'), PF(change_frame, 0x200, 'v'),
    PF(motion_count, 0x204, 'v'), PF(bounce_clink, 0x208, 'v'), PF(color_hold_frames, 0x234, 'v'), PF(endless, 0x238, 'v'),
    PF(r, 0x23c, 'v'), PF(g, 0x240, 'v'), PF(b, 0x244, 'v'), PF(r_speed, 0x248, 'v'), PF(g_speed, 0x24c, 'v'), PF(b_speed, 0x250, 'v'),
    PF(loop_motion, 0x254, 'v'), PF(alpha_key_frames, 0x258, 'k'), PF(alpha_key_values, 0x264, 'k'), PF(alpha_key_ceilings, 0x270, 'k'),
    PF(alpha_key_floors, 0x27c, 'k'), PF(arc_degrees, 0x288, 'v'), PF(arc_offset, 0x28c, 'v'), PF(swing_axis, 0x290, 'v'),
    PF(next_speed, 0x29c, 'v'), PF(next_accel, 0x2a0, 'v'), PF(next_velocity, 0x2a4, 'v'), PF(next_velocity_accel, 0x2b0, 'v'),
    PF(speed, 0x2bc, 'v'), PF(accel, 0x2c0, 'v'), PF(next_size1_accel, 0x2c4, 'v'), PF(next_size1_speed, 0x2c8, 'v'),
    PF(next_size2_speed, 0x2cc, 'v'), PF(next_size2_accel, 0x2d0, 'v'), PF(alpha_max, 0x2d4, 'v'), PF(alpha_min, 0x2d8, 'v'),
    PF(tex_u_left, 0x2dc, 'v'), PF(tex_u_right, 0x2e0, 'v'), PF(tex_v_top, 0x2e4, 'v'), PF(tex_v_bottom, 0x2e8, 'v'),
    PF(fall_speed, 0x2ec, 'v'), PF(gravity, 0x2f0, 'v'), PF(radius, 0x2f4, 'v'), PF(radius_speed, 0x2fc, 'v'), PF(radius_accel, 0x304, 'v'),
    PF(angle, 0x30c, 'v'), PF(scale, 0x318, 'v'), PF(scale_speed, 0x31c, 'v'), PF(scale_accel, 0x320, 'v'), PF(alpha, 0x324, 'v'),
    PF(alpha_speed, 0x328, 'v'), PF(fade_scale, 0x32c, 'v'), PF(angle_speed, 0x330, 'v'), PF(angle_accel, 0x33c, 'v'),
    PF(size, 0x348, 'v'), PF(size2_max, 0x36c, 'v'), PF(velocity, 0x370, 'v'), PF(master_pos, 0x37c, 'v'), PF(delta, 0x388, 'v'),
    PF(delta2, 0x394, 'v'), PF(drift, 0x3a0, 'v'), PF(velocity_accel, 0x3ac, 'v'), PF(corners, 0x3b8, 'v'), PF(variant, 0x3e8, 'u'),
    PF(camera_yaw, 0x3ea, 'x'), PF(camera_pitch, 0x3ec, 'x'), PF(records, 0x3e8, 'r'), PF(particles, 0x6b8, 'v'),
    PF(triangles, 0x2210, 't'), PF(master_sprite_lift, 0xfa10, 'v'),
};

static const struct FieldMap effect_map[] = {
    EF(master, 0x138, 'p'), EF(msg_params, 0x13c, 'v'), EF(master_roty, 0x14c, 'v'), EF(pitch, 0x150, 'v'), EF(screen_pos, 0x154, 'v'),
    EF(has_str_offset, 0x160, 'v'), EF(aux_flag, 0x164, 'v'), EF(effect_id, 0x168, 'v'), EF(repeat_count, 0x16c, 'v'),
    EF(id_state, 0x170, 'v'), EF(hit_count, 0x174, 'v'), EF(frame, 0x178, 'v'), EF(duration, 0x17c, 'v'), EF(offset, 0x180, 'v'),
    EF(offset2, 0x18c, 'v'), EF(head_offset, 0x198, 'v'), EF(str_max_loops, 0x11c80, 'v'), EF(last_prim, 0x11c84, 'p'),
    EF(color_r, 0x11c88, 'v'), EF(color_g, 0x11c8c, 'v'), EF(color_b, 0x11c90, 'v'), EF(dest_blend, 0x11c94, 'v'),
    EF(always_draw, 0x11c98, 'v'), EF(show_id, 0x11c9c, 'v'),
};

static void mirror_prim(unsigned char *orig, const struct EffectPrim *lift)
{
    static const struct { unsigned from, to; } ranges[] = { { 0x10, 0x1c }, { 0x28, 0xf8 }, { 0x100, 0x10c }, { 0x114, 0x128 } };
    const unsigned char *bytes = (const unsigned char *)lift;
    unsigned r, i;
    for (r = 0; r < sizeof ranges / sizeof *ranges; r++)
        memcpy(orig + base_orig_offset(ranges[r].from), bytes + ranges[r].from, ranges[r].to - ranges[r].from);
    for (i = 0; i < sizeof prim_map / sizeof *prim_map; i++)
        if (prim_map[i].kind == 'v' || prim_map[i].kind == 't' || prim_map[i].kind == 'r')
            memcpy(orig + prim_map[i].orig_offset, bytes + prim_map[i].lift_offset, prim_map[i].size);
}

struct RecordField { const char *member; int kind; unsigned lift_off, orig_off, size; };
struct RecordMember { const char *name; unsigned lift_base, lift_elem, count; };
struct KindRecord { int kind; const char *member; };
#include "rag_record_map.inc"
#define ORIG_RECORDS 0x3e8
#define ORIG_RECORD_SLOT 0xb4

static int differs(const char *where, const char *field, const unsigned char *lift_bytes, const unsigned char *orig_bytes, unsigned n,
                   char *report, size_t report_size);

static int records_differ(const char *where, int kind, const unsigned char *lift_prim, const unsigned char *orig_prim, char *report, size_t size)
{
    unsigned k, m, f, e;
    for (k = 0; k < sizeof kind_records / sizeof *kind_records; k++) {
        const struct RecordMember *member = NULL;
        if (kind_records[k].kind != kind)
            continue;
        for (m = 0; m < sizeof record_members / sizeof *record_members; m++)
            if (strcmp(record_members[m].name, kind_records[k].member) == 0)
                member = &record_members[m];
        if (!member)
            continue;
        for (e = 0; e < member->count; e++)
            for (f = 0; f < sizeof record_fields / sizeof *record_fields; f++) {
                const struct RecordField *field = &record_fields[f];
                const unsigned char *lift_elem = lift_prim + offsetof(struct EffectPrim, records) + member->lift_base + e * member->lift_elem;
                const unsigned char *orig_slot = orig_prim + ORIG_RECORDS + e * ORIG_RECORD_SLOT;
                char name[64];
                unsigned alias, aliased = 0;
                if (strcmp(field->member, member->name) != 0 || (field->kind != 0 && field->kind != kind))
                    continue;
                if (memcmp(lift_elem + field->lift_off, orig_slot + field->orig_off, field->size) == 0)
                    continue;
                for (alias = 0; alias < sizeof record_fields / sizeof *record_fields && !aliased; alias++)
                    if (alias != f && strcmp(record_fields[alias].member, member->name) == 0 && record_fields[alias].orig_off == field->orig_off &&
                        (record_fields[alias].kind == 0 || record_fields[alias].kind == kind) && record_fields[alias].size == field->size &&
                        memcmp(lift_elem + record_fields[alias].lift_off, orig_slot + field->orig_off, field->size) == 0)
                        aliased = 1;
                if (aliased)
                    continue;
                snprintf(name, sizeof name, "%s[%u]+%x", member->name, e, field->lift_off);
                if (differs(where, name, lift_elem + field->lift_off, orig_slot + field->orig_off, field->size, report, size))
                    return 1;
            }
    }
    return 0;
}

static const char *pointer_text(const void *p)
{
    return who(p);
}

static void hex_text(char *out, size_t size, const unsigned char *bytes, unsigned n)
{
    unsigned i, len = 0;
    out[0] = 0;
    for (i = 0; i < n && len + 3 < size; i++)
        len += (unsigned)snprintf(out + len, size - len, "%02x", bytes[i]);
}

static int differs(const char *where, const char *field, const unsigned char *lift_bytes, const unsigned char *orig_bytes, unsigned n,
                   char *report, size_t report_size)
{
    char l[64], o[64];
    unsigned first = 0, shown;
    if (memcmp(lift_bytes, orig_bytes, n) == 0)
        return 0;
    if (n % 4 == 0) {
        for (first = 0; first < n; first += 4)
            if (memcmp(lift_bytes + first, orig_bytes + first, 4) != 0 &&
                !(isnan(*(const float *)(lift_bytes + first)) && isnan(*(const float *)(orig_bytes + first))))
                break;
        if (first == n)
            return 0;
    }
    shown = n - first > 24 ? 24 : n - first;
    hex_text(l, sizeof l, lift_bytes + first, shown);
    hex_text(o, sizeof o, orig_bytes + first, shown);
    if (first)
        snprintf(report, report_size, "%s.%s+%x: lift %s original %s", where, field, first, l, o);
    else
        snprintf(report, report_size, "%s.%s: lift %s original %s", where, field, l, o);
    return 1;
}

static int keys_differ(const char *where, const char *field, const struct Vec *lift_vec, const struct Vec *orig_vec, char *report, size_t size)
{
    unsigned lift_count = lift_vec->first ? (unsigned)VEC_COUNT(lift_vec, float) : 0;
    unsigned orig_count = orig_vec->first ? (unsigned)VEC_COUNT(orig_vec, float) : 0, i;
    if (lift_count != orig_count) {
        snprintf(report, size, "%s.%s: lift has %u keys, the original %u", where, field, lift_count, orig_count);
        return 1;
    }
    for (i = 0; i < orig_count; i++)
        if (VEC_AT(lift_vec, float, i) != VEC_AT(orig_vec, float, i)) {
            snprintf(report, size, "%s.%s[%u]: lift %f original %f", where, field, i, VEC_AT(lift_vec, float, i), VEC_AT(orig_vec, float, i));
            return 1;
        }
    return 0;
}

static int pointer_differs(const char *where, const char *field, const void *lift_p, const void *orig_p, char *report, size_t size)
{
    char l[24];
    strncpy(l, pointer_text(lift_p), sizeof l - 1);
    l[sizeof l - 1] = 0;
    if (strcmp(l, pointer_text(orig_p)) == 0)
        return 0;
    snprintf(report, size, "%s.%s: lift %s original %s", where, field, l, pointer_text(orig_p));
    return 1;
}

static int base_differs(const char *where, const unsigned char *lift_base, const unsigned char *orig_base, char *report, size_t size)
{
    static const struct { unsigned from, to; } ranges[] = { { 0x10, 0x1c }, { 0x28, 0xf8 }, { 0x100, 0x10c }, { 0x114, 0x128 } };
    unsigned r, off;
    for (r = 0; r < sizeof ranges / sizeof *ranges; r++)
        for (off = ranges[r].from; off < ranges[r].to; off += 4) {
            unsigned n = ranges[r].to - off < 4 ? ranges[r].to - off : 4;
            char field[16];
            snprintf(field, sizeof field, "base+%x", off);
            if (differs(where, field, lift_base + off, orig_base + base_orig_offset(off), n, report, size))
                return 1;
        }
    return 0;
}

static int map_differs(const char *where, const struct FieldMap *map, unsigned count, const unsigned char *lift_object,
                       const unsigned char *orig_object, char *report, size_t size)
{
    unsigned i;
    for (i = 0; i < count; i++) {
        const struct FieldMap *f = &map[i];
        const unsigned char *l = lift_object + f->lift_offset, *o = orig_object + f->orig_offset;
        switch (f->kind) {
        case 'p':
            if (pointer_differs(where, f->name, *(void *const *)l, *(void *const *)o, report, size))
                return 1;
            break;
        case 'u': {
            unsigned char lift_bytes[2] = { 0, 0 };
            memcpy(lift_bytes, l, f->size);
            if ((lift_bytes[0] || lift_bytes[1]) && differs(where, f->name, l, o, f->size, report, size))
                return 1;
            break;
        }
        case 'r':
            if (records_differ(where, *(const int *)(lift_object + offsetof(struct EffectPrim, kind)), lift_object, orig_object, report, size))
                return 1;
            break;
        case 'x':
            break;
        case 'n':
            if ((*(void *const *)l == NULL) != (*(void *const *)o == NULL)) {
                snprintf(report, size, "%s.%s: lift %s original %s", where, f->name, *(void *const *)l ? "set" : "null",
                         *(void *const *)o ? "set" : "null");
                return 1;
            }
            break;
        case 'k':
            if (keys_differ(where, f->name, (const struct Vec *)l, (const struct Vec *)o, report, size))
                return 1;
            break;
        case 't': {
            unsigned t;
            for (t = 0; t < PRIM_TRIANGLES; t++) {
                const unsigned char *lt = l + t * sizeof(struct PrimTriangle), *ot = o + t * sizeof(struct PrimTriangle);
                char field[32];
                snprintf(field, sizeof field, "triangles[%u]", t);
                if (differs(where, field, lt, ot, 0x60, report, size) ||
                    pointer_differs(where, field, *(void *const *)(lt + 0x60), *(void *const *)(ot + 0x60), report, size) ||
                    differs(where, field, lt + 0x64, ot + 0x64, sizeof(struct PrimTriangle) - 0x64, report, size))
                    return 1;
            }
            break;
        }
        default:
            if (differs(where, f->name, l, o, f->size, report, size))
                return 1;
        }
    }
    return 0;
}

static void reset_fakes(int effect_id, int option)
{
    int i;

    memset(orig_effect, 0, sizeof orig_effect);
    *(void **)orig_effect = orig_rag_vt;
    *(void **)(orig_effect + 0x138) = orig_master;
    *(int *)(orig_effect + 0x168) = effect_id;
    memcpy(orig_effect + 0x10, orig_master + 0x10, 12);
    *(float *)(orig_effect + 0x14c) = *(float *)(orig_master + 0x4c);
    *(int *)(orig_effect + 0x17c) = g_effectDurations[effect_id];
    *(int *)(orig_effect + 0x16c) = 1;
    ((float *)(orig_effect + 0x180))[0] = 160.0f;
    ((float *)(orig_effect + 0x180))[1] = 0.0f;
    ((float *)(orig_effect + 0x180))[2] = 240.0f;
    for (i = 0; i < 4; i++)
        *(int *)(orig_effect + 0x11c88 + i * 4) = -1;
    *(int *)oracle_addr(ORIG_EFFECT_OPTION) = option;
    *(int *)oracle_addr(ORIG_SKIP_FRAME) = 1;

    memset(orig_master, 0, sizeof orig_master);
    *(void **)orig_master = &orig_master_vt[1];
    ((float *)(orig_master + 0x10))[0] = 100.0f;
    ((float *)(orig_master + 0x10))[1] = 5.0f;
    ((float *)(orig_master + 0x10))[2] = 200.0f;
    *(float *)(orig_master + 0x4c) = 123.0f;
    *(unsigned *)(orig_master + base_orig_offset(offsetof(struct RenderObject, argb))) = 0xffffffffu;
    orig_master[base_orig_offset(offsetof(struct RenderObject, drawn))] = 1;
    orig_master[base_orig_offset(offsetof(struct RenderObject, visible))] = 1;
    orig_master[base_orig_offset(offsetof(struct RenderObject, revealed))] = 1;
    memcpy(orig_effect + 0x10, orig_master + 0x10, 12);
    *(float *)(orig_effect + 0x14c) = 123.0f;
    orig_prim_count = 0;

    memset(&lift_effect, 0, sizeof lift_effect);
    lift_effect.master = &lift_master;
    lift_effect.effect_id = effect_id;
    lift_effect.duration = g_effectDurations[effect_id];
    lift_effect.repeat_count = 1;
    lift_effect.offset.x = 160.0f;
    lift_effect.offset.y = 0.0f;
    lift_effect.offset.z = 240.0f;
    lift_effect.color_r = lift_effect.color_g = lift_effect.color_b = lift_effect.dest_blend = -1;
    memset(&g_session1, 0, sizeof g_session1);
    g_session1.map_rule_flags[0] = option;
    g_session1.effectOn = 1;
    g_isSkipFrame = 1;

    memset(&lift_master, 0, sizeof lift_master);
    lift_master.ops = &lift_master_ops.base;
    lift_master.pos.x = 100.0f;
    lift_master.pos.y = 5.0f;
    lift_master.pos.z = 200.0f;
    lift_master.roty = 123.0f;
    lift_master.argb = 0xffffffffu;
    lift_master.drawn = 1;
    lift_master.visible = 1;
    lift_master.revealed = 1;
    lift_effect.base.pos = lift_master.pos;
    lift_effect.master_roty = 123.0f;
    lift_prim_count = 0;
}

int main(int argc, char **argv)
{
    const char *exe = argc > 1 ? argv[1] : getenv("ORACLE_EXE");
    process_fn process;
    int effect_id, option, frame, compared = 0, i;
    unsigned mismatches = 0, faults = 0, reported = 0, recorded = 0, prims_compared = 0, steps_compared = 0;
    const int verbose = getenv("ORACLE_VERBOSE") != NULL;
    const int only = getenv("ORACLE_ONLY") ? atoi(getenv("ORACLE_ONLY")) : -1;
    const unsigned max_reported = getenv("ORACLE_REPORT") ? (unsigned)atoi(getenv("ORACLE_REPORT")) : MAX_REPORTED;

    setvbuf(stdout, NULL, _IONBF, 0);
    if (getenv("ORACLE_RECORD_DUMP"))
        record_dump = fopen(getenv("ORACLE_RECORD_DUMP"), "w");
    if (!exe) {
        fprintf(stderr, "usage: test_rag_effect_process <original exe>\n");
        return 2;
    }
    if (!oracle_load(exe))
        return 2;
    if (getenv("ORACLE_FIRST_LOOK"))
        AddVectoredExceptionHandler(1, first_look);
    orig_prims =(unsigned char *)malloc((size_t)PRIMS * ORIG_PRIM_SIZE);
    lift_prims = (struct EffectPrim *)malloc((size_t)PRIMS * sizeof *lift_prims);
    if (!orig_prims || !lift_prims) {
        fprintf(stderr, "no memory for the prim pools\n");
        return 2;
    }
    for (i = 0; i < ORIG_RAG_VTABLE_SLOTS; i++)
        orig_rag_vt[i] = ((void **)oracle_addr(ORIG_RAG_VTABLE))[i];
    orig_rag_vt[ORIG_RAG_VT_SEND_MSG] = (void *)o_rag_send_msg;
    for (i = 0; i < ORIG_PRIM_VTABLE_SLOTS; i++)
        orig_prim_vt[i] = ((void **)oracle_addr(ORIG_PRIM_VTABLE))[i];
    orig_prim_vt[ORIG_ACTOR_VT_SEND_MSG] = (void *)o_actor_send_msg;
    lift_prim_ops.base.send_msg = lift_master_send;
    lift_prim_ops.set_action = lift_master_set_action;
    lift_prim_ops.get_effect_state = lift_master_effect_state;
    orig_master_vt[0] = ((void **)oracle_addr(ORIG_RAG_VTABLE))[-1];
    for (i = 0; i < 64; i++) {
        orig_master_vt[i + 1] = traps[i];
        orig_mode_vt[i] = traps[i];
    }
    orig_master_vt[1 + ORIG_ACTOR_VT_SEND_MSG] = (void *)o_actor_send_msg;
    orig_master_vt[1 + ORIG_ACTOR_VT_SET_ACTION] = (void *)o_set_action;
    orig_master_vt[1 + ORIG_ACTOR_VT_EFFECT_STATE] = (void *)o_get_effect_state;
    orig_master_vt[1 + ORIG_ACTOR_VT_SET_STATE] = (void *)o_set_state;
    orig_mode_vt[ORIG_MODE_VT_SEND_MSG] = (void *)o_mode_send_msg;
    memset(orig_mode, 0, sizeof orig_mode);
    *(void **)orig_mode = orig_mode_vt;
    *(void **)oracle_addr(ORIG_CURRENT_MODE) = orig_mode;
    *(void **)(orig_mode + ORIG_MODE_WORLD) = orig_world;
    *(void **)(orig_mode + ORIG_MODE_VIEW) = orig_view;
    *(float *)(orig_view + ORIG_VIEW_LONGITUDE) = CAMERA_HEADING;
    *(float *)(orig_view + ORIG_VIEW_LATITUDE) = CAMERA_PITCH;
    *(void **)(orig_world + ORIG_WORLD_PLAYER) = orig_player;
    *(void **)(orig_world + ORIG_WORLD_ATTR) = orig_attr;
    ((float *)(orig_player + 0x10))[0] = 300.0f;
    ((float *)(orig_player + 0x10))[1] = 0.0f;
    ((float *)(orig_player + 0x10))[2] = 400.0f;
    lift_player.pos.x = 300.0f;
    lift_player.pos.y = 0.0f;
    lift_player.pos.z = 400.0f;
    lift_view.longitude = CAMERA_HEADING;
    lift_view.latitude = CAMERA_PITCH;
    for (i = 0; i < FAKE_ACTIONS; i++) {
        orig_actions[i][0] = 0;
        orig_actions[i][1] = orig_actions[i][2] = (int)(fake_motions((unsigned)i) * ORIG_MOTION_SIZE);
    }
    *(void **)(orig_actres + ORIG_ACTRES_ACTIONS) = orig_actions;
    *(void **)(orig_actres + ORIG_ACTRES_ACTIONS + 4) = orig_actions + FAKE_ACTIONS;
    lift_renderer.proj[0] = 1024.0f;
    g_renderer = &lift_renderer;
    lift_master_ops.base.send_msg = lift_master_send;
    lift_master_ops.set_action = lift_master_set_action;
    lift_master_ops.get_effect_state = lift_master_effect_state;
    lift_mode.base.ops = &lift_mode_ops;
    math_init_degree_tables();
    memcpy(oracle_addr(ORIG_RADIAN_TABLE), g_radianTable, sizeof g_radianTable);
    memcpy(oracle_addr(ORIG_NEG_RADIAN_TABLE), g_negRadianTable, sizeof g_negRadianTable);
    memcpy(oracle_addr(ORIG_SIN_TABLE), g_sinTable, sizeof g_sinTable);
    memcpy(oracle_addr(ORIG_NEG_SIN_TABLE), g_negSinTable, sizeof g_negSinTable);
    memcpy(oracle_addr(ORIG_COS_TABLE), g_cosTable, sizeof g_cosTable);
    memcpy(oracle_addr(ORIG_NEG_COS_TABLE), g_negCosTable, sizeof g_negCosTable);
    memcpy(oracle_addr(ORIG_DURATIONS), g_effectDurations, sizeof g_effectDurations);

    oracle_patch_jump(ORIG_LAUNCH_PRIM, (void *)o_launch_prim);
    oracle_patch_jump(ORIG_PLAY_WAVE, (void *)o_play_wave);
    oracle_patch_jump(ORIG_GET_GAME_MODE, (void *)o_get_mode);
    oracle_patch_jump(ORIG_ATTR_HEIGHT, (void *)o_height);
    oracle_patch_jump(ORIG_LOAD_ACT, (void *)o_load_act);
    oracle_patch_jump(ORIG_LOAD_SPR, (void *)o_load_spr);
    oracle_patch_jump(ORIG_TEXTURE_GET, (void *)o_texture_get);
    oracle_patch_jump(ORIG_RES_MGR, (void *)o_res_mgr);
    oracle_patch_jump(ORIG_RES_GET, (void *)o_res_get);
    oracle_patch_jump(ORIG_MESSAGE_BOX, (void *)o_message_box);
    oracle_patch_jump(ORIG_SET_ACTION_BASE, (void *)o_set_action_base);
    oracle_patch_jump(ORIG_PLAY_MOTION, (void *)o_motion);
    oracle_patch_jump(ORIG_SPAWN, (void *)o_spawn);
    oracle_patch_jump(ORIG_ACTOR_BY_AID, (void *)o_actor_by_aid);
    oracle_patch_jump(ORIG_ACTOR_OR_PLAYER, (void *)o_actor_by_aid);
    oracle_patch_jump(ORIG_PROJECT, (void *)o_project);
    oracle_patch_jump(ORIG_PROJECT_VERTEX, (void *)o_project_vertex);
    oracle_patch_jump(ORIG_GET_JOB, (void *)o_get_job);
    oracle_patch_jump(ORIG_STR_REWIND, (void *)o_str_rewind);
    oracle_patch_jump(ORIG_LUA_CALL, (void *)o_lua_call);
    oracle_patch_jump(ORIG_LAUNCH_EFFECT, (void *)o_launch_effect);
    oracle_patch_jump(ORIG_NEW, (void *)o_new);
    oracle_patch_jump(ORIG_NEW_2, (void *)o_new);
    oracle_patch_jump(ORIG_NEW_3, (void *)o_new);
    trace = getenv("ORACLE_TRACE") != NULL;
    oracle_bind_import("WINMM.dll", "timeGetTime", (void *)o_time);
    process = (process_fn)oracle_addr(ORIG_ON_PROCESS);

    for (option = 0; option < 2; option++) {
        for (effect_id = 0; effect_id < ID_LIMIT; effect_id++) {
            int frames, failed = 0;
            char report[256];

            if ((only >= 0 && effect_id != only) || makes_game_objects(effect_id))
                continue;
            compared++;
            reset_fakes(effect_id, option);
            frames = lift_effect.duration < FRAMES ? lift_effect.duration : FRAMES;
            if (frames < 1)
                frames = 1;
            srand(1);
            for (frame = 0; frame < frames && !failed; frame++) {
                unsigned char orig_result = 0, lift_result;
                char orig_text[EVENTS * EVENT_LEN], lift_text[EVENTS * EVENT_LEN];
                unsigned orig_seed_state = (unsigned)rand();

                if (verbose) { fprintf(stderr, "option %d effect %d frame %d\n", option, effect_id, frame); fflush(stderr); }
                srand(orig_seed_state);
                event_count = 0;
                overflow = 0;
                if (!call_original(process, &orig_result)) {
                    faults++;
                    failed = 1;
                    if (reported++ < max_reported)
                        printf("  option %d effect %d frame %d: the original faulted (code %08x at 0x%x)\n", option, effect_id, frame, fault_code, fault_va);
                    break;
                }
                recorded += event_count;
                joined(orig_text, sizeof orig_text);

                srand(orig_seed_state);
                event_count = 0;
                overflow = 0;
                lift_result = rag_effect_on_process((struct GameObject *)&lift_effect);
                joined(lift_text, sizeof lift_text);

                report[0] = 0;
                if (orig_result != lift_result)
                    snprintf(report, sizeof report, "result: lift %d original %d", lift_result, orig_result);
                else if (strcmp(orig_text, lift_text) != 0)
                    snprintf(report, sizeof report, "events differ");
                else if (lift_prim_count != orig_prim_count)
                    snprintf(report, sizeof report, "prims: lift %d original %d", lift_prim_count, orig_prim_count);
                else if (!base_differs("master", (unsigned char *)&lift_master, orig_master, report, sizeof report) &&
                         !base_differs("effect", (unsigned char *)&lift_effect, orig_effect, report, sizeof report))
                    map_differs("effect", effect_map, sizeof effect_map / sizeof *effect_map, (unsigned char *)&lift_effect, orig_effect, report,
                                sizeof report);
                if (report[0]) {
                    mismatches++;
                    failed = 1;
                    if (reported++ < max_reported) {
                        printf("  option %d effect %d frame %d: %s\n", option, effect_id, frame, report);
                        if (strcmp(orig_text, lift_text) != 0)
                            printf("    original: %s\n    lift:     %s\n", orig_text, lift_text);
                    }
                }
            }
            if (failed)
                continue;
            if (record_dump) {
                for (i = 0; i < lift_prim_count; i++) {
                    const unsigned char *lr = (const unsigned char *)&lift_prims[i].records, *orr = orig_prims + (size_t)i * ORIG_PRIM_SIZE + 0x3e8;
                    unsigned b;
                    fprintf(record_dump, "%d %d %d ", effect_id, option, lift_prims[i].kind);
                    for (b = 0; b < sizeof(union PrimRecords); b++)
                        fprintf(record_dump, "%02x", lr[b]);
                    fputc(' ', record_dump);
                    for (b = 0; b < 0x2d0; b++)
                        fprintf(record_dump, "%02x", orr[b]);
                    fputc('\n', record_dump);
                }
            }
            for (i = 0; i < lift_prim_count; i++) {
                char where[16];
                snprintf(where, sizeof where, "p%d", i);
                prims_compared++;
                if (base_differs(where, (unsigned char *)&lift_prims[i], orig_prims + (size_t)i * ORIG_PRIM_SIZE, report, sizeof report) ||
                    map_differs(where, prim_map, sizeof prim_map / sizeof *prim_map, (unsigned char *)&lift_prims[i],
                                orig_prims + (size_t)i * ORIG_PRIM_SIZE, report, sizeof report)) {
                    mismatches++;
                    failed = 1;
                    if (reported++ < max_reported)
                        printf("  option %d effect %d: %s\n", option, effect_id, report);
                    break;
                }
            }
            if (failed)
                continue;
            for (i = 0; i < lift_prim_count && !failed; i++) {
                unsigned char *orig_prim = orig_prims + (size_t)i * ORIG_PRIM_SIZE;
                int step;
                char where[24];
                for (step = 0; step < PRIM_STEPS && !failed; step++) {
                    unsigned char orig_result = 0, lift_result;
                    char orig_text[EVENTS * EVENT_LEN], lift_text[EVENTS * EVENT_LEN];
                    unsigned seed_state = (unsigned)rand();

                    snprintf(where, sizeof where, "p%d step %d", i, step);
                    srand(seed_state);
                    event_count = 0;
                    overflow = 0;
                    if (!step_original(orig_prim, &orig_result)) {
                        faults++;
                        failed = 1;
                        if (reported++ < max_reported)
                            printf("  option %d effect %d %s: the original faulted (code %08x at 0x%x)\n", option, effect_id, where, fault_code, fault_va);
                        break;
                    }
                    joined(orig_text, sizeof orig_text);
                    srand(seed_state);
                    event_count = 0;
                    overflow = 0;
                    lift_result = effect_prim_on_process((struct GameObject *)&lift_prims[i]);
                    joined(lift_text, sizeof lift_text);
                    steps_compared++;
                    report[0] = 0;
                    if (orig_result != lift_result)
                        snprintf(report, sizeof report, "result: lift %d original %d", lift_result, orig_result);
                    else if (strcmp(orig_text, lift_text) != 0)
                        snprintf(report, sizeof report, "events differ");
                    else if (!base_differs(where, (unsigned char *)&lift_prims[i], orig_prim, report, sizeof report))
                        map_differs(where, prim_map, sizeof prim_map / sizeof *prim_map, (unsigned char *)&lift_prims[i], orig_prim, report, sizeof report);
                    if (report[0]) {
                        mismatches++;
                        failed = 1;
                        if (reported++ < max_reported) {
                            printf("  option %d effect %d %s: %s\n", option, effect_id, where, report);
                            if (strcmp(orig_text, lift_text) != 0)
                                printf("    original: %s\n    lift:     %s\n", orig_text, lift_text);
                        }
                    }
                    if (!orig_result)
                        break;
                }
            }
        }
    }
    printf("Rag effect process: %d effect runs compared against the original (%u events recorded, %u prims compared, %u prim steps), %u mismatches, %u faults%s\n",
           compared, recorded, prims_compared, steps_compared, mismatches, faults, reported > max_reported ? " (not all shown)" : "");
    return mismatches || faults ? 1 : 0;
}
