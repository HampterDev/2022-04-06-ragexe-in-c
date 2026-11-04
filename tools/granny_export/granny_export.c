/* granny_export: converts the game's Granny 2.1 models and animations (.gr2) to glTF 2.0 binaries
   that the portable client loads in their place (src/resources/granny_gltf.c). It runs on Windows,
   or under Wine, next to the game's granny2.dll, which does the decoding:

       granny_export <game directory>

   Every .gr2 file in the game's data folder and in the GRF archives DATA.INI names (the data folder
   first, then the archives in DATA.INI order) is written to
   <game directory>/data/model/gltf/<crc32 of the .gr2 file>.glb. Models keep their skeleton, rest
   pose, inverse bind matrices, skinned or rigid meshes, triangle groups, materials and textures;
   animations are sampled with Granny on their model's skeleton 60 times a second, one translation,
   rotation and scale track per bone. */
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <math.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "zlib.h"
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"

/* ---- the part of Granny 2.1.0.5 this uses (32-bit layouts, checked against the type tables) ---- */

struct granny_transform {
    unsigned Flags;
    float Position[3];
    float Orientation[4];
    float ScaleShear[3][3];
};

struct granny_bone {
    const char *Name;
    int ParentIndex;
    struct granny_transform LocalTransform;
    float InverseWorld4x4[16];
    void *LightInfo, *CameraInfo, *ExtendedData[2];
};

struct granny_skeleton {
    const char *Name;
    int BoneCount;
    struct granny_bone *Bones;
    int LODType;
};

struct granny_bone_binding {
    const char *BoneName;
    float OBBMin[3], OBBMax[3];
    int TriangleIndexCount;
    int *TriangleIndices;
};

struct granny_material {
    const char *Name;
};

struct granny_mesh {
    const char *Name;
    void *PrimaryVertexData;
    int MorphTargetCount;
    void *MorphTargets;
    void *PrimaryTopology;
    int MaterialBindingCount;
    struct granny_material **MaterialBindings;
    int BoneBindingCount;
    struct granny_bone_binding *BoneBindings;
};

struct granny_model {
    const char *Name;
    struct granny_skeleton *Skeleton;
    struct granny_transform InitialPlacement;
    int MeshBindingCount;
    struct granny_mesh **MeshBindings;
};

struct granny_texture {
    const char *FromFileName;
    int TextureType, Width, Height, Encoding, SubFormat, Layout[9], ImageCount;
    void *Images;
};

struct granny_animation {
    const char *Name;
    float Duration, TimeStep;
    int TrackGroupCount;
    void **TrackGroups;
};

struct granny_file_info {
    void *ArtToolInfo, *ExporterInfo;
    const char *FromFileName;
    int FileType;
    int TextureCount;
    struct granny_texture **Textures;
    int MaterialCount;
    struct granny_material **Materials;
    int SkeletonCount;
    struct granny_skeleton **Skeletons;
    int VertexDataCount;
    void **VertexDatas;
    int TriTopologyCount;
    void **TriTopologies;
    int MeshCount;
    struct granny_mesh **Meshes;
    int ModelCount;
    struct granny_model **Models;
    int TrackGroupCount;
    void **TrackGroups;
    int AnimationCount;
    struct granny_animation **Animations;
};

struct granny_tri_group {
    int MaterialIndex, TriFirst, TriCount;
};

#define GI __declspec(dllimport)
GI const char *__stdcall _GrannyGetVersionString(void);
GI void *__stdcall _GrannyReadEntireFileFromMemory(int size, void *memory);
GI struct granny_file_info *__stdcall _GrannyGetFileInfo(void *file);
GI void __stdcall _GrannyFreeFile(void *file);
GI void *__stdcall _GrannyInstantiateModel(struct granny_model *model);
GI void __stdcall _GrannyFreeModelInstance(void *instance);
GI void *__stdcall _GrannyNewLocalPose(int bones);
GI void __stdcall _GrannyFreeLocalPose(void *pose);
GI struct granny_transform *__stdcall _GrannyGetLocalPoseTransform(void *pose, int bone);
GI void *__stdcall _GrannyPlayControlledAnimation(float start, struct granny_animation *animation, void *instance);
GI void __stdcall _GrannySetControlLoopCount(void *control, int count);
GI void __stdcall _GrannyFreeControl(void *control);
GI void __stdcall _GrannySetModelClock(void *instance, float t);
GI void __stdcall _GrannySampleModelAnimations(void *instance, int first, int count, void *pose);
GI int __stdcall _GrannyFindBoneByName(struct granny_skeleton *skeleton, const char *name, int *bone);
GI int __stdcall _GrannyGetMeshVertexCount(struct granny_mesh *mesh);
GI int __stdcall _GrannyGetMeshIndexCount(struct granny_mesh *mesh);
GI void __stdcall _GrannyCopyMeshIndices(struct granny_mesh *mesh, int bytes, void *out);
GI void __stdcall _GrannyCopyMeshVertices(struct granny_mesh *mesh, const void *type, void *out);
GI BYTE __stdcall _GrannyMeshIsRigid(struct granny_mesh *mesh);
GI int __stdcall _GrannyGetMeshTriangleGroupCount(struct granny_mesh *mesh);
GI struct granny_tri_group *__stdcall _GrannyGetMeshTriangleGroups(struct granny_mesh *mesh);
GI struct granny_texture *__stdcall _GrannyGetMaterialTextureByType(struct granny_material *material, int type);
GI BYTE __stdcall _GrannyTextureHasAlpha(struct granny_texture *texture);
GI void __stdcall _GrannyCopyTextureImage(struct granny_texture *texture, int image, int mip, const void *layout,
                                          int width, int height, int stride, void *pixels);
