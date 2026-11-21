/* Granny's actors skinned by the device (render/shaders/skin.hlsl and skin.hlsli; render/gfx.h,
   gfx_create_skin and gfx_draw_skin) on this machine's GPU, against the CPU code they replaced
   (tests/fixtures/granny_reference.c: the shim's deformer, a mesh's vertices for the frame and the
   three loops of actors/granny.c, as they were), within a tolerance: plans/gpu-renderer.md lets the
   GPU's floats part from the CPU's here.

   - Every corner of every triangle of every model the folder's converted files hold, in its bind
     pose, in each of the folder's animations that moves it at nine times through it, and in fuzzed
     poses, placed, turned and lit as the lit model, the corpse and the guild flag are
     (render/skin_layout.h, skin_pose_kind), through realistic cameras on screens of several sizes:
     what the shader computes of it on the GPU's vertex stage (tests/shaders/skin_probe.hlsl), its
     clip position as skin.hlsl gives it, against the CPU's corner: the pixel within 1/16 of
     renderer_project_vertex's, the rhw within 1e-5 of its, relative, the texture coordinates within
     1/4096 and each byte of the colour within one step. The depth is held to 1e-5 of its own from
     twice the near plane on (a view depth of 20, the depth 0.5) and to 1e-5 of the depth range
     nearer than that: near 0 a bound relative to the depth would measure the CPU's own
     cancellation (the view depth there is the small sum of large terms, the camera's distance and
     the vertex's place), and a depth buffer compares absolute depths. Every other fuzzed pose
     leaves each bone that no vertex of the model weights not a number, which a vertex's weight of 0
     on such a bone must not read; the corners that have one are counted. The corner's place in view
     space against the CPU's (its world point, world_corner's, through the view) is reported.
   - Pictures through render/gpu_device.c itself: actors of the folder's models drawn by the device from
     the models the client makes of them (resources/granny_mesh.c, granny_model_res_skin) in their
     bind poses and animations (a fuzzed pose folds a mesh onto itself, where faces of one depth
     z-fight), and the CPU's corners of the same actors drawn as pre-transformed triangles, at the sample
     count asked for, half the scenes fogged, with a texture of gentle gradients (a texel's
     neighbours a few steps apart, so that interpolated coordinates a hair apart read the same
     colour within a step), within the budget of the scene captures: the pixels that differ by more
     than a step at most 0.05 % of the screen, in groups of 16 at most. Such pixels lie where corners
     a hundredth of a pixel apart cover a pixel's centre otherwise, along the triangles' edges and
     where faces cross. A scene beyond the budget leaves its two pictures in the working directory
     (skin_<scene>_device.bmp and skin_<scene>_cpu.bmp).

   usage: test_skin_instances msaa gltf-folder [seed] [fuzzed poses a model] [scenes]
   Exits 77 where no GPU device can be made, or the folder holds no model. */
#include "resources/granny_res.h"
#include "resources/granny_sdk.h"
#include "render/skin_layout.h"
#include "render/renderer.h"
#include "render/render_api.h"
#include "render/projection.h"
#include "render/math3d.h"
#include "render/gfx.h"
#include "render/gpu_layer.h"
#include "render/device.h"
#include "render/shaders/shader_code.h"
#include "shaders/skin_probe.h"
#include "fixtures/granny_reference.h"
#include "picture_budget.h"
#include "fileio/file_api.h"
#include "platform/system.h"
#include "diag/perf.h"
#include <SDL3/SDL.h>
#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* ---- what the client files reach ---- */

struct perf_counters g_perf;
struct Renderer *g_renderer;
DWORD g_fogColor;
float g_fogNear, g_fogFar;
int g_lightmap;
struct C3dDevice g_3dDevice;

int c3d_device_clear(struct C3dDevice *device, DWORD color) { (void)device; (void)color; return 0; }
int c3d_device_clear_z(struct C3dDevice *device) { (void)device; return 0; }
void c3d_device_restore_frame(struct C3dDevice *device) { (void)device; }
struct C3dGround15 *game_current_ground(void) { return NULL; }
int game_world_to_cell(float x, float z, int *cell_x, int *cell_y)
{
    (void)x; (void)z; *cell_x = *cell_y = 0;
    return 0;
}
int c3d_ground15_get_vertex_colour(struct C3dGround15 *self, int ax, int ay, float *r, float *g, float *b)
{
    (void)self; (void)ax; (void)ay; *r = *g = *b = 1.0f;
    return 0;
}

/* resources/granny_gltf.c reads the converted file the client names; here, the file being loaded */
static void *g_serve;
static size_t g_serveSize;

struct File *file_construct(struct File *self)
{
    memset(self, 0, sizeof *self);
    return self;
}
unsigned char file_open(struct File *self, const char *name, int mode, unsigned char resolve, unsigned char diskOnly)
{
    (void)name; (void)mode; (void)resolve; (void)diskOnly;
    if (!g_serve || g_serveSize > 0x7fffffff)
        return 0;
    self->buffer = g_serve;
    self->size = (int)g_serveSize;
    return 1;
}
void file_destruct(struct File *self) { self->buffer = NULL; }
void platform_debug_output(const char *text) { fputs(text, stderr); }

/* ---- random numbers ---- */

static uint64_t g_rng;

static uint64_t next(void)
{
    uint64_t z = (g_rng += 0x9e3779b97f4a7c15ull);
    z = (z ^ (z >> 30)) * 0xbf58476d1ce4e5b9ull;
    z = (z ^ (z >> 27)) * 0x94d049bb133111ebull;
    return z ^ (z >> 31);
}

static float uniform(float lo, float hi)
{
    return lo + (hi - lo) * (float)((double)(next() >> 11) / 9007199254740992.0);
}

static int irange(int lo, int hi)
{
    return lo + (int)(next() % (uint64_t)(hi - lo + 1));
}

static int chance(int percent)
{
    return (int)(next() % 100) < percent;
}

/* ---- the folder's models and their poses ---- */

#define MODELS_MAX 64
#define POSES_MAX 512

struct Model {
    char name[64];
    granny_file *file;
    granny_model *source;
    int bones;
    /* as the client keeps it (resources/granny_res.h): the meshes, each one's rigid bone and
       triangles in the device's model, and that model */
    struct C3dGrannyModelRes res;
    struct GrannyResModel model;
    struct GrannyResMesh *meshes;
    /* each mesh's vertices as the file holds them */
    struct SkinSource **sources;
    /* its poses: Granny's composites, boneCount (one at least) by 16 floats each, the first `real`
       of them its bind pose and its animations'; the poisoned ones leave every bone no vertex
       weights (`weighted` 0) not a number */
    float *poses[POSES_MAX];
    unsigned char poisoned[POSES_MAX];
    unsigned char weighted[SKIN_BONES_MAX];
    int nposes, real, animated, unweighted;
};

