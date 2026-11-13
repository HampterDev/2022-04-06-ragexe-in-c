/* custom: the static world's rules (render/static_world.h): the shared buffers' layout, the draw
   records, the records of the vertices, their order (a merge of sorted lists giving what a sort of
   them all gives) and the runs the indirect draws take, the cull against the projection
   (render/rp_screen.c) and world/frustum.c, the cascades' cull against the boxes worked out in
   double, and the texture arrays. Fuzzed with a fixed seed.
   tests/test_static_world_maps.c checks the records against the draws the Direct3D 9 client's code
   made on real maps, and the GPU's cull against these rules. */
#include "render/static_world.h"
#include "render/device.h"
#include "render/render_api.h"
#include "render/renderer.h"
#include "world/frustum.h"

#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* what render/rp_project.c uses beside the projection */
struct Renderer *g_renderer;
float g_fogNear, g_fogFar;
DWORD g_fogColor;
int g_lightmap;
int gfx_shadows_supported(void) { return 1; }
/* and render/rp_screen.c beside renderer_set_projection */
struct C3dDevice g_3dDevice;
int c3d_device_clear(struct C3dDevice *self, DWORD colour) { (void)self; (void)colour; return 1; }
int c3d_device_clear_z(struct C3dDevice *self) { (void)self; return 1; }
void c3d_device_restore_frame(struct C3dDevice *self) { (void)self; }

static uint32_t seed = 0x2545f491u;

static uint32_t next(void)
{
    seed ^= seed << 13;
    seed ^= seed >> 17;
    seed ^= seed << 5;
    return seed;
}

static float uniform(float lo, float hi)
{
    return lo + (hi - lo) * (float)(next() & 0xffffff) / (float)0xffffff;
}

static void test_layout(void)
{
    struct SwLayout layout = {0, 0};
    struct SwPlacement a, b, c;

    assert(sw_layout_place(&layout, 4, 6, &a));
    assert(sw_layout_place(&layout, SW_MESH_VERTICES, 3, &b));
    assert(sw_layout_place(&layout, 3, 3, &c));
    assert(a.baseVertex == 0 && a.firstIndex == 0 && a.vertexCount == 4 && a.indexCount == 6);
    assert(b.baseVertex == 4 && b.firstIndex == 6);
    assert(c.baseVertex == 4 + SW_MESH_VERTICES && c.firstIndex == 9);
    assert(layout.vertexCount == 7 + SW_MESH_VERTICES && layout.indexCount == 12);
    /* counts it cannot take leave the layout as it was */
    assert(!sw_layout_place(&layout, 0, 3, &c));
    assert(!sw_layout_place(&layout, SW_MESH_VERTICES + 1, 3, &c));
    assert(!sw_layout_place(&layout, 3, 0, &c));
    assert(!sw_layout_place(&layout, 3, 4, &c));
    assert(!sw_layout_place(NULL, 3, 3, &c));
    assert(layout.vertexCount == 7 + SW_MESH_VERTICES && layout.indexCount == 12);
    layout.vertexCount = 0xffffffffu - 10;
    assert(!sw_layout_place(&layout, 11, 3, &c));
    assert(sw_layout_place(&layout, 10, 3, &c) && layout.vertexCount == 0xffffffffu);
    layout.indexCount = 0xffffffffu - 3;
    layout.vertexCount = 0;
    assert(!sw_layout_place(&layout, 3, 6, &c));
    assert(sw_layout_place(&layout, 3, 3, &c) && layout.indexCount == 0xffffffffu);

    assert(sw_layout_grow(0, 5, 0) == 8);
    assert(sw_layout_grow(16, 10, 4) == 16);
    assert(sw_layout_grow(16, 17, 4) == 32);
    assert(sw_layout_grow(0, 100, 1024) == 1024);
    assert(sw_layout_grow(0, 1025, 1024) == 2048);
    assert(sw_layout_grow(0x80000001u, 0xffffffffu, 0) == 0xffffffffu);
    assert(sw_layout_grow(3, 0xffffffffu, 0) == 0xffffffffu);
    assert(sw_layout_grow(5, 0, 0) == 5);
}

/* four corners of a quad, x y z and two more floats each */
static const float quad[4][5] = {
    {-1, 0, -2, 9, 9}, {3, 0, -2, 9, 9}, {-1, 5, 4, 9, 9}, {3, 5, 4, 9, 9},
};
static const uint16_t quadIndices[6] = {0, 1, 2, 2, 1, 3};

