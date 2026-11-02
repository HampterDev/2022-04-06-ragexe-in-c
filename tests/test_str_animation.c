#include "render/rp_pool.h"
#include "resources/ezeffect.h"
#include "effects/rag/rag_effect.h"
#include "render/render_api.h"
#include "ui/window_access.h"
#include <assert.h>
#include <math.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static struct Renderer renderer;
struct Renderer *g_renderer = &renderer;

int game_copy_view_matrix(float view[12])
{
    memset(view, 0, 12 * sizeof(float));
    view[0] = view[4] = view[8] = 1.0f;
    return 1;
}

void renderer_project_vertex(struct Renderer *r, const Vec3 *pos,
                             const float *view, struct TexDrawVertex *out)
{
    (void)r;
    (void)view;
    memset(out, 0, sizeof *out);
    out->x = pos->x;
    out->y = pos->y;
    out->z = pos->z;
}

struct TexDrawVertex *prim_project_vertex(struct TexDrawVertex *out, const float *view, Vec3 pos)
{
    renderer_project_vertex(&renderer, &pos, view, out);
    return out;
}

struct UIWindow *ui_window_find(int id)
{
    (void)id;
    assert(!"world particles do not query UI anchors");
    return NULL;
}

void res_lock(struct Res *res)
{
    (void)res;
    assert(!"these steps must not rewind");
}

static float step(int type, float frame, float delay, int textures, int elapsed)
{
    struct StrKeyState state = { 0 }, delta = { 0 };
    state.aniframe = frame;
    delta.anitype = type;
    delta.delay = delay;
    str_key_apply_morph(&state, &delta, textures, elapsed);
    return state.aniframe;
}

static void arithmetic(void)
{
    struct StrKeyState state = { 0 }, delta = { 0 };
    int i;

    assert(step(STR_ANI_LOOP, 3.75f, 0.5f, 4, 1) == 0.25f);
    assert(step(STR_ANI_LOOP, 0.25f, 21.0f, 4, 1) == 1.25f);
    assert(step(STR_ANI_REVERSE, 0.25f, 1.0f, 4, 1) == 3.25f);
    assert(step(STR_ANI_REVERSE, 0.25f, 21.0f, 4, 1) == 3.25f);
    assert(step(STR_ANI_REVERSE, 0.0f, 8.0f, 4, 1) == 0.0f);
    assert(step(STR_ANI_LOOP, 1.0f, -2.0f, 4, 1) == 3.0f);
    assert(step(STR_ANI_ONCE, 1.0f, 1.5f, 3, 1) == 2.5f);
    assert(step(STR_ANI_ONCE, 2.5f, 1.0f, 3, 1) == 2.0f);
    assert(step(STR_ANI_ONCE, 1.0f, -2.0f, 3, 1) == 0.0f);
    assert(step(STR_ANI_PINGPONG, 0.0f, 1.0f, 4, 3) == 3.0f);
    assert(step(STR_ANI_PINGPONG, 0.0f, 1.0f, 4, 4) == 2.0f);
    assert(step(STR_ANI_PINGPONG, 0.0f, 1.0f, 4, 6) == 0.0f);
    assert(step(STR_ANI_PINGPONG, 0.0f, -1.0f, 4, 1) == 1.0f);
    assert(step(STR_ANI_PINGPONG, 0.0f, 1.0f, 1, 100) == 0.0f);
    for (i = STR_ANI_ONCE; i <= STR_ANI_PINGPONG; ++i) {
        assert(step(i, 3.0f, 1.0f, 0, 100) == 0.0f);
        assert(step(i, 3.0f, 1.0f, -1, 100) == 0.0f);
        assert(step(i, NAN, 1.0f, 4, 1) == 0.0f);
        assert(step(i, 3.0f, INFINITY, 4, 1) == (i == STR_ANI_ONCE ? 3.0f : 0.0f));
    }
    for (i = 0; i < 64; ++i) {
        float initial = (float)i / 16.0f;
        float next = step(STR_ANI_LOOP, initial, 100.25f, 4, 1);
        assert(step(STR_ANI_REVERSE, next, 100.25f, 4, 1) == initial);
    }

    state.x = 10.0f;
    state.y = 20.0f;
    state.aniframe = 2.0f;
    state.src_blend = 7;
    delta.x = -2.0f;
    delta.y = 3.0f;
    delta.angle = 5.0f;
    delta.anitype = STR_ANI_STEP;
    delta.aniframe = 0.25f;
    delta.src_blend = 9;
    for (i = 0; i < 4; ++i) {
        state.corner_x[i] = (float)i;
        delta.corner_x[i] = 10.0f;
        delta.color[i] = 25.0f;
    }
    str_key_apply_morph(&state, &delta, 4, 1);
    assert(state.x == 8.0f && state.y == 23.0f && state.angle == 5.0f);
    assert(state.aniframe == 2.25f && state.src_blend == 7);
    for (i = 0; i < 4; ++i) {
        assert(state.corner_x[i] == (float)i + 10.0f);
        assert(state.color[i] == 25.0f);
    }
}

