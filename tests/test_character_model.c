#include <assert.h>
#include <math.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ui/render/character_render.c>
#include "character_model_unreached.h"
#ifdef CHARACTER_MODEL_REFERENCE
#define CharacterRender CCharacterRender
#define MODEL_ROBE CCharacterRender_SetRobe
#define MODEL_WORLD CCharacterRender_RenderWorld
#define MODEL_LIST_FREE(list) std_list_destroy(list)
static struct CharacterRender *model_create(int w, int h)
{
    struct CharacterRender *p = mem_alloc(sizeof *p);
    return p ? CCharacterRender_Construct(p, w, h) : NULL;
}
static void model_destroy(struct CharacterRender *p)
{
    CCharacterRender_Destroy(p, 1);
}
#else
#define model_create character_render_create
#define model_destroy character_render_destroy
#define MODEL_ROBE character_render_set_robe
#define MODEL_WORLD character_render_collect_world_quads
#define MODEL_LIST_FREE(list) list_free(list, NULL)
#endif

static struct {
    unsigned char *p;
    size_t n;
    unsigned id;
} blocks[512];
static unsigned live_blocks, allocation_id, fail_next, scenario, trace[100000], trace_count;
static unsigned prim_count, lua_fail, have_view = 1;
static struct Renderer renderer;
struct Renderer *g_renderer = &renderer;
struct Session g_session1;
float g_viewDistance = 400.0f, g_spriteLayerDepthStep = 9e-6f, g_spriteScaleDepthBias = 3.6e-5f;
float g_spriteScaleDivisor = 1.34f, g_spriteRowDepthStep = 0.001f, g_depthPerPixelCot = 0.001f;
static struct SprRes sprites[9];
static struct PaletteRes palettes[9];
static struct SprImg images[9];
static struct Motion motions[9];
static struct SprClip clips[9][2];
static struct AttachPointInfo anchors[9];
static int actions[9], textures[9], manager;
static struct TexDrawPrim prims[64];
static struct TexDrawVertex vertices[64][4];

static void note(unsigned n)
{
    assert(trace_count < 100000);
    trace[trace_count++] = n;
}
static void note_float(float f)
{
    unsigned n;
    memcpy(&n, &f, sizeof n);
    note(n);
}
void *mem_alloc(size_t n)
{
    unsigned i;
    unsigned char *p;
    if (fail_next) {
        fail_next = 0;
        return NULL;
    }
    p = malloc(n + 16);
    assert(p);
    memset(p, 0xa5, n + 16);
    for (i = 0; i < 512 && blocks[i].p; ++i) {
    }
    assert(i < 512);
    blocks[i].p = p;
    blocks[i].n = n;
    blocks[i].id = ++allocation_id;
    ++live_blocks;
    note(800);
    note(blocks[i].id);
    return p;
}
void mem_free(void *p)
{
    unsigned i, j;
    if (!p)
        return;
    for (i = 0; i < 512 && blocks[i].p != p; ++i) {
    }
    assert(i < 512);
    for (j = 0; j < 16; ++j)
        assert(blocks[i].p[blocks[i].n + j] == 0xa5);
    note(801);
    note(blocks[i].id);
    free(p);
    blocks[i].p = NULL;
    --live_blocks;
}
__declspec(noreturn) void fatal_error(const char *message)
{
    fprintf(stderr, "%s\n", message);
    abort();
}