static struct SwMesh quad_mesh(void)
{
    struct SwMesh mesh;

    memset(&mesh, 0, sizeof mesh);
    mesh.positions = &quad[0][0];
    mesh.stride = 5;
    mesh.vertexCount = 4;
    mesh.indices = quadIndices;
    mesh.indexCount = 6;
    mesh.transform = 7;
    return mesh;
}

static void test_records(void)
{
    struct SwMesh mesh = quad_mesh();
    struct SwPlacement place = {100, 4, 30, 6};
    struct SwRun runs[2];
    struct SwRecord r[2];
    float moved[12] = {0, 0, 2, 0, 1, 0, -1, 0, 0, 10, 20, 30}; /* a turn about y, then a move */
    float nan[4][5];

    memset(runs, 0, sizeof runs);
    runs[0].first = 0;
    runs[0].count = 3;
    runs[0].pass = SW_PASS_ALPHA_CLAMP;
    runs[0].textureArray = 2;
    runs[0].textureLayer = 5;
    runs[0].lightmapArray = SW_NO_ARRAY;
    runs[0].uv = 3;
    runs[0].sort[0] = 11;
    runs[0].flags = SW_CULL_NONE;
    runs[1].first = 3;
    runs[1].count = 3;
    runs[1].pass = SW_PASS_GROUND;
    assert(sw_records_build(&mesh, &place, runs, 2, 40, r));
    assert(r[0].firstIndex == 30 && r[0].indexCount == 3 && r[0].baseVertex == 100);
    assert(r[0].id == 40 && r[1].id == 41);
    assert(r[0].textureArray == 2 && r[0].textureLayer == 5 && r[0].lightmapArray == SW_NO_ARRAY);
    assert(r[0].uv == 3 && r[0].transform == 7);
    assert(r[0].pass == SW_PASS_ALPHA_CLAMP && r[0].flags == SW_CULL_NONE && r[0].sort[0] == 11);
    /* corners 0 1 2: x -1..3, y 0..5, z -2..4 */
    assert(r[0].centre[0] == 1 && r[0].centre[1] == 2.5f && r[0].centre[2] == 1);
    assert(r[0].half[0] == 2 && r[0].half[1] == 2.5f && r[0].half[2] == 3);
    assert(r[0].key[0] == -1 && r[0].key[1] == 0 && r[0].key[2] == -2);
    /* corners 2 1 3: x -1..3, y 0..5, z -2..4, keyed by corner 2 */
    assert(r[1].firstIndex == 33 && r[1].key[0] == -1 && r[1].key[1] == 5 && r[1].key[2] == 4);

    /* placed: bounds and key in the world */
    mesh.placed = moved;
    assert(sw_records_build(&mesh, &place, runs, 1, 0, r));
    /* (x, y, z) -> (10 - z, 20 + y, 30 + 2x) */
    assert(r[0].centre[0] == 9 && r[0].half[0] == 3);
    assert(r[0].centre[1] == 22.5f && r[0].half[1] == 2.5f);
    assert(r[0].centre[2] == 32 && r[0].half[2] == 4);
    assert(r[0].key[0] == 12 && r[0].key[1] == 20 && r[0].key[2] == 28);
    /* animated: in the mesh's own space, the matrix ignored */
    mesh.animated = 1;
    assert(sw_records_build(&mesh, &place, runs, 1, 0, r));
    assert(r[0].flags == (SW_CULL_NONE | SW_ANIMATED) && r[0].centre[0] == 1 && r[0].key[2] == -2);
    mesh.animated = 0;
    mesh.placed = NULL;

    /* what does not fit writes nothing */
    memset(r, 0xcd, sizeof r);
    runs[1].count = 4;
    assert(!sw_records_build(&mesh, &place, runs, 2, 0, r));
    runs[1].count = 6;
    assert(!sw_records_build(&mesh, &place, runs, 2, 0, r));
    runs[1].count = 0;
    assert(!sw_records_build(&mesh, &place, runs, 2, 0, r));
    runs[1].count = 3;
    runs[1].pass = SW_PASS_COUNT;
    assert(!sw_records_build(&mesh, &place, runs, 2, 0, r));
    runs[1].pass = SW_PASS_GROUND;
    runs[1].flags = 0x08;
    assert(!sw_records_build(&mesh, &place, runs, 2, 0, r));
    runs[1].flags = 0;
    runs[1].first = 0xfffffffeu;
    assert(!sw_records_build(&mesh, &place, runs, 2, 0, r));
    runs[1].first = 3;
    place.vertexCount = 5;
    assert(!sw_records_build(&mesh, &place, runs, 2, 0, r));
    place.vertexCount = 4;
    mesh.vertexCount = 3; /* index 3 now lies outside the vertices */
    place.vertexCount = 3;
    assert(!sw_records_build(&mesh, &place, runs, 2, 0, r));
    assert(r[0].firstIndex == 0xcdcdcdcdu);
    mesh.vertexCount = 4;
    place.vertexCount = 4;
    assert(sw_records_build(&mesh, &place, runs, 0, 0, NULL));

    /* a corner that is not finite: unbounded, kept by every view */
    memcpy(nan, quad, sizeof nan);
    nan[1][1] = NAN;
    mesh.positions = &nan[0][0];
    assert(sw_records_build(&mesh, &place, runs, 1, 0, r));
    assert(r[0].centre[0] == 0 && r[0].half[0] >= 1e30f && r[0].half[1] >= 1e30f);
}