GI extern const void *GrannyRGBA8888PixelFormat;
GI extern const void *GrannyPWNT3432VertexType;

#define GRANNY_DIFFUSE_COLOR_TEXTURE 2
#define SAMPLE_RATE 60.0f

struct Pwnt3432 {
    float p[3];
    unsigned char w[4], i[4];
    float n[3], t[2];
};

/* ---- growing byte buffers, JSON text and the glTF binary chunk ---- */

struct Buf {
    unsigned char *data;
    size_t size, cap;
};

static void buf_add(struct Buf *b, const void *data, size_t size)
{
    if (b->size + size > b->cap) {
        size_t cap = b->cap ? b->cap : 4096;
        while (cap < b->size + size)
            cap *= 2;
        b->data = realloc(b->data, cap);
        if (!b->data) {
            fprintf(stderr, "out of memory\n");
            exit(1);
        }
        b->cap = cap;
    }
    if (size)
        memcpy(b->data + b->size, data, size);
    b->size += size;
}

static void text(struct Buf *b, const char *format, ...)
{
    char line[4096];
    va_list args;
    int n;
    va_start(args, format);
    n = vsnprintf(line, sizeof line, format, args);
    va_end(args);
    if (n < 0 || n >= (int)sizeof line) {
        fprintf(stderr, "JSON line too long\n");
        exit(1);
    }
    buf_add(b, line, (size_t)n);
}

/* A JSON string from a CP949 name: printable ASCII as is, everything else as \u00XX escapes of the
   raw bytes, so the names survive unchanged and compare like the client's. */
static void json_string(struct Buf *b, const char *s)
{
    buf_add(b, "\"", 1);
    for (; s && *s; s++) {
        unsigned char c = (unsigned char)*s;
        if (c == '"' || c == '\\')
            text(b, "\\%c", c);
        else if (c < 0x20 || c > 0x7e)
            text(b, "\\u%04x", c);
        else
            buf_add(b, s, 1);
    }
    buf_add(b, "\"", 1);
}

struct Gltf {
    struct Buf bin, views, accessors;
    int nviews, naccessors;
};

static int add_view(struct Gltf *g, const void *data, size_t size, int target)
{
    static const unsigned char zero[4];
    size_t offset;
    buf_add(&g->bin, zero, (4 - g->bin.size % 4) % 4);
    offset = g->bin.size;
    buf_add(&g->bin, data, size);
    text(&g->views, "%s{\"buffer\":0,\"byteOffset\":%u,\"byteLength\":%u", g->nviews ? "," : "", (unsigned)offset,
         (unsigned)size);
    if (target)
        text(&g->views, ",\"target\":%d", target);
    text(&g->views, "}");
    return g->nviews++;
}

enum { CT_FLOAT = 5126, CT_UBYTE = 5121, CT_USHORT = 5123 };

static int add_accessor(struct Gltf *g, const void *data, int count, int type, int components, int normalized,
                        int target, int bounds)
{
    static const char *const kinds[] = {"", "SCALAR", "VEC2", "VEC3", "VEC4"};
    size_t unit = type == CT_FLOAT ? 4 : type == CT_USHORT ? 2 : 1;
    int view = add_view(g, data, (size_t)count * components * unit, target), i, c;
    text(&g->accessors, "%s{\"bufferView\":%d,\"componentType\":%d,\"count\":%d,\"type\":\"%s\"",
         g->naccessors ? "," : "", view, type, count, components == 16 ? "MAT4" : kinds[components]);
    if (normalized)
        text(&g->accessors, ",\"normalized\":true");
    if (bounds && type == CT_FLOAT && count) {
        const float *f = data;
        for (c = 0; c < 2; c++) {
            text(&g->accessors, c ? ",\"max\":[" : ",\"min\":[");
            for (i = 0; i < components; i++) {
                float v = f[i];
                int k;
                for (k = 1; k < count; k++) {
                    float x = f[k * components + i];
                    v = c ? (x > v ? x : v) : (x < v ? x : v);
                }
                text(&g->accessors, "%s%.9g", i ? "," : "", v);
            }
            text(&g->accessors, "]");
        }
    }
    text(&g->accessors, "}");
    return g->naccessors++;
}

