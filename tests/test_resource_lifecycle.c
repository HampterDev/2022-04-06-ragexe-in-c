#include "render/texture_cache.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "owned_storage_test_allocator.h"
#define GRANNY_IMPORT
#define MSS_IMPORT
static void *lifecycle_calloc(size_t count, size_t size)
{
    void *p = checked_malloc(count * size);
    if (p) memset(p, 0, count * size);
    return p;
}
#define malloc checked_malloc
#define calloc lifecycle_calloc
#define free checked_free
#include "../src/resources/granny_res.c"
#include "../src/resources/wave.c"
#undef malloc
#undef calloc
#undef free

#define AIL_FILE_WAV 1

static unsigned sdk_handles, file_closes, errors, wave_frees;
static int file_available = 1, sdk_version = 1, sdk_read = 1, sdk_info = 1;
static int wave_kind = AIL_FILE_WAV, rigid = 1, has_alpha = 1;
static int vertex_count = 3, index_count = 3, invalid_index;
static unsigned texture_copies;
static int path_available = 1;
static unsigned last_lookup_job;
static const char *job_model_name = "guildflag90_1.gr2";
static char last_opened_name[512];
static unsigned char file_data[68];
static granny_file_info info;
static granny_skeleton skeleton;
static granny_mesh mesh;
static granny_model model;
static granny_texture texture;
static granny_model_mesh_binding mesh_binding;
static granny_material_binding material_binding;
static granny_model *models[] = {&model};
static granny_skeleton *skeletons[] = {&skeleton};
static granny_texture *textures[] = {&texture};
struct TexCache g_textureCache;
const granny_pixel_layout *GrannyRGBA8888PixelFormat;
const granny_data_type_definition *GrannyPNT332VertexType;

void *mem_alloc(size_t size) { return checked_malloc(size); }

void mem_free(void *p)
{
    checked_free(p);
}
void *std_allocate_bytes(size_t size) { return checked_malloc(size); }

void fatal_error(const char *message) { (void)message; abort(); }
struct Res *res_construct(struct Res *res) { memset(res, 0, sizeof *res); return res; }
void res_destruct(struct Res *res) { res->ops = NULL; }
void error_msg(const char *text, int flags) { assert(text && !flags); ++errors; }
struct File *file_construct(struct File *file) { memset(file, 0, sizeof *file); return file; }
BYTE file_open(struct File *file, const char *name, int mode, BYTE resolve, BYTE disk)
{
    assert(name && !mode && resolve && !disk);
    assert(strlen(name) < sizeof last_opened_name);
    strcpy(last_opened_name, name);
    if (!file_available) return 0;
    file->buffer = file_data; file->size = sizeof file_data; return 1;
}
BYTE file_read(struct File *file, void *out, int size)
{ assert(file->buffer && size == sizeof file_data); memcpy(out, file_data, size); return 1; }
void file_close(struct File *file) { assert(file->buffer); file->buffer = NULL; ++file_closes; }
void file_destruct(struct File *file) { if (file->buffer) file_close(file); }