/* each vertex carries the record whose triangles use it */
static void test_vertex_ids(void)
{
    static const float six[6][3] = {{0, 0, 0}, {1, 0, 0}, {0, 1, 0}, {1, 1, 0}, {2, 0, 0}, {2, 1, 0}};
    static const uint16_t indices[9] = {0, 1, 2, 2, 1, 3, 3, 4, 5};
    struct SwMesh mesh;
    struct SwRun runs[2];
    uint32_t ids[7];

    memset(&mesh, 0, sizeof mesh);
    mesh.positions = &six[0][0];
    mesh.stride = 3;
    mesh.vertexCount = 6;
    mesh.indices = indices;
    mesh.indexCount = 6;
    memset(runs, 0, sizeof runs);
    runs[0].first = 0;
    runs[0].count = 3;
    runs[1].first = 3;
    runs[1].count = 3;
    /* run 1 shares vertices 1 and 2 with run 0 */
    assert(!sw_vertex_ids(&mesh, runs, 2, 7, ids));
    runs[1].count = 0;
    assert(sw_vertex_ids(&mesh, runs, 1, 7, ids));
    assert(ids[0] == 7 && ids[1] == 7 && ids[2] == 7 && ids[3] == SW_NO_RECORD && ids[5] == SW_NO_RECORD);
    /* two runs of their own vertices; a vertex no index reaches has none */
    mesh.indexCount = 9;
    runs[0].count = 6;
    runs[1].first = 6;
    runs[1].count = 3;
    assert(!sw_vertex_ids(&mesh, runs, 2, 0, ids));
    {
        static const uint16_t apart[9] = {0, 1, 2, 2, 1, 3, 4, 5, 4};
        mesh.indices = apart;
        assert(sw_vertex_ids(&mesh, runs, 2, 100, ids));
        assert(ids[0] == 100 && ids[3] == 100 && ids[4] == 101 && ids[5] == 101);
        runs[1].first = 7;
        assert(!sw_vertex_ids(&mesh, runs, 2, 100, ids));
        runs[1].first = 6;
        mesh.vertexCount = 5;
        assert(!sw_vertex_ids(&mesh, runs, 2, 100, ids));
    }
}

/* the runs an indirect call each draws: records of one pass, one texture array, one lightmap
   array and one cull mode in a row */
