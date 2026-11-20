/* render/skin_layout.c, Granny's actors as the skinning shader reads them: a vertex packed for its
   skeleton (bones that are not the skeleton's taken as bone 0, a rigid mesh's vertex on its bone
   alone), a mesh's indices placed in the model's, a pose's rows where skin.hlsli reads them, a
   vertex where Granny's deformer put it against that deformer's own code, and the batches of draws:
   fuzzed sequences of draws, joined as skin_batch_join joins them and drawn as the GPU draws a
   batch (instance by instance, each the whole range), draw the triangles in the order given.
   Fuzzed with a fixed seed. tests/test_skin_instances.c runs the shader on what this packs. */
#include "render/skin_layout.h"

#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <string.h>

static uint32_t seed = 0x2545f491u;

static uint32_t next(void)
{
    seed ^= seed << 13;
    seed ^= seed >> 17;
    seed ^= seed << 5;
    return seed;
}

static float small(void)
{
    return (float)((int)(next() % 17) - 8);
}

static uint32_t bits(float f)
{
    uint32_t u;
    memcpy(&u, &f, sizeof u);
    return u;
}

static void vertices(void)
{
    struct SkinSource in;
    struct SkinVertex out;
    int k;
    assert(sizeof out == 56);
    memset(&in, 0, sizeof in);
    in.position[0] = 1.5f;
    in.position[2] = -2.0f;
    in.normal[1] = 0.25f;
    in.uv[0] = 0.75f;
    in.uv[1] = -3.0f;
    for (k = 0; k < 4; k++)
        in.weights[k] = 0.125f * (float)(k + 1);
    in.bones[0] = 2;
    in.bones[1] = -1;
    in.bones[2] = 7;
    in.bones[3] = 6;
    /* skinned: its own bones, those outside a skeleton of 7 taken as 0 */
    skin_pack_vertex(&in, -1, 7, &out);
    assert(out.position[0] == 1.5f && out.position[2] == -2.0f && out.normal[1] == 0.25f);
    assert(out.uv[0] == 0.75f && out.uv[1] == -3.0f && !out.flags);
    assert(out.bones[0] == 2 && out.bones[1] == 0 && out.bones[2] == 0 && out.bones[3] == 6);
    for (k = 0; k < 4; k++)
        assert(out.weights[k] == in.weights[k]);
    /* a skeleton larger than the device's takes its first SKIN_BONES_MAX alone */
    in.bones[2] = SKIN_BONES_MAX - 1;
    in.bones[3] = SKIN_BONES_MAX;
    skin_pack_vertex(&in, -1, SKIN_BONES_MAX + 40, &out);
    assert(out.bones[2] == SKIN_BONES_MAX - 1 && out.bones[3] == 0);
    /* rigid: its mesh's bone alone, weighed 1, its own bones and weights not read */
    skin_pack_vertex(&in, 5, 7, &out);
    assert(out.flags == SKIN_RIGID && out.bones[0] == 5 && out.weights[0] == 1.0f);
    for (k = 1; k < 4; k++)
        assert(out.bones[k] == 0 && out.weights[k] == 0.0f);
    assert(out.position[0] == 1.5f && out.normal[1] == 0.25f);
    skin_pack_vertex(&in, 9, 7, &out);
    assert(out.flags == SKIN_RIGID && out.bones[0] == 0);
    skin_pack_vertex(&in, 0, 0, &out);
    assert(out.flags == SKIN_RIGID && out.bones[0] == 0);
}

static void indices(void)
{
    static const uint16_t in[6] = { 0, 4, 2, 4, 1, 3 };
    uint32_t out[6];
    int i;
    assert(skin_pack_indices(in, 6, 5, 100, out));
    for (i = 0; i < 6; i++)
        assert(out[i] == 100u + in[i]);
    /* an index past the mesh's vertices, or a place past 2^32 - 1 */
    assert(!skin_pack_indices(in, 6, 4, 100, out));
    assert(skin_pack_indices(in, 6, 5, 0xffffffffu - 4, out) && out[1] == 0xffffffffu);
    assert(!skin_pack_indices(in, 6, 5, 0xffffffffu - 3, out));
    assert(!skin_pack_indices(in, 6, 0, 0, out));
    assert(skin_pack_indices(in, 0, 0, 0, out));
}