struct Session g_session1;
struct Renderer *g_renderer;
struct Mat34 g_identityMatrix;
BYTE file_make_file_name(const char *name, int flags) { assert(name && !flags); return (BYTE)path_available; }
int crt_sprintf(char *out, const char *format, ...) { (void)out; (void)format; abort(); }
uint32_t number_truncate_low32(double value) { (void)value; abort(); }
float float_sqrt(float value) { (void)value; abort(); }
void mat_multiply(float *out, const float *a, const float *b) { (void)out; (void)a; (void)b; abort(); }
void mat_set_rotate_z(float *out, float angle) { (void)out; (void)angle; abort(); }
const char *session_get_job_name(struct Session *session, unsigned job, int sex)
{ assert(session == &g_session1 && job > 0 && sex == -1); last_lookup_job = job; return job_model_name; }
granny_control *__stdcall _GrannyPlayControlledAnimation(float time, granny_animation *a, granny_model_instance *m)
{ (void)time; (void)a; (void)m; abort(); }
void __stdcall _GrannySetControlLoopCount(granny_control *p, int count) { (void)p; (void)count; abort(); }
void __stdcall _GrannySetControlActive(granny_control *p, BYTE active) { (void)p; (void)active; abort(); }
void __stdcall _GrannyFreeControl(granny_control *p) { (void)p; abort(); }
void __stdcall _GrannySetModelClock(granny_model_instance *p, float time) { (void)p; (void)time; abort(); }
void __stdcall _GrannySampleModelAnimations(granny_model_instance *p, int first, int count, granny_local_pose *pose)
{ (void)p; (void)first; (void)count; (void)pose; abort(); }
void __stdcall _GrannyBuildWorldPose(granny_skeleton *p, int first, int count, granny_local_pose *pose,
    const float *offset, granny_world_pose *world)
{ (void)p; (void)first; (void)count; (void)pose; (void)offset; (void)world; abort(); }
void __stdcall _GrannyFreeCompletedModelControls(granny_model_instance *p) { (void)p; abort(); }
float *__stdcall _GrannyGetWorldPoseComposite4x4Array(granny_world_pose *p) { (void)p; abort(); }
float *__stdcall _GrannyGetWorldPoseComposite4x4(granny_world_pose *p, int bone) { (void)p; (void)bone; abort(); }
int *__stdcall _GrannyGetMeshBindingToBoneIndices(granny_mesh_binding *p) { (void)p; abort(); }
int *__stdcall _GrannyGetMeshTriangleGroups(granny_mesh *p) { (void)p; abort(); }
void *__stdcall _GrannyGetMeshVertices(granny_mesh *p) { (void)p; abort(); }
void __stdcall _GrannyDeformVertices(granny_mesh_deformer *p, const int *bones, const float *matrix,
    int count, const void *from, void *to)
{ (void)p; (void)bones; (void)matrix; (void)count; (void)from; (void)to; abort(); }
int __stdcall _GrannyGetMeshBytesPerIndex(granny_mesh *p) { (void)p; abort(); }
void *__stdcall _GrannyGetMeshIndices(granny_mesh *p) { (void)p; abort(); }