static void test_runs(void)
{
    static struct SwRecord r[8];
    struct SwDrawRun runs[8];
    size_t n;
    int i;

    memset(r, 0, sizeof r);
    for (i = 0; i < 8; i++) {
        r[i].pass = SW_PASS_OPAQUE_CLAMP;
        r[i].lightmapArray = SW_NO_ARRAY;
        r[i].indexCount = 3u << i;
    }
    r[2].textureArray = 1;
    r[3].textureArray = 1;
    r[3].flags = SW_CULL_CCW;
    r[4].textureArray = 1;
    r[4].flags = SW_CULL_CCW | SW_ANIMATED;
    r[5].pass = SW_PASS_GROUND;
    r[5].textureArray = 1;
    r[5].lightmapArray = 0;
    r[6].pass = SW_PASS_GROUND;
    r[6].textureArray = 1;
    r[6].lightmapArray = 1;
    r[7].pass = SW_PASS_GROUND;
    r[7].textureArray = 1;
    r[7].lightmapArray = 1;
    n = sw_draw_runs(r, 8, runs);
    assert(n == 5);
    assert(runs[0].first == 0 && runs[0].count == 2 && runs[0].textureArray == 0 && runs[0].cull == 0);
    assert(runs[0].indices == 3 + 6 && runs[4].indices == (3u << 6) + (3u << 7));
    assert(runs[1].first == 2 && runs[1].count == 1 && runs[1].textureArray == 1);
    assert(runs[2].first == 3 && runs[2].count == 2 && runs[2].cull == SW_CULL_CCW);
    assert(runs[3].first == 5 && runs[3].count == 1 && runs[3].pass == SW_PASS_GROUND && runs[3].lightmapArray == 0);
    assert(runs[4].first == 6 && runs[4].count == 2 && runs[4].lightmapArray == 1);
    assert(sw_draw_runs(r, 0, runs) == 0);
}

static void random_record(struct SwRecord *r)
{
    memset(r, 0, sizeof *r);
    r->pass = (uint8_t)(next() % SW_PASS_COUNT);
    r->sort[0] = next() % 5;
    r->sort[1] = next() % 3;
    r->firstIndex = next() % 7 * 3;
    r->indexCount = 3 + next() % 2 * 3;
    r->baseVertex = next() % 3;
    r->textureArray = (uint8_t)(next() % 2);
    r->textureLayer = (uint16_t)(next() % 4);
    r->lightmapArray = (uint8_t)(next() % 2);
    r->lightmapLayer = (uint16_t)(next() % 2);
    r->transform = (uint16_t)(next() % 2);
    r->uv = (uint16_t)(next() % 2);
    r->flags = (uint8_t)(next() % 2);
    r->id = next() % 1000;
}

static int same_draw(const struct SwRecord *a, const struct SwRecord *b)
{
    return a->pass == b->pass && a->sort[0] == b->sort[0] && a->sort[1] == b->sort[1] && a->firstIndex == b->firstIndex
           && a->indexCount == b->indexCount && a->baseVertex == b->baseVertex
           && a->textureArray == b->textureArray && a->textureLayer == b->textureLayer
           && a->lightmapArray == b->lightmapArray && a->lightmapLayer == b->lightmapLayer
           && a->transform == b->transform && a->uv == b->uv && a->flags == b->flags && a->id == b->id;
}

static void test_sort(void)
{
    enum { N = 300 };
    static struct SwRecord a[N], b[N];
    int round, i;

    for (round = 0; round < 200; round++) {
        int n = (int)(next() % N);

        for (i = 0; i < n; i++)
            random_record(&a[i]);
        memcpy(b, a, sizeof a);
        for (i = n - 1; i > 0; i--) {
            int j = (int)(next() % (uint32_t)(i + 1));
            struct SwRecord t = b[i];

            b[i] = b[j];
            b[j] = t;
        }
        sw_records_sort(a, (size_t)n);
        /* b's two halves sorted apart and merged by sw_record_before: the same order */
        {
            static struct SwRecord merged[N];
            int half = n ? (int)(next() % (uint32_t)n) : 0, x = 0, y = half, k = 0;

            sw_records_sort(b, (size_t)half);
            sw_records_sort(b + half, (size_t)(n - half));
            while (x < half || y < n)
                merged[k++] = y >= n || (x < half && sw_record_before(&b[x], &b[y])) ? b[x++] : b[y++];
            memcpy(b, merged, (size_t)n * sizeof *b);
        }
        for (i = 0; i < n; i++)
            assert(same_draw(&a[i], &b[i]));
        for (i = 1; i < n; i++)
            assert(sw_record_before(&a[i - 1], &a[i]) && !sw_record_before(&a[i], &a[i - 1]));
        for (i = 1; i < n; i++) {
            const struct SwRecord *x = &a[i - 1], *y = &a[i];

            assert(x->pass <= y->pass);
            if (x->pass == y->pass) {
                assert(x->sort[0] <= y->sort[0]);
                if (x->sort[0] == y->sort[0]) {
                    assert(x->sort[1] <= y->sort[1]);
                    if (x->sort[1] == y->sort[1]) {
                        assert((x->flags & SW_CULL_MASK) <= (y->flags & SW_CULL_MASK));
                        if ((x->flags & SW_CULL_MASK) == (y->flags & SW_CULL_MASK))
                            assert(x->firstIndex <= y->firstIndex);
                    }
                }
            }
        }
    }
    sw_records_sort(NULL, 5);
}