static struct Model g_models[MODELS_MAX];
static int g_nmodels;
static granny_animation *g_animations[MODELS_MAX];
static int g_nanimations;

static granny_file *load_file(const char *path)
{
    size_t size = 0;
    char one = 0;
    granny_file *f;
    g_serve = SDL_LoadFile(path, &size);
    g_serveSize = size;
    f = g_serve ? _GrannyReadEntireFileFromMemory(1, &one) : NULL;
    SDL_free(g_serve);
    g_serve = NULL;
    return f;
}

/* the composites of a pose, the instance's controls sampled at time t */
static float *sample(struct Model *m, granny_model_instance *instance, float t)
{
    granny_local_pose *local = _GrannyNewLocalPose(m->bones);
    granny_world_pose *world = _GrannyNewWorldPose(m->bones);
    size_t n = (size_t)(m->bones ? m->bones : 1) * 16;
    float *out = malloc(n * sizeof *out);
    if (!local || !world || !out) {
        fprintf(stderr, "skin_instances: no memory for a pose\n");
        exit(1);
    }
    _GrannySetModelClock(instance, t);
    _GrannySampleModelAnimations(instance, 0, m->bones, local);
    _GrannyBuildWorldPose(m->source->Skeleton, 0, m->bones, local, NULL, world);
    memcpy(out, _GrannyGetWorldPoseComposite4x4Array(world), n * sizeof *out);
    _GrannyFreeLocalPose(local);
    _GrannyFreeWorldPose(world);
    return out;
}

static void add_pose(struct Model *m, float *pose)
{
    if (m->nposes < POSES_MAX)
        m->poses[m->nposes++] = pose;
    else
        free(pose);
}

/* an affine matrix as Granny's, row vectors: turned about an axis, scaled and moved */
static void fuzzed_bone(float *out)
{
    float r[12];
    Vec3 axis;
    float s = uniform(0.6f, 1.4f), len;
    int i;
    axis.x = uniform(-1.0f, 1.0f);
    axis.y = uniform(-1.0f, 1.0f);
    axis.z = uniform(-1.0f, 1.0f);
    len = sqrtf(axis.x * axis.x + axis.y * axis.y + axis.z * axis.z);
    if (len < 0.01f)
        axis.y = len = 1.0f;
    axis.x /= len;
    axis.y /= len;
    axis.z /= len;
    mat_set_rotate_axis(r, &axis, uniform(-3.14f, 3.14f));
    memset(out, 0, 16 * sizeof *out);
    for (i = 0; i < 3; i++) {
        out[i * 4] = r[i * 3] * s;
        out[i * 4 + 1] = r[i * 3 + 1] * s;
        out[i * 4 + 2] = r[i * 3 + 2] * s;
    }
    out[12] = uniform(-40.0f, 40.0f);
    out[13] = uniform(-40.0f, 40.0f);
    out[14] = uniform(-40.0f, 40.0f);
    out[15] = 1.0f;
}

static int load_folder(const char *folder, int fuzzed)
{
    char **files;
    int count = 0, i, a, k;
    files = SDL_GlobDirectory(folder, "*.glb", 0, &count);
    if (!files)
        return 0;
    for (i = 0; i < count; i++) {
        char path[1024];
        granny_file *f;
        granny_file_info *info;
        snprintf(path, sizeof path, "%s/%s", folder, files[i]);
        f = load_file(path);
        info = f ? _GrannyGetFileInfo(f) : NULL;
        if (!info)
            continue;
        if (info->ModelCount > 0 && info->Models[0]->MeshBindingCount > 0 && g_nmodels < MODELS_MAX) {
            struct Model *m = &g_models[g_nmodels++];
            granny_model *src = info->Models[0];
            snprintf(m->name, sizeof m->name, "%s", files[i]);
            m->file = f;
            m->source = src;
            m->bones = src->Skeleton->BoneCount;
            m->meshes = calloc((size_t)src->MeshBindingCount, sizeof *m->meshes);
            m->sources = calloc((size_t)src->MeshBindingCount, sizeof *m->sources);
            if (!m->meshes || !m->sources)
                return 0;
            /* what resources/granny_res.c keeps of the meshes */
            for (k = 0; k < src->MeshBindingCount; k++) {
                granny_mesh *mesh = src->MeshBindings[k].Mesh;
                int v = _GrannyGetMeshVertexCount(mesh);
                m->meshes[k].mesh = mesh;
                m->meshes[k].rigidBone = -1;
                if (_GrannyMeshIsRigid(mesh)) {
                    granny_mesh_binding *binding = _GrannyNewMeshBinding(mesh, src->Skeleton, src->Skeleton);
                    m->meshes[k].rigidBone = _GrannyGetMeshBindingToBoneIndices(binding)[0];
                    _GrannyFreeMeshBinding(binding);
                }
                m->sources[k] = calloc((size_t)(v ? v : 1), sizeof *m->sources[k]);
                if (!m->sources[k])
                    return 0;
                if (v)
                    _GrannyCopyMeshSkinVertices(mesh, 0, v, m->sources[k]);
                /* the bones the mesh's vertices weight, as the device keeps them */
                for (a = 0; a < v; a++) {
                    struct SkinVertex packed;
                    int s;
                    skin_pack_vertex(&m->sources[k][a], m->meshes[k].rigidBone, m->bones, &packed);
                    for (s = 0; s < 4; s++)
                        if (packed.weights[s] != 0.0f)
                            m->weighted[packed.bones[s]] = 1;
                }
            }
            for (a = 0; a < m->bones && a < SKIN_BONES_MAX; a++)
                m->unweighted += !m->weighted[a];
            m->model.meshCount = src->MeshBindingCount;
            m->model.meshes = m->meshes;
            m->res.model = &m->model;
            m->res.skeleton = src->Skeleton;
            m->res.grannyModel = src;
        } else if (info->AnimationCount > 0 && g_nanimations < MODELS_MAX) {
            g_animations[g_nanimations++] = info->Animations[0];
        } else {
            _GrannyFreeFile(f);
        }
    }
    SDL_free(files);
    /* the poses: the bind pose, each animation that moves the model at nine times through it, and
       fuzzed ones */
    for (i = 0; i < g_nmodels; i++) {
        struct Model *m = &g_models[i];
        granny_model_instance *instance = _GrannyInstantiateModel(m->source);
        float *bind = sample(m, instance, 0.0f);
        size_t n = (size_t)(m->bones ? m->bones : 1) * 16;
        add_pose(m, bind);
        for (a = 0; a < g_nanimations; a++) {
            granny_control *control = _GrannyPlayControlledAnimation(0.0f, g_animations[a], instance);
            float duration, *probe;
            _GrannySetModelClock(instance, 0.0f);
            duration = _GrannyGetControlDurationLeft(control);
            probe = sample(m, instance, duration * 0.5f);
            if (!memcmp(probe, bind, n * sizeof *probe)) {
                free(probe);
            } else {
                free(probe);
                m->animated++;
                for (k = 0; k <= 8; k++)
                    add_pose(m, sample(m, instance, duration * (float)k / 8.0f));
            }
            _GrannyFreeControl(control);
        }
        _GrannyFreeModelInstance(instance);
        m->real = m->nposes;
        for (a = 0; a < fuzzed; a++) {
            float *pose = malloc(n * sizeof *pose);
            int b, poisoned = (a & 1) && m->unweighted;
            if (!pose)
                return 0;
            for (b = 0; b < (m->bones ? m->bones : 1); b++) {
                fuzzed_bone(pose + b * 16);
                if (poisoned && b < SKIN_BONES_MAX && !m->weighted[b])
                    for (k = 0; k < 16; k++)
                        pose[b * 16 + k] = NAN;
            }
            if (m->nposes < POSES_MAX)
                m->poisoned[m->nposes] = (unsigned char)poisoned;
            add_pose(m, pose);
        }
    }
    return g_nmodels;
}

