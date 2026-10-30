#include "actor/object_ops.h"
#include "actor/render_ops.h"
#include "resources/sprite.h"
#include "resources/animation.h"
#include <assert.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "game/ground_item.h"
#include "replay/snapshot.h"

struct SprClip;
struct SprImg;
struct SqQuad;
struct PcActor;
struct SprClip;
struct SpriteQuad;

static int fail_object, objects, strings, constructed, destroyed;
static unsigned char *object_storage;
static size_t object_size;
static void *allocate_object(size_t size)
{
    if (fail_object) return NULL;
    assert(!objects); object_storage=malloc(size+16); assert(object_storage);
    object_size=size; memset(object_storage,0xa7,size+16); ++objects;
    return object_storage+8;
}
static void release_object(void *p)
{
    size_t i;
    assert(objects==1 && p==object_storage+8);
    for(i=0;i<8;++i) assert(object_storage[i]==0xa7 && object_storage[8+object_size+i]==0xa7);
    free(object_storage);object_storage=NULL;--objects;
}
#define malloc allocate_object
#define free release_object
#include "../src/game/ground_item_object.c"
#undef malloc
#undef free
#include "../src/game/ground_items.c"
#include "../src/game/cell_coordinates.c"

struct Session g_session1;
struct EffectMgr *g_effectMgr;
struct Renderer *g_renderer;
struct ActorPickNode g_actorPickNode;
static struct GameMode mode;
static struct World world;
static struct C3dAttr attr;
static struct ListNode ih,oh;
static const char *format="[%d]";
static int sprites,actions,effects;

void *mem_alloc(size_t size) { void *p=malloc(size);assert(p);++strings;return p; }
void mem_free(void *p)
{ if(p){--strings;free(p);} }
void fatal_error(const char *s) { (void)s;abort(); }
struct AbleToMakeEffect *able_to_make_effect_construct(struct AbleToMakeEffect *p)
{ memset(p,0,sizeof *p);++constructed;return p; }
void able_to_make_effect_dc_able_to_make_effect(struct AbleToMakeEffect *p) { assert(p);++destroyed; }
struct GameMode *game_current_mode(void) { return &mode; }
float c3d_attr_get_height(struct C3dAttr *p,float x,float z) { assert(p==&attr);(void)x;(void)z;return 10.0f; }
const char *session_get_item_display_name(const char *name,int identified)
{ assert(name && (identified==0 || identified==1));return "test-item"; }
struct String *item_info_get_display_name(const struct ItemInfo *item,struct String *out)
{ assert(!strcmp(string_cstr(&item->id_text),"501"));string_assign_cstr(out, "an item name");return out; }
char *msg_str(int id) { assert(id==0xb7);return (char *)format; }
void render_object_load_spr_res(struct RenderObject *p,const char *name)
{ assert(p && !strcmp(name,"test-item.spr"));++sprites; }
void render_object_load_act_res(struct RenderObject *p,const char *name)
{ assert(p && !strcmp(name,"test-item.act"));++actions; }
void *render_object_get_act_res(struct RenderObject *p) { assert(p);return NULL; }
int item_info_effect_id(int id) { assert(id==501);return 5; }
struct Effect *effect_mgr_spawn(struct EffectMgr *mgr,int aid,int id,Vec3 pos,int a,int b,unsigned char c,int d)
{ (void)mgr;assert(aid==42 && (id==5 || id==0x74d));assert(pos.y==10 && !a && b==-1 && !c && !d);++effects;return (struct Effect *)1; }
struct RagEffect *game_actor_launch_effect(struct GameActor *p,int id,Vec3 pos,float rot)
{ (void)p;(void)id;(void)pos;(void)rot;abort(); }
struct Item *world_get_item_by_aid(struct World *w,int aid)
{ struct ListNode *n;assert(w==&world);for(n=ih.next;n!=&ih;n=n->next)if((*LIST_VALUE(n,struct Item *))->aid==aid)return *LIST_VALUE(n,struct Item *);return NULL; }
void *list_insert(struct List *l,struct ListNode *where,const void *value,size_t size)
{ struct ListNode *n=malloc(sizeof *n+size);assert(l && n && value);memcpy(n+1,value,size);n->next=where;n->prev=where->prev;where->prev->next=n;where->prev=n;++l->size;return n+1; }
BYTE world_remove_item(struct World *w,int aid) { (void)w;(void)aid;abort(); }
void game_mode_remove_pickup_request(struct GameMode *m, unsigned aid) { (void)m;(void)aid;abort(); }

