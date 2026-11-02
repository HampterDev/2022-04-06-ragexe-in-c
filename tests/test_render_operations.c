#define GRANNY_IMPORT
#include "actor/render_ops.h"
#include "actor/sprite_render.h"
#include "actor/granny_actor.h"
#include "actor/granny_actor.h"
#include "contents/body_layer.h"
#include "game/context.h"
#include "game/own_states.h"
#include "fileio/resource_api.h"
#include "resources/animation.h"
#include "render/math3d.h"
#include "render/projection.h"
#include "session/state.h"
#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

struct Session g_session1;
struct Renderer *g_renderer;
float g_sinTable[361];
static DWORD tick;
static float camera_scale = 2.0f, sampled_time;
static int finished, samples, plays, use_hover;
static unsigned draws;
static struct SqQuad submitted[16];
static int draw_flags[16];
static struct RenderObject *expected_object;
static const int rect[4] = {10,20,30,40};
static struct SprClip clip;
static struct SprImg image;
static int palette;

DWORD render_test_time(void) { return tick; }
float game_camera_scale(void) { return camera_scale; }
struct GameMode *game_current_mode(void) { return NULL; }
struct ResMgr *g_resMgr(void) { abort(); }
void *res_mgr_get(struct ResMgr *m,const char *s) { (void)m;(void)s;abort(); }
unsigned actres_frame_count(const struct ActRes *r,unsigned a)
{ assert(r && a==11); return 5; }
const struct OwnStateEntry *own_states_find(int state) { assert(state==0xb8); return NULL; }
BYTE effect_state_bit6(DWORD state) { (void)state;abort(); }
BYTE effect_state_bit15(DWORD state) { assert(state==123);return (BYTE)use_hover; }
unsigned short render_object_hover_height(struct RenderObject *o)
{ assert(o==expected_object);return 0x8001; }
float renderer_screen_scale(struct Renderer *r,float z) { (void)r;(void)z;abort(); }
float renderer_scale_x(struct Renderer *r) { (void)r;abort(); }
double session_get_baby_job_scale(void *s,int j) { (void)s;(void)j;abort(); }
int game_actor_get_job(struct GameActor *a) { (void)a;abort(); }
void render_object_submit_spr_quad(struct RenderObject *o,struct SqQuad *q,
    const void *p,int flags,float x,float y)
{
    assert(o==expected_object && p==&palette && x==0 && y==0 && draws<16);
    submitted[draws]=*q;draw_flags[draws++]=flags;
}
GrannyClock __stdcall _GrannyGetSystemSeconds(void) { GrannyClock c={{0}};return c; }
float __stdcall _GrannyGetSecondsElapsed(const GrannyClock *a,const GrannyClock *b)
{ assert(a&&b);return 0.25f; }
int granny_playback_finished(const struct GrannyPlayback *p,int a)
{ assert(p&&a==2);return finished; }
struct granny_world_pose *granny_playback_pose(struct GrannyPlayback *p,float t)
{ assert(p);sampled_time=t;samples++;return NULL; }
int granny_playback_play(struct GrannyPlayback *p,int a,int loops)
{ assert(p&&a==0&&loops==0);plays++;return 1; }
static int direction(struct GameObject *o,float angle) { assert(o==(struct GameObject *)expected_object&&angle==90);return 3; }
int game_actor_get_effect_state(struct RenderObject *o) { assert(o==expected_object);return 123; }
static void callers(void)
{
    struct RenderObject object={0};struct Pc pc={0};
    struct SqQuad q={0};int flags=5;
    object.overlay_flags=0x200;expected_object=&object;
    render_object_draw_clip_overlays(&object,&q,&clip,rect,2,3,4,&palette,&image,&flags,0xffffffff);
    assert(object.ring_phase==1&&q.color==0x96ffffff&&!draws);
    pc.overlay_flags=0x200;expected_object=(struct RenderObject *)&pc;
    use_hover=1;
    q.top=10;q.bottom=20;q.color=0;
    pc_draw_layer_overlays(&pc,&q,&clip,6,rect,2,1,3,4,0,0,&palette,&image,&flags,0,0);
    assert(pc.ring_phase==1&&q.color==0x96ffffff);
    assert(pc.hover_height==(short)0x8001&&q.top==32777&&q.bottom==32787&&!draws);
    use_hover=0;
}
static void overlays(void)
{
    struct RenderObject object={0};struct SqQuad q={0};unsigned i;
    expected_object=&object;object.overlay_flags=0x1000;object.overlay_step=2;
    draws=0;render_object_draw_echoes(&object,&clip,rect,2,3,4,&palette,&image,4,-3);
    assert(draws==4&&submitted[0].x0==28&&submitted[1].x0==-4);
    assert(submitted[2].top==39&&submitted[3].top==7);
    for(i=0;i<4;i++)assert(submitted[i].color==0x8cffffff&&draw_flags[i]==0x201&&submitted[i].image==&image);
    object.overlay_flags=0x200;object.ring_phase=199;draws=0;
    render_object_draw_rings(&object,&q,&clip,rect,2,3,4,&palette,&image,7,-3);
    assert(draws==4&&!object.ring_phase&&q.color==0x96ffffff);
    assert(submitted[0].top==13&&submitted[0].x0==2&&submitted[0].color==0x4bffffff);
    draws=0;render_object_draw_rings(&object,&q,&clip,rect,2,3,4,&palette,&image,5,0);
    assert(!draws&&object.ring_phase==1&&q.color==0x96ffffff);
    object.overlay_flags=0x2000;
    render_object_jitter_quad(&object,&q,rect,2,0,5);
    assert(q.top==10&&q.bottom==30&&q.x0==2&&q.x1==22);
    object.overlay_flags=0;q.top=777;
    render_object_jitter_quad(&object,&q,rect,2,11,5);assert(q.top==777);
}
static void motions(void)
{
    struct RenderObjectOps ops={0};struct RenderObject object={0};
    struct C3dGrannyGameActor actor={0};struct GrannyModelState model={0};
    ops.base.get_direction=direction;ops.update_motion=render_object_update_motion_base;
    object.ops=&ops.base;object.roty=90;object.base_action=8;object.motion_delay=2;
    object.act=(struct ActRes *)&object;expected_object=&object;tick=288;
    render_object_update_motion(&object);assert(object.action==11&&object.motion==1&&!object.motion_finished);
    object.play_mode=1;object.plays=1;render_object_update_motion(&object);
    assert(object.motion==4&&object.motion_finished);
    object.motion=20;render_object_update_motion(&object);assert(object.motion==4);
    ops.update_motion=c3d_granny_game_actor_update_motion;actor.ops=&ops.base;
    actor.state=2;actor.attack_delay_factor=0.5f;
    render_object_update_motion((struct RenderObject *)&actor);assert(!actor.motion_finished&&!samples);
    model.playback=(struct GrannyPlayback *)&model;actor.model=&model;actor.anim=2;actor.anim_time=3;
    finished=1;render_object_update_motion((struct RenderObject *)&actor);
    assert(samples==1&&sampled_time==4&&plays==1&&actor.anim==0&&actor.attack_delay_factor==1);
    render_object_update_motion((struct RenderObject *)&actor);assert(samples==1&&!actor.motion_finished);
    actor.state=0;render_object_update_motion((struct RenderObject *)&actor);assert(actor.motion_finished);
}
int main(void)
{
    callers();overlays();motions();
    puts("Render operations: real overlay callers, signed hover, geometry and two motion targets passed");
    return 0;
}