/* a camera at `eye` looking along yaw and pitch, as the client's view matrices are made: rows of
   a rotation, then the eye moved into view space */
static void random_view(float *m)
{
    float yaw = uniform(-3.14159f, 3.14159f), pitch = uniform(-1.5f, 1.5f), roll = uniform(-0.2f, 0.2f);
    float cy = cosf(yaw), sy = sinf(yaw), cp = cosf(pitch), sp = sinf(pitch), cr = cosf(roll), sr = sinf(roll);
    float f[3] = {sy * cp, sp, cy * cp}, r[3] = {cy, 0, -sy}, u[3], r2[3], eye[3];
    int i;

    u[0] = f[1] * r[2] - f[2] * r[1];
    u[1] = f[2] * r[0] - f[0] * r[2];
    u[2] = f[0] * r[1] - f[1] * r[0];
    for (i = 0; i < 3; i++) {
        r2[i] = r[i] * cr + u[i] * sr;
        u[i] = u[i] * cr - r[i] * sr;
        eye[i] = uniform(-2000, 2000);
    }
    for (i = 0; i < 3; i++) {
        m[i * 3 + 0] = r2[i];
        m[i * 3 + 1] = u[i];
        m[i * 3 + 2] = f[i];
    }
    for (i = 0; i < 3; i++)
        m[9 + i] = -(eye[0] * m[i] + eye[1] * m[3 + i] + eye[2] * m[6 + i]);
}

static void random_box(float *centre, float *half)
{
    int k;

    for (k = 0; k < 3; k++) {
        centre[k] = uniform(-3000, 3000);
        half[k] = next() % 8 == 0 ? 0.0f : uniform(0, 400);
    }
}

static void world_record(struct SwRecord *r, const float *centre, const float *half, int pass)
{
    memset(r, 0, sizeof *r);
    memcpy(r->centre, centre, sizeof r->centre);
    memcpy(r->half, half, sizeof r->half);
    memcpy(r->key, centre, sizeof r->key);
    r->pass = (uint8_t)pass;
}

static float plane_at(const float *n, const float *p)
{
    return n[0] * p[0] + n[1] * p[1] + n[2] * p[2] + n[3];
}

static struct Renderer screen;

/* the projection a renderer of w by h pixels draws with (render/rp_screen.c) */
static void screen_projection(struct SwProjection *p, int w, int h)
{
    renderer_set_projection(&screen, w, h);
    p->proj[0] = screen.proj[0];
    p->proj[1] = screen.proj[1];
    p->centre[0] = (float)screen.centreX;
    p->centre[1] = (float)screen.centreY;
    p->size[0] = (float)screen.width;
    p->size[1] = (float)screen.height;
    p->nearZ = RENDER_NEAR_Z;
    p->farZ = RENDER_FAR_Z;
}

/* the sides of the drawn volume a view-space point lies beyond (bit i: side i, in sw_frustum's
   order), with `slack` of each side's own size to spare */
static int beyond(const struct SwProjection *p, const double *v, double slack)
{
    double x = p->proj[0] * v[0], y = p->proj[1] * v[1], z = v[2];
    double side[6] = {x + p->centre[0] * z, (p->size[0] - p->centre[0]) * z - x, y + p->centre[1] * z,
                      (p->size[1] - p->centre[1]) * z - y, z - p->nearZ, p->farZ - z};
    double size[6] = {fabs(x) + fabs(p->centre[0] * z), fabs((p->size[0] - p->centre[0]) * z) + fabs(x),
                      fabs(y) + fabs(p->centre[1] * z), fabs((p->size[1] - p->centre[1]) * z) + fabs(y),
                      fabs(z) + p->nearZ, fabs(z) + p->farZ};
    int i, mask = 0;

    for (i = 0; i < 6; i++)
        if (side[i] < slack * (1.0 + size[i]))
            mask |= 1 << i;
    return mask;
}

/* world/frustum.c's screen for the same projection */
static void frustum_screen(struct FrustumScreen *out, const struct SwProjection *p)
{
    out->proj[0] = p->proj[0];
    out->proj[1] = p->proj[1];
    out->centre[0] = p->centre[0];
    out->centre[1] = p->centre[1];
    out->size[0] = p->size[0];
    out->size[1] = p->size[1];
}

