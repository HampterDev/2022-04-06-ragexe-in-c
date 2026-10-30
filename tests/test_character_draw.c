#include <assert.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "ui/character/character_draw.h"
#include <ui/character/char_draw.c>
#include "character_draw_unreached.h"

static struct {
    unsigned char *p;
    size_t n;
    unsigned id;
} blocks[256];
static unsigned allocation_id;
static unsigned live_blocks, texture_draws, scaled_draws, test_case;
static unsigned trace[65536], trace_count;
static struct SprRes sprites[8];
static struct SprImg images[8][2];
static struct Motion motions[8];
static struct SprClip clips[8][2];
static struct AttachPointInfo anchors[8];
static struct PaletteRes palettes[8];
static int acts[8], textures[8], resource_manager, canvas;
struct Session g_session1;
struct Renderer *g_renderer;

static void note(unsigned value)
{
    assert(trace_count < sizeof trace / sizeof *trace);
    trace[trace_count++] = value;
}

void *mem_alloc(size_t n)
{
    unsigned i;
    unsigned char *p = malloc(n + 16);
    assert(p);
    memset(p, 0xa5, n + 16);
    for (i = 0; i < 256 && blocks[i].p; ++i) {
    }
    assert(i < 256);
    blocks[i].p = p;
    blocks[i].n = n;
    blocks[i].id = ++allocation_id;
    note(800);
    note(blocks[i].id);
    ++live_blocks;
    return p;
}

