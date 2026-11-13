#include "world/world_gpu.h"
#include "render/gfx.h"
#include "render/math3d.h"
#include "render/render_api.h"
#include "render/renderer.h"
#include "render/rp_pool.h"
#include "render/texture_types.h"
#include "resources/model_res.h"
#include "world/ground.h"
#include "world/model.h"
#include "world/model_node_util.h"
#include "world/scene_graph.h"
#include "world/state.h"

#include <math.h>
#include <stdlib.h>
#include <string.h>

/* custom: what the frame needs of each part, and its records' order, as the original drew the
   ground's and the models' triangles (tests/test_static_world_maps.c keeps the code that drew them
   before the static world, against which it checks this):
   - a ground block (32 by 32 cells) is one mesh, its runs of surfaces with one texture and one
     lightmap page each a record of the ground's pass, sorted by the texture's and the lightmap's
     creation order (render/crenderer.c sorted the ground's draws so), drawn with the camera's view
     (transform slot 0) and casting together;
   - a model node is one mesh, its runs of faces with one texture and one two-sidedness each a
     record of the pass its texture's alpha, the model's layer and its texture-coordinate animation
     send it to (render/rp_pool.c, renderer_add_prim's flags), culled as the node's winding and the
     model's mirroring say, sorted by its texture's creation order, drawn with its node's transform
     slot: the node's matrix under its parent's up to the model's own, which the device multiplies
     as the CPU did. A node with keys (or under one) gives the device its matrix each frame; a
     record under such a node is animated, its bounds in the node's space.
   Each frame takes a model's pose (its animated nodes' matrices and texture-coordinate animations)
   when the frame first draws it, as the CPU drew its triangles with the pose its first tick of the
   frame left (world/view_camera.c ticks a model once for each leaf it stands in); and queues its
   alpha-blended records then, in the order its nodes' runs were queued. Every record casts shadows;
   the device picks each cascade's casters itself (render/gfx.h). */

struct WorldTexture {
    const struct Texture *texture;
    uint8_t array;
    uint16_t layer;
};

struct WorldUv {
    struct C3dNode *node;
    int texture;
};

/* A model of the static world: its records (ids), its vertices, its transform slots (its own
   first), its nodes whose matrices the frame takes (animated) and its texture-coordinate
   animations (uvs), each a range of the world's; the frames (WorldGpu.frame) whose pose the device
   has of it and in which its alpha-blended records were queued; NULL once given up. */
struct WorldModel {
    struct C3dActor *actor;
    uint32_t firstRecord, records, vertices;
    uint32_t firstSlot, slots;
    uint32_t firstAnimated, animated, firstUv, uvs;
    uint32_t alphas;
    unsigned posed, queued;
};

/* the most faces of a node: its mesh's 16-bit indices reach no further */
#define NODE_FACES (0x10000 / 3)
/* the vertices of models given up after which the world is built again without them */
#define WORLD_GPU_GARBAGE (1u << 20)
/* no model (C3dActor.gpuModel) */
#define NO_MODEL 0xffffffffu

struct WorldGpu {
    int generation;   /* the device's world it built (gfx_world_generation) */
    int failed;       /* the ground could not be built: nothing is drawn of the map */
    size_t taken;     /* the models taken, world->models.data[0..taken) */
    unsigned stamp;   /* the leaves' (gpuSeen) */
    /* the frame begun last (world_gpu_update): its number, its view and frustum */
    unsigned frame;
    int framing;
    float view[12];
    struct SwFrustum frustum;
    struct GfxWorldDraw models, ground;
    struct SwLayout layout;
    struct SwTextureArray arrayStorage[SW_NO_ARRAY];
    struct SwTextureArrays arrays;
    struct WorldTexture *textures;
    size_t textureCount, textureCapacity;
    struct SwRecord *records;  /* by id */
    size_t recordCount, recordCapacity;
    float (*keyPoints)[3];     /* by id: the first corner of the record's first triangle, its mesh's own */
    size_t keyPointCapacity;
    unsigned char *dead;       /* by id: the record's model was given up */
    size_t deadCapacity;
    uint32_t garbage;          /* the vertices of models given up */
    /* the live records in draw order as the device has them, the ids it has, and whether they
       changed since (records built, or given up) */
    struct SwRecord *sorted, *merged;
    size_t sortedCount, sortedCapacity, mergedCapacity, sortedIds;
    int recordsDue;
    struct WorldModel *list;
    size_t modelCount, modelCapacity;
    struct GfxWorldSlot *slots;
    size_t slotCount, slotCapacity, slotsSent;
    struct C3dNode **animatedNodes;  /* the node of each of the frame's animated matrices */
    float (*animated)[12];
    size_t animatedCount, animatedCapacity, animatedMatrixCapacity;
    struct WorldUv *uvs;
    float (*uvMatrices)[6];
    /* the box every static record lies in, which with the animated records in their pose bounds
       the shadow maps along the light (renderer_shadow_bounds) */
    struct ShadowFitBox bounds;
    size_t uvCount, uvCapacity, uvMatrixCapacity;
    float (*placed)[12], (*views)[12];  /* by slot: a model's places in the world and views, as the frame needs them */
    size_t placedCapacity, viewCapacity;
    struct GfxWaterWave *waves;
    size_t waveCapacity;
    /* one mesh at a time */
    struct GfxMeshVertex *verts;
    WORD *indices;
    uint32_t *ids;
    struct SwRun *runs;
    struct GndBlockRun *blockRuns;
};