static void view_point(const float *m, const float *p, double *out)
{
    int k;

    for (k = 0; k < 3; k++)
        out[k] = (double)p[0] * m[k] + (double)p[1] * m[3 + k] + (double)p[2] * m[6 + k] + m[9 + k];
}

/* The cull's planes are the projection's own, for every screen size, odd ones included: a box the
   cull leaves out has every corner beyond one side of what the projection draws. Against
   world/frustum.c, which the per-leaf queuing culls with and which builds its sides from the
   projection too: the two differ only within rounding of a side, at any size, and both keep what
   shows only in the last column or row of an odd screen. */
static void test_frustum(void)
{
    static struct ViewFrustum reference;
    struct SwProjection p;
    struct SwFrustum f;
    int round, i, k, disagree = 0;

    for (round = 0; round < 2000; round++) {
        float view[12], centre[3], half[3];
        int w = 640 + (int)(next() % 1400), h = 480 + (int)(next() % 900);
        struct FrustumScreen fs;
        struct SwRecord r;
        int n;

        screen_projection(&p, w, h);
        random_view(view);
        sw_frustum(&f, &p, view);
        frustum_screen(&fs, &p);
        view_frustum_build(&reference, &fs, view, NULL);
        for (i = 0; i < 20; i++) {
            struct FrustumObb obb;
            int corner, all = 0x3f, any = 0, today;

            random_box(centre, half);
            world_record(&r, centre, half, SW_PASS_OPAQUE_CLAMP);
            n = sw_record_in_view(&r, NULL, &f);
            for (corner = 0; corner < 8; corner++) {
                float q[3];
                double v[3];

                for (k = 0; k < 3; k++)
                    q[k] = centre[k] + (corner & (1 << k) ? half[k] : -half[k]);
                view_point(view, q, v);
                all &= beyond(&p, v, 1e-5);
                any |= beyond(&p, v, -1e-5);
            }
            if (n == 0)
                assert(all);
            memset(&obb, 0, sizeof obb);
            for (k = 0; k < 3; k++) {
                obb.centre[k] = centre[k];
                obb.extent[k] = half[k];
                obb.axis[k][k] = 1.0f;
            }
            today = view_frustum_test_obb(&reference, &obb) != FRUSTUM_OUTSIDE;
            if (today != (n != 0)) {
                /* within rounding of a side: some corner near it */
                assert(any);
                disagree++;
            }
        }
    }
    assert(disagree < 20);
    printf("static_world: world/frustum.c decides %d of the boxes otherwise, each at a side\n", disagree);

    /* a screen 1025 by 769 pixels wide: a box that shows only in the last column */
    {
        float view[12] = {1, 0, 0, 0, 1, 0, 0, 0, 1, 0, 0, 0}, centre[3], half[3] = {0.001f, 0.001f, 0.001f};
        struct FrustumScreen fs;
        struct FrustumObb obb;
        struct SwRecord r;

        screen_projection(&p, 1025, 769);
        sw_frustum(&f, &p, view);
        frustum_screen(&fs, &p);
        view_frustum_build(&reference, &fs, view, NULL);
        centre[2] = 100.0f;
        centre[0] = (1024.5f - p.centre[0]) * centre[2] / p.proj[0];
        centre[1] = 0.0f;
        world_record(&r, centre, half, SW_PASS_OPAQUE_CLAMP);
        assert(sw_record_in_view(&r, NULL, &f));
        memset(&obb, 0, sizeof obb);
        memcpy(obb.centre, centre, sizeof obb.centre);
        memcpy(obb.extent, half, sizeof obb.extent);
        for (k = 0; k < 3; k++)
            obb.axis[k][k] = 1.0f;
        assert(view_frustum_test_obb(&reference, &obb) != FRUSTUM_OUTSIDE);
    }
}

/* a turn, a scale (some mirrored) and a move */
static void random_transform(float *m)
{
    float a = uniform(-3.2f, 3.2f), b = uniform(-3.2f, 3.2f), s[3];
    float ca = cosf(a), sa = sinf(a), cb = cosf(b), sb = sinf(b);
    float rot[9] = {ca, sa * cb, sa * sb, -sa, ca * cb, ca * sb, 0, -sb, cb};
    int i, k;

    for (k = 0; k < 3; k++)
        s[k] = uniform(0.2f, 3.0f) * (next() % 5 == 0 ? -1.0f : 1.0f);
    for (i = 0; i < 3; i++)
        for (k = 0; k < 3; k++)
            m[i * 3 + k] = rot[i * 3 + k] * s[i];
    for (k = 0; k < 3; k++)
        m[9 + k] = uniform(-1000, 1000);
}