static void *sdk_alloc(void) { void *p = checked_malloc(1); if (p) ++sdk_handles; return p; }
static void sdk_free(void *p) { assert(p && sdk_handles); --sdk_handles; checked_free(p); }
BYTE __stdcall _GrannyVersionsMatch_(int a, int b, int c, int d)
{ assert(a == 2 && b == 1 && !c && d == 5); return (BYTE)sdk_version; }
granny_file *__stdcall _GrannyReadEntireFileFromMemory(int size, void *data)
{ assert(size == sizeof file_data && data == file_data); return sdk_read ? sdk_alloc() : NULL; }
granny_file_info *__stdcall _GrannyGetFileInfo(granny_file *file)
{ assert(file); return sdk_info ? &info : NULL; }
void __stdcall _GrannyFreeFileSection(granny_file *file, int section)
{ assert(file && (section == 1 || section == 2 || section == 4 || section == 5)); }
void __stdcall _GrannyFreeFile(granny_file *p) { sdk_free(p); }
granny_local_pose *__stdcall _GrannyNewLocalPose(int bones) { assert(bones == 1); return sdk_alloc(); }
void __stdcall _GrannyFreeLocalPose(granny_local_pose *p) { sdk_free(p); }
granny_world_pose *__stdcall _GrannyNewWorldPose(int bones) { assert(bones == 1); return sdk_alloc(); }
void __stdcall _GrannyFreeWorldPose(granny_world_pose *p) { sdk_free(p); }
granny_model_instance *__stdcall _GrannyInstantiateModel(granny_model *m) { assert(m == &model); return sdk_alloc(); }
void __stdcall _GrannyFreeModelInstance(granny_model_instance *p) { sdk_free(p); }
granny_skeleton *__stdcall _GrannyGetSourceSkeleton(granny_model_instance *p) { assert(p); return &skeleton; }
granny_mesh_binding *__stdcall _GrannyNewMeshBinding(granny_mesh *m, granny_skeleton *a, granny_skeleton *b)
{ assert(m == &mesh && a == &skeleton && b == &skeleton); return sdk_alloc(); }
void __stdcall _GrannyFreeMeshBinding(granny_mesh_binding *p) { sdk_free(p); }
int __stdcall _GrannyGetMeshIndexCount(granny_mesh *m) { assert(m == &mesh); return index_count; }
void __stdcall _GrannyCopyMeshIndices(granny_mesh *m, int bytes, void *out)
{ assert(m == &mesh && bytes == 2); memset(out, 0, (size_t)index_count * 2); if (invalid_index) ((unsigned short *)out)[0] = 3; }
int __stdcall _GrannyGetMeshVertexCount(granny_mesh *m) { assert(m == &mesh); return vertex_count; }
BYTE __stdcall _GrannyMeshIsRigid(granny_mesh *m) { assert(m == &mesh); return (BYTE)rigid; }
void __stdcall _GrannyCopyMeshVertices(granny_mesh *m, const granny_data_type_definition *type, void *out)
{ assert(m == &mesh && type == GrannyPNT332VertexType); memset(out, 0, 96); }
const granny_data_type_definition *__stdcall _GrannyGetMeshVertexType(granny_mesh *m)
{ assert(m == &mesh); return GrannyPNT332VertexType; }
granny_mesh_deformer *__stdcall _GrannyNewMeshDeformer(const granny_data_type_definition *a,
    const granny_data_type_definition *b, int kind)
{ assert(a == b && kind == GRANNY_DEFORM_POSITION_NORMAL); return sdk_alloc(); }
void __stdcall _GrannyFreeMeshDeformer(granny_mesh_deformer *p) { sdk_free(p); }
granny_texture *__stdcall _GrannyGetMaterialTextureByType(granny_material *material, int kind)
{ (void)material; assert(kind == GRANNY_DIFFUSE_COLOR_TEXTURE); return &texture; }
BYTE __stdcall _GrannyTextureHasAlpha(granny_texture *t) { assert(t == &texture); return (BYTE)has_alpha; }
void __stdcall _GrannyCopyTextureImage(granny_texture *t, int image, int mip,
    const granny_pixel_layout *layout, int w, int h, int stride, void *out)
{
    assert(t == &texture && !image && !mip && layout == GrannyRGBA8888PixelFormat);
    assert(w == 1 && h == 1 && stride == 4);
    memcpy(out, "\x12\x34\x56\x78", 4); ++texture_copies;
}
void surface_destroy(struct Surface *surface) { checked_free(surface); }
struct Texture *texture_from_pixels(int w, int h,
    const DWORD *bits, int alpha, int a, int b)
{
    struct Texture *texture = checked_malloc(sizeof *texture);
    assert(w == 1 && h == 1 && bits && alpha == has_alpha && !a && !b);
    assert(*bits == (has_alpha ? 0x78123456u : 0xff123456u));
    if (!texture) return NULL;
    memset(texture, 0, sizeof *texture);
    return texture;
}

int __stdcall _AIL_file_type(const void *data, unsigned size)
{ assert(data && size == sizeof file_data); return wave_kind; }
int __stdcall _AIL_WAV_info(const void *data, struct AilSoundInfo *out)
{
    assert(data); memset(out, 0, sizeof *out);
    out->data = (const BYTE *)data + 60; out->dataLen = 8; out->rate = 8000;
    out->bits = 4; out->channels = 1; out->samples = 9; out->blockSize = 8;
    return 1;
}
int __stdcall _AIL_decompress_ADPCM(const struct AilSoundInfo *in, void **out, unsigned *size)
{ assert(in && size); *out = sdk_alloc(); *size = 62; return 1; }
int __stdcall _AIL_decompress_ASI(const void *data, unsigned size, const char *name,
    void **out, unsigned *out_size, void *callback)
{ assert(data && size == sizeof file_data && name && out_size && !callback); *out = sdk_alloc(); *out_size = 62; return 1; }
void __stdcall _AIL_mem_free_lock(void *p) { sdk_free(p); ++wave_frees; }