/* the deformer's sum, as resources/granny_gltf.c's _GrannyDeformVertices computed each vertex */
static void deformer(const struct SkinVertex *v, const float *composite, float *p)
{
    int k;
    p[0] = p[1] = p[2] = 0.0f;
    for (k = 0; k < 4; k++) {
        const float *m;
        float w = v->weights[k];
        if (w == 0.0f)
            continue;
        m = composite + v->bones[k] * 16;
        p[0] += w * (v->position[0] * m[0] + v->position[1] * m[4] + v->position[2] * m[8] + m[12]);
        p[1] += w * (v->position[0] * m[1] + v->position[1] * m[5] + v->position[2] * m[9] + m[13]);
        p[2] += w * (v->position[0] * m[2] + v->position[1] * m[6] + v->position[2] * m[10] + m[14]);
    }
}

static float dot4(const float *row, const float *p, float w)
{
    return p[0] * row[0] + p[1] * row[1] + p[2] * row[2] + w * row[3];
}

static void poses(void)
{
    static float composite[SKIN_BONES_MAX + 1][16];
    static float rows[SKIN_POSE_HEADER + 3 * (SKIN_BONES_MAX + 1)][4];
    struct SkinPose pose;
    uint32_t u;
    int round, b, i;
    assert(skin_pose_size(0) == SKIN_POSE_HEADER + 3 && skin_pose_size(1) == SKIN_POSE_HEADER + 3);
    assert(skin_pose_size(SKIN_BONES_MAX) == SKIN_POSE_HEADER + 3 * SKIN_BONES_MAX && !skin_pose_size(SKIN_BONES_MAX + 1));
    for (round = 0; round < 200; round++) {
        float p[3], inView[3];
        memset(&pose, 0, sizeof pose);
        pose.boneCount = round % 7 == 0 ? 0 : 1 + next() % SKIN_BONES_MAX;
        for (b = 0; b < (int)pose.boneCount; b++)
            for (i = 0; i < 16; i++)
                composite[b][i] = small();
        pose.bones = &composite[0][0];
        for (i = 0; i < 12; i++)
            pose.view[i] = small();
        pose.proj[0] = small();
        pose.proj[1] = small();
        pose.centre[0] = small();
        pose.centre[1] = small();
        pose.nearZ = small();
        pose.depthScale = small();
        for (i = 0; i < 3; i++)
            pose.light[i] = small();
        pose.scale = small();
        pose.offset = small() + 0.5f;
        pose.add = next();
        pose.alpha = (uint8_t)next();
        pose.rigidStill = (int)(next() & 1);
        memset(rows, 0xab, sizeof rows);
        assert(skin_pose_rows(&pose, rows) == skin_pose_size(pose.boneCount));
        /* the header */
        memcpy(&u, &rows[0][0], 4);
        assert(u == (pose.boneCount ? pose.boneCount : 1));
        memcpy(&u, &rows[0][1], 4);
        assert(u == ((pose.rigidStill ? 1u : 0u) | (uint32_t)pose.alpha << 8));
        memcpy(&u, &rows[0][2], 4);
        assert(u == pose.add);
        assert(bits(rows[0][3]) == bits(pose.offset));
        assert(rows[4][0] == pose.light[0] && rows[4][1] == pose.light[1] && rows[4][2] == pose.light[2]);
        assert(rows[4][3] == pose.scale);
        assert(rows[5][0] == pose.proj[0] && rows[5][1] == pose.proj[1] && rows[5][2] == pose.centre[0]);
        assert(rows[5][3] == pose.centre[1] && rows[6][0] == pose.nearZ && rows[6][1] == pose.depthScale);
        assert(rows[6][2] == 0.0f && rows[6][3] == 0.0f);
        /* the view matrix's columns take a point as render/math3d.h's 4x3 does */
        for (i = 0; i < 3; i++)
            p[i] = small();
        for (i = 0; i < 3; i++) {
            inView[i] = p[0] * pose.view[i] + p[1] * pose.view[3 + i] + p[2] * pose.view[6 + i] + pose.view[9 + i];
            assert(dot4(rows[1 + i], p, 1.0f) == inView[i]);
        }
        /* each bone's columns take a point as its composite does, row vectors */
        for (b = 0; b < (int)pose.boneCount; b++)
            for (i = 0; i < 3; i++) {
                const float *m = composite[b];
                float want = p[0] * m[i] + p[1] * m[4 + i] + p[2] * m[8 + i] + m[12 + i];
                assert(dot4(rows[SKIN_POSE_HEADER + b * 3 + i], p, 1.0f) == want);
                assert(dot4(rows[SKIN_POSE_HEADER + b * 3 + i], p, 0.0f) == want - m[12 + i]);
            }
        if (!pose.boneCount)
            for (i = 0; i < 3; i++)
                assert(!rows[SKIN_POSE_HEADER + i][0] && !rows[SKIN_POSE_HEADER + i][3]);
        /* nothing past its rows */
        u = (uint32_t)skin_pose_size(pose.boneCount);
        assert(bits(rows[u][0]) == 0xabababab);
    }
    pose.boneCount = SKIN_BONES_MAX + 1;
    memset(rows, 0xab, sizeof rows);
    assert(!skin_pose_rows(&pose, rows) && bits(rows[0][0]) == 0xabababab);
}