static void transform_point(const float *m, const float *p, float *out)
{
    out[0] = p[0] * m[0] + p[1] * m[3] + p[2] * m[6] + m[9];
    out[1] = p[0] * m[1] + p[1] * m[4] + p[2] * m[7] + m[10];
    out[2] = p[0] * m[2] + p[1] * m[5] + p[2] * m[8] + m[11];
}

/* an animated record is moved by its transform: dropped only where every corner of the moved box
   lies outside one plane */
static void test_animated(void)
{
    static float transforms[4][12];
    struct SwProjection projection;
    struct SwFrustum f;
    int round, i, k, dropped = 0;

    for (round = 0; round < 3000; round++) {
        float view[12], centre[3], half[3];
        struct SwRecord r;

        random_view(view);
        screen_projection(&projection, 1024, 768);
        sw_frustum(&f, &projection, view);
        for (i = 0; i < 4; i++)
            random_transform(transforms[i]);
        random_box(centre, half);
        for (k = 0; k < 3; k++)
            centre[k] *= 0.3f;
        world_record(&r, centre, half, SW_PASS_ALPHA_WRAP);
        r.flags = SW_ANIMATED;
        r.transform = (uint16_t)(next() % 4);
        if (sw_record_in_view(&r, (const float (*)[12])transforms, &f))
            continue;
        dropped++;
        {
            int plane, wholly = 0;

            for (plane = 0; plane < 6 && !wholly; plane++) {
                int corner, all = 1;

                for (corner = 0; corner < 8 && all; corner++) {
                    float p[3], q[3];

                    for (k = 0; k < 3; k++)
                        p[k] = centre[k] + (corner & (1 << k) ? half[k] : -half[k]);
                    transform_point(transforms[r.transform], p, q);
                    all = plane_at(f.plane[plane], q) <= 1e-2f * (1.0f + fabsf(f.plane[plane][3]));
                }
                wholly = all;
            }
            assert(wholly);
        }
    }
    assert(dropped > 100);
    /* without transforms an animated record is not drawn */
    {
        float view[12], centre[3] = {0, 0, 0}, half[3] = {1, 1, 1};
        struct SwRecord r;

        random_view(view);
        world_record(&r, centre, half, SW_PASS_GROUND);
        r.flags = SW_ANIMATED;
        assert(!sw_record_in_view(&r, NULL, &f));
    }
}

/* Whether a box reaches a cascade, worked out in double: its extent along each of the cascade's
   axes overlaps the square across the light and the depth along it. 1 or 0 where it does or does
   not by more than a thousandth of a unit, -1 within that, where floats may round either way. */
static int box_reaches(const struct SwCascade *c, const float *centre, const float *half)
{
    double lo[3], hi[3], least = 1e300;
    int a, k;

    for (a = 0; a < 3; a++) {
        double mid = 0.0, ext = 0.0;

        for (k = 0; k < 3; k++) {
            mid += (double)centre[k] * c->axis[a][k];
            ext += (double)half[k] * fabs((double)c->axis[a][k]);
        }
        lo[a] = mid - ext;
        hi[a] = mid + ext;
    }
    for (a = 0; a < 3; a++) {
        double from = a < 2 ? c->lo[a] : c->depth[0], to = a < 2 ? c->hi[a] : c->depth[1];
        double over = (hi[a] < to ? hi[a] : to) - (lo[a] > from ? lo[a] : from);

        least = over < least ? over : least;
    }
    return least > 1e-3 ? 1 : least < -1e-3 ? 0 : -1;
}