static int grow(void **p, size_t *capacity, size_t need, size_t size)
{
    size_t c = *capacity ? *capacity : 64;
    void *q;

    if (need <= *capacity)
        return 1;
    while (c < need)
        c *= 2;
    q = realloc(*p, c * size);
    if (!q)
        return 0;
    *p = q;
    *capacity = c;
    return 1;
}

static void reset(struct WorldGpu *g)
{
    int i;

    g->failed = 0;
    g->taken = 0;
    memset(&g->layout, 0, sizeof g->layout);
    g->arrays.arrays = g->arrayStorage;
    g->arrays.count = 0;
    g->arrays.capacity = SW_NO_ARRAY;
    g->arrays.maxLayers = GFX_WORLD_LAYERS;
    g->textureCount = g->recordCount = g->sortedCount = g->sortedIds = g->modelCount = 0;
    g->slotCount = g->slotsSent = g->animatedCount = g->uvCount = 0;
    g->garbage = 0;
    g->recordsDue = 1;
    for (i = 0; i < 3; i++) {
        g->bounds.min[i] = HUGE_VALF;
        g->bounds.max[i] = -HUGE_VALF;
    }
}

/* a box made to hold another, centre and half its size, where that one is bounded (a record with
   a corner that is not finite is bounded by nothing) */
static void bound(struct ShadowFitBox *box, const float *centre, const float *half)
{
    int i;

    for (i = 0; i < 3; i++)
        if (!(half[i] < 1e30f && isfinite(centre[i])))
            return;
    for (i = 0; i < 3; i++) {
        box->min[i] = centre[i] - half[i] < box->min[i] ? centre[i] - half[i] : box->min[i];
        box->max[i] = centre[i] + half[i] > box->max[i] ? centre[i] + half[i] : box->max[i];
    }
}

/* A texture's array and layer, placed on its first use; SW_NO_ARRAY for a texture without pixels
   or one the device does not take (drawn white, as a stage without a texture reads). */
static void place_texture(struct WorldGpu *g, const struct Texture *t, uint8_t *array, uint16_t *layer)
{
    size_t i;
    int w, h, levels, placed = 0;

    *array = SW_NO_ARRAY;
    *layer = 0;
    if (!t || !t->base.surface)
        return;
    for (i = g->textureCount; i-- > 0;)
        if (g->textures[i].texture == t) {
            *array = g->textures[i].array;
            *layer = g->textures[i].layer;
            return;
        }
    if (!grow((void **)&g->textures, &g->textureCapacity, g->textureCount + 1, sizeof *g->textures))
        return;
    gfx_get_size(t->base.surface, &w, &h);
    levels = gfx_get_levels(t->base.surface);
    if (w > 0 && h > 0 && w <= 0xffff && h <= 0xffff && levels > 0 && levels <= 0xff)
        placed = sw_texture_place(&g->arrays, (uint16_t)w, (uint16_t)h, (uint8_t)levels, array, layer);
    /* an array the device does not make is taken back; a layer it does not fill stays placed,
       unread, as the device's arrays stay in step with the placement */
    if (placed == 2 && !gfx_world_array(*array, t->base.surface)) {
        sw_texture_unplace(&g->arrays, *array, *layer);
        placed = 0;
    }
    if (!placed || !gfx_world_texture(t->base.surface, *array, *layer)) {
        *array = SW_NO_ARRAY;
        *layer = 0;
    }
    g->textures[g->textureCount].texture = t;
    g->textures[g->textureCount].array = *array;
    g->textures[g->textureCount].layer = *layer;
    g->textureCount++;
}

static uint32_t serial(const struct Texture *t)
{
    return t ? t->serial : 0;
}

/* A mesh into the shared buffers with the records of its runs; 0, with nothing added, where it
   does not fit. */
static int add_mesh(struct WorldGpu *g, uint32_t nverts, uint32_t nindices, uint16_t transform, int animated,
                    const float *placed, size_t count, struct SwPlacement *place)
{
    struct SwMesh mesh;
    struct SwLayout before = g->layout;
    size_t i;

    memset(&mesh, 0, sizeof mesh);
    mesh.positions = &g->verts[0].x;
    mesh.stride = sizeof(struct GfxMeshVertex) / sizeof(float);
    mesh.vertexCount = nverts;
    mesh.indices = g->indices;
    mesh.indexCount = nindices;
    mesh.transform = transform;
    mesh.animated = animated;
    mesh.placed = placed;
    if (!count || !grow((void **)&g->records, &g->recordCapacity, g->recordCount + count, sizeof *g->records)
        || !grow((void **)&g->dead, &g->deadCapacity, g->recordCount + count, sizeof *g->dead)
        || !grow((void **)&g->keyPoints, &g->keyPointCapacity, g->recordCount + count, sizeof *g->keyPoints)
        || !sw_layout_place(&g->layout, nverts, nindices, place))
        return 0;
    if (!sw_records_build(&mesh, place, g->runs, count, (uint32_t)g->recordCount, g->records + g->recordCount)
        || !sw_vertex_ids(&mesh, g->runs, count, (uint32_t)g->recordCount, g->ids)
        || !gfx_world_mesh(place, g->verts, g->ids, g->indices)) {
        g->layout = before;
        return 0;
    }
    for (i = 0; i < count; i++) {
        const struct GfxMeshVertex *v = &g->verts[g->indices[g->runs[i].first]];
        const struct SwRecord *r = &g->records[g->recordCount + i];

        g->keyPoints[g->recordCount + i][0] = v->x;
        g->keyPoints[g->recordCount + i][1] = v->y;
        g->keyPoints[g->recordCount + i][2] = v->z;
        if (!(r->flags & SW_ANIMATED))
            bound(&g->bounds, r->centre, r->half);
    }
    memset(g->dead + g->recordCount, 0, count);
    g->recordCount += count;
    g->recordsDue = 1;
    return 1;
}