/* ---- one file ---- */

static void transform_json(struct Buf *j, const struct granny_transform *t, const char *name)
{
    const float (*s)[3] = t->ScaleShear;
    if (fabsf(s[0][1]) + fabsf(s[0][2]) + fabsf(s[1][0]) + fabsf(s[1][2]) + fabsf(s[2][0]) + fabsf(s[2][1]) > 1e-4f)
        fprintf(stderr, "  warning: bone %s has shear, which glTF cannot hold; only its scale is kept\n", name);
    text(j, ",\"translation\":[%.9g,%.9g,%.9g]", t->Position[0], t->Position[1], t->Position[2]);
    text(j, ",\"rotation\":[%.9g,%.9g,%.9g,%.9g]", t->Orientation[0], t->Orientation[1], t->Orientation[2],
         t->Orientation[3]);
    text(j, ",\"scale\":[%.9g,%.9g,%.9g]", s[0][0], s[1][1], s[2][2]);
}

static void skeleton_nodes(struct Buf *j, const struct granny_skeleton *skeleton)
{
    int b, c, first;
    text(j, "\"nodes\":[");
    for (b = 0; b < skeleton->BoneCount; b++) {
        const struct granny_bone *bone = &skeleton->Bones[b];
        text(j, "%s{\"name\":", b ? "," : "");
        json_string(j, bone->Name);
        transform_json(j, &bone->LocalTransform, bone->Name);
        for (c = 0, first = 1; c < skeleton->BoneCount; c++)
            if (skeleton->Bones[c].ParentIndex == b) {
                text(j, first ? ",\"children\":[%d" : ",%d", c);
                first = 0;
            }
        if (!first)
            text(j, "]");
        text(j, "}");
    }
}

static int texture_index(const struct granny_file_info *info, const struct granny_texture *texture)
{
    int i;
    for (i = 0; i < info->TextureCount; i++)
        if (info->Textures[i] == texture)
            return i;
    return -1;
}

static int material_index(const struct granny_file_info *info, const struct granny_material *material)
{
    int i;
    for (i = 0; i < info->MaterialCount; i++)
        if (info->Materials[i] == material)
            return i;
    return -1;
}

static void png_out(void *context, void *data, int size)
{
    buf_add(context, data, (size_t)size);
}

static void export_images(struct Gltf *g, struct Buf *j, const struct granny_file_info *info)
{
    int i;
    if (!info->TextureCount)
        return;
    text(j, ",\"images\":[");
    for (i = 0; i < info->TextureCount; i++) {
        struct granny_texture *t = info->Textures[i];
        int alpha = _GrannyTextureHasAlpha(t) != 0, w = t->Width, h = t->Height, k;
        unsigned char *rgba = malloc((size_t)w * h * 4);
        struct Buf png = {0};
        if (!rgba)
            exit(1);
        _GrannyCopyTextureImage(t, 0, 0, GrannyRGBA8888PixelFormat, w, h, w * 4, rgba);
        if (!alpha)
            for (k = 0; k < w * h; k++) {
                rgba[k * 3] = rgba[k * 4];
                rgba[k * 3 + 1] = rgba[k * 4 + 1];
                rgba[k * 3 + 2] = rgba[k * 4 + 2];
            }
        if (!stbi_write_png_to_func(png_out, &png, w, h, alpha ? 4 : 3, rgba, w * (alpha ? 4 : 3))) {
            fprintf(stderr, "  cannot encode texture %s\n", t->FromFileName);
            exit(1);
        }
        text(j, "%s{\"name\":", i ? "," : "");
        json_string(j, t->FromFileName);
        text(j, ",\"mimeType\":\"image/png\",\"bufferView\":%d,\"extras\":{\"alpha\":%s,\"textureType\":%d,"
             "\"imageCount\":%d}}", add_view(g, png.data, png.size, 0), alpha ? "true" : "false", t->TextureType,
             t->ImageCount);
        free(png.data);
        free(rgba);
    }
    text(j, "],\"textures\":[");
    for (i = 0; i < info->TextureCount; i++)
        text(j, "%s{\"source\":%d}", i ? "," : "", i);
    text(j, "]");
}

static void export_materials(struct Buf *j, const struct granny_file_info *info)
{
    int i;
    if (!info->MaterialCount)
        return;
    text(j, ",\"materials\":[");
    for (i = 0; i < info->MaterialCount; i++) {
        struct granny_material *m = info->Materials[i];
        int t = texture_index(info, _GrannyGetMaterialTextureByType(m, GRANNY_DIFFUSE_COLOR_TEXTURE));
        text(j, "%s{\"name\":", i ? "," : "");
        json_string(j, m && m->Name ? m->Name : "");
        text(j, ",\"pbrMetallicRoughness\":{\"metallicFactor\":0");
        if (t >= 0)
            text(j, ",\"baseColorTexture\":{\"index\":%d}", t);
        text(j, "}}");
    }
    text(j, "]");
}