/* the casters a cascade keeps: those whose boxes reach it, in record order */
static void test_cascade(void)
{
    static struct SwRecord r[64];
    static uint32_t out[64];
    int round, i, a, k;

    for (round = 0; round < 3000; round++) {
        struct SwCascade c;
        float t[12];
        size_t n, j = 0;

        random_transform(t);
        for (a = 0; a < 3; a++) {
            float len = sqrtf(t[a * 3] * t[a * 3] + t[a * 3 + 1] * t[a * 3 + 1] + t[a * 3 + 2] * t[a * 3 + 2]);

            for (k = 0; k < 3; k++)
                c.axis[a][k] = t[a * 3 + k] / len;
        }
        for (k = 0; k < 2; k++) {
            float mid = uniform(-2000, 2000), radius = uniform(8, 1500);

            c.lo[k] = mid - radius;
            c.hi[k] = mid + radius;
        }
        c.depth[0] = uniform(-3000, 0);
        c.depth[1] = c.depth[0] + uniform(10, 5000);
        for (i = 0; i < 64; i++) {
            float centre[3], half[3];

            random_box(centre, half);
            world_record(&r[i], centre, half, (int)(next() % SW_PASS_COUNT));
        }
        n = sw_cull_cascade(r, 64, NULL, &c, out);
        for (i = 0; i < 64; i++) {
            int reaches = box_reaches(&c, r[i].centre, r[i].half);

            if (j < n && out[j] == (uint32_t)i) {
                assert(reaches != 0);
                j++;
            } else {
                assert(reaches != 1);
            }
        }
        assert(j == n);
    }
}

static void test_textures(void)
{
    struct SwTextureArray storage[4];
    struct SwTextureArrays arrays = {storage, 0, 4, 2};
    uint16_t l;
    uint8_t a;

    /* two layers an array: a third texture of a size opens a second array of it; another number of
       mipmap levels is another array */
    assert(sw_texture_place(&arrays, 256, 256, 9, &a, &l) == 2 && a == 0 && l == 0);
    assert(sw_texture_place(&arrays, 128, 256, 9, &a, &l) == 2 && a == 1 && l == 0);
    assert(sw_texture_place(&arrays, 256, 256, 9, &a, &l) == 1 && a == 0 && l == 1);
    assert(sw_texture_place(&arrays, 256, 256, 9, &a, &l) == 2 && a == 2 && l == 0);
    assert(sw_texture_place(&arrays, 128, 256, 9, &a, &l) == 1 && a == 1 && l == 1);
    assert(sw_texture_place(&arrays, 256, 256, 9, &a, &l) == 1 && a == 2 && l == 1);
    assert(sw_texture_place(&arrays, 256, 256, 1, &a, &l) == 2 && a == 3 && l == 0);
    assert(arrays.count == 4 && storage[0].layers == 2 && storage[2].layers == 2 && storage[3].levels == 1);
    /* full: a size with room still places, a new array does not */
    assert(sw_texture_place(&arrays, 256, 256, 1, &a, &l) == 1 && a == 3 && l == 1);
    assert(!sw_texture_place(&arrays, 256, 256, 1, &a, &l));
    assert(!sw_texture_place(&arrays, 32, 32, 6, &a, &l));
    assert(!sw_texture_place(&arrays, 0, 32, 1, &a, &l));
    assert(!sw_texture_place(&arrays, 32, 32, 0, &a, &l));
    assert(arrays.count == 4 && storage[3].layers == 2);
    /* taken back: the layer placed last, and the array that placement opened */
    sw_texture_unplace(&arrays, 3, 1);
    assert(arrays.count == 4 && storage[3].layers == 1);
    sw_texture_unplace(&arrays, 3, 0);
    assert(arrays.count == 3);
    assert(sw_texture_place(&arrays, 64, 64, 7, &a, &l) == 2 && a == 3 && l == 0 && storage[3].width == 64);
    sw_texture_unplace(&arrays, 1, 0); /* not the layer placed last: nothing */
    assert(storage[1].layers == 2);
    arrays.maxLayers = 0;
    assert(!sw_texture_place(&arrays, 256, 256, 9, &a, &l));
    /* at most SW_NO_ARRAY arrays, whatever the capacity */
    {
        static struct SwTextureArray many[300];
        struct SwTextureArrays all = {many, 0, 300, 1};
        int i;

        for (i = 0; i < SW_NO_ARRAY; i++)
            assert(sw_texture_place(&all, (uint16_t)(1 << (i % 16)), (uint16_t)(1 << (i / 16)), 1, &a, &l) == 2 && a == i);
        assert(!sw_texture_place(&all, 32768, 32768, 1, &a, &l) && all.count == SW_NO_ARRAY);
    }
}

int main(void)
{
    test_layout();
    test_records();
    test_vertex_ids();
    test_sort();
    test_runs();
    test_frustum();
    test_animated();
    test_cascade();
    test_textures();
    puts("static_world: ok");
    return 0;
}