/* ---- an actor: a model in a pose, placed, turned and lit ---- */

struct View {
    float view[12];
    struct Renderer renderer;
};

static void make_view(struct View *s, int width, int height)
{
    float m[12], off[3], latitude, longitude, distance;
    Vec3 eye, target, up = {0.0f, 1.0f, 0.0f};
    int i;
    memset(s, 0, sizeof *s);
    renderer_set_projection(&s->renderer, width, height);
    latitude = uniform(-85.0f, -5.0f);
    distance = uniform(100.0f, 2500.0f);
    longitude = uniform(0.0f, 360.0f);
    mat_set_rotate_x(m, latitude / 180.0f * 3.141592f);
    mat_rotate_y(m, longitude / 180.0f * 3.141592f);
    for (i = 0; i < 3; i++)
        off[i] = -distance * m[6 + i] + m[9 + i];
    target.x = uniform(-1000.0f, 1000.0f);
    target.y = uniform(-60.0f, 20.0f);
    target.z = uniform(-1000.0f, 1000.0f);
    eye.x = off[0] + target.x;
    eye.y = target.y + off[1];
    eye.z = target.z + off[2];
    mat_look_at(s->view, &eye, &target, &up);
}

/* a world point the view sees at screen point (px, py) and view depth `depth` */
static void point_at(const struct View *s, float px, float py, float depth, Vec3 *out)
{
    const float *m = s->view;
    float rx = (px - (float)s->renderer.centreX) / s->renderer.proj[0];
    float ry = (py - (float)s->renderer.centreY) / s->renderer.proj[1];
    float eye[3], p[3];
    int i;
    for (i = 0; i < 3; i++)
        eye[i] = -(m[9] * m[3 * i] + m[10] * m[3 * i + 1] + m[11] * m[3 * i + 2]);
    for (i = 0; i < 3; i++)
        p[i] = eye[i] + (m[3 * i + 2] + rx * m[3 * i] + ry * m[3 * i + 1]) * depth;
    out->x = p[0];
    out->y = p[1];
    out->z = p[2];
}

struct Actor {
    struct Model *model;
    const float *pose;
    int kind;
    float r[12];
    struct GrannyReferenceLight light;
    unsigned char alpha;
    /* its pose leaves the bones no vertex weights not a number */
    int poisoned;
};

/* an actor of the model in the pose somewhere the view sees, as actors/granny.c places each kind:
   the lit model 2 units off its cell, the others on it, a model whose name holds "90" stood up */
static void make_actor(struct Actor *a, struct Model *m, const float *pose, const struct View *s, int onScreen)
{
    Vec3 pos;
    float facing = uniform(0.0f, 360.0f);
    int width = s->renderer.width, height = s->renderer.height;
    memset(a, 0, sizeof *a);
    a->model = m;
    a->pose = pose;
    a->kind = irange(0, 2);
    point_at(s, uniform(onScreen ? 0.1f : -0.25f, onScreen ? 0.9f : 1.25f) * (float)width,
             uniform(onScreen ? 0.1f : -0.25f, onScreen ? 0.9f : 1.25f) * (float)height,
             uniform(onScreen ? 200.0f : 40.0f, 2500.0f), &pos);
    mat_set_rotate_x(a->r, chance(50) ? 1.570796f : 3.141592f);
    mat_rotate_y(a->r, (facing + 180.0f) / 180.0f * 3.141592f);
    if (a->kind == GRANNY_REFERENCE_LIT) {
        pos.x -= 2.0f;
        pos.z -= 2.0f;
    }
    mat_translate(a->r, &pos);
    a->light.sun.x = uniform(-1.0f, 1.0f);
    a->light.sun.y = uniform(-1.0f, 1.0f);
    a->light.sun.z = uniform(-1.0f, 1.0f);
    a->light.viewDir.x = uniform(-1.0f, 1.0f);
    a->light.viewDir.y = uniform(-1.0f, -0.1f);
    a->light.viewDir.z = uniform(-1.0f, 1.0f);
    a->light.intensity = chance(50) ? 0x80 : irange(0, 255);
    a->light.r = chance(50) ? 200 : irange(0, 255);
    a->light.g = chance(50) ? 200 : irange(0, 255);
    a->light.b = chance(50) ? 200 : irange(0, 255);
    a->alpha = (unsigned char)(chance(60) ? 0xff : irange(0, 255));
}

/* The actor's pose through the view packed as actors/granny.c packs each kind's (actor_pose) and the
   renderer projects it (renderer_skin_pose), with the light the loops lit by (light_out of
   granny_reference_corners); answers its rows' count. */