/* A transform slot under `parent` (-1: the camera's view); with `node`, the frame's matrix of the
   node takes local's place. -1 where there is no room. */
static int add_slot(struct WorldGpu *g, const float *local, int parent, struct C3dNode *node)
{
    struct GfxWorldSlot *s;

    if (g->slotCount >= SW_NONE || !grow((void **)&g->slots, &g->slotCapacity, g->slotCount + 1, sizeof *g->slots)
        || !grow((void **)&g->placed, &g->placedCapacity, g->slotCount + 1, sizeof *g->placed)
        || !grow((void **)&g->views, &g->viewCapacity, g->slotCount + 1, sizeof *g->views))
        return -1;
    s = &g->slots[g->slotCount];
    memcpy(s->local, local, sizeof s->local);
    s->parent = parent;
    s->animated = -1;
    if (node) {
        if (!grow((void **)&g->animatedNodes, &g->animatedCapacity, g->animatedCount + 1, sizeof *g->animatedNodes)
            || !grow((void **)&g->animated, &g->animatedMatrixCapacity, g->animatedCount + 1, sizeof *g->animated))
            return -1;
        s->animated = (int)g->animatedCount;
        g->animatedNodes[g->animatedCount] = node;
        memcpy(g->animated[g->animatedCount], node->matrix, sizeof g->animated[0]);
        g->animatedCount++;
    }
    return (int)g->slotCount++;
}

/* ---- the ground ---- */

static void build_ground(struct WorldGpu *g, struct C3dGround15 *ground)
{
    static const float identity[12] = {1, 0, 0, 0, 1, 0, 0, 0, 1, 0, 0, 0};
    int bx, by, i, cols, rows;

    ground_forget_water(ground);
    /* slot 0: the world itself, drawn with the camera's view as it is */
    if (add_slot(g, identity, -1, NULL) != 0) {
        g->failed = 1;
        return;
    }
    if (ground->width <= 0 || ground->height <= 0)
        return;
    cols = (ground->width + GND_GPU_BLOCK - 1) / GND_GPU_BLOCK;
    rows = (ground->height + GND_GPU_BLOCK - 1) / GND_GPU_BLOCK;
    for (by = 0; by < rows; by++)
        for (bx = 0; bx < cols; bx++) {
            struct SwPlacement place;
            int indexCount, n = c3d_ground15_block_mesh(ground, bx, by, g->verts, g->indices, &indexCount, g->blockRuns);

            for (i = 0; i < n; i++) {
                const struct GndBlockRun *d = &g->blockRuns[i];
                struct SwRun *r = &g->runs[i];

                memset(r, 0, sizeof *r);
                r->first = (uint32_t)d->first;
                r->count = (uint32_t)d->count;
                r->sort[0] = serial(d->texture);
                r->sort[1] = serial(d->lightmap);
                place_texture(g, d->texture, &r->textureArray, &r->textureLayer);
                place_texture(g, d->lightmap, &r->lightmapArray, &r->lightmapLayer);
                r->uv = SW_NONE;
                r->pass = SW_PASS_GROUND;
            }
            if (n)
                add_mesh(g, (uint32_t)indexCount / 6 * 4, (uint32_t)indexCount, 0, 0, NULL, (size_t)n, &place);
        }
}

static void build_water(struct C3dGround15 *ground)
{
    size_t n = c3d_ground15_water_cells(ground, NULL);
    struct GfxWaterCell *cells = n ? malloc(n * sizeof *cells) : NULL;

    if (!n || !cells) {
        free(cells);
        return;
    }
    c3d_ground15_water_cells(ground, cells);
    if (!gfx_world_water(cells, n, g_sinTable))
        ground_forget_water(ground);
    free(cells);
}

/* ---- the models ---- */

/* the share of the light c3d_node_light lit a corner with */
static BYTE light_share(const struct C3dNode *node, const struct AnimMeshFace *face, int corner)
{
    const struct AnimMesh *mesh = node->mesh;
    int shadeType = node->actor ? node->actor->shadeType : 0;

    if (shadeType != 1 && shadeType != 2)
        return 0;
    return mesh->texVertCount && face->tv[corner] < mesh->texVertCount && mesh->texVerts[face->tv[corner]].colour != 0
               ? 128
               : 255;
}

struct NodeFace {
    int face;
    int key[2];
};