void mem_free(void *p)
{
    unsigned i, j;
    if (!p)
        return;
    for (i = 0; i < 256 && blocks[i].p != p; ++i) {
    }
    assert(i < 256);
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

static struct String *resource_name(struct String *out, int layer, int kind, int job, int sex,
                                       int value, int extra)
{
    char name[80];
    note(100);
    note(layer);
    note(kind);
    note(job);
    note(sex);
    note(value);
    note(extra);
    snprintf(name, sizeof name, "fixture/resource/layer/%d/kind/%d", layer, kind);
    string_init(out);
    string_assign_cstr(out, name);
    return out;
}

struct String *session_get_body_spr_name_variant1(struct String *o, int j, int s, int b, int z)
{
    return resource_name(o, 1, 0, j, s, b, z);
}
struct String *session_get_body_spr_name_variant0(struct String *o, int j, int s, int b, int z)
{
    return resource_name(o, 1, 1, j, s, b, z);
}
struct String *session_get_body_pal_name(struct String *o, int j, int s, int p, int b, int z)
{
    note(b);
    return resource_name(o, 1, 2, j, s, p, z);
}
struct String *session_get_head_spr_name1(struct String *o, int j, int s, int *h)
{
    note(*h);
    *h += 1;
    return resource_name(o, 2, 0, j, s, *h, 0);
}
struct String *session_get_head_spr_name0(struct String *o, int j, int s, int *h)
{
    return resource_name(o, 2, 1, j, s, *h, 0);
}
struct String *session_get_head_pal_name(struct String *o, int h, int j, int s, int p)
{
    return resource_name(o, 2, 2, j, s, h, p);
}
static int item_layer(int item)
{
    return item == 181 ? 4 : item == 103 ? 3 : item == 105 ? 5 : 6;
}
struct String *session_get_res_name1(struct String *o, int j, int s, int h, int i)
{
    return resource_name(o, item_layer(i), 0, j, s, h, i);
}
struct String *session_get_res_name0(struct String *o, int j, int s, int h, int i)
{
    return resource_name(o, item_layer(i), 1, j, s, h, i);
}
struct String *session_get_robe_spr_name(struct String *o, int j, int s, int robe, int riding)
{
    return resource_name(o, 7, 0, j, s, robe, riding);
}
struct String *session_get_robe_act_name(struct String *o, int j, int s, int robe, int riding)
{
    return resource_name(o, 7, 1, j, s, robe, riding);
}

struct ResMgr *g_resMgr(void)
{
    return (struct ResMgr *)&resource_manager;
}
void *res_mgr_get_string(struct ResMgr *self, struct String name)
{
    int layer, kind;
    void *result = NULL;
    assert(self == g_resMgr());
    if (name.size) {
        assert(sscanf(string_cstr(&name), "fixture/resource/layer/%d/kind/%d", &layer, &kind) ==
               2);
        assert(layer >= 0 && layer < 8 && kind >= 0 && kind < 3);
        note(200);
        note(layer);
        note(kind);
        if (test_case % 12 != 11 && !(test_case % 12 == 10 && layer == 2))
            result = kind == 0   ? (void *)&sprites[layer]
                     : kind == 1 ? (void *)&acts[layer]
                                 : (void *)&palettes[layer];
    }
    string_free(&name);
    return result;
}

int session_get_mounted_job(int job)
{
    note(300);
    note(job);
    return job + 1000;
}
int session_get_sex(struct Session *self)
{
    assert(self == &g_session1);
    return 0;
}
BYTE session_is_doram_job(struct Session *self, int job)
{
    (void)self;
    return job == 4218;
}
BYTE session_is_head_replacing_view(struct Session *self, int id)
{
    (void)self;
    return test_case % 4 == 2 && id == 103;
}
BYTE session_is_top_layer_robe(struct Session *self, int robe)
{
    (void)self;
    (void)robe;
    return (test_case & 4) != 0;
}
float session_get_baby_job_scale(struct Session *self, int job)
{
    assert(self == &g_session1);
    note(301);
    note(job);
    return 0.75f;
}
char layer_is_ignored_riding_state(int id, int job, int action, char riding)
{
    note(302);
    note(id);
    note(job);
    note(action);
    note(riding);
    return riding;
}
int layer_body_attach_dy(struct Motion *m, int action, int frame, int job, int sex, int body)
{
    assert(m);
    note(303);
    note(action);
    note(frame);
    note(job);
    note(sex);
    note(body);
    return 9;
}

int client_lua_call_table(struct ClientLua *lua, const char *table, const char *func,
                          const char *signature, ...)
{
    va_list ap;
    int robe;
    (void)lua;
    assert(!strcmp(table, "TB_Layer_Priority") && !strcmp(func, "IsHelmRobe") &&
           !strcmp(signature, "d>b"));
    va_start(ap, signature);
    robe = va_arg(ap, int);
    *va_arg(ap, BYTE *) = (BYTE)(test_case & 1);
    va_end(ap);
    note(400);
    note(robe);
    return 1;
}

int client_lua_call(struct ClientLua *lua, const char *func, const char *signature, ...)
{
    va_list ap;
    int i, kind, item;
    (void)lua;
    va_start(ap, signature);
    if (!strcmp(func, "DrawOnTop") || !strcmp(func, "_New_DrawOnTop")) {
        note(401);
        for (i = 0; i < 5; ++i)
            note(va_arg(ap, int));
        *va_arg(ap, BYTE *) = (BYTE)((test_case >> 1) & 1);
        if (!strcmp(func, "_New_DrawOnTop"))
            *va_arg(ap, int *) = 88;
    } else if (!strcmp(func, "GetLayerPriority")) {
        assert(va_arg(ap, int) == 0);
        kind = va_arg(ap, int);
        item = va_arg(ap, int);
        note(402);
        note(kind);
        note(item);
        *va_arg(ap, int *) = test_case % 8 == 7 ? 10 : kind * 10 + (item == 107);
    } else {
        assert(!strcmp(func, "OffsetItemPos_GetOffsetForDoram"));
        note(403);
        for (i = 0; i < 3; ++i)
            note(va_arg(ap, int));
        *va_arg(ap, int *) = 3;
        *va_arg(ap, int *) = -2;
    }
    va_end(ap);
    return test_case % 8 == 6 ? 0 : 1;
}

struct Motion *act_res_get_motion(const struct ActRes *act, unsigned action, unsigned frame)
{
    unsigned i;
    note(500);
    note(action);
    note(frame);
    if (!act)
        return NULL;
    for (i = 0; i < 8 && (const void *)act != &acts[i]; ++i) {
    }
    assert(i < 8);
    note(i);
    return &motions[i];
}
struct SprClip *motion_get_clip(const struct Motion *motion, unsigned index)
{
    assert(motion && index < 2);
    note(501);
    note(index);
    return &motion->clips[index];
}
void *session_get_stepped_motion(void *master, void *slave, int action, int frame, DWORD start,
                               int *out)
{
    note(502);
    note(start);
    assert(master || test_case % 12 == 10);
    *out = (test_case & 1) ? frame + 1 : frame;
    return act_res_get_motion(slave, action, *out);
}
static int animation_shift;
static unsigned steppings;
int session_get_motion_stepping(void *master, void *slave, int action, int block, DWORD start,
                                struct MotionStepping *out)
{
    (void)action;
    if (!master || !slave)
        return 0;
    ++steppings;
    out->ratio = 1;
    out->delay = 4;
    out->block = block;
    out->start_time = start;
    return 1;
}
int motion_stepping_frame(const struct MotionStepping *stepping)
{
    return stepping->block + (int)(test_case & 1) + animation_shift;
}

struct Texture *sprite_atlas_resolve(struct Renderer *renderer, const struct SprImg *image,
                                      const DWORD *palette, struct SpriteRegion *region)
{
    unsigned i;
    (void)renderer;
    for (i = 0; i < 8 && image != &images[i][0]; ++i) {
    }
    assert(i < 8 && palette);
    note(600);
    note(i);
    note(palette[0]);
    region->x = 4;
    region->y = 5;
    return (struct Texture *)&textures[i];
}
void dc_bitmap_draw_texels(struct DcBitmap *dst, int x, int y, struct Texture *tex, int sx, int sy,
                          int w, int h, int mirror, int cw, int ch)
{
    assert(dst == (struct DcBitmap *)&canvas && tex);
    ++texture_draws;
    note(601);
    note(x);
    note(y);
    note(sx);
    note(sy);
    note(w);
    note(h);
    note(mirror);
    note(cw);
    note(ch);
}
void ui_window_draw_spr_clip(struct UIWindow *window, int x, int y, struct SprImg *img,
                          struct SprClip *clip, float scale, float angle, DWORD color,
                          DWORD *palette, float zoom)
{
    assert(window && img && clip && scale == 0.75f && zoom == 1.0f && color == 0x12345678);
    ++scaled_draws;
    note(602);
    note(x);
    note(y);
    note((unsigned)angle);
    note(palette[0]);
    note(clip->sprIndex);
}
DWORD ui_window_get_color(struct UIWindow *window)
{
    assert(window);
    return 0x12345678;
}

static void run_case(unsigned n)
{
    struct UIWindow window = {0};
    struct CharacterDrawOptions options = {.x = 41,
                                           .y = -7,
                                           .sex = 1,
                                           .job = 7,
                                           .body = 9,
                                           .head = 10,
                                           .accessory = 105,
                                           .accessory2 = 181,
                                           .accessory3 = 103,
                                           .robe = 107,
                                           .body_palette = 2,
                                           .head_palette = 3,
                                           .action = 9,
                                           .motion = 2,
                                           .start_time = 0xabcdef12},
                                before;
    unsigned texture_before = texture_draws, scaled_before = scaled_draws, flag = n % 3;
    test_case = n;
    note(700);
    note(n);
    window.canvas = (struct DcBitmap *)&canvas;
    options.job = n % 5 == 0 ? 4218 : 7;
    options.scaled = (n & 8) != 0;
    options.riding = (unsigned char)((n & 16) != 0);
    options.action = n % 4 == 0 ? 1 : 9;
    options.body_palette = n % 7 == 0 ? 0 : 2;
    options.head_palette = n % 7 == 0 ? 0 : 3;
    g_session1.new_layer_table_loaded = (n & 2) != 0;
    before = options;
#ifdef DRAW_OWNERS_REFERENCE
    {
        struct FuncDrawWNDChar draw;
        FuncDrawWNDChar_Construct(&draw, &window, options.x, options.y, options.sex, options.job, 8,
                                  options.body, options.head, options.accessory, options.accessory2,
                                  options.accessory3, options.robe, options.riding, options.scaled,
                                  options.body_palette, options.head_palette, options.action,
                                  options.motion, options.start_time);
        FuncDrawWNDChar_Draw(&draw, (BYTE)flag);
        FuncDrawWNDChar_Destruct(&draw);
    }
#else
    ui_draw_character(&window, &options, (unsigned char)flag);
#endif
    assert(!memcmp(&before, &options, sizeof options));
    assert(live_blocks == 0);
    if (n % 12 == 11)
        assert(texture_before == texture_draws && scaled_before == scaled_draws);
    else if (flag == 1)
        assert(texture_before == texture_draws && scaled_before < scaled_draws);
    else
        assert(texture_before < texture_draws && scaled_before == scaled_draws);
}

#ifndef DRAW_OWNERS_REFERENCE
static void check_animation(void)
{
    static struct UIAnimation animation;
    struct UIWindow window = {0};
    struct CharacterDrawOptions options = {.x = 41, .y = -7, .sex = 1, .job = 7, .body = 9, .head = 10,
                                           .accessory = 105, .accessory2 = 181, .accessory3 = 103, .robe = 107,
                                           .body_palette = 2, .head_palette = 3, .action = 9, .motion = 2,
                                           .start_time = 0xabcdef12};
    unsigned before = steppings;

    test_case = 3;
    g_session1.new_layer_table_loaded = 1;
    window.canvas = (struct DcBitmap *)&canvas;
    ui_draw_character(&window, &options, 0);
    assert(steppings == before);
    options.animation = &animation;
    ui_animation_reset(&animation);
    ui_draw_character(&window, &options, 0);
    assert(animation.count > 0 && steppings == before + (unsigned)animation.count);
    assert(!ui_animation_due(&animation));
    animation_shift = 1;
    assert(ui_animation_due(&animation));
    animation_shift = 0;
    animation.count = UI_ANIMATION_STEPS + 1;
    assert(ui_animation_due(&animation));
    assert(live_blocks == 0);
}
#endif

int main(int argc, char **argv)
{
    unsigned i, j;
    for (i = 0; i < 8; ++i) {
        sprites[i].palette[0] = 1000 + i;
        palettes[i].palette[0] = 2000 + i;
        for (j = 0; j < 2; ++j) {
            images[i][j].width = 20;
            images[i][j].height = 30;
            images[i][j].texture = (struct Texture *)&textures[i];
            sprites[i].images[j].items = &images[i][j];
            sprites[i].images[j].count = 1;
            clips[i][j].x = (int)i;
            clips[i][j].y = (int)i - 2;
            clips[i][j].sprIndex = 0;
            clips[i][j].clipType = i % 2;
            clips[i][j].zoomX = 1.25f;
            clips[i][j].zoomY = 0.0f;
            clips[i][j].angle = (int)i * 7;
        }
        anchors[i].x = (int)i * 3;
        anchors[i].y = (int)i * 4;
        anchors[i].attr = 1;
        motions[i].clips = clips[i];
        motions[i].clip_count = 2;
        motions[i].attachments = &anchors[i];
        motions[i].attachment_count = 1;
    }
    for (i = 0; i < 48; ++i)
        run_case(i);
    for (i = 0; i < 8; ++i)
        assert(sprites[i].palette[0] == 1000 + i && palettes[i].palette[0] == 2000 + i);
    if (argc > 1) {
        FILE *f = fopen(argv[1], "wb");
        assert(f);
        assert(fwrite(trace, sizeof *trace, trace_count, f) == trace_count);
        assert(!fclose(f));
    }
#ifndef DRAW_OWNERS_REFERENCE
    check_animation();
#endif
    printf("Character drawing: 48 cases, scaled and canvas paths, layer ordering, borrowed "
           "resources and guarded cleanup passed; %u trace words.\n",
           trace_count);
    return 0;
}