static void export_mesh(struct Gltf *g, struct Buf *j, const struct granny_file_info *info,
                        struct granny_skeleton *skeleton, struct granny_mesh *mesh, int index)
{
    int vc = _GrannyGetMeshVertexCount(mesh), ic = _GrannyGetMeshIndexCount(mesh);
    int groups = _GrannyGetMeshTriangleGroupCount(mesh), rigid = _GrannyMeshIsRigid(mesh) != 0;
    struct granny_tri_group *group = _GrannyGetMeshTriangleGroups(mesh);
    struct Pwnt3432 *v = calloc((size_t)vc + 1, sizeof *v);
    float *pos = malloc(sizeof(float) * 3 * (vc + 1)), *nrm = malloc(sizeof(float) * 3 * (vc + 1));
    float *uv = malloc(sizeof(float) * 2 * (vc + 1));
    unsigned short *joints = malloc(sizeof(unsigned short) * 4 * (vc + 1));
    unsigned char *weights = malloc(4 * (size_t)(vc + 1));
    unsigned short *idx = malloc(sizeof(unsigned short) * (ic + 1));
    int *bones = calloc((size_t)mesh->BoneBindingCount + 1, sizeof *bones);
    int a_pos, a_nrm, a_uv, a_joints, a_weights, k, b;
    if (!v || !pos || !nrm || !uv || !joints || !weights || !idx || !bones)
        exit(1);
    for (b = 0; b < mesh->BoneBindingCount; b++)
        if (!(_GrannyFindBoneByName(skeleton, mesh->BoneBindings[b].BoneName, &bones[b]) & 0xff)) {
            fprintf(stderr, "  warning: mesh %s binds bone %s, which the skeleton lacks\n", mesh->Name,
                    mesh->BoneBindings[b].BoneName);
            bones[b] = 0;
        }
    _GrannyCopyMeshVertices(mesh, GrannyPWNT3432VertexType, v);
    _GrannyCopyMeshIndices(mesh, 2, idx);
    for (k = 0; k < vc; k++) {
        memcpy(pos + k * 3, v[k].p, sizeof v[k].p);
        memcpy(nrm + k * 3, v[k].n, sizeof v[k].n);
        memcpy(uv + k * 2, v[k].t, sizeof v[k].t);
        for (b = 0; b < 4; b++) {
            int local = rigid ? 0 : v[k].i[b];
            joints[k * 4 + b] = (unsigned short)(local < mesh->BoneBindingCount ? bones[local] : 0);
            weights[k * 4 + b] = rigid ? (b ? 0 : 255) : v[k].w[b];
        }
        if (!rigid && weights[k * 4] + weights[k * 4 + 1] + weights[k * 4 + 2] + weights[k * 4 + 3] == 0)
            weights[k * 4] = 255;
    }
    a_pos = add_accessor(g, pos, vc, CT_FLOAT, 3, 0, 34962, 1);
    a_nrm = add_accessor(g, nrm, vc, CT_FLOAT, 3, 0, 34962, 0);
    a_uv = add_accessor(g, uv, vc, CT_FLOAT, 2, 0, 34962, 0);
    a_joints = add_accessor(g, joints, vc, CT_USHORT, 4, 0, 34962, 0);
    a_weights = add_accessor(g, weights, vc, CT_UBYTE, 4, 1, 34962, 0);
    text(j, "%s{\"name\":", index ? "," : "");
    json_string(j, mesh->Name);
    text(j, ",\"extras\":{\"rigid\":%s,\"materials\":[", rigid ? "true" : "false");
    for (k = 0; k < mesh->MaterialBindingCount; k++)
        text(j, "%s%d", k ? "," : "", material_index(info, mesh->MaterialBindings[k]));
    text(j, "]},\"primitives\":[");
    if (groups <= 0 || !group) {
        static struct granny_tri_group whole;
        whole.MaterialIndex = 0;
        whole.TriFirst = 0;
        whole.TriCount = ic / 3;
        group = &whole;
        groups = 1;
    }
    for (k = 0; k < groups; k++) {
        int first = group[k].TriFirst * 3, count = group[k].TriCount * 3, material = -1;
        int a_idx = add_accessor(g, idx + first, count, CT_USHORT, 1, 0, 34963, 0);
        if (group[k].MaterialIndex >= 0 && group[k].MaterialIndex < mesh->MaterialBindingCount)
            material = material_index(info, mesh->MaterialBindings[group[k].MaterialIndex]);
        text(j, "%s{\"attributes\":{\"POSITION\":%d,\"NORMAL\":%d,\"TEXCOORD_0\":%d,\"JOINTS_0\":%d,"
             "\"WEIGHTS_0\":%d},\"indices\":%d", k ? "," : "", a_pos, a_nrm, a_uv, a_joints, a_weights, a_idx);
        if (material >= 0)
            text(j, ",\"material\":%d", material);
        text(j, ",\"extras\":{\"materialBinding\":%d}}", group[k].MaterialIndex);
    }
    text(j, "]}");
    free(v);
    free(pos);
    free(nrm);
    free(uv);
    free(joints);
    free(weights);
    free(idx);
    free(bones);
}