/* where Granny's deformer put a vertex: the deformer's own code, bit for bit, for skeletons of up
   to SKIN_BONES_MAX bones and every kind of weight; a rigid vertex where it lies */
static void positions(void)
{
    static float composite[SKIN_BONES_MAX][16];
    struct SkinVertex v;
    float got[3], want[3];
    int round, b, i, k;
    for (round = 0; round < 100000; round++) {
        uint32_t bones = 1 + next() % SKIN_BONES_MAX;
        memset(&v, 0, sizeof v);
        for (i = 0; i < 3; i++)
            v.position[i] = (float)(int)(next() % 2001 - 1000) / 7.0f;
        for (k = 0; k < 4; k++) {
            v.bones[k] = (uint8_t)(next() % bones);
            v.weights[k] = next() % 3 == 0 ? 0.0f : (float)(next() % 256) / 255.0f;
        }
        for (b = 0; b < 4; b++)
            for (i = 0; i < 16; i++)
                composite[v.bones[b]][i] = (float)(int)(next() % 20001 - 10000) / 13.0f;
        deformer(&v, &composite[0][0], want);
        skin_vertex_position(&v, &composite[0][0], bones, got);
        assert(!memcmp(got, want, sizeof got));
    }
    /* a weight of 0 is not taken, whatever its bone's matrix holds */
    memset(&v, 0, sizeof v);
    v.position[0] = 1.0f;
    v.weights[0] = 1.0f;
    v.bones[1] = 1;
    for (i = 0; i < 16; i++) {
        composite[0][i] = i % 5 == 0 ? 1.0f : 0.0f;
        composite[1][i] = INFINITY;
    }
    skin_vertex_position(&v, &composite[0][0], 2, got);
    assert(got[0] == 1.0f && got[1] == 0.0f && got[2] == 0.0f);
    /* a bone the pose does not have is its last; no bones, the origin */
    v.bones[0] = 5;
    composite[1][12] = 3.0f;
    for (i = 0; i < 16; i++)
        composite[1][i] = i % 5 == 0 ? 1.0f : i == 12 ? 3.0f : 0.0f;
    skin_vertex_position(&v, &composite[0][0], 2, got);
    assert(got[0] == 4.0f);
    skin_vertex_position(&v, &composite[0][0], 0, got);
    assert(got[0] == 0.0f && got[1] == 0.0f && got[2] == 0.0f);
    v.flags = SKIN_RIGID;
    v.position[1] = -2.0f;
    skin_vertex_position(&v, &composite[0][0], 0, got);
    assert(got[0] == 1.0f && got[1] == -2.0f && got[2] == 0.0f);
}