static void test_wave(void)
{
    const int kinds[] = {AIL_FILE_WAV, AIL_FILE_ADPCM, AIL_FILE_ASI};
    unsigned i;
    for (i = 0; i < sizeof kinds / sizeof *kinds; ++i) {
        struct Res *res = wave_resource_create();
        struct Wave *wave = (struct Wave *)res;
        assert(res && res->ops == &wave_resource_ops && !resource_is_resident(res));
        resource_update(res); wave_kind = kinds[i];
        assert(resource_load(res, "sound.wav") && wave->data);
        wave_resource_reset(wave); wave_resource_reset(wave);
        assert(!wave->data && !wave->milesOwned && !sdk_handles && outstanding == 1);
        assert(resource_load(res, "sound.wav"));
        assert(resource_load(res, "replacement.wav"));
        resource_destroy(res); assert(!outstanding && !sdk_handles);
    }
    {
        struct Res *res = wave_resource_create();
        file_available = 0; assert(!resource_load(res, "missing.wav"));
        resource_destroy(res); file_available = 1; assert(!outstanding);
    }
    assert(wave_frees == 6);
}

static void fixture(void)
{
    memset(&info, 0, sizeof info); memset(&model, 0, sizeof model);
    memset(&mesh, 0, sizeof mesh); memset(&texture, 0, sizeof texture);
    material_binding.Material = (granny_material *)1;
    skeleton.BoneCount = 1; model.Skeleton = &skeleton;
    model.MeshBindingCount = 1; model.MeshBindings = &mesh_binding; mesh_binding.Mesh = &mesh;
    mesh.MaterialBindingCount = 1; mesh.MaterialBindings = &material_binding;
    texture.Width = texture.Height = texture.ImageCount = 1; texture.FromFileName = "texture";
    info.ModelCount = info.SkeletonCount = info.TextureCount = 1;
    info.Models = models; info.Skeletons = skeletons; info.Textures = textures;
}

static void test_granny(void)
{
    int failure, kind;
    for (failure = 0; failure < 9; ++failure) {
        struct Res *res = granny_model_resource_create();
        struct C3dGrannyModelRes *granny = (struct C3dGrannyModelRes *)res;
        fixture();
        assert(res && res->ops == &granny_model_resource_ops && !resource_is_resident(res));
        resource_update(res);
        granny_model_resource_clear(granny); granny_model_resource_clear(granny);
        if (failure == 0) { resource_destroy(res); assert(!outstanding); continue; }
        sdk_version = failure != 1; file_available = failure != 2;
        sdk_read = failure != 3; sdk_info = failure != 4;
        if (failure == 5) info.ModelCount = 0;
        if (failure == 6) info.SkeletonCount = 0;
        if (failure == 7) info.Models = NULL;
        if (failure == 8) info.TextureCount = -1;
        assert(!resource_load(res, "model_12.gr2"));
        resource_destroy(res); assert(!outstanding && !sdk_handles);
    }
    sdk_version = file_available = sdk_read = sdk_info = 1;
    for (kind = 0; kind < 2; ++kind) {
        struct Res *res = granny_model_resource_create();
        struct C3dGrannyModelRes *granny = (struct C3dGrannyModelRes *)res;
        fixture(); rigid = kind;
        assert(resource_load(res, "model_1234.gr2"));
        assert(granny->id == 1234 && granny->model && granny->textureCount == 1 && sdk_handles == (rigid ? 1u : 2u));
        assert(resource_load(res, "model_42.gr2"));
        assert(granny->id == 42 && granny->model && sdk_handles == (rigid ? 1u : 2u));
        granny_model_resource_clear(granny); granny_model_resource_clear(granny);
        assert(outstanding == 1 && !sdk_handles && !granny->textures && !granny->file);
        assert(resource_load(res, "model_7.gr2"));
        resource_destroy(res); assert(!outstanding && !sdk_handles);
    }
    {
        struct Res *res = granny_model_resource_create();
        char name[261]; memset(name, 'x', sizeof name - 1); name[sizeof name - 1] = 0;
        assert(!resource_load(res, NULL)); assert(!resource_load(res, name));
        assert(!resource_load(res, "no-number.gr2"));
        resource_destroy(res); assert(!outstanding);
    }
}

static void test_granny_animation_bounds(void)
{
    struct C3dGrannyBoneRes bones = {0};
    granny_animation *animation = (granny_animation *)1;
    bones.entries[0][0].animation = animation;
    bones.entries[9][4].animation = animation;
    assert(granny_animation_cache_get(&bones, 0, 0) == animation);
    assert(granny_animation_cache_get(&bones, 9, 4) == animation);
    assert(!granny_animation_cache_get(&bones, -1, 0));
    assert(!granny_animation_cache_get(&bones, 10, 0));
    assert(!granny_animation_cache_get(&bones, 0, -1));
    assert(!granny_animation_cache_get(&bones, 9, 5));
    assert(!granny_animation_cache_get(NULL, 0, 0));
}