static size_t actor_rows(const struct Actor *a, const struct View *s, const float *light, float (*rows)[4])
{
    static const int kinds[3] = {SKIN_ACTOR_LIT, SKIN_ACTOR_CORPSE, SKIN_ACTOR_GUILD_FLAG};
    struct SkinPose pose;
    memset(&pose, 0, sizeof pose);
    pose.bones = a->pose;
    pose.boneCount = (uint32_t)a->model->bones;
    mat_multiply(pose.view, a->r, s->view);
    pose.proj[0] = s->renderer.proj[0];
    pose.proj[1] = s->renderer.proj[1];
    pose.centre[0] = (float)s->renderer.centreX;
    pose.centre[1] = (float)s->renderer.centreY;
    pose.nearZ = RENDER_NEAR_Z;
    pose.depthScale = RENDER_DEPTH_SCALE;
    memcpy(pose.light, light, sizeof pose.light);
    pose.alpha = a->alpha;
    skin_pose_kind(&pose, kinds[a->kind], a->light.intensity, a->light.r, a->light.g, a->light.b);
    return skin_pose_rows(&pose, rows);
}

/* The CPU's corners of mesh k of the actor (granny_reference_corners), with their world points and
   the light the loop lit by; answers the triangles, 0 for a mesh without one. */
static int reference_mesh(struct Renderer *renderer, const struct Actor *a, int k, const float *view,
                          struct TexDrawVertex *out, float (*world)[3], float *light)
{
    static struct GrannyReferenceVertex verts[65536];
    static unsigned short indices[3 * 65536];
    const struct Model *m = a->model;
    granny_mesh *mesh = m->meshes[k].mesh;
    int v = _GrannyGetMeshVertexCount(mesh), n = _GrannyGetMeshIndexCount(mesh), tris = n / 3;
    float matrix[12];
    if (tris <= 0 || v > 65536 || n > 3 * 65536)
        return 0;
    _GrannyCopyMeshIndices(mesh, 2, indices);
    granny_reference_mesh(m->sources[k], v, m->meshes[k].rigidBone, a->pose, verts, matrix);
    granny_reference_corners(renderer, a->kind, verts, indices, tris, matrix, a->r, view, &a->light, a->alpha, out,
                             world, light);
    return tris;
}

/* ---- the GPU's vertex stage, through tests/shaders/skin_probe.hlsl ---- */

#define SIDE 512
#define PER_BATCH (SIDE * SIDE)
#define ROWS_MAX (1u << 20)

static SDL_GPUDevice *g_dev;
static SDL_GPUGraphicsPipeline *g_probe;
static SDL_GPUTexture *g_targets[4];
static SDL_GPUBuffer *g_vertices, *g_poseRows, *g_rows;

/* A batch: corners as the device keeps vertices, the row of the pose each is skinned in, the
   poses' rows, and draws of corners one after the other, each onto a screen of its size */
struct Draw {
    uint32_t first, count;
    float half[2];
    char label[160];
};
static struct {
    struct SkinVertex *corners;
    uint32_t *poses;
    float (*rows)[4];
    struct Draw draws[4096];
    uint32_t ncorners, nrows, ndraws;
} B;

static SDL_GPUShader *shader(const struct GpuShaderCode *code)
{
    SDL_GPUShaderCreateInfo info;
    SDL_zero(info);
#ifdef __APPLE__
    info.code = code->msl;
    info.code_size = code->mslSize;
    info.entrypoint = "main0";
    info.format = SDL_GPU_SHADERFORMAT_MSL;
#else
    info.code = code->spirv;
    info.code_size = code->spirvSize;
    info.entrypoint = "main";
    info.format = SDL_GPU_SHADERFORMAT_SPIRV;
#endif
    info.stage = code->stage == GPU_SHADER_VERTEX ? SDL_GPU_SHADERSTAGE_VERTEX : SDL_GPU_SHADERSTAGE_FRAGMENT;
    info.num_samplers = code->samplers;
    info.num_storage_textures = code->storageTextures;
    info.num_storage_buffers = code->storageBuffers;
    info.num_uniform_buffers = code->uniformBuffers;
    return SDL_CreateGPUShader(g_dev, &info);
}

static SDL_GPUBuffer *buffer(SDL_GPUBufferUsageFlags usage, Uint32 size)
{
    SDL_GPUBufferCreateInfo info;
    SDL_zero(info);
    info.usage = usage;
    info.size = size;
    return SDL_CreateGPUBuffer(g_dev, &info);
}

static void need(int ok, const char *what)
{
    if (!ok) {
        fprintf(stderr, "skin_instances: %s: %s\n", what, SDL_GetError());
        exit(1);
    }
}

static void probe_init(void)
{
    SDL_GPUShader *vs = shader(&skin_probe_vs), *fs = shader(&skin_probe_fs);
    SDL_GPUGraphicsPipelineCreateInfo info;
    SDL_GPUVertexBufferDescription vb[2];
    SDL_GPUVertexAttribute va[7];
    SDL_GPUColorTargetDescription ct[4];
    SDL_GPUTextureCreateInfo tex;
    int i;
    need(vs && fs, "probe shaders");
    SDL_zero(info);
    SDL_zero(vb);
    SDL_zero(va);
    SDL_zero(ct);
    vb[0].slot = 0;
    vb[0].pitch = sizeof(struct SkinVertex);
    vb[0].input_rate = SDL_GPU_VERTEXINPUTRATE_VERTEX;
    vb[1].slot = 1;
    vb[1].pitch = 4;
    vb[1].input_rate = SDL_GPU_VERTEXINPUTRATE_VERTEX;
    /* the layout of render/gpu_device.c's PROG_SKIN, and the pose's row beside */
    va[0].location = 0; va[0].format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT3; va[0].offset = offsetof(struct SkinVertex, position);
    va[1].location = 1; va[1].format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT3; va[1].offset = offsetof(struct SkinVertex, normal);
    va[2].location = 2; va[2].format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT2; va[2].offset = offsetof(struct SkinVertex, uv);
    va[3].location = 3; va[3].format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT4; va[3].offset = offsetof(struct SkinVertex, weights);
    va[4].location = 4; va[4].format = SDL_GPU_VERTEXELEMENTFORMAT_UBYTE4; va[4].offset = offsetof(struct SkinVertex, bones);
    va[5].location = 5; va[5].format = SDL_GPU_VERTEXELEMENTFORMAT_UINT; va[5].offset = offsetof(struct SkinVertex, flags);
    va[6].location = 6; va[6].format = SDL_GPU_VERTEXELEMENTFORMAT_UINT; va[6].offset = 0; va[6].buffer_slot = 1;
    for (i = 0; i < 4; i++)
        ct[i].format = SDL_GPU_TEXTUREFORMAT_R32G32B32A32_FLOAT;
    info.vertex_shader = vs;
    info.fragment_shader = fs;
    info.vertex_input_state.vertex_buffer_descriptions = vb;
    info.vertex_input_state.num_vertex_buffers = 2;
    info.vertex_input_state.vertex_attributes = va;
    info.vertex_input_state.num_vertex_attributes = 7;
    info.primitive_type = SDL_GPU_PRIMITIVETYPE_POINTLIST;
    info.rasterizer_state.enable_depth_clip = true;
    info.target_info.color_target_descriptions = ct;
    info.target_info.num_color_targets = 4;
    g_probe = SDL_CreateGPUGraphicsPipeline(g_dev, &info);
    SDL_ReleaseGPUShader(g_dev, vs);
    SDL_ReleaseGPUShader(g_dev, fs);
    need(g_probe != NULL, "probe pipeline");
    SDL_zero(tex);
    tex.type = SDL_GPU_TEXTURETYPE_2D;
    tex.format = SDL_GPU_TEXTUREFORMAT_R32G32B32A32_FLOAT;
    tex.usage = SDL_GPU_TEXTUREUSAGE_COLOR_TARGET;
    tex.width = tex.height = SIDE;
    tex.layer_count_or_depth = 1;
    tex.num_levels = 1;
    for (i = 0; i < 4; i++) {
        g_targets[i] = SDL_CreateGPUTexture(g_dev, &tex);
        need(g_targets[i] != NULL, "probe targets");
    }
    g_vertices = buffer(SDL_GPU_BUFFERUSAGE_VERTEX, PER_BATCH * (Uint32)sizeof(struct SkinVertex));
    g_poseRows = buffer(SDL_GPU_BUFFERUSAGE_VERTEX, PER_BATCH * 4);
    g_rows = buffer(SDL_GPU_BUFFERUSAGE_GRAPHICS_STORAGE_READ, ROWS_MAX * 16);
    B.corners = malloc(PER_BATCH * sizeof *B.corners);
    B.poses = malloc(PER_BATCH * sizeof *B.poses);
    B.rows = malloc(ROWS_MAX * sizeof *B.rows);
    need(g_vertices && g_poseRows && g_rows && B.corners && B.poses && B.rows, "probe buffers");
}

