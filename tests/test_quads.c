/* render/quads.c: the records of the client's quads and triangles, word for word as
   render/shaders/instances.hlsli reads them (tests/test_quad_instances.c reads them on the GPU):
   the instances a primitive makes, whether it is flat, which strips are rectangles, and where each
   kind keeps what. */
#include "render/quads.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>

static uint32_t bits(float f)
{
    uint32_t u;
    memcpy(&u, &f, 4);
    return u;
}

static float float_of(uint32_t u)
{
    float f;
    memcpy(&f, &u, 4);
    return f;
}

static void rect(struct TexDrawVertex *v)
{
    int i;
    for (i = 0; i < 4; i++) {
        v[i].x = i & 1 ? 40.5f : 10.25f;
        v[i].y = i & 2 ? 30.0f : -5.0f;
        v[i].z = i & 2 ? 0.75f : 0.25f;
        v[i].rhw = 0.999f;
        v[i].diffuse = 0x80c0ffeeu;
        v[i].u = i & 1 ? 0.5f : 0.125f;
        v[i].v = i & 2 ? 0.875f : 0.0625f;
    }
}

static void check_counts(void)
{
    struct TexDrawVertex v[9];
    float rhw[3] = { 0.5f, 0.5f, 0.5f };
    int i;
    /* a strip of four is a quad; a list a triangle of each three; nothing else */
    assert(quad_instance_count(4, 1) == 1 && quad_instance_count(3, 1) == 0);
    assert(quad_instance_count(6, 1) == 0 && quad_instance_count(0, 1) == 0);
    assert(quad_instance_count(3, 0) == 1 && quad_instance_count(9, 0) == 3);
    assert(quad_instance_count(4, 0) == 0 && quad_instance_count(0, 0) == 0);
    assert(quad_instance_count(-3, 0) == 0);
    memset(v, 0, sizeof v);
    for (i = 0; i < 9; i++)
        v[i].rhw = 0.25f;
    assert(quad_vertices_flat(v, 9) && quad_vertices_flat(v, 1) && quad_vertices_flat(v, 0));
    /* bit for bit: 0 and -0 differ, as do two not-a-numbers of other bits */
    v[8].rhw = -0.0f;
    v[0].rhw = 0.0f;
    assert(!quad_vertices_flat(v, 9));
    assert(quad_rhw_flat(rhw, 3));
    rhw[2] = float_of(bits(0.5f) + 1);
    assert(!quad_rhw_flat(rhw, 3) && quad_rhw_flat(rhw, 2));
}

static void check_rect(void)
{
    struct TexDrawVertex v[4], w[4];
    struct QuadHeader h = { 0x12345u, 77, 1, 9 };
    uint32_t words[QUAD_WORDS_MAX];
    int i, field;
    rect(v);
    assert(quad_rect_words(v, &h, words) == 16);
    /* QUAD_RECT, the snap, the layer's low 16 bits, the record, the colour, the rhw */
    assert(words[0] == (2u | 32u | 0x2345u << 16) && words[1] == 77 && words[2] == 0x80c0ffeeu);
    assert(words[3] == bits(0.999f));
    assert(words[4] == bits(10.25f) && words[5] == bits(40.5f) && words[6] == bits(-5.0f) && words[7] == bits(30.0f));
    assert(words[8] == bits(0.25f) && words[9] == bits(0.75f) && !words[10] && !words[11]);
    assert(words[12] == bits(0.125f) && words[13] == bits(0.0625f) && words[14] == bits(0.5f));
    assert(words[15] == bits(0.875f));
    h.snap = 0;
    assert(quad_rect_words(v, &h, words) == 16 && words[0] == (2u | 0x2345u << 16));
    /* every field of every corner is held equal to another corner's, so any of them a bit off
       makes it no rectangle */
    for (i = 0; i < 4; i++)
        for (field = 0; field < 7; field++) {
            uint32_t *p;
            memcpy(w, v, sizeof w);
            p = (uint32_t *)&w[i] + field;
            *p ^= field == 4 ? 0x100u : 1u;
            assert(!quad_rect_words(w, &h, words));
        }
    /* 0 and -0 are not the same z */
    memcpy(w, v, sizeof w);
    w[1].z = -0.0f;
    w[0].z = 0.0f;
    assert(!quad_rect_words(w, &h, words));
}