static void independent_layers(void)
{
    struct EzEffectRes *resource = calloc(1, sizeof *resource);
    struct StrAnim *anim = resource ? &resource->anim : NULL;
    struct Ez2StrParticle *particles = calloc(2, sizeof *particles);
    struct RagEffect *effects = calloc(2, sizeof *effects);
    struct RenderObject master = { 0 };
    struct StrKey keys[2][2] = { 0 };
    Vec3 offset = { 0 };
    int reverse_first, instance, i;

    assert(anim && particles && effects);
    anim->layer_count = 2;
    for (reverse_first = 0; reverse_first < 2; ++reverse_first) {
        memset(particles, 0, 2 * sizeof *particles);
        memset(effects, 0, 2 * sizeof *effects);
        for (i = 0; i < 2; ++i) {
            int reverse = (i == reverse_first);
            anim->layers[i].texture_count = reverse ? 4 : 3;
            anim->layers[i].key_count = 2;
            anim->layers[i].keys = keys[i];
            keys[i][0].type = 1;
            keys[i][0].s.anitype = reverse ? STR_ANI_REVERSE : STR_ANI_LOOP;
            keys[i][0].s.delay = reverse ? 1.0f : 13.0f;
            keys[i][1].frame = 100;
            for (instance = 0; instance < 2; ++instance) {
                particles[instance].state[i].aniframe = reverse ? 0.25f : 1.25f;
                effects[instance].str_state[i].aniframe = reverse ? 0.25f : 1.25f;
            }
        }
        for (instance = 0; instance < 2; ++instance) {
            struct Ez2StrParticle *p = &particles[instance];
            struct RagEffect *r = &effects[instance];
            p->res = resource;
            p->anim = anim;
            p->layers_to_play = 2;
            p->frame = 1;
            r->str_res = resource;
            r->str_anim = anim;
            r->master = &master;
            r->str_layers_to_play = 2;
            r->frame = 1;
            assert(ez2_str_particle_process(p, offset));
            assert(rag_str_step(r, 0.0f, 0.0f, 0.0f));
            assert(p->frame == 2 && r->frame == 1);
            for (i = 0; i < 2; ++i) {
                float expected = i == reverse_first ? 3.25f : 2.25f;
                assert(p->state[i].aniframe == expected && p->drawn[i]);
                assert(r->str_state[i].aniframe == expected && r->str_drawn[i]);
            }
        }
    }
    free(effects);
    free(particles);
    free(resource);
}

static void prepare_players(struct EzEffectRes *res, struct Ez2StrParticle *p,
                            struct RagEffect *r, struct RenderObject *master)
{
    memset(p, 0, sizeof *p); memset(r, 0, sizeof *r);
    p->res = res;
    r->str_res = res; r->str_anim = &res->anim; r->master = master;
    ez2_str_particle_rewind(p); rag_str_rewind(r);
}

static void play_frame(struct Ez2StrParticle *p, struct RagEffect *r, int frame, int playing)
{
    Vec3 offset = {0};
    p->frame = r->frame = frame;
    assert(ez2_str_particle_process(p, offset) == playing);
    assert(rag_str_step(r, 0, 0, 0) == playing);
}