static int node_face_less(const void *a, const void *b)
{
    const struct NodeFace *x = a, *y = b;
    int i;

    for (i = 0; i < 2; i++)
        if (x->key[i] != y->key[i])
            return x->key[i] < y->key[i] ? -1 : 1;
    return x->face < y->face ? -1 : x->face > y->face;
}

/* One run of a node's mesh: faces with one texture and one two-sidedness. */
struct NodeRun {
    int first, count;
    WORD texture;
    BYTE twoSided;
};

/* custom: a node's faces as one mesh: a face's three corners keep their position, texture
   coordinates and what the original made their colour from: diffuse the node's own lit colour,
   with the share of the light it was lit with as its alpha (128 or 255 by the corner's texture
   vertex, as c3d_node_light takes it, 0 for a model lit without its light), colour2 the ground's
   light under the corner; sorted by texture and two-sidedness, runs into out. Answers the number of
   runs, 0 for a node without faces or with faces it cannot take. */
static int node_mesh(const struct C3dNode *node, struct GfxMeshVertex *verts, WORD *indices, struct NodeRun *out)
{
    static struct NodeFace list[NODE_FACES];
    const struct AnimMesh *mesh = node->mesh;
    int f, i, k, n, count = 0;

    if (!mesh || !mesh->faces || !mesh->positions || mesh->faceCount <= 0 || mesh->faceCount > NODE_FACES
        || !node->faceInfo || !node->groundLight)
        return 0;
    n = mesh->faceCount;
    for (f = 0; f < n; f++)
        for (k = 0; k < 3; k++)
            if (mesh->faces[f].v[k] >= mesh->vertexCount)
                return 0;
    for (f = 0; f < n; f++) {
        list[f].face = f;
        list[f].key[0] = mesh->faces[f].texture;
        list[f].key[1] = mesh->faces[f].twoSided != 0;
    }
    qsort(list, (size_t)n, sizeof *list, node_face_less);
    for (i = 0; i < n; i++) {
        const struct AnimMeshFace *face = &mesh->faces[list[i].face];
        const struct FaceInfo *info = &node->faceInfo[list[i].face];
        const int *li = node->groundLight + list[i].face * 9;
        const Vec3 *a = &mesh->positions[face->v[0]], *b = &mesh->positions[face->v[1]];
        const Vec3 *c = &mesh->positions[face->v[2]];
        float e1[3] = {b->x - a->x, b->y - a->y, b->z - a->z}, e2[3] = {c->x - a->x, c->y - a->y, c->z - a->z};
        float nrm[3] = {e1[1] * e2[2] - e1[2] * e2[1], e1[2] * e2[0] - e1[0] * e2[2], e1[0] * e2[1] - e1[1] * e2[0]};
        float len = sqrtf(nrm[0] * nrm[0] + nrm[1] * nrm[1] + nrm[2] * nrm[2]);

        for (k = 0; k < 3 && len > 0.0f; k++)
            nrm[k] /= len;
        for (k = 0; k < 3; k++) {
            struct GfxMeshVertex *v = &verts[i * 3 + k];
            const Vec3 *p = &mesh->positions[face->v[k]];

            v->x = p->x;
            v->y = p->y;
            v->z = p->z;
            v->nx = nrm[0];
            v->ny = nrm[1];
            v->nz = nrm[2];
            v->diffuse = (info->colour[k] & 0x00ffffff) | (DWORD)light_share(node, face, k) << 24;
            v->colour2 = pack(0xff, (BYTE)li[k * 3], (BYTE)li[k * 3 + 1], (BYTE)li[k * 3 + 2]);
            v->u0 = info->uv[k][0];
            v->v0 = info->uv[k][1];
            v->u1 = v->v1 = 0.0f;
            indices[i * 3 + k] = (WORD)(i * 3 + k);
        }
        if (i == 0 || list[i - 1].key[0] != list[i].key[0] || list[i - 1].key[1] != list[i].key[1]) {
            struct NodeRun *r = &out[count++];

            r->texture = face->texture;
            r->twoSided = (BYTE)list[i].key[1];
            r->first = i * 3;
            r->count = 0;
        }
        out[count - 1].count += 3;
    }
    return count;
}

/* whether a node's matrix changes when its model animates (render/keyframes.c: a channel with one
   key or none stays put), whatever the model's animType says now: a skill's model starts still
   and is set going later (effects/skills/skill_msg.c) */
static int node_keys(const struct C3dNode *node, int version)
{
    if (version == 1)
        return ((const struct ModelResNodeV1 *)node->resource)->rotKeyCount > 1;
    {
        const struct ModelResNodeV2 *r = (const struct ModelResNodeV2 *)node->resource;

        return r->scaleKeyCount > 1 || r->rotKeyCount > 1 || r->posKeyCount > 1;
    }
}

static BYTE mirrored(const struct C3dActor *a)
{
    BYTE m = 0.0f > a->scale[0];

    if (0.0f > a->scale[1])
        m = m == 0;
    if (0.0f > a->scale[2])
        m = m == 0;
    return m;
}

/* the pass renderer_add_prim's flags for a model's quads sent a run of a node to: blended where
   its texture has alpha (a model's own alpha, which the original faded a picked model by, is
   always whole: no model is ever pickable) */