static void bake_animation(struct Gltf *g, struct Buf *j, struct granny_animation *anim, struct granny_model *model,
                           int index)
{
    struct granny_skeleton *skeleton = model->Skeleton;
    int bones = skeleton->BoneCount, frames = (int)ceilf(anim->Duration * SAMPLE_RATE) + 1, f, b, c, a_time;
    void *instance = _GrannyInstantiateModel(model), *pose = _GrannyNewLocalPose(bones), *control;
    float *times, *t, *r, *s;
    if (frames < 2)
        frames = 2;
    times = malloc(sizeof(float) * frames);
    t = malloc(sizeof(float) * 3 * frames * bones);
    r = malloc(sizeof(float) * 4 * frames * bones);
    s = malloc(sizeof(float) * 3 * frames * bones);
    if (!instance || !pose || !times || !t || !r || !s)
        exit(1);
    control = _GrannyPlayControlledAnimation(0.0f, anim, instance);
    _GrannySetControlLoopCount(control, 1);
    for (f = 0; f < frames; f++) {
        times[f] = f == frames - 1 ? anim->Duration : (float)f / SAMPLE_RATE;
        _GrannySetModelClock(instance, times[f]);
        _GrannySampleModelAnimations(instance, 0, bones, pose);
        for (b = 0; b < bones; b++) {
            const struct granny_transform *x = _GrannyGetLocalPoseTransform(pose, b);
            float *q = r + (b * frames + f) * 4;
            memcpy(t + (b * frames + f) * 3, x->Position, sizeof x->Position);
            memcpy(q, x->Orientation, sizeof x->Orientation);
            if (f && q[0] * q[-4] + q[1] * q[-3] + q[2] * q[-2] + q[3] * q[-1] < 0)
                for (c = 0; c < 4; c++)
                    q[c] = -q[c];
            s[(b * frames + f) * 3] = x->ScaleShear[0][0];
            s[(b * frames + f) * 3 + 1] = x->ScaleShear[1][1];
            s[(b * frames + f) * 3 + 2] = x->ScaleShear[2][2];
        }
    }
    _GrannyFreeControl(control);
    _GrannyFreeLocalPose(pose);
    _GrannyFreeModelInstance(instance);
    a_time = add_accessor(g, times, frames, CT_FLOAT, 1, 0, 0, 1);
    text(j, "%s{\"name\":", index ? "," : "");
    json_string(j, anim->Name);
    text(j, ",\"extras\":{\"duration\":%.9g,\"timeStep\":%.9g},\"samplers\":[", anim->Duration, anim->TimeStep);
    for (b = 0; b < bones; b++) {
        text(j, "%s{\"input\":%d,\"output\":%d,\"interpolation\":\"LINEAR\"}", b ? "," : "", a_time,
             add_accessor(g, t + b * frames * 3, frames, CT_FLOAT, 3, 0, 0, 0));
        text(j, ",{\"input\":%d,\"output\":%d,\"interpolation\":\"LINEAR\"}", a_time,
             add_accessor(g, r + b * frames * 4, frames, CT_FLOAT, 4, 0, 0, 0));
        text(j, ",{\"input\":%d,\"output\":%d,\"interpolation\":\"LINEAR\"}", a_time,
             add_accessor(g, s + b * frames * 3, frames, CT_FLOAT, 3, 0, 0, 0));
    }
    text(j, "],\"channels\":[");
    for (b = 0; b < bones; b++)
        text(j, "%s{\"sampler\":%d,\"target\":{\"node\":%d,\"path\":\"translation\"}},"
             "{\"sampler\":%d,\"target\":{\"node\":%d,\"path\":\"rotation\"}},"
             "{\"sampler\":%d,\"target\":{\"node\":%d,\"path\":\"scale\"}}",
             b ? "," : "", b * 3, b, b * 3 + 1, b, b * 3 + 2, b);
    text(j, "]}");
    free(times);
    free(t);
    free(r);
    free(s);
}

/* The glTF of one .gr2: a model file gets its skeleton, meshes, materials, textures and its own
   animations; an animation file (target set) gets the skeleton of the model it animates and the
   animation, sampled on that model as the client plays it there. */