static void probe_free(void)
{
    int i;
    for (i = 0; i < 4; i++)
        SDL_ReleaseGPUTexture(g_dev, g_targets[i]);
    SDL_ReleaseGPUBuffer(g_dev, g_vertices);
    SDL_ReleaseGPUBuffer(g_dev, g_poseRows);
    SDL_ReleaseGPUBuffer(g_dev, g_rows);
    SDL_ReleaseGPUGraphicsPipeline(g_dev, g_probe);
    free(B.corners);
    free(B.poses);
    free(B.rows);
}

/* Runs the batch's corners through the probe; out gets each corner's 16 floats: its clip position,
   its colour, its texture coordinates and its point in view space. */
static void probe_run(float *out)
{
    Uint32 targetBytes = SIDE * SIDE * 16, sizes[3], offsets[3], total = 0, i;
    SDL_GPUTransferBufferCreateInfo tinfo;
    SDL_GPUTransferBuffer *up, *down;
    SDL_GPUBuffer *dests[3];
    SDL_GPUCommandBuffer *cmd;
    SDL_GPUCopyPass *copy;
    SDL_GPUFence *fence;
    SDL_GPUColorTargetInfo ct[4];
    SDL_GPURenderPass *rp;
    unsigned char *p;
    float step = 2.0f / (float)SIDE;
    int t;
    sizes[0] = B.ncorners * (Uint32)sizeof(struct SkinVertex);
    sizes[1] = B.ncorners * 4;
    sizes[2] = B.nrows * 16;
    dests[0] = g_vertices;
    dests[1] = g_poseRows;
    dests[2] = g_rows;
    for (i = 0; i < 3; i++) {
        offsets[i] = total;
        total += (sizes[i] + 15) & ~15u;
    }
    SDL_zero(tinfo);
    tinfo.usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD;
    tinfo.size = total;
    up = SDL_CreateGPUTransferBuffer(g_dev, &tinfo);
    tinfo.usage = SDL_GPU_TRANSFERBUFFERUSAGE_DOWNLOAD;
    tinfo.size = targetBytes * 4;
    down = SDL_CreateGPUTransferBuffer(g_dev, &tinfo);
    need(up && down, "transfer buffers");
    p = SDL_MapGPUTransferBuffer(g_dev, up, false);
    memcpy(p + offsets[0], B.corners, sizes[0]);
    memcpy(p + offsets[1], B.poses, sizes[1]);
    memcpy(p + offsets[2], B.rows, sizes[2]);
    SDL_UnmapGPUTransferBuffer(g_dev, up);
    cmd = SDL_AcquireGPUCommandBuffer(g_dev);
    copy = SDL_BeginGPUCopyPass(cmd);
    for (i = 0; i < 3; i++) {
        SDL_GPUTransferBufferLocation src;
        SDL_GPUBufferRegion dst;
        src.transfer_buffer = up;
        src.offset = offsets[i];
        dst.buffer = dests[i];
        dst.offset = 0;
        dst.size = sizes[i];
        SDL_UploadToGPUBuffer(copy, &src, &dst, false);
    }
    SDL_EndGPUCopyPass(copy);
    SDL_zero(ct);
    for (i = 0; i < 4; i++) {
        ct[i].texture = g_targets[i];
        ct[i].load_op = SDL_GPU_LOADOP_CLEAR;
        ct[i].store_op = SDL_GPU_STOREOP_STORE;
        ct[i].cycle = true;
    }
    rp = SDL_BeginGPURenderPass(cmd, ct, 4, NULL);
    SDL_BindGPUGraphicsPipeline(rp, g_probe);
    SDL_BindGPUVertexStorageBuffers(rp, 0, &g_rows, 1);
    for (i = 0; i < B.ndraws; i++) {
        struct { Uint32 probe[4]; float screen[4]; } u;
        SDL_GPUBufferBinding bind[2];
        u.probe[0] = SIDE;
        memcpy(&u.probe[1], &step, 4);
        u.probe[2] = B.draws[i].first;
        u.probe[3] = 0;
        u.screen[0] = B.draws[i].half[0];
        u.screen[1] = B.draws[i].half[1];
        u.screen[2] = u.screen[3] = 0.0f;
        bind[0].buffer = g_vertices;
        bind[0].offset = B.draws[i].first * (Uint32)sizeof(struct SkinVertex);
        bind[1].buffer = g_poseRows;
        bind[1].offset = B.draws[i].first * 4;
        SDL_BindGPUVertexBuffers(rp, 0, bind, 2);
        SDL_PushGPUVertexUniformData(cmd, 0, &u, sizeof u);
        SDL_DrawGPUPrimitives(rp, B.draws[i].count, 1, 0, 0);
    }
    SDL_EndGPURenderPass(rp);
    copy = SDL_BeginGPUCopyPass(cmd);
    for (i = 0; i < 4; i++) {
        SDL_GPUTextureRegion src;
        SDL_GPUTextureTransferInfo dst;
        SDL_zero(src);
        SDL_zero(dst);
        src.texture = g_targets[i];
        src.w = src.h = SIDE;
        src.d = 1;
        dst.transfer_buffer = down;
        dst.offset = i * targetBytes;
        SDL_DownloadFromGPUTexture(copy, &src, &dst);
    }
    SDL_EndGPUCopyPass(copy);
    fence = SDL_SubmitGPUCommandBufferAndAcquireFence(cmd);
    need(fence && SDL_WaitForGPUFences(g_dev, true, &fence, 1), "probe submit");
    SDL_ReleaseGPUFence(g_dev, fence);
    p = SDL_MapGPUTransferBuffer(g_dev, down, false);
    for (i = 0; i < B.ncorners; i++)
        for (t = 0; t < 4; t++)
            memcpy(out + (size_t)i * 16 + t * 4, p + (size_t)t * targetBytes + (size_t)i * 16, 16);
    SDL_UnmapGPUTransferBuffer(g_dev, down);
    SDL_ReleaseGPUTransferBuffer(g_dev, up);
    SDL_ReleaseGPUTransferBuffer(g_dev, down);
}