static int model_pass(const struct Texture *t, int uvAnimated, BYTE layer)
{
    if (!(t && t->format == 1))
        return uvAnimated ? SW_PASS_OPAQUE_WRAP : SW_PASS_OPAQUE_CLAMP;
    if (uvAnimated)
        return SW_PASS_ALPHA_WRAP;
    return layer & 1 ? SW_PASS_ALPHA_DEPTH : SW_PASS_ALPHA_CLAMP;
}

static uint16_t uv_slot(struct WorldGpu *g, struct C3dNode *node, int texture)
{
    size_t i;

    for (i = 0; i < g->uvCount; i++)
        if (g->uvs[i].node == node && g->uvs[i].texture == texture)
            return (uint16_t)i;
    if (g->uvCount >= SW_NONE || !grow((void **)&g->uvs, &g->uvCapacity, g->uvCount + 1, sizeof *g->uvs)
        || !grow((void **)&g->uvMatrices, &g->uvMatrixCapacity, g->uvCount + 1, sizeof *g->uvMatrices))
        return SW_NONE;
    g->uvs[g->uvCount].node = node;
    g->uvs[g->uvCount].texture = texture;
    uv_turn_matrix(uv_xform_find((const struct C3dNodeV2 *)node, texture), g->uvMatrices[g->uvCount]);
    return (uint16_t)g->uvCount++;
}

static void build_node(struct WorldGpu *g, struct C3dActor *a, struct C3dNode *node, int version, int parent,
                       const float *parentPlaced, int parentAnimated, int depth)
{
    static struct NodeRun nodeRuns[NODE_FACES];
    int keys = node_keys(node, version), animated = parentAnimated || keys, slot, n, i;
    int cull = (version == 1) == (mirrored(a) == 0) ? 0 : SW_CULL_CCW;
    float placed[12];
    size_t child;

    /* a slot's chain to the camera's view is GFX_WORLD_CHAIN long at most: the actor's slot, then
       its nodes */
    if (depth >= GFX_WORLD_CHAIN)
        return;
    mat_multiply(placed, node->matrix, parentPlaced);
    slot = add_slot(g, node->matrix, parent, keys ? node : NULL);
    if (slot < 0)
        return;
    n = node_mesh(node, g->verts, g->indices, nodeRuns);
    for (i = 0; i < n; i++) {
        const struct NodeRun *d = &nodeRuns[i];
        struct Texture *t = run_texture(node, d->texture);
        int uvAnimated = version == 2 && uv_xform_find((const struct C3dNodeV2 *)node, d->texture);
        struct SwRun *r = &g->runs[i];

        memset(r, 0, sizeof *r);
        r->first = (uint32_t)d->first;
        r->count = (uint32_t)d->count;
        r->sort[0] = serial(t);
        place_texture(g, t, &r->textureArray, &r->textureLayer);
        r->lightmapArray = SW_NO_ARRAY;
        r->uv = uvAnimated ? uv_slot(g, node, d->texture) : SW_NONE;
        r->pass = (uint8_t)model_pass(t, uvAnimated, a->layer);
        g->list[g->modelCount - 1].alphas += r->pass >= SW_PASS_ALPHA_CLAMP;
        r->flags = (uint8_t)(d->twoSided ? SW_CULL_NONE : cull);
    }
    if (n) {
        struct SwPlacement place;
        uint32_t faces = (uint32_t)node->mesh->faceCount;

        if (add_mesh(g, faces * 3, faces * 3, (uint16_t)slot, animated, placed, (size_t)n, &place))
            g->list[g->modelCount - 1].vertices += place.vertexCount;
    }
    for (child = 0; child < node->childCount; child++)
        build_node(g, a, node->children[child], version, slot, placed, animated, depth + 1);
}

/* A model's records under its slot: its matrix as world/model_load.c made it when it lit the model
   (the light op builds it), which its animation never changes. A model without room for it gets
   nothing (NO_MODEL). */
static void build_actor(struct WorldGpu *g, struct C3dActor *a)
{
    int version = a->ops->version, slot;
    struct WorldModel *m;

    a->gpuGeneration = g->generation;
    a->gpuModel = NO_MODEL;
    if (!grow((void **)&g->list, &g->modelCapacity, g->modelCount + 1, sizeof *g->list))
        return;
    m = &g->list[g->modelCount++];
    memset(m, 0, sizeof *m);
    m->actor = a;
    m->firstRecord = (uint32_t)g->recordCount;
    m->firstSlot = (uint32_t)g->slotCount;
    m->firstAnimated = (uint32_t)g->animatedCount;
    m->firstUv = (uint32_t)g->uvCount;
    a->gpuModel = (uint32_t)(g->modelCount - 1);
    slot = add_slot(g, a->matrix, -1, NULL);
    if (slot >= 0) {
        if (version == 1) {
            build_node(g, a, ((struct C3dActorV1 *)a)->root, 1, slot, a->matrix, 0, 1);
        } else {
            struct C3dActorV2 *v2 = (struct C3dActorV2 *)a;
            size_t i;

            for (i = 0; i < v2->nodeCount; i++)
                build_node(g, a, v2->nodes[i], 2, slot, a->matrix, 0, 1);
        }
    }
    m->records = (uint32_t)g->recordCount - m->firstRecord;
    m->slots = (uint32_t)g->slotCount - m->firstSlot;
    m->animated = (uint32_t)g->animatedCount - m->firstAnimated;
    m->uvs = (uint32_t)g->uvCount - m->firstUv;
}