static void test_granny_failures(void)
{
    struct C3dGrannyModelRes *resource;
    struct C3dGrannyModelRes prior;
    unsigned calls, start, live, i, copies;
    int kind, invalid;
    fail_at = allocation_calls + 1;
    assert(!granny_model_resource_create()); fail_at = 0;
    for (kind = 0; kind < 2; kind++) {
        fixture(); rigid = kind; has_alpha = kind;
        resource = (struct C3dGrannyModelRes *)granny_model_resource_create(); assert(resource);
        start = allocation_calls;
        assert(c3d_granny_model_res_load(resource, "model_7.gr2"));
        calls = allocation_calls - start; prior = *resource; live = outstanding;
        for (i = 1; i <= calls; i++) {
            fail_at = allocation_calls + i;
            assert(!c3d_granny_model_res_load(resource, "model_8.gr2")); fail_at = 0;
            assert(!memcmp(resource, &prior, sizeof prior) && outstanding == live);
            assert(sdk_handles == (rigid ? 1u : 2u));
        }
        resource->res.lockCount = 1; start = allocation_calls;
        assert(!c3d_granny_model_res_load(resource, "model_8.gr2") && start == allocation_calls);
        resource->res.lockCount = 0;
        assert(!memcmp(resource, &prior, sizeof prior));
        for (invalid = 0; invalid < 15; invalid++) {
            fixture();
            switch (invalid) {
            case 0: model.MeshBindingCount = -1; break;
            case 1: model.MeshBindings = NULL; break;
            case 2: mesh_binding.Mesh = NULL; break;
            case 3: vertex_count = -1; break;
            case 4: index_count = -1; break;
            case 5: index_count = 1; break;
            case 6: invalid_index = 1; break;
            case 7: mesh.MaterialBindingCount = -1; break;
            case 8: mesh.MaterialBindings = NULL; break;
            case 9: texture.Width = 0; break;
            case 10: texture.Height = -1; break;
            case 11: texture.Width = INT_MAX; break;
            case 12: texture.Width = texture.Height = 65536; break;
            case 13: textures[0] = NULL; break;
            case 14: skeleton.BoneCount = -1; break;
            }
            copies = texture_copies;
            assert(!c3d_granny_model_res_load(resource, "model_9.gr2"));
            assert(!memcmp(resource, &prior, sizeof prior) && outstanding == live);
            if (invalid >= 9 && invalid <= 12) assert(copies == texture_copies);
            vertex_count = index_count = 3; invalid_index = 0; textures[0] = &texture;
        }
        fixture();
        assert(c3d_granny_model_res_load(resource, "model_9.gr2"));
        resource_destroy(&resource->res); assert(!outstanding && !sdk_handles);
    }
    fixture(); has_alpha = 1;
}

static void test_animation_files(void)
{
    struct C3dGrannyBoneRes cache;
    granny_animation *animations[] = {(granny_animation *)1};
    unsigned before;
    int job, part, failure;
    granny_animation_cache_init(&cache);
    fixture();
    info.AnimationCount = 1; info.Animations = animations;
    for (failure = 0; failure < 9; failure++) {
        switch (failure) {
        case 0: sdk_version = 0; break;
        case 1: file_available = 0; break;
        case 2: sdk_read = 0; break;
        case 3: sdk_info = 0; break;
        case 4: info.AnimationCount = 0; break;
        case 5: info.AnimationCount = 2; break;
        case 6: info.Animations = NULL; break;
        case 7: animations[0] = NULL; break;
        case 8: fail_at = allocation_calls + 1; break;
        }
        assert(!granny_animation_cache_load(&cache, "model_1.gr2", 0, 0));
        assert(!cache.entries[0][0].file && !cache.entries[0][0].animation);
        assert(!outstanding && !sdk_handles);
        sdk_version = file_available = sdk_read = sdk_info = 1;
        info.AnimationCount = 1; info.Animations = animations;
        animations[0] = (granny_animation *)1; fail_at = 0;
    }
    before = allocation_calls;
    assert(!granny_animation_cache_load(&cache, NULL, 0, 0));
    assert(!granny_animation_cache_load(&cache, "model.gr2", -1, 0));
    assert(!granny_animation_cache_load(&cache, "model.gr2", 0, 5));
    assert(allocation_calls == before);
    for (job = 0; job < GRANNY_BONE_JOB_COUNT; job++)
        for (part = 0; part < GRANNY_BONE_ANIMATION_COUNT; part++) {
            assert(granny_animation_cache_load(&cache, "model_1.gr2", job, part));
            before = allocation_calls;
            assert(granny_animation_cache_load(&cache, "model_1.gr2", job, part));
            assert(granny_animation_cache_get(&cache, job, part) == animations[0]);
            assert(allocation_calls == before && cache.entries[job][part].file);
        }
    assert(outstanding == 50 && sdk_handles == 50);
    granny_animation_cache_clear(&cache); granny_animation_cache_clear(&cache);
    assert(!outstanding && !sdk_handles);
}