static int export_file(const struct granny_file_info *info, struct granny_model *target, const char *source,
                       struct Buf *glb)
{
    struct granny_model *model = !target && info->ModelCount > 0 ? info->Models[0] : NULL;
    struct granny_model *animated = model ? model : target;
    struct granny_skeleton *skeleton = animated ? animated->Skeleton : NULL;
    struct Gltf g = {{0}};
    struct Buf j = {0};
    int b, m, a, first;
    unsigned header[3], chunk[2];
    static const char spaces[4] = "   ";
    static const unsigned char zeros[4];
    if (!skeleton) {
        fprintf(stderr, "  no model to take the skeleton from\n");
        return 0;
    }
    text(&j, "{\"asset\":{\"version\":\"2.0\",\"generator\":\"granny_export (Granny %s)\",\"extras\":{\"source\":",
         _GrannyGetVersionString());
    json_string(&j, source);
    text(&j, ",\"model\":");
    json_string(&j, animated->Name);
    text(&j, "}},\"scene\":0,\"scenes\":[{\"nodes\":[");
    for (b = 0, first = 1; b < skeleton->BoneCount; b++)
        if (skeleton->Bones[b].ParentIndex < 0) {
            text(&j, first ? "%d" : ",%d", b);
            first = 0;
        }
    if (model)
        for (m = 0; m < model->MeshBindingCount; m++)
            text(&j, first && !m ? "%d" : ",%d", skeleton->BoneCount + m);
    text(&j, "]}],");
    skeleton_nodes(&j, skeleton);
    if (model)
        for (m = 0; m < model->MeshBindingCount; m++) {
            text(&j, ",{\"name\":");
            json_string(&j, model->MeshBindings[m]->Name);
            text(&j, ",\"mesh\":%d,\"skin\":0}", m);
        }
    text(&j, "]");
    if (model) {
        float *ibm = malloc(sizeof(float) * 16 * skeleton->BoneCount);
        if (!ibm)
            exit(1);
        for (b = 0; b < skeleton->BoneCount; b++)
            memcpy(ibm + b * 16, skeleton->Bones[b].InverseWorld4x4, sizeof(float) * 16);
        text(&j, ",\"skins\":[{\"name\":");
        json_string(&j, skeleton->Name);
        text(&j, ",\"inverseBindMatrices\":%d,\"joints\":[", add_accessor(&g, ibm, skeleton->BoneCount, CT_FLOAT, 16, 0, 0, 0));
        for (b = 0; b < skeleton->BoneCount; b++)
            text(&j, b ? ",%d" : "%d", b);
        text(&j, "]}],\"meshes\":[");
        free(ibm);
        for (m = 0; m < model->MeshBindingCount; m++)
            export_mesh(&g, &j, info, skeleton, model->MeshBindings[m], m);
        text(&j, "]");
        export_images(&g, &j, info);
        export_materials(&j, info);
    }
    if (info->AnimationCount > 0) {
        text(&j, ",\"animations\":[");
        for (a = 0; a < info->AnimationCount; a++)
            bake_animation(&g, &j, info->Animations[a], animated, a);
        text(&j, "]");
    }
    text(&j, ",\"bufferViews\":[");
    buf_add(&j, g.views.data, g.views.size);
    text(&j, "],\"accessors\":[");
    buf_add(&j, g.accessors.data, g.accessors.size);
    text(&j, "],\"buffers\":[{\"byteLength\":%u}]}", (unsigned)g.bin.size);
    buf_add(&j, spaces, (4 - j.size % 4) % 4);
    buf_add(&g.bin, zeros, (4 - g.bin.size % 4) % 4);
    header[0] = 0x46546c67u;
    header[1] = 2;
    header[2] = (unsigned)(12 + 8 + j.size + 8 + g.bin.size);
    buf_add(glb, header, sizeof header);
    chunk[0] = (unsigned)j.size;
    chunk[1] = 0x4e4f534au;
    buf_add(glb, chunk, sizeof chunk);
    buf_add(glb, j.data, j.size);
    chunk[0] = (unsigned)g.bin.size;
    chunk[1] = 0x004e4942u;
    buf_add(glb, chunk, sizeof chunk);
    buf_add(glb, g.bin.data, g.bin.size);
    free(j.data);
    free(g.bin.data);
    free(g.views.data);
    free(g.accessors.data);
    return 1;
}

/* ---- finding the .gr2 files: the data folder, then the GRF archives in DATA.INI order ---- */

struct Gr2 {
    char name[256];
    unsigned char *data;
    int size;
    void *file;
    struct granny_file_info *info;
};

static struct Gr2 g_files[256];
static int g_count;

static int lower(int c)
{
    return c >= 'A' && c <= 'Z' ? c - 'A' + 'a' : c;
}

static int same_name(const char *a, const char *b)
{
    for (; *a && *b; a++, b++)
        if (lower((unsigned char)*a) != lower((unsigned char)*b) && !((*a == '/' || *a == '\\') && (*b == '/' || *b == '\\')))
            return 0;
    return !*a && !*b;
}

static int ends_with_gr2(const char *name)
{
    size_t n = strlen(name);
    return n > 4 && same_name(name + n - 4, ".gr2");
}