/* every model in a leaf of the scene graph takes the stamp */
static void stamp_leaves(struct SceneGraphNode *node, int level, unsigned stamp)
{
    size_t i;

    if (!node)
        return;
    if (level < SCENE_GRAPH_DEPTH) {
        for (i = 0; i < 4; i++)
            stamp_leaves(node->child[i], level + 1, stamp);
        return;
    }
    for (i = 0; i < node->objects.count; i++)
        node->objects.data[i]->gpuSeen = stamp;
}

/* The device's records and slots brought up to date: the records built since sorted among
   themselves and merged into the live ones (the same order a sort of them all gives), those of
   models given up left out. What the device does not take is given again with the next. */
static void send(struct WorldGpu *g)
{
    size_t fresh = g->recordCount - g->sortedIds, live = 0, i, a, b, n;

    if (!g->recordsDue)
        return;
    if (!grow((void **)&g->merged, &g->mergedCapacity, g->sortedCount + fresh, sizeof *g->merged)
        || !grow((void **)&g->sorted, &g->sortedCapacity, g->sortedCount + fresh, sizeof *g->sorted))
        return;
    for (i = 0; i < g->sortedCount; i++)
        if (!g->dead[g->sorted[i].id])
            g->sorted[live++] = g->sorted[i];
    n = live;
    for (i = g->sortedIds; i < g->recordCount; i++)
        if (!g->dead[i])
            g->sorted[n++] = g->records[i];
    sw_records_sort(g->sorted + live, n - live);
    for (a = 0, b = live, i = 0; a < live || b < n;)
        g->merged[i++] = b >= n || (a < live && sw_record_before(&g->sorted[a], &g->sorted[b])) ? g->sorted[a++]
                                                                                              : g->sorted[b++];
    {
        struct SwRecord *swap = g->sorted;
        size_t capacity = g->sortedCapacity;

        g->sorted = g->merged;
        g->sortedCapacity = g->mergedCapacity;
        g->merged = swap;
        g->mergedCapacity = capacity;
    }
    g->sortedCount = n;
    g->sortedIds = g->recordCount;
    if (g->slotCount > g->slotsSent) {
        if (!gfx_world_slots(g->slots + g->slotsSent, g->slotsSent, g->slotCount - g->slotsSent))
            return;
        g->slotsSent = g->slotCount;
    }
    if (gfx_world_records(g->sorted, g->sortedCount, (uint32_t)g->recordCount))
        g->recordsDue = 0;
}

/* the mesh's working buffers, all of them or none */
static int scratch(struct WorldGpu *g)
{
    size_t verts = (size_t)GND_BLOCK_SURFACES * 4 > (size_t)NODE_FACES * 3 ? (size_t)GND_BLOCK_SURFACES * 4
                                                                           : (size_t)NODE_FACES * 3;
    size_t runs = GND_BLOCK_SURFACES > NODE_FACES ? GND_BLOCK_SURFACES : NODE_FACES;

    if (g->verts)
        return 1;
    g->verts = malloc(verts * sizeof *g->verts);
    g->indices = malloc(verts * 2 * sizeof *g->indices);
    g->ids = malloc(verts * sizeof *g->ids);
    g->runs = malloc(runs * sizeof *g->runs);
    g->blockRuns = malloc(GND_BLOCK_SURFACES * sizeof *g->blockRuns);
    if (g->verts && g->indices && g->ids && g->runs && g->blockRuns)
        return 1;
    free(g->verts);
    free(g->indices);
    free(g->ids);
    free(g->runs);
    free(g->blockRuns);
    g->verts = NULL;
    g->indices = NULL;
    g->ids = NULL;
    g->runs = NULL;
    g->blockRuns = NULL;
    return 0;
}

void world_gpu_begin(struct World *world, const float *view, const struct GfxWorldDraw *models,
                     const struct GfxWorldDraw *ground)
{
    struct WorldGpu *g = world->gpu;
    struct Renderer *r = g_renderer;
    struct SwProjection projection;
    size_t i;

    if (!world->ground)
        return;
    if (!g) {
        g = calloc(1, sizeof *g);
        if (!g)
            return;
        g->generation = -1;
        world->gpu = g;
    }
    g->framing = 0;
    if (!scratch(g))
        return;
    if (g->generation != gfx_world_generation() || g->garbage > WORLD_GPU_GARBAGE) {
        /* the device made its world again (a new device, a failed upload, textures reloaded), or
           the models given up take too much of it: built again from the start */
        if (g->generation == gfx_world_generation())
            gfx_world_clear();
        reset(g);
        g->generation = gfx_world_generation();
        build_ground(g, world->ground);
        build_water(world->ground);
    }
    if (g->failed)
        return;
    if (g->taken < world->models.inserted) {
        stamp_leaves(&world->root, 0, ++g->stamp);
        for (i = g->taken; i < world->models.inserted; i++) {
            struct C3dActor *a = world->models.data[i];

            a->gpuModel = NO_MODEL;
            a->gpuGeneration = -1;
            if (a->gpuSeen == g->stamp)
                build_actor(g, a);
        }
        g->taken = world->models.inserted;
    }
    send(g);

    /* the frame begins */
    g->frame++;
    memcpy(g->view, view, sizeof g->view);
    projection.proj[0] = r->proj[0];
    projection.proj[1] = r->proj[1];
    projection.centre[0] = (float)r->centreX;
    projection.centre[1] = (float)r->centreY;
    projection.size[0] = (float)r->width;
    projection.size[1] = (float)r->height;
    projection.nearZ = RENDER_NEAR_Z;
    projection.farZ = RENDER_FAR_Z;
    sw_frustum(&g->frustum, &projection, view);
    g->models = *models;
    g->ground = *ground;
    g->framing = 1;
}