static struct String *name_for(struct String *o, int layer, int kind, int a, int b, int c,
                                  int d)
{
    char s[80];
    note(100);
    note(layer);
    note(kind);
    note(a);
    note(b);
    note(c);
    note(d);
    snprintf(s, sizeof s, "fixture/character/model/layer/%d/kind/%d", layer, kind);
    string_init(o);
    string_assign_cstr(o, s);
    return o;
}
struct String *session_get_imf_name(struct String *o, int j, int s)
{
    return name_for(o, 0, 3, j, s, 0, 0);
}
struct String *session_get_body_spr_name_variant0(struct String *o, int j, int s, int b, int z)
{
    return name_for(o, 0, 1, j, s, b, z);
}
struct String *session_get_body_spr_name_variant1(struct String *o, int j, int s, int b, int z)
{
    return name_for(o, 0, 0, j, s, b, z);
}
struct String *session_get_body_spr_name(struct String *o, int kind, int j2, int j, int s,
                                          int z)
{
    note(j2);
    return name_for(o, 0, !kind, j, s, 0, z);
}
struct String *session_get_body_pal_name(struct String *o, int j, int s, int p, int b, int z)
{
    note(b);
    return name_for(o, 0, 2, j, s, p, z);
}
struct String *session_get_head_spr_name0(struct String *o, int j, int s, int *h)
{
    return name_for(o, 1, 1, j, s, *h, 0);
}
struct String *session_get_head_spr_name1(struct String *o, int j, int s, int *h)
{
    return name_for(o, 1, 0, j, s, *h, 0);
}
struct String *session_get_head_pal_name(struct String *o, int h, int j, int s, int p)
{
    return name_for(o, 1, 2, j, s, h, p);
}
struct String *session_get_res_name0(struct String *o, int j, int s, int h, int i)
{
    return name_for(o, i % 100, 1, j, s, h, i);
}
struct String *session_get_res_name1(struct String *o, int j, int s, int h, int i)
{
    return name_for(o, i % 100, 0, j, s, h, i);
}
static void *resource(const char *name)
{
    int l, k;
    assert(sscanf(name, "fixture/character/model/layer/%d/kind/%d", &l, &k) == 2);
    assert(l >= 0 && l < 9 && k >= 0 && k <= 3);
    note(200);
    note(l);
    note(k);
    if (k == 3 || scenario % 8 == 7)
        return NULL;
    return k == 0 ? (void *)&sprites[l] : k == 1 ? (void *)&actions[l] : (void *)&palettes[l];
}
struct ResMgr *g_resMgr(void)
{
    return (struct ResMgr *)&manager;
}
void *res_mgr_get_string(struct ResMgr *m, struct String name)
{
    void *p;
    assert(m == g_resMgr());
    p = resource(string_cstr(&name));
    string_free(&name);
    return p;
}
void *res_mgr_get(struct ResMgr *m, const char *name)
{
    assert(m == g_resMgr());
    return resource(name);
}
int session_get_mounted_job(int j)
{
    note(300);
    note(j);
    return j + 1000;
}
float session_get_baby_job_scale(struct Session *s, int j)
{
    assert(s == &g_session1);
    note(301);
    note(j);
    return .75f;
}
BYTE session_is_head_replacing_view(struct Session *s, int item)
{
    (void)s;
    return scenario % 4 == 2 && item == 102;
}
BYTE session_is_top_layer_robe(struct Session *s, int robe)
{
    (void)s;
    (void)robe;
    return 0;
}
char layer_is_ignored_riding_state(int item, int job, int base, char riding)
{
    note(302);
    note(item);
    note(job);
    note(base);
    return riding;
}
int layer_body_attach_dy(struct Motion *m, int a, int f, int j, int s, int b)
{
    assert(m);
    note(303);
    note(a);
    note(f);
    note(j);
    note(s);
    note(b);
    return 4;
}
int client_lua_call_table(struct ClientLua *lua, const char *table, const char *fn, const char *sig,
                          ...)
{
    va_list ap;
    int robe;
    BYTE *out;
    (void)lua;
    assert(!strcmp(table, "TB_Layer_Priority") && !strcmp(fn, "IsHelmRobe") && !strcmp(sig, "d>b"));
    va_start(ap, sig);
    robe = va_arg(ap, int);
    out = va_arg(ap, BYTE *);
    va_end(ap);
    note(400);
    note(robe);
    note(lua_fail);
    if (!lua_fail)
        *out = (BYTE)(robe != 0);
    return !lua_fail;
}
int client_lua_call(struct ClientLua *lua, const char *fn, const char *sig, ...)
{
    va_list ap;
    int i;
    (void)lua;
    va_start(ap, sig);
    if (!strcmp(fn, "GetLayerPriority")) {
        int dir = va_arg(ap, int), layer = va_arg(ap, int), item = va_arg(ap, int);
        note(401);
        note(dir);
        note(layer);
        note(item);
        *va_arg(ap, int *) = scenario % 3 == 0 ? 10 : 20 - layer;
    } else if (!strcmp(fn, "OffsetItemPos_GetUppingBottomItem")) {
        note(402);
        note(va_arg(ap, int));
        *va_arg(ap, BYTE *) = 0;
        *va_arg(ap, double *) = 0;
        *va_arg(ap, double *) = 0;
    } else {
        assert(!"unexpected Lua call");
    }
    va_end(ap);
    return 1;
}
struct Motion *act_res_get_motion(const struct ActRes *act, unsigned a, unsigned f)
{
    unsigned i;
    if (!act)
        return NULL;
    for (i = 0; i < 9 && (const void *)act != &actions[i]; ++i) {
    }
    assert(i < 9);
    note(500);
    note(i);
    note(a);
    note(f);
    return &motions[i];
}
struct SprClip *motion_get_clip(const struct Motion *m, unsigned i)
{
    assert(m && i < 2);
    return &m->clips[i];
}
unsigned actres_frame_count(const struct ActRes *act, unsigned a)
{
    (void)act;
    (void)a;
    return 4;
}
void *session_get_stepped_motion(void *master, void *slave, int a, int block, DWORD start, int *f)
{
    assert(master && start == 0);
    *f = block;
    return act_res_get_motion(slave, a, block);
}
int imf_layer_at(const struct ImfRes *imf, int order, int base, int action, int frame)
{
    (void)imf;
    note(501);
    note(order);
    note(base);
    note(action);
    note(frame);
    return 8 - order;
}