static void test_animation_lookup(void)
{
    struct C3dGrannyBoneRes cache = {0};
    granny_animation *animations[] = {(granny_animation *)1};
    char long_name[400];
    unsigned before;
    fixture(); info.AnimationCount = 1; info.Animations = animations;
    file_available = 0;
    assert(!granny_animation_cache_get(&cache, 1, 0));
    assert(!cache.entries[1][0].file && !cache.entries[1][0].animation);
    assert(!outstanding && !sdk_handles);
    file_available = 1;
    assert(granny_animation_cache_get(&cache, 1, 0) == animations[0]);
    assert(last_lookup_job == 722);
    assert(cache.entries[1][0].file && !cache.entries[5][0].file);
    assert(!strcmp(last_opened_name, "model\\3dmob\\guildflag90_1.gr2"));
    assert(granny_animation_cache_get(&cache, 9, 4) == animations[0]);
    assert(!strcmp(last_opened_name, "model\\3dmob_bone\\9_damage.gr2"));
    before = allocation_calls;
    assert(granny_animation_cache_get(&cache, 9, 4) == animations[0]);
    assert(granny_animation_cache_get(&cache, 1, 0) == animations[0]);
    path_available = 0;
    assert(!granny_animation_cache_get(&cache, 2, 2)); path_available = 1;
    memset(long_name, 'x', sizeof long_name - 1); long_name[sizeof long_name - 1] = 0;
    job_model_name = long_name;
    assert(!granny_animation_cache_get(&cache, 2, 0));
    job_model_name = NULL;
    assert(!granny_animation_cache_get(&cache, 2, 0));
    job_model_name = "guildflag90_1.gr2";
    assert(before == allocation_calls);
    granny_animation_cache_clear(&cache);
    assert(!outstanding && !sdk_handles);
    assert(granny_animation_cache_get(&cache, 1, 0) == animations[0]);
    granny_animation_cache_clear(&cache);
    assert(!outstanding && !sdk_handles);
}

int main(void)
{
    static const unsigned char wave_bytes[68] = {
        'R','I','F','F',60,0,0,0,'W','A','V','E','f','m','t',' ',20,0,0,0,
        17,0,1,0,64,31,0,0,199,27,0,0,8,0,4,0,2,0,9,0,
        'f','a','c','t',4,0,0,0,9,0,0,0,'d','a','t','a',8,0,0,0
    };
    memcpy(file_data, wave_bytes, sizeof file_data);
    resource_destroy(NULL);
    test_wave(); test_granny(); test_granny_animation_bounds();
    test_granny_failures(); test_animation_files(); test_animation_lookup();
    assert(!outstanding && !sdk_handles && file_closes && errors);
    puts("resource lifecycle: typed dispatch, wave allocator ownership, Granny load failures, replacement and complete/repeated cleanup passed");
    puts("Granny resources: every allocation failure preserves the old model, pinned reload rejected, invalid geometry bounded, RGBA converted, 50 animation files owned and released exactly once");
    puts("Guild flag: lazy loading, job-722 lookup, missing-file retry, cache reuse and reload after clear passed");
    return 0;
}