/* the world's model of an actor, built on its first sight; NULL where it has none */
static struct WorldModel *model_of(struct World *world, struct C3dActor *actor)
{
    struct WorldGpu *g = world ? world->gpu : NULL;

    if (!g || !g->framing || g->failed || g->generation != gfx_world_generation())
        return NULL;
    if (actor->gpuGeneration != g->generation)
        build_actor(g, actor);
    return actor->gpuModel < g->modelCount && g->list[actor->gpuModel].actor == actor ? &g->list[actor->gpuModel]
                                                                                       : NULL;
}

/* the model's pose as it stands: its animated nodes' matrices and its texture-coordinate
   animations, which the device takes for the frame */
static void pose(struct WorldGpu *g, struct WorldModel *m)
{
    uint32_t i;

    for (i = m->firstAnimated; i < m->firstAnimated + m->animated; i++)
        memcpy(g->animated[i], g->animatedNodes[i]->matrix, sizeof g->animated[i]);
    for (i = m->firstUv; i < m->firstUv + m->uvs; i++)
        uv_turn_matrix(uv_xform_find((const struct C3dNodeV2 *)g->uvs[i].node, g->uvs[i].texture), g->uvMatrices[i]);
    m->posed = g->frame;
}

/* the model's slots' places in the world in its pose, as the transform pass computes them (render/gfx.h,
   GfxWorldSlot), and with `view` their views as the CPU computed a node's: a slot's matrix times
   its parent's, a model's slot first */
static void places(struct WorldGpu *g, const struct WorldModel *m, const float *view)
{
    uint32_t s;

    for (s = m->firstSlot; s < m->firstSlot + m->slots; s++) {
        const struct GfxWorldSlot *slot = &g->slots[s];
        const float *local = slot->animated >= 0 ? g->animated[slot->animated] : slot->local;

        if (slot->parent < 0) {
            memcpy(g->placed[s], local, sizeof g->placed[s]);
            if (view)
                mat_multiply(g->views[s], local, view);
        } else {
            mat_multiply(g->placed[s], local, g->placed[slot->parent]);
            if (view)
                mat_multiply(g->views[s], local, g->views[slot->parent]);
        }
    }
}

/* whether a view or a box keeps one of the model's records in the pose places() put it in, as the
   device's culls decide (the view's, and each cascade's of its casters) */
static int model_kept(const struct WorldGpu *g, const struct WorldModel *m, const struct SwFrustum *frustum)
{
    uint32_t id;

    for (id = m->firstRecord; id < m->firstRecord + m->records; id++)
        if (!g->dead[id] && sw_record_in_view(&g->records[id], (const float (*)[12])g->placed, frustum))
            return 1;
    return 0;
}

/* The model's alpha-blended records the view keeps, queued in the order of its nodes and their
   runs, the order the client queued a model's alpha-blended draws in, keyed by the rhw of their
   first corner as the CPU projected it through the node's view (render/rp_project.c), to the bit. */
static void queue_alpha(struct WorldGpu *g, struct WorldModel *m)
{
    static const int queues[SW_PASS_COUNT] = {0, 0, 0, RENDER_ALPHA_CLAMP, RENDER_ALPHA_WRAP, RENDER_ALPHA_WRITE_DEPTH};
    struct RPWorldDraw draw;
    uint32_t id;
    int placedOnce = 0;

    m->queued = g->frame;
    if (!m->alphas)
        return;
    memset(&draw, 0, sizeof draw);
    draw.kind = RP_WORLD_RECORD;
    draw.draw = g->models;
    for (id = m->firstRecord; id < m->firstRecord + m->records; id++) {
        const struct SwRecord *rec = &g->records[id];
        const float *p = g->keyPoints[id];
        struct TexDrawVertex corner;
        Vec3 v;

        if (rec->pass < SW_PASS_ALPHA_CLAMP || g->dead[id])
            continue;
        if (!placedOnce) {
            places(g, m, g->view);
            placedOnce = 1;
        }
        if (!sw_record_in_view(rec, (const float (*)[12])g->placed, &g->frustum))
            continue;
        v.x = p[0];
        v.y = p[1];
        v.z = p[2];
        renderer_project_vertex(g_renderer, &v, g->views[rec->transform], &corner);
        draw.record = *rec;
        renderer_add_world_draw(g_renderer, queues[rec->pass], &draw, NULL, corner.rhw);
    }
}