static void identity(float *m)
{
    unsigned i;
    for (i = 0; i < 12; ++i)
        m[i] = 0;
    m[0] = m[4] = m[8] = 1;
    m[11] = 100;
}
void mat_set_rotate_x(float *m, float a)
{
    note(600);
    note_float(a);
    identity(m);
}
void mat_rotate_y(float *m, float a)
{
    (void)m;
    note(601);
    note_float(a);
}
void mat_look_at(float *m, const Vec3 *eye, const Vec3 *target, const Vec3 *up)
{
    note(602);
    note_float(eye->x);
    note_float(eye->y);
    note_float(eye->z);
    note_float(target->x);
    note_float(target->y);
    note_float(target->z);
    note_float(up->y);
    identity(m);
}
int game_copy_view_matrix(float m[12])
{
    identity(m);
    return have_view;
}
void renderer_project_centred(struct Renderer *r, const Vec3 *p, const float *m, float *x,
                              float *y, float *rhw, float w, float h)
{
    assert(r == g_renderer);
    (void)m;
    note(603);
    note_float(w);
    note_float(h);
    note_float(p->z);
    *x = 128;
    *y = 96;
    *rhw = .01f;
}
void renderer_project_point(struct Renderer *r, const Vec3 *p, const float *m, float *x, float *y,
                            float *rhw)
{
    renderer_project_centred(r, p, m, x, y, rhw, 640, 480);
}
float renderer_project_depth(float rhw)
{
    return 1 - rhw;
}
float renderer_screen_scale(struct Renderer *r, float rhw)
{
    assert(r == g_renderer);
    note_float(rhw);
    return 1;
}
float get_cos_deg(int deg)
{
    return cosf(deg * .0174532925f);
}
float get_sin_deg(int deg)
{
    return sinf(deg * .0174532925f);
}
struct Texture *sprite_atlas_resolve(struct Renderer *r, const struct SprImg *img,
                                      const DWORD *palette, struct SpriteRegion *region)
{
    unsigned i;
    assert(r == g_renderer);
    for (i = 0; i < 9 && img != &images[i]; ++i) {
    }
    assert(i < 9);
    note(604);
    note(i);
    note(palette[0]);
    region->u1 = region->v1 = 1;
    return (struct Texture *)&textures[i];
}
struct TexDrawPrim *renderer_new_prim(struct Renderer *r)
{
    unsigned i = prim_count++;
    assert(r == g_renderer && i < 64);
    memset(&prims[i], 0, sizeof prims[i]);
    memset(vertices[i], 0, sizeof vertices[i]);
    prims[i].base.verts = vertices[i];
    return &prims[i];
}

static void trace_quads(struct List *list, int world)
{
    struct ListNode *n;
    unsigned seen = 0;
    note(700);
    note(world);
    note(list->size);
    for (n = list->head->next; n != list->head; n = n->next) {
        struct TexDrawPrim *quad;
        unsigned i, j;
        if (world) {
            struct ChrQuadPair *pair = LIST_VALUE(n, struct ChrQuadPair);
            note(pair->flags);
            quad = pair->quad;
        } else
            quad = *(struct TexDrawPrim **)((unsigned char *)n + sizeof(struct ListNode));
        assert(quad >= prims && quad < prims + prim_count);
        note(quad->base.srcBlend);
        note(quad->base.destBlend);
        for (i = 0; i < 9 && quad->base.texture != (struct Texture *)&textures[i]; ++i) {
        }
        assert(i < 9);
        note(i);
        for (j = 0; j < 4; ++j) {
            struct TexDrawVertex *v = &quad->base.verts[j];
            note_float(v->x);
            note_float(v->y);
            note_float(v->z);
            note_float(v->rhw);
            note(v->diffuse);
            note_float(v->u);
            note_float(v->v);
        }
        ++seen;
    }
    assert(seen == list->size);
    if (scenario % 8 == 7)
        assert(!seen);
    else
        assert(seen >= 2);
    MODEL_LIST_FREE(list);
}