static void add_file(const char *name, unsigned char *data, int size)
{
    int i;
    for (i = 0; i < g_count; i++)
        if (same_name(g_files[i].name, name)) {
            free(data);
            return;
        }
    if (g_count == (int)(sizeof g_files / sizeof g_files[0]) || strlen(name) >= sizeof g_files[0].name) {
        free(data);
        return;
    }
    strcpy(g_files[g_count].name, name);
    g_files[g_count].data = data;
    g_files[g_count].size = size;
    g_count++;
}

static unsigned char *read_whole(const char *path, int *size)
{
    FILE *f = fopen(path, "rb");
    unsigned char *data;
    long n;
    if (!f)
        return NULL;
    fseek(f, 0, SEEK_END);
    n = ftell(f);
    fseek(f, 0, SEEK_SET);
    data = malloc(n > 0 ? (size_t)n : 1);
    if (!data || fread(data, 1, (size_t)n, f) != (size_t)n) {
        free(data);
        fclose(f);
        return NULL;
    }
    fclose(f);
    *size = (int)n;
    return data;
}

static void scan_folder(const char *root, const char *relative)
{
    char pattern[MAX_PATH], path[MAX_PATH], rel[MAX_PATH];
    WIN32_FIND_DATAA found;
    HANDLE h;
    snprintf(pattern, sizeof pattern, "%s\\%s\\*", root, relative);
    h = FindFirstFileA(pattern, &found);
    if (h == INVALID_HANDLE_VALUE)
        return;
    do {
        if (!strcmp(found.cFileName, ".") || !strcmp(found.cFileName, ".."))
            continue;
        snprintf(rel, sizeof rel, "%s\\%s", relative, found.cFileName);
        if (found.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
            scan_folder(root, rel);
        } else if (ends_with_gr2(found.cFileName)) {
            int size;
            unsigned char *data;
            snprintf(path, sizeof path, "%s\\%s", root, rel);
            data = read_whole(path, &size);
            if (data)
                add_file(rel, data, size);
        }
    } while (FindNextFileA(h, &found));
    FindClose(h);
}

static unsigned get32(const unsigned char *p)
{
    return p[0] | p[1] << 8 | p[2] << 16 | (unsigned)p[3] << 24;
}

static void scan_grf(const char *path)
{
    FILE *f = fopen(path, "rb");
    unsigned char header[46], sizes[8], *packed, *table;
    unsigned offset, version, csize, usize, p;
    if (!f)
        return;
    if (fread(header, 1, sizeof header, f) != sizeof header || memcmp(header, "Master of Magic", 15)) {
        fclose(f);
        return;
    }
    offset = get32(header + 30);
    version = get32(header + 42);
    if (version != 0x200 || _fseeki64(f, (__int64)offset + 46, SEEK_SET) || fread(sizes, 1, 8, f) != 8) {
        fprintf(stderr, "%s: not a GRF 0x200 archive\n", path);
        fclose(f);
        return;
    }
    csize = get32(sizes);
    usize = get32(sizes + 4);
    packed = malloc(csize);
    table = malloc(usize);
    if (!packed || !table || fread(packed, 1, csize, f) != csize) {
        fclose(f);
        free(packed);
        free(table);
        return;
    }
    {
        uLongf out = usize;
        if (uncompress(table, &out, packed, csize) != Z_OK)
            usize = 0;
    }
    free(packed);
    for (p = 0; p < usize;) {
        const char *name = (const char *)table + p;
        size_t n = strnlen(name, usize - p);
        const unsigned char *e = table + p + n + 1;
        unsigned ecsize, easize, ersize, eoffset;
        unsigned char flags;
        if (p + n + 1 + 17 > usize)
            break;
        ecsize = get32(e);
        easize = get32(e + 4);
        ersize = get32(e + 8);
        flags = e[12];
        eoffset = get32(e + 13);
        p += (unsigned)n + 1 + 17;
        if (!(flags & 1) || !ends_with_gr2(name))
            continue;
        if (flags & 6) {
            fprintf(stderr, "%s: %s is encrypted, skipped\n", path, name);
            continue;
        }
        {
            unsigned char *in = malloc(easize), *out = malloc(ersize ? ersize : 1);
            uLongf got = ersize;
            if (!in || !out || _fseeki64(f, (__int64)eoffset + 46, SEEK_SET) || fread(in, 1, easize, f) != easize ||
                uncompress(out, &got, in, ecsize) != Z_OK || got != ersize) {
                fprintf(stderr, "%s: cannot read %s\n", path, name);
                free(in);
                free(out);
                continue;
            }
            free(in);
            add_file(name, out, (int)ersize);
        }
    }
    free(table);
    fclose(f);
}