/* ---- the corners ---- */

/* what a corner should be: the CPU's, its point in view space and its screen's size */
struct Expect {
    struct TexDrawVertex v;
    float inView[3];
    int width, height;
};

struct Tally {
    long corners, behind, clipped, outside, wrong, poisoned, notNumber;
    double pixel, rhw, depth, nearDepth, uv, inView;
    int colour;
};

static double relative(double got, double want)
{
    double scale = fabs(want) > 1e-30 ? fabs(want) : 1e-30;
    return fabs(got - want) / scale;
}

static void compare(struct Tally *t, const float *gpu, const struct Expect *e, const char *label)
{
    double w = gpu[3], x, y, z, rhw, viewError = 0.0, uv;
    int k, colour = 0, wrong = 0;
    t->corners++;
    for (k = 0; k < 3; k++) {
        double d = fabs((double)gpu[12 + k] - e->inView[k]) / (fabs(e->inView[k]) > 1.0 ? fabs(e->inView[k]) : 1.0);
        if (d > viewError)
            viewError = d;
    }
    if (viewError > t->inView)
        t->inView = viewError;
    for (k = 0; k < 4; k++) {
        static const int shift[4] = {16, 8, 0, 24};
        int want = (int)(e->v.diffuse >> shift[k] & 255), got = (int)floor(gpu[4 + k] * 255.0 + 0.5);
        if (abs(got - want) > colour)
            colour = abs(got - want);
    }
    if (colour > t->colour)
        t->colour = colour;
    uv = fabs((double)gpu[8] - e->v.u);
    if (fabs((double)gpu[9] - e->v.v) > uv)
        uv = fabs((double)gpu[9] - e->v.v);
    if (uv > t->uv)
        t->uv = uv;
    wrong = colour > 1 || uv > 1.0 / 4096;
    if ((w != w || gpu[0] != gpu[0] || gpu[1] != gpu[1] || gpu[2] != gpu[2]) && e->v.rhw == e->v.rhw) {
        /* not a number where the CPU's corner is one: a corner no draw shows */
        t->notNumber++;
        wrong = 1;
    } else if (!(w > 0.0) || !(e->v.rhw > 0.0f)) {
        /* behind the eye, where neither the CPU's projection nor a GPU's draws anything */
        t->behind++;
    } else if (!(e->v.z >= 0.0f && e->v.z <= 1.0f)) {
        /* outside the depth range, which clips it: nearer than the near plane, its depth the
           difference of large terms (as test_quad_instances counts such corners) */
        t->clipped++;
    } else {
        x = (gpu[0] / w + 1.0) * e->width / 2;
        y = (1.0 - gpu[1] / w) * e->height / 2;
        z = gpu[2] / w;
        rhw = 1.0 / w;
        if (relative(rhw, e->v.rhw) > t->rhw)
            t->rhw = relative(rhw, e->v.rhw);
        if (e->v.z >= 0.5f) {
            if (relative(z, e->v.z) > t->depth)
                t->depth = relative(z, e->v.z);
            wrong = wrong || relative(z, e->v.z) > 1e-5;
        } else {
            if (fabs(z - e->v.z) > t->nearDepth)
                t->nearDepth = fabs(z - e->v.z);
            wrong = wrong || fabs(z - e->v.z) > 1e-5;
        }
        wrong = wrong || relative(rhw, e->v.rhw) > 1e-5;
        /* the pixel, where the screen is (a guard band of the screen's size around it) */
        if (fabs(e->v.x) > 2.0 * e->width || fabs(e->v.y) > 2.0 * e->height) {
            t->outside++;
        } else {
            double d = fabs(x - e->v.x) > fabs(y - e->v.y) ? fabs(x - e->v.x) : fabs(y - e->v.y);
            if (d > t->pixel)
                t->pixel = d;
            wrong = wrong || d > 1.0 / 16;
        }
    }
    if (wrong && t->wrong++ < 8)
        fprintf(stderr, "skin_instances: %s: GPU clip (%g, %g, %g, %g) colour (%g, %g, %g, %g) uv (%g, %g); CPU (%.9g, %.9g, "
                "%.9g, rhw %.9g) colour %08x uv (%g, %g)\n", label, gpu[0], gpu[1], gpu[2], gpu[3], gpu[4], gpu[5], gpu[6],
                gpu[7], gpu[8], gpu[9], e->v.x, e->v.y, e->v.z, e->v.rhw, (unsigned)e->v.diffuse, e->v.u, e->v.v);
}

static struct Expect *g_expect;
static float *g_out;

static void flush_batch(struct Tally *t)
{
    uint32_t d, i;
    if (!B.ncorners)
        return;
    probe_run(g_out);
    for (d = 0; d < B.ndraws; d++)
        for (i = B.draws[d].first; i < B.draws[d].first + B.draws[d].count; i++)
            compare(t, g_out + (size_t)i * 16, &g_expect[i], B.draws[d].label);
    B.ncorners = B.nrows = B.ndraws = 0;
}