static void check_vertices(void)
{
    struct QuadHeader h = { 3, 41, 0, 0 };
    uint32_t words[QUAD_WORDS_MAX];
    /* a strip: QUAD_SCREEN, its vertices from word `first` on */
    assert(quad_vertex_words(4, 1, 0, 100, 0, &h, words) == 4);
    assert(words[0] == (3u | 3u << 16) && words[1] == 41 && words[2] == 0 && words[3] == 100);
    assert(!quad_vertex_words(4, 1, 0, 100, 1, &h, words) && !quad_vertex_words(5, 1, 0, 100, 0, &h, words));
    /* a list: triangles, each three vertices (21 words) on */
    assert(quad_vertex_words(9, 0, 0, 100, 2, &h, words) == 4);
    assert(words[0] == (3u | 16u | 3u << 16) && words[3] == 142);
    assert(!quad_vertex_words(9, 0, 0, 100, 3, &h, words) && !quad_vertex_words(9, 0, 0, 100, -1, &h, words));
    /* world vertices: QUAD_CAMERA, never snapped by the device (it gives snap 0); a list alone */
    assert(quad_vertex_words(3, 0, 1, 7, 0, &h, words) == 4);
    assert(words[0] == (5u | 16u | 3u << 16) && words[3] == 7);
    assert(!quad_vertex_words(4, 1, 1, 7, 0, &h, words));
}

static void check_sprite(void)
{
    struct GfxQuad q;
    struct QuadHeader h = { 65535, 5, 1, 0 };
    uint32_t words[QUAD_WORDS_MAX];
    memset(&q, 0, sizeof q);
    q.kind = GFX_QUAD_SPRITE;
    q.sprite.x0 = 1; q.sprite.x1 = 2; q.sprite.top = 3; q.sprite.bottom = 4;
    q.sprite.rhwBottom = 5; q.sprite.rhwTop = 6; q.sprite.offsetX = 7; q.sprite.offsetY = 8;
    q.sprite.cosine = 9; q.sprite.sine = 10;
    q.sprite.u0 = 11; q.sprite.v0 = 12; q.sprite.u1 = 13; q.sprite.v1 = 14;
    q.sprite.colour = 0xdeadbeefu;
    assert(quad_words(&q, &h, words) == 20);
    assert(words[0] == (1u | 32u | 0xffffu << 16) && words[1] == 5 && words[2] == 0xdeadbeefu);
    assert(words[4] == bits(1) && words[5] == bits(2) && words[6] == bits(3) && words[7] == bits(4));
    assert(words[8] == bits(5) && words[9] == bits(6) && words[10] == bits(7) && words[11] == bits(8));
    assert(words[12] == bits(9) && words[13] == bits(10) && !words[14] && !words[15]);
    assert(words[16] == bits(11) && words[17] == bits(12) && words[18] == bits(13) && words[19] == bits(14));
    q.sprite.turned = 1;
    h.snap = 0;
    assert(quad_words(&q, &h, words) == 20 && words[0] == (1u | 64u | 0xffffu << 16));
}

static void check_world(void)
{
    struct GfxQuad q;
    struct QuadHeader h = { 0, 6, 0, 12 };
    uint32_t words[QUAD_WORDS_MAX];
    int i;
    memset(&q, 0, sizeof q);
    q.kind = GFX_QUAD_WORLD;
    for (i = 0; i < 4; i++) {
        q.world.point[i][0] = (float)(10 * i + 1);
        q.world.point[i][1] = (float)(10 * i + 2);
        q.world.point[i][2] = (float)(10 * i + 3);
        q.world.u[i] = (float)(100 + i);
        q.world.v[i] = (float)(200 + i);
        q.world.u1[i] = (float)(300 + i);
        q.world.v1[i] = (float)(400 + i);
    }
    q.world.rhw = GFX_RHW_ADD;
    q.world.depth = GFX_DEPTH_ADD;
    q.world.rhwValue = 0.5f;
    q.world.depthValue = -0.25f;
    q.world.colour = 0x11223344u;
    q.world.viewSpace = 1;
    assert(quad_words(&q, &h, words) == 28);
    /* QUAD_WORLD, the rhw added, z added, in view space; the matrix's first row in w */
    assert(words[0] == (4u | 256u | 2048u | 4096u) && words[1] == 6 && words[2] == 0x11223344u);
    assert(words[3] == 12);
    /* the points, the rhw's value with the first, the z's with the second */
    assert(words[4] == bits(1) && words[5] == bits(2) && words[6] == bits(3) && words[7] == bits(0.5f));
    assert(words[8] == bits(11) && words[11] == bits(-0.25f));
    assert(words[12] == bits(21) && !words[15] && words[16] == bits(31) && words[18] == bits(33) && !words[19]);
    assert(words[20] == bits(100) && words[23] == bits(103) && words[24] == bits(200) && words[27] == bits(203));
    q.world.stage2 = 1;
    q.world.viewSpace = 0;
    q.world.rhw = GFX_RHW_PROJECTED;
    q.world.depth = GFX_DEPTH_ZERO;
    assert(quad_words(&q, &h, words) == 36);
    assert(words[0] == (4u | 128u | 1024u) && words[16] == bits(31));
    assert(words[28] == bits(300) && words[31] == bits(303) && words[32] == bits(400) && words[35] == bits(403));
    /* a kind that is none */
    q.kind = 0;
    assert(!quad_words(&q, &h, words));
}

int main(void)
{
    check_counts();
    check_rect();
    check_vertices();
    check_sprite();
    check_world();
    puts("Quads: primitives' instances, flatness, rectangles, and every kind's words in place passed.");
    return 0;
}