static void scan_data_ini(const char *root)
{
    char path[MAX_PATH], line[512], grfs[16][256];
    int order[16], count = 0, i, k;
    FILE *f;
    snprintf(path, sizeof path, "%s\\DATA.INI", root);
    f = fopen(path, "r");
    if (!f) {
        fprintf(stderr, "no DATA.INI in %s\n", root);
        return;
    }
    while (fgets(line, sizeof line, f) && count < 16) {
        char *eq = strchr(line, '='), *end;
        if (!eq || line[0] < '0' || line[0] > '9')
            continue;
        order[count] = atoi(line);
        for (end = eq + 1; *end && *end != '\r' && *end != '\n'; end++)
            ;
        *end = 0;
        snprintf(grfs[count], sizeof grfs[count], "%s", eq + 1);
        count++;
    }
    fclose(f);
    for (i = 0; i < count; i++)
        for (k = i + 1; k < count; k++)
            if (order[k] < order[i]) {
                char name[256];
                int o = order[i];
                order[i] = order[k];
                order[k] = o;
                strcpy(name, grfs[i]);
                strcpy(grfs[i], grfs[k]);
                strcpy(grfs[k], name);
            }
    for (i = 0; i < count; i++) {
        snprintf(path, sizeof path, "%s\\%s", root, grfs[i]);
        scan_grf(path);
    }
}

static const char *leaf(const char *name)
{
    const char *a = strrchr(name, '\\'), *b = strrchr(name, '/');
    if (b > a)
        a = b;
    return a ? a + 1 : name;
}

static int model_id(const char *name)
{
    const char *underscore = strrchr(leaf(name), '_');
    return underscore ? atoi(underscore + 1) : -1;
}

/* The client plays model\3dmob_bone\<id>_<part>.gr2 on the model whose name ends in _<id>. */
static int is_animation_file(const char *name)
{
    const char *s = leaf(name);
    if (*s < '0' || *s > '9')
        return 0;
    while (*s >= '0' && *s <= '9')
        s++;
    return *s == '_';
}

static void make_dirs(const char *root)
{
    char path[MAX_PATH];
    snprintf(path, sizeof path, "%s\\data", root);
    CreateDirectoryA(path, NULL);
    snprintf(path, sizeof path, "%s\\data\\model", root);
    CreateDirectoryA(path, NULL);
    snprintf(path, sizeof path, "%s\\data\\model\\gltf", root);
    CreateDirectoryA(path, NULL);
}

int main(int argc, char **argv)
{
    const char *root = argc > 1 ? argv[1] : ".";
    int i, k, pass, written = 0, failed = 0;
    printf("granny_export with Granny %s\n", _GrannyGetVersionString());
    scan_folder(root, "data");
    scan_data_ini(root);
    if (!g_count) {
        fprintf(stderr, "no .gr2 files found in %s\n", root);
        return 1;
    }
    for (i = 0; i < g_count; i++) {
        g_files[i].file = _GrannyReadEntireFileFromMemory(g_files[i].size, g_files[i].data);
        g_files[i].info = g_files[i].file ? _GrannyGetFileInfo(g_files[i].file) : NULL;
        if (!g_files[i].info)
            fprintf(stderr, "%s: Granny cannot read it\n", g_files[i].name);
    }
    make_dirs(root);
    /* Models first, then the animations, which are sampled on their model's skeleton. */
    for (pass = 0; pass < 2; pass++)
        for (i = 0; i < g_count; i++) {
            struct Gr2 *file = &g_files[i];
            struct granny_model *target = NULL;
            struct Buf glb = {0};
            char out[MAX_PATH];
            unsigned crc;
            FILE *f;
            if (!file->info || is_animation_file(file->name) != (pass == 1))
                continue;
            if (pass == 1) {
                int id = atoi(leaf(file->name));
                for (k = 0; k < g_count; k++)
                    if (g_files[k].info && g_files[k].info->ModelCount > 0 && !is_animation_file(g_files[k].name) &&
                        model_id(g_files[k].name) == id)
                        target = g_files[k].info->Models[0];
                if (!target) {
                    fprintf(stderr, "%s: no model ends in _%d\n", file->name, id);
                    failed++;
                    continue;
                }
            }
            printf("%s", file->name);
            if (!export_file(file->info, target, file->name, &glb)) {
                failed++;
                continue;
            }
            crc = (unsigned)crc32(0, file->data, (uInt)file->size);
            snprintf(out, sizeof out, "%s\\data\\model\\gltf\\%08x.glb", root, crc);
            f = fopen(out, "wb");
            if (!f || fwrite(glb.data, 1, glb.size, f) != glb.size || fclose(f)) {
                fprintf(stderr, "  cannot write %s\n", out);
                failed++;
            } else {
                printf(" -> data\\model\\gltf\\%08x.glb (%u bytes)\n", crc, (unsigned)glb.size);
                written++;
            }
            free(glb.data);
        }
    printf("%d written, %d failed\n", written, failed);
    return failed ? 1 : 0;
}