/* Every corner of every mesh of the actor, through the view: the CPU's, and the shader's input. */
static void actor_corners(struct Tally *t, const struct Actor *a, struct View *s, const char *label)
{
    static struct TexDrawVertex corners[3 * 65536];
    static float world[3 * 65536][3];
    static struct SkinVertex packed[65536];
    static unsigned short indices[3 * 65536];
    float light[3];
    size_t nrows = skin_pose_size((uint32_t)a->model->bones);
    uint32_t pose = 0;
    int k, i, posed = 0;
    for (k = 0; k < a->model->model.meshCount; k++) {
        granny_mesh *mesh = a->model->meshes[k].mesh;
        int tris = reference_mesh(&s->renderer, a, k, s->view, corners, world, light), v, n, c;
        if (!tris)
            continue;
        v = _GrannyGetMeshVertexCount(mesh);
        n = tris * 3;
        if (B.ncorners + (uint32_t)n > PER_BATCH || B.nrows + nrows > ROWS_MAX || B.ndraws == 4096)
            flush_batch(t), posed = 0;
        if (!posed) {
            pose = B.nrows;
            B.nrows += (uint32_t)actor_rows(a, s, light, B.rows + pose);
            posed = 1;
        }
        _GrannyCopyMeshIndices(mesh, 2, indices);
        for (c = 0; c < v; c++)
            skin_pack_vertex(&a->model->sources[k][c], a->model->meshes[k].rigidBone, a->model->bones, &packed[c]);
        B.draws[B.ndraws].first = B.ncorners;
        B.draws[B.ndraws].count = (uint32_t)n;
        B.draws[B.ndraws].half[0] = (float)s->renderer.width * 0.5f;
        B.draws[B.ndraws].half[1] = (float)s->renderer.height * 0.5f;
        snprintf(B.draws[B.ndraws].label, sizeof B.draws[B.ndraws].label, "%s mesh %d", label, k);
        B.ndraws++;
        for (c = 0; c < n; c++) {
            struct Expect *e = &g_expect[B.ncorners];
            B.corners[B.ncorners] = packed[indices[c]];
            B.poses[B.ncorners] = pose;
            if (a->poisoned)
                for (i = 0; i < 4; i++)
                    if (packed[indices[c]].weights[i] == 0.0f && !a->model->weighted[packed[indices[c]].bones[i]]) {
                        t->poisoned++;
                        break;
                    }
            e->v = corners[c];
            for (i = 0; i < 3; i++)
                e->inView[i] = world[c][0] * s->view[i] + world[c][1] * s->view[3 + i] + world[c][2] * s->view[6 + i]
                               + s->view[9 + i];
            e->width = s->renderer.width;
            e->height = s->renderer.height;
            B.ncorners++;
        }
    }
}

static int test_vertices(int placements)
{
    static const int sizes[][2] = {{1024, 664}, {1280, 720}, {1600, 900}, {1920, 1080}, {3840, 2160}, {800, 600}};
    struct Tally t;
    int i, p, k, ok;
    memset(&t, 0, sizeof t);
    for (i = 0; i < g_nmodels; i++) {
        struct Model *m = &g_models[i];
        for (p = 0; p < m->nposes; p++)
            for (k = 0; k < placements; k++) {
                struct View s;
                struct Actor a;
                int size = irange(0, 5);
                char label[160];
                make_view(&s, sizes[size][0], sizes[size][1]);
                make_actor(&a, m, m->poses[p], &s, chance(80));
                a.poisoned = m->poisoned[p];
                snprintf(label, sizeof label, "%s pose %d kind %d", m->name, p, a.kind);
                actor_corners(&t, &a, &s, label);
            }
    }
    flush_batch(&t);
    ok = !t.wrong && t.corners > 0;
    printf("vertices: %ld corners of %d models (%ld behind the eye, %ld outside the depth range, %ld off the "
           "screen's guard band, %ld not a number); largest "
           "differences: pixel %.3g, rhw %.3g and depth %.3g relative (nearer than twice the near plane: %.3g of "
           "the depth range), texture coordinates %.3g, "
           "colour %d step%s, point in view space %.3g relative; %ld beyond the tolerance; %ld with a weight "
           "of 0 on a bone that is not a number\n",
           t.corners, g_nmodels, t.behind, t.clipped, t.outside, t.notNumber, t.pixel, t.rhw, t.depth, t.nearDepth, t.uv, t.colour,
           t.colour == 1 ? "" : "s", t.inView, t.wrong, t.poisoned);
    return ok;
}

/* ---- pictures through render/gpu_device.c ---- */

#define PICTURE_W 1280
#define PICTURE_H 720

static void save_picture(uint32_t *pixels, int scene, const char *side)
{
    SDL_Surface *s = SDL_CreateSurfaceFrom(PICTURE_W, PICTURE_H, SDL_PIXELFORMAT_XRGB8888, pixels, PICTURE_W * 4);
    char name[64];
    snprintf(name, sizeof name, "skin_%d_%s.bmp", scene, side);
    if (s) {
        SDL_SaveBMP(s, name);
        SDL_DestroySurface(s);
    }
}

/* 64 texels square: red along u, green along v, blue along both, a few steps from texel to texel */
static gfx_surface *gradients(void)
{
    gfx_surface *t = gfx_create_texture(64, 64, GFX_FMT_8888);
    struct gfx_lock lk;
    int x, y;
    need(t && gfx_lock(t, NULL, &lk) == 0, "the texture");
    for (y = 0; y < 64; y++)
        for (x = 0; x < 64; x++) {
            DWORD c = 0xff000000u | (DWORD)(x * 4) << 16 | (DWORD)(y * 4) << 8 | (DWORD)((x + y) * 2);
            memcpy((BYTE *)lk.bits + y * lk.pitch + x * 4, &c, 4);
        }
    gfx_unlock(t);
    return t;
}