static void run_case(unsigned n)
{
    struct CharacterRender *model;
    struct List list;
    BYTE drawn;
    int key;
    scenario = n;
    note(701);
    note(n);
    model = model_create(n % 2 ? 320 : 0, n % 2 ? 200 : 0);
    assert(model);
    assert(model->view_width == (n % 2 ? 320 : 0) && model->view_height == (n % 2 ? 200 : 0));
    assert(model->job2 == 0 && model->accessory_top == 0 && model->accessory_mid == 0 &&
           model->accessory_bottom == 0);
    assert(model->head_low_location == 0 && model->head_top_location == 0 && model->head_mid_location == 0 && model->riding == 0 &&
           model->body_style == 0);
    assert(model->centre_dx == 0 && model->centre_dy == 80 && model->body_flags == -1 &&
           model->body_x == -1 && model->body_y == -1);
    assert(model->distance == 400 && model->pitch == -45 && model->yaw == 0 && model->up[0] == 0 &&
           model->up[1] == 1 && model->up[2] == 0);
    assert(model->screen_x == 0 && model->screen_y == 0 && !model->layers.size && !model->robe &&
           !model->is_helm_robe);
    assert(!model->action && !model->motion && model->shadow == 1 && !model->pos[0] &&
           !model->pos[1] && !model->pos[2]);
    MODEL_ROBE(model, 108);
    assert(model->robe == 108 && model->is_helm_robe == 1);
    lua_fail = 1;
    MODEL_ROBE(model, 0);
    assert(!model->robe && model->is_helm_robe == 1);
    lua_fail = 0;
    MODEL_ROBE(model, n % 2 ? 108 : 0);
    model->job = n % 3 == 0 ? CHR_JOB_WEDDING : n % 3 == 1 ? 7 : 4218;
    model->job2 = 4001;
    model->sex = n % 2;
    model->head = 17;
    model->body_palette = 2;
    model->head_palette = 3;
    model->body_style = 1;
    model->accessory_top = 102;
    model->accessory_mid = 103;
    model->accessory_bottom = 104;
    model->riding = (BYTE)(n % 2);
    model->pos[0] = 3;
    model->pos[1] = 4;
    model->pos[2] = 20;
    model->shadow = 0;
    model->action = 9;
    model->motion = 2;
    g_viewDistance = n % 2 ? 200.0f : 400.0f;
    for (key = 2; key <= 4; ++key)
        *layer_view(model, key) = 100 + key;
    *layer_view(model, 8) = 108;
    have_view = 0;
    MODEL_WORLD(model, &list, 1, &drawn);
    assert(!list.size && !drawn);
    MODEL_LIST_FREE(&list);
    have_view = 1;
    prim_count = 0;
    MODEL_WORLD(model, &list, 1, &drawn);
    assert(drawn == (n % 8 != 7));
    trace_quads(&list, 1);
    note_float(model->screen_x);
    note_float(model->screen_y);
    note_float(model->bbox.left);
    note_float(model->bbox.bottom);
    model_destroy(model);
    assert(!live_blocks);
}
int main(int argc, char **argv)
{
    unsigned i, j;
    renderer.width = 640;
    renderer.height = 480;
    for (i = 0; i < 9; ++i) {
        sprites[i].palette[0] = 1000 + i;
        palettes[i].palette[0] = 2000 + i;
        images[i].width = 20;
        images[i].height = 30;
        sprites[i].images[0].items = &images[i];
        sprites[i].images[0].count = 1;
        for (j = 0; j < 2; ++j) {
            clips[i][j].x = (int)i;
            clips[i][j].y = (int)i - 2;
            clips[i][j].sprIndex = 0;
            clips[i][j].zoomX = 1;
            clips[i][j].zoomY = 1;
            clips[i][j].color[3] = 255;
            clips[i][j].angle = (int)i * 5;
        }
        anchors[i].x = (int)i * 2;
        anchors[i].y = (int)i * 3;
        anchors[i].attr = 1;
        motions[i].clips = clips[i];
        motions[i].clip_count = 2;
        motions[i].attachments = &anchors[i];
        motions[i].attachment_count = 1;
    }
    fail_next = 1;
    assert(!model_create(1, 2) && !live_blocks);
    for (i = 0; i < 24; ++i)
        run_case(i);
    for (i = 0; i < 9; ++i)
        assert(sprites[i].palette[0] == 1000 + i && palettes[i].palette[0] == 2000 + i);
    if (argc > 1) {
        FILE *f = fopen(argv[1], "wb");
        assert(f);
        assert(fwrite(trace, sizeof *trace, trace_count, f) == trace_count);
        assert(!fclose(f));
    }
    printf("Character model: 24 world render cases, initialization, Lua failure, allocation "
           "failure, borrowed resources and cleanup passed; %u trace words.\n",
           trace_count);
    return 0;
}