static void batches(void)
{
    struct SkinBatch b = { 6, 6, 1, 3 };
    /* an actor's next mesh runs on; another actor's same mesh, or the same again, is an instance */
    assert(skin_batch_join(&b, 12, 3, 3) == SKIN_JOIN_EXTEND);
    assert(skin_batch_join(&b, 6, 6, 4) == SKIN_JOIN_INSTANCE);
    assert(skin_batch_join(&b, 6, 6, 3) == SKIN_JOIN_INSTANCE);
    assert(skin_batch_join(&b, 12, 3, 4) == SKIN_JOIN_NEW);
    assert(skin_batch_join(&b, 0, 6, 3) == SKIN_JOIN_NEW);
    assert(skin_batch_join(&b, 6, 3, 3) == SKIN_JOIN_NEW);
    /* once instanced, a batch runs on no more */
    b.instances = 2;
    assert(skin_batch_join(&b, 12, 3, 3) == SKIN_JOIN_NEW);
    assert(skin_batch_join(&b, 6, 6, 9) == SKIN_JOIN_INSTANCE);
    /* nor past 2^32 - 1 indices */
    b.instances = 1;
    b.first = 0;
    b.count = 0xfffffff0u;
    assert(skin_batch_join(&b, 0xfffffff0u, 15, 3) == SKIN_JOIN_EXTEND);
    assert(skin_batch_join(&b, 0xfffffff0u, 18, 3) == SKIN_JOIN_NEW);
}

/* Draws joined as the device joins them, then expanded as the GPU draws each batch, give the
   triangles (pose, first index) in the order of the draws. */
#define DRAWS 64
static void batch_order(void)
{
    struct { uint32_t first, count; int pose; } draws[DRAWS];
    static uint32_t given[DRAWS * 8][2], drawn[DRAWS * 8][2];
    struct SkinBatch batches[DRAWS];
    int poses[DRAWS][DRAWS];
    int round, i, n, nb, ng, nd;
    for (round = 0; round < 20000; round++) {
        /* a model of meshes of 3, 6 or 9 indices one behind the other, drawn by a few actors */
        uint32_t places[8], sizes[8];
        int meshes = 1 + (int)(next() % 8), actors = 1 + (int)(next() % 4);
        n = 1 + (int)(next() % DRAWS);
        places[0] = 0;
        for (i = 0; i < meshes; i++) {
            sizes[i] = 3 * (1 + next() % 3);
            if (i + 1 < meshes)
                places[i + 1] = places[i] + sizes[i];
        }
        ng = 0;
        for (i = 0; i < n; i++) {
            int m = (int)(next() % (uint32_t)meshes);
            uint32_t t;
            /* actors' meshes in order now and then, so that runs on and instances come up */
            if (i && next() % 2 && draws[i - 1].first + draws[i - 1].count < places[meshes - 1] + sizes[meshes - 1]) {
                draws[i].first = draws[i - 1].first + draws[i - 1].count;
                for (m = 0; places[m] != draws[i].first; m++)
                    ;
                draws[i].pose = draws[i - 1].pose;
            } else {
                draws[i].first = places[m];
                draws[i].pose = (int)(next() % (uint32_t)actors);
            }
            draws[i].count = sizes[m];
            for (t = 0; t < draws[i].count; t += 3) {
                given[ng][0] = (uint32_t)draws[i].pose;
                given[ng++][1] = draws[i].first + t;
            }
        }
        nb = 0;
        for (i = 0; i < n; i++) {
            int join = nb ? skin_batch_join(&batches[nb - 1], draws[i].first, draws[i].count, draws[i].pose)
                          : SKIN_JOIN_NEW;
            if (join == SKIN_JOIN_EXTEND) {
                batches[nb - 1].count += draws[i].count;
            } else if (join == SKIN_JOIN_INSTANCE) {
                poses[nb - 1][batches[nb - 1].instances++] = draws[i].pose;
                batches[nb - 1].pose = draws[i].pose;
            } else {
                batches[nb].first = draws[i].first;
                batches[nb].count = draws[i].count;
                batches[nb].instances = 1;
                batches[nb].pose = draws[i].pose;
                poses[nb++][0] = draws[i].pose;
            }
        }
        nd = 0;
        for (i = 0; i < nb; i++) {
            uint32_t k, t;
            for (k = 0; k < batches[i].instances; k++)
                for (t = 0; t < batches[i].count; t += 3) {
                    drawn[nd][0] = (uint32_t)poses[i][k];
                    drawn[nd++][1] = batches[i].first + t;
                }
        }
        assert(nd == ng && !memcmp(drawn, given, (size_t)ng * sizeof given[0]));
    }
}

int main(void)
{
    vertices();
    indices();
    poses();
    positions();
    batches();
    batch_order();
    puts("skin layout: vertices bounded for their skeleton, rigid ones on their bone, indices placed, poses' rows, "
         "the deformer's positions bit for bit, and batches that keep the draws' order passed");
    return 0;
}