static int test_pictures(int scenes)
{
    static uint32_t a[PICTURE_W * PICTURE_H], c[PICTURE_W * PICTURE_H];
    static struct TexDrawVertex corners[3 * 65536];
    static float world[3 * 65536][3];
    static float rows[SKIN_POSE_HEADER + 3 * SKIN_BONES_MAX][4];
    gfx_surface *tex = gradients();
    long covered = 0, over = 0, largest = 0, worst = 0;
    int scene, i, k, side, ok = 1, fogged = 0;
    for (i = 0; i < g_nmodels; i++)
        need(granny_model_res_skin(&g_models[i].res) && g_models[i].res.skin, "a model on the device");
    for (scene = 0; scene < scenes; scene++) {
        struct View s;
        struct Actor actors[8];
        struct GfxFog fog;
        int n = irange(1, 8);
        struct PictureDifference diff;
        make_view(&s, PICTURE_W, PICTURE_H);
        for (i = 0; i < n; i++) {
            struct Model *m = &g_models[irange(0, g_nmodels - 1)];
            make_actor(&actors[i], m, m->poses[irange(0, m->real - 1)], &s, 1);
            if (chance(80))
                actors[i].alpha = 0xff;
        }
        /* every other scene fogged as a map's fog, its end before its start now and then */
        memset(&fog, 0, sizeof fog);
        fog.on = scene & 1;
        fog.colour = 0xff8090a0;
        fog.start = uniform(0.0f, 0.5f);
        fog.end = chance(20) ? uniform(0.0f, fog.start) : uniform(fog.start, 1.0f);
        fog.origin = 10.0f;
        fog.span = uniform(500.0f, 3000.0f);
        fogged += fog.on;
        for (side = 0; side < 2; side++) {
            gfx_clear(GFX_CLEAR_TARGET | GFX_CLEAR_ZBUFFER, 0xff203040, 1.0f, NULL);
            gfx_set_mode(DRAW_OPAQUE_CLAMP);
            gfx_set_fog(fog.on ? &fog : NULL);
            gfx_set_texture(0, tex);
            for (i = 0; i < n; i++) {
                const struct Actor *ac = &actors[i];
                int pose = -1;
                float light[3];
                for (k = 0; k < ac->model->model.meshCount; k++) {
                    int tris = reference_mesh(&s.renderer, ac, k, s.view, corners, world, light);
                    const struct GrannyResMesh *mesh = &ac->model->meshes[k];
                    if (!tris)
                        continue;
                    if (side) {
                        gfx_draw_vertices(corners, tris * 3, 0);
                        continue;
                    }
                    if (pose < 0) {
                        size_t r = actor_rows(ac, &s, light, rows);
                        pose = gfx_skin_pose((const float (*)[4])rows, r);
                    }
                    gfx_draw_skin(ac->model->res.skin, mesh->firstIndex, mesh->indexCount, pose);
                }
            }
            gfx_set_texture(0, NULL);
            gfx_set_fog(NULL);
            gfx_end_scene();
            need(gfx_read_frame(NULL, side ? c : a, PICTURE_W * 4) == 0, "gfx_read_frame");
        }
        picture_difference(a, c, PICTURE_W, PICTURE_H, &diff);
        for (i = 0; i < PICTURE_W * PICTURE_H; i++)
            covered += (a[i] & 0xffffff) != 0x203040u;
        over += diff.over;
        if (diff.over > worst)
            worst = diff.over;
        if (diff.largest > largest)
            largest = diff.largest;
        if (!picture_within_budget(&diff, PICTURE_W, PICTURE_H)) {
            ok = 0;
            save_picture(a, scene, "device");
            save_picture(c, scene, "cpu");
        }
    }
    gfx_release(tex);
    printf("pictures through render/gpu_device.c: %d scenes of %d by %d (%d fogged), %ld pixels drawn; pixels more "
           "than a step apart: %ld in all, at most %ld in a scene (%.4f %% of the screen), the largest group %ld\n",
           scenes, PICTURE_W, PICTURE_H, fogged, covered, over, worst, 100.0 * (double)worst / (PICTURE_W * PICTURE_H),
           largest);
    return ok;
}

int main(int argc, char **argv)
{
    static struct Renderer renderer;
    struct C3dDevice *d = &g_3dDevice;
    const char *folder = argc > 2 ? argv[2] : "";
    uint64_t seed = argc > 3 ? strtoull(argv[3], NULL, 0) : 20261113u;
    int fuzzed = argc > 4 ? (int)strtol(argv[4], NULL, 0) : 20;
    int scenes = argc > 5 ? (int)strtol(argv[5], NULL, 0) : 20, ok = 1, i, p;
    if (argc > 1)
        SDL_setenv_unsafe("RAGEXE_MSAA", argv[1], 1);
    g_rng = seed;
    g_renderer = &renderer;
    renderer_set_projection(&renderer, PICTURE_W, PICTURE_H);
    if (!load_folder(folder, fuzzed)) {
        printf("skin_instances: skipped: no converted Granny model in \"%s\"\n", folder);
        return 77;
    }
    if (!SDL_Init(SDL_INIT_VIDEO)) {
        printf("skin_instances: skipped: %s\n", SDL_GetError());
        return 77;
    }
    d->hwnd = SDL_CreateWindow("skin instances", PICTURE_W, PICTURE_H, SDL_WINDOW_HIDDEN);
    need(d->hwnd != NULL, "SDL_CreateWindow");
    d->width = PICTURE_W;
    d->height = PICTURE_H;
    need(gfx_open(d->hwnd) == 0, "gfx_open");
    d->target = gfx_create_screen(d->width, d->height);
    if (!d->target) {
        printf("skin_instances: skipped: no GPU device\n");
        return 77;
    }
    need(gfx_get_caps(&g_gfxCaps) == 0 && gfx_create_zbuffer() == 0, "the device's screen");
    g_dev = gpu_dev();
    printf("skin_instances: %s on %s, seed %llu, RAGEXE_MSAA %s; %d models, %d animation files\n",
           SDL_GetGPUDeviceDriver(g_dev),
           SDL_GetStringProperty(SDL_GetGPUDeviceProperties(g_dev), SDL_PROP_GPU_DEVICE_NAME_STRING, "?"),
           (unsigned long long)seed, argc > 1 ? argv[1] : "unset", g_nmodels, g_nanimations);
    for (i = 0; i < g_nmodels; i++)
        printf("  %s: %d bones, %d meshes, %d poses (%d animations move it)\n", g_models[i].name, g_models[i].bones,
               g_models[i].model.meshCount, g_models[i].nposes, g_models[i].animated);
    g_expect = malloc(PER_BATCH * sizeof *g_expect);
    g_out = malloc((size_t)PER_BATCH * 16 * sizeof *g_out);
    need(g_expect && g_out, "memory");
    probe_init();
    ok = test_vertices(3);
    probe_free();
    if (scenes > 0)
        ok = test_pictures(scenes) && ok;
    for (i = 0; i < g_nmodels; i++) {
        gfx_skin_release(g_models[i].res.skin);
        for (p = 0; p < g_models[i].nposes; p++)
            free(g_models[i].poses[p]);
    }
    gfx_close();
    gfx_release(d->target);
    SDL_DestroyWindow(d->hwnd);
    SDL_Quit();
    printf("skin_instances: %s\n", ok ? "every corner within the tolerance, every picture within the budget"
                                       : "DIFFERENCES");
    return ok ? 0 : 1;
}