static void key_boundaries(void)
{
    SYSTEM_INFO system;
    DWORD previous;
    unsigned char *pages;
    struct StrKey *keys;
    struct EzEffectRes *res = calloc(1, sizeof *res);
    struct Ez2StrParticle *p = calloc(1, sizeof *p);
    struct RagEffect *r = calloc(1, sizeof *r);
    struct RenderObject master = {0};
    struct StrLayer *layer;
    int i;
    GetSystemInfo(&system);
    pages = VirtualAlloc(NULL, 2 * system.dwPageSize, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
    assert(res && p && r && pages);
    assert(VirtualProtect(pages + system.dwPageSize, system.dwPageSize, PAGE_NOACCESS, &previous));

    prepare_players(res, p, r, &master);
    assert(!p->layers_to_play && !r->str_layers_to_play);
    play_frame(p, r, 0, 0);

    keys = (struct StrKey *)(pages + system.dwPageSize) - 1;
    memset(keys, 0, sizeof *keys); keys[0].frame = 3; keys[0].s.x = 41.0f;
    res->anim.layer_count = 1; layer = &res->anim.layers[0];
    layer->keys = keys; layer->key_count = 1;
    prepare_players(res, p, r, &master);
    assert(p->layers_to_play == 1 && r->str_layers_to_play == 1);
    play_frame(p, r, 0, 1);
    assert(!p->drawn[0] && !r->str_drawn[0]);
    play_frame(p, r, 3, 1);
    assert(p->drawn[0] && r->str_drawn[0] && p->state[0].x == 41.0f && r->str_state[0].x == 41.0f);
    assert(p->key_index[0] == -1 && r->str_key_index[0] == -1);
    assert(p->layers_done == 1 && r->str_layers_done == 1);
    play_frame(p, r, 4, 0);

    keys = (struct StrKey *)(pages + system.dwPageSize) - 2;
    memset(keys, 0, 2 * sizeof *keys);
    keys[0].s.x = 10; keys[1].type = 1; keys[1].s.x = 2;
    layer->keys = keys; layer->key_count = 2;
    prepare_players(res, p, r, &master);
    play_frame(p, r, 0, 1);
    assert(p->state[0].x == 10 && r->str_state[0].x == 10 && p->key_index[0] == 1);
    assert(!p->layers_done && !r->str_layers_done);
    play_frame(p, r, 1, 1);
    assert(p->state[0].x == 12 && r->str_state[0].x == 12);
    assert(p->layers_done == 1 && r->str_layers_done == 1);
    play_frame(p, r, 2, 0);

    keys = (struct StrKey *)(pages + system.dwPageSize) - 3;
    memset(keys, 0, 3 * sizeof *keys);
    keys[0].s.x = 10; keys[1].type = 1; keys[1].s.x = 2;
    keys[2].frame = 3; keys[2].s.x = 50;
    layer->keys = keys; layer->key_count = 3;
    prepare_players(res, p, r, &master);
    for (i = 0; i <= 3; ++i) {
        float expected = i == 3 ? 50.0f : 10.0f + 2.0f * i;
        play_frame(p, r, i, 1);
        assert(p->state[0].x == expected && r->str_state[0].x == expected);
    }
    play_frame(p, r, 4, 0);
    prepare_players(res, p, r, &master);
    p->key_index[0] = r->str_key_index[0] = layer->key_count;
    play_frame(p, r, 0, 1);
    assert(!p->drawn[0] && !r->str_drawn[0] && p->layers_done == 1 && r->str_layers_done == 1);

    memset(&res->anim, 0, sizeof res->anim); res->anim.layer_count = STR_LAYERS;
    layer = &res->anim.layers[STR_LAYERS - 1];
    keys = (struct StrKey *)(pages + system.dwPageSize) - 1;
    memset(keys, 0, sizeof *keys); keys[0].s.x = 75;
    layer->keys = keys; layer->key_count = 1;
    prepare_players(res, p, r, &master);
    assert(p->layers_to_play == 1 && r->str_layers_to_play == 1);
    play_frame(p, r, 0, 1);
    assert(p->drawn[STR_LAYERS - 1] && r->str_drawn[STR_LAYERS - 1]);
    assert(p->state[STR_LAYERS - 1].x == 75 && p->layers_done == 1);
    assert(!p->drawn[0] && !r->str_drawn[0]);
    play_frame(p, r, 1, 0);

    keys[0].frame = INT_MIN; keys[0].type = 1; keys[0].s.x = 1;
    prepare_players(res, p, r, &master); play_frame(p, r, INT_MAX, 1);
    assert(p->frame == INT_MAX && p->state[STR_LAYERS - 1].x == 1);
    keys[0].frame = INT_MAX;
    prepare_players(res, p, r, &master); play_frame(p, r, INT_MIN, 1);
    assert(!p->drawn[STR_LAYERS - 1] && !r->str_drawn[STR_LAYERS - 1]);

    assert(VirtualFree(pages, 0, MEM_RELEASE));
    free(r); free(p); free(res);
}

int main(void)
{
    arithmetic();
    independent_layers();
    key_boundaries();
    puts("STR playback: guard-page key bounds, empty layers, final keys, rewind and frame limits passed");
    puts("STR animation: fractional wrapping, reverse playback, empty/single textures and independent player/layer state passed");
    return 0;
}