void world_gpu_seen(struct World *world, struct C3dActor *actor)
{
    struct WorldModel *m = model_of(world, actor);

    if (!m)
        return;
    if (m->posed != world->gpu->frame)
        pose(world->gpu, m);
    if (m->queued != world->gpu->frame)
        queue_alpha(world->gpu, m);
}

void world_gpu_forget(struct World *world, struct C3dActor *actor)
{
    struct WorldGpu *g = world ? world->gpu : NULL;
    struct WorldModel *m;
    uint32_t id;

    if (!g || actor->gpuGeneration != g->generation || actor->gpuModel >= g->modelCount)
        return;
    m = &g->list[actor->gpuModel];
    actor->gpuGeneration = -1;
    actor->gpuModel = NO_MODEL;
    if (m->actor != actor)
        return;
    for (id = m->firstRecord; id < m->firstRecord + m->records; id++)
        g->dead[id] = 1;
    g->garbage += m->vertices;
    g->recordsDue = 1;
    m->actor = NULL;
}

/* The box everything that receives and casts the frame's shadows lies in: every static record, and
   every animated record in the pose the frame gives it (places, which the world's slots are left
   in). */
static void frame_bounds(struct WorldGpu *g, struct ShadowFitBox *box)
{
    size_t i;
    uint32_t id;

    *box = g->bounds;
    for (i = 0; i < g->modelCount; i++) {
        const struct WorldModel *m = &g->list[i];

        if (!m->actor || !m->animated)
            continue;
        places(g, m, NULL);
        for (id = m->firstRecord; id < m->firstRecord + m->records; id++) {
            float centre[3], half[3];

            if (!g->dead[id] && (g->records[id].flags & SW_ANIMATED)
                && sw_record_bounds(&g->records[id], (const float (*)[12])g->placed, centre, half))
                bound(box, centre, half);
        }
    }
}

void world_gpu_frame(struct World *world, void (*tick)(struct C3dActor *actor))
{
    struct WorldGpu *g = world->gpu;
    struct Renderer *r = g_renderer;
    struct GfxWorldFrame frame;
    struct SwFrustum reach;
    struct ShadowFitBox box;
    size_t i;
    int shadows;

    if (!g || !g->framing || g->generation != gfx_world_generation() || !world->ground)
        return;
    g->framing = 0;
    if (!grow((void **)&g->waves, &g->waveCapacity, world->ground->waterCount, sizeof *g->waves))
        return;
    /* the models the frame did not draw: their pose as it stands, and where the view keeps a
       record of one that animates, or the box whatever can shade the view lies in keeps one, as
       the view's and the cascades' culls decide, the tick the walk would have given it had it
       reached the model, so that no part moves in view, nor its shadow, standing still; then their
       alpha-blended records the view keeps */
    shadows = renderer_shadow_reach(r, &reach);
    for (i = 0; i < g->modelCount; i++) {
        struct WorldModel *m = &g->list[i];

        if (!m->actor || m->posed == g->frame)
            continue;
        pose(g, m);
        if (!(m->animated || m->uvs) || !tick)
            continue;
        places(g, m, NULL);
        if (model_kept(g, m, &g->frustum) || (shadows && model_kept(g, m, &reach))) {
            tick(m->actor);
            pose(g, m);
        }
    }
    for (i = 0; i < g->modelCount; i++)
        if (g->list[i].actor && g->list[i].queued != g->frame)
            queue_alpha(g, &g->list[i]);
    send(g);
    for (i = 0; i < world->ground->waterCount; i++) {
        const struct GndWater *w = &world->ground->waters[i];

        g->waves[i].phase = w->phase;
        g->waves[i].pitch = w->wavePitch;
        g->waves[i].height = w->waveHeight;
        g->waves[i].level = w->level;
    }
    if (shadows) {
        frame_bounds(g, &box);
        renderer_shadow_bounds(r, &box);
    }
    memset(&frame, 0, sizeof frame);
    renderer_world_view(r, g->view, &frame.camera);
    frame.frustum = g->frustum;
    frame.animated = (const float (*)[12])g->animated;
    frame.animatedCount = g->animatedCount;
    frame.uv = (const float (*)[6])g->uvMatrices;
    frame.uvCount = g->uvCount;
    frame.waves = g->waves;
    frame.waveCount = world->ground->waterCount;
    frame.shadow = r->shadowSet ? &r->shadow : NULL;
    gfx_world_frame(&frame);
    renderer_set_world(r, &g->models, &g->ground);
}

void world_gpu_free(struct World *world)
{
    struct WorldGpu *g = world->gpu;

    if (!g)
        return;
    if (world->ground)
        ground_forget_water(world->ground);
    gfx_world_clear();
    free(g->textures);
    free(g->records);
    free(g->dead);
    free(g->keyPoints);
    free(g->sorted);
    free(g->merged);
    free(g->list);
    free(g->slots);
    free(g->animatedNodes);
    free(g->animated);
    free(g->placed);
    free(g->views);
    free(g->uvs);
    free(g->uvMatrices);
    free(g->waves);
    free(g->verts);
    free(g->indices);
    free(g->ids);
    free(g->runs);
    free(g->blockRuns);
    free(g);
    world->gpu = NULL;
}