void able_to_make_effect_send_msg(struct GameObject *object, struct GameObject *source,
    RenderMsgArg msg, RenderMsgArg a, RenderMsgArg b, RenderMsgArg c, RenderMsgArg d)
{
    struct AbleToMakeEffect *p = (struct AbleToMakeEffect *)object;
    struct GameActor *sender = (struct GameActor *)source;
 (void)p;(void)sender;(void)msg;(void)a;(void)b;(void)c;(void)d;abort(); }
void render_object_render_shadow(struct RenderObject *p,const float *matrix,float scale)
{ (void)p;(void)matrix;(void)scale;abort(); }
void render_object_render_sprite(struct RenderObject *p,const float *matrix,unsigned char frame,float angle)
{ (void)p;(void)matrix;(void)frame;(void)angle;abort(); }
static struct PickInfo last_pick;
static int pick_count, position_count;
float renderer_scale_x(struct Renderer *p) { assert(p==g_renderer);return 1; }
void actor_pick_node_insert(struct ActorPickNode *p,struct PickInfo *box)
{ assert(p==&g_actorPickNode);last_pick=*box;++pick_count;mem_free(box); }

int game_object_dir_of(struct GameObject *self, float roty) { abort(); }
void render_object_set_screen_position(struct RenderObject *self, float x, float y)
{ assert(self && x==12.5f && y==24.5f);++position_count; }
void render_object_set_action_base(struct RenderObject *self, int action, int type) { abort(); }
void render_object_update_motion_base(struct RenderObject *self) { abort(); }
int render_object_return_zero(struct RenderObject *self) { abort(); }

int main(void)
{
    struct Item *item;
    struct PacketGroundItemEntry entry={0x9d,42,501,1,40,50,12,9,10};
    struct PacketGroundItemFall fall={0x9e,42,501,0,41,51,8,7,32767};
    struct PacketGroundItemFall4 fall4={0x84b,42,501,7,1,42,52,6,5,-2};
    struct ItemDropInfo info={0};
    mode.world=&world;world.attr=&attr;attr.width=100;attr.height=100;attr.zoom=5;
    world.items.head=&ih;ih.next=ih.prev=&ih;
    world.gameObjects.head=&oh;oh.next=oh.prev=&oh;
    fail_object=1;assert(!item_create());game_mode_item_entry(&mode,&entry);
    assert(!objects && !constructed && !world.items.size && !world.gameObjects.size);
    fail_object=0;format="%s %d pieces";game_mode_item_entry(&mode,&entry);
    item=*LIST_VALUE(ih.next,struct Item *);
    assert(object_size==sizeof *item && objects==1 && constructed==1);
    assert(world.items.size==1 && world.gameObjects.size==1 && *LIST_VALUE(oh.next,struct Item *)==item);
    assert(item->count==12 && item->aid==42 && item->item_id==501 && item->item_id_short==501 && item->identified==1);
    assert(item->base.pos.x==-47.1875f && item->base.pos.z==3.125f && item->base.pos.y==10.0f);
    assert(!strcmp(string_cstr(&item->name),"an item name 12 pieces"));
    format="%s / quantity=%d";game_mode_item_fall_entry(&mode,&fall);
    assert(item->count==32767 && !item->identified && item->falling && item->base.pos.y==-5.0f);
    game_mode_item_fall_entry4(&mode,&fall4);
    assert(item->count==-2 && item->type==7 && item->identified==1);
    strcpy(info.item_id_text,"501");info.aid=42;info.count=99999;info.x=45;info.y=45;info.is_identified=1;
    info.show_drop_effect=1;info.drop_effect_mode=1;item_place(item,&mode,&info);
    assert(item->count==99999 && effects==2 && !item->falling);
    assert(constructed==1 && sprites==4 && actions==4 && world.items.size==1);
    {
        struct ScreenBounds bounds={20,30,10,40,0.5f};
        struct RenderObject *object=(struct RenderObject *)&item->base;
        item->base.pickable=1;
        render_object_render_body(object,&bounds,12.5f,24.5f);
        assert(pick_count==1 && position_count==1);
        assert(last_pick.left==-2 && last_pick.right==32);
        assert(last_pick.top==18 && last_pick.bottom==52);
        assert(last_pick.near_z==300.5f && last_pick.far_z==300.5f);
        assert(last_pick.aid==42 && last_pick.kind==1 && last_pick.job==0x7d01);
        item->base.pickable=0;
        render_object_render_body(object,&bounds,12.5f,24.5f);
        assert(pick_count==1 && position_count==2);
    }
    item_deleting_destroy((struct GameObject *)item);assert(!objects && !strings && destroyed==1);
    free(ih.next);free(oh.next);
    puts("ground items: complete allocation, failed creation, typed packet fields, stored quantity, label changes, placement, effects and destruction passed");
    return 0;
}
