#include "render/screen_bounds.h"
#include <stdint.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "render/texture_cache.h"
#include "core/numeric.h"
#include "fileio/file.h"
#include "render/cdc.h"
#include "render/renderer.h"
#include "render/math3d.h"
#include "resources/granny_res.h"
#include "platform/error_message.h"
#include "std/string.h"
#include "resources/resource_ops.h"
#include "fileio/resource_api.h"
#include "fileio/res_mgr.h"
#include "resources/resource_factories.h"

struct C3dGrannyBoneRes g_grannyBoneRes;
const char *(*g_grannyJobModelName)(int job);

static const int g_grannyBoneJobIds[] = {1288, 722, 1324, 1000, 1000, 1000, 1000, 1286, 1285, 1287};
static const char *const g_grannyBonePartNames[] = {"stand", "move", "attack", "dead", "damage"};

struct Res *granny_model_resource_create(void)
{
    struct C3dGrannyModelRes *model = calloc(1, sizeof *model);
    if (!model)
        return NULL;
    res_construct(&model->res);
    model->res.ops = &granny_model_resource_ops;
    return &model->res;
}

void granny_model_resource_clear(struct C3dGrannyModelRes *self)
{
    int i;
    if (self->model) {
        if (self->model->meshes) {
            for (i = 0; i < self->model->meshCount; i++) {
                struct GrannyResMesh *mesh = &self->model->meshes[i];
                if (mesh->deformer) _GrannyFreeMeshDeformer(mesh->deformer);
                free(mesh->textures);
                free(mesh->indices);
                free(mesh->rigidVerts);
            }
        }
        free(self->model->meshes);
        free(self->model);
    }
    if (self->textures)
        for (i = 0; i < self->textureCount; i++)
            texture_destroy(self->textures[i].texture);
    free(self->textures);
    if (self->file) _GrannyFreeFile(self->file);
    self->model = NULL;
    self->file = NULL;
    self->textures = NULL;
    self->textureCount = 0;
    self->grannyModel = NULL;
    self->skeleton = NULL;
    self->boneRes = NULL;
    self->id = 0;
    self->name[0] = 0;
    memset(&self->bounds, 0, sizeof self->bounds);
}

void granny_model_resource_finish(struct C3dGrannyModelRes *self)
{
    granny_model_resource_clear(self);
    res_destruct(&self->res);
}

static int c3d_granny_model_res_create_texture(struct GrannyResTexture *out, granny_texture *texture)
{
    BYTE *pixels;
    size_t count, i;
    int w, h, alpha;
    if (!texture || !texture->FromFileName)
        return 0;
    if (texture->TextureType != GRANNY_COLOR_MAP_TEXTURE_TYPE || texture->ImageCount != 1) {
        error_msg("This sample only works with single-image : ColorMapTextureType textures", 0);
        return 0;
    }
    w = texture->Width; h = texture->Height;
    if (w <= 0 || h <= 0 || w > INT_MAX / 4 || (size_t)w > SIZE_MAX / 4 / (size_t)h)
        return 0;
    count = (size_t)w * (size_t)h;
    pixels = malloc(count * 4);
    if (!pixels)
        return 0;
    alpha = !!_GrannyTextureHasAlpha(texture);
    _GrannyCopyTextureImage(texture, 0, 0, GrannyRGBA8888PixelFormat, w, h, w * 4, pixels);
    for (i = 0; i < count; i++) {
        BYTE red = pixels[4 * i];
        pixels[4 * i] = pixels[4 * i + 2];
        pixels[4 * i + 2] = red;
        if (!alpha) pixels[4 * i + 3] = 0xff;
    }
    out->texture = texture_from_pixels(w, h, (const DWORD *)pixels, alpha, 0, 0);
    free(pixels);
    out->name = texture->FromFileName;
    return out->texture != NULL;
}

static int c3d_granny_model_res_create_model(struct C3dGrannyModelRes *self, granny_model *source)
{
    struct GrannyResModel *model;
    int i, j, k;
    if (source->MeshBindingCount < 0 || (source->MeshBindingCount && !source->MeshBindings) ||
        (size_t)source->MeshBindingCount > SIZE_MAX / sizeof(struct GrannyResMesh))
        return 0;
    model = calloc(1, sizeof *model);
    if (!model)
        return 0;
    self->model = model;
    model->meshCount = source->MeshBindingCount;
    if (!model->meshCount)
        return 1;
    model->meshes = calloc((size_t)model->meshCount, sizeof *model->meshes);
    if (!model->meshes)
        return 0;
    for (i = 0; i < model->meshCount; i++) {
        struct GrannyResMesh *out = &model->meshes[i];
        granny_mesh *mesh = source->MeshBindings[i].Mesh;
        int indices, vertices;
        if (!mesh)
            return 0;
        out->mesh = mesh;
        indices = _GrannyGetMeshIndexCount(mesh);
        vertices = _GrannyGetMeshVertexCount(mesh);
        if (indices < 0 || indices % 3 || vertices < 0 || (indices && !vertices) ||
            (size_t)indices > SIZE_MAX / sizeof *out->indices ||
            (size_t)vertices > SIZE_MAX / sizeof *out->rigidVerts ||
            mesh->MaterialBindingCount < 0 || (mesh->MaterialBindingCount && !mesh->MaterialBindings) ||
            (size_t)mesh->MaterialBindingCount > SIZE_MAX / sizeof *out->textures)
            return 0;
        if (indices) {
            out->indices = malloc((size_t)indices * sizeof *out->indices);
            if (!out->indices)
                return 0;
            _GrannyCopyMeshIndices(mesh, 2, out->indices);
            for (j = 0; j < indices; j++)
                if (out->indices[j] >= vertices)
                    return 0;
        }
        if (vertices && _GrannyMeshIsRigid(mesh)) {
            out->rigidVerts = malloc((size_t)vertices * sizeof *out->rigidVerts);
            if (!out->rigidVerts)
                return 0;
            _GrannyCopyMeshVertices(mesh, GrannyPNT332VertexType, out->rigidVerts);
        } else if (vertices) {
            out->deformer = _GrannyNewMeshDeformer(_GrannyGetMeshVertexType(mesh), GrannyPNT332VertexType,
                                                 GRANNY_DEFORM_POSITION_NORMAL);
            if (!out->deformer) {
                error_msg("Granny didn't find a matching deformer for the vertex format used by mesh", 0);
                return 0;
            }
        }
        out->materialCount = mesh->MaterialBindingCount;
        if (out->materialCount) {
            out->textures = calloc((size_t)out->materialCount, sizeof *out->textures);
            if (!out->textures)
                return 0;
        }
        for (j = 0; j < out->materialCount; j++) {
            granny_material *material = mesh->MaterialBindings[j].Material;
            granny_texture *texture = material ? _GrannyGetMaterialTextureByType(material, GRANNY_DIFFUSE_COLOR_TEXTURE) : NULL;
            if (texture && texture->FromFileName)
                for (k = 0; k < self->textureCount; k++)
                    if (!strcmp(self->textures[k].name, texture->FromFileName)) {
                        out->textures[j] = &self->textures[k];
                        break;
                    }
        }
        if (indices / 3 > 10000)
            error_msg("too many vertex granny model!", 0);
    }
    return 1;
}

BYTE c3d_granny_model_res_load(struct C3dGrannyModelRes *self, const char *name)
{
    struct C3dGrannyModelRes next = {0};
    struct File file;
    granny_file_info *info;
    const char *number;
    char id[5];
    int i;
    if (self->res.lockCount || !name || strlen(name) >= sizeof next.name)
        return 0;
    if (!_GrannyVersionsMatch_(2, 1, 0, 5)) {
        error_msg("doesn't match the .h file used during compilation", 0);
        return 0;
    }
    number = strchr(name, '_');
    if (!number)
        return 0;
    number++;
    for (i = 0; i < 4 && number[i] && number[i] != '.'; i++) id[i] = number[i];
    id[i] = 0;
    next.id = atoi(id);
    strcpy(next.name, name);
    file_construct(&file);
    if (!file_open(&file, name, 0, 1, 0)) {
        char message[sizeof next.name + sizeof "Cannot find File : "];
        snprintf(message, sizeof message, "Cannot find File : %s", name);
        error_msg(message, 0);
        file_destruct(&file);
        return 0;
    }
    next.file = _GrannyReadEntireFileFromMemory(file.size, file.buffer);
    file_destruct(&file);
    if (!next.file) {
        error_msg("Unable to load file .gr2 (C3dGrannyModelRes :: Load)", 0);
        return 0;
    }
    info = _GrannyGetFileInfo(next.file);
    if (!info || info->ModelCount <= 0 || !info->Models || !info->Models[0] ||
        !info->Models[0]->Skeleton || info->Models[0]->Skeleton->BoneCount < 0 ||
        info->SkeletonCount <= 0 || !info->Skeletons || !info->Skeletons[0] ||
        info->TextureCount < 0 || (info->TextureCount && !info->Textures) ||
        (size_t)info->TextureCount > SIZE_MAX / sizeof *next.textures) {
        error_msg("Unable to access file data .gr2", 0);
        goto fail;
    }
    next.textureCount = info->TextureCount;
    if (next.textureCount) {
        next.textures = calloc((size_t)next.textureCount, sizeof *next.textures);
        if (!next.textures)
            goto fail;
    }
    for (i = 0; i < next.textureCount; i++)
        if (!c3d_granny_model_res_create_texture(&next.textures[i], info->Textures[i]))
            goto fail;
    if (!c3d_granny_model_res_create_model(&next, info->Models[0]))
        goto fail;
    next.grannyModel = info->Models[0];
    next.skeleton = next.grannyModel->Skeleton;
    next.boneRes = &g_grannyBoneRes;
    next.bounds.half_width = next.bounds.half_height = 10.0f;
    granny_model_resource_clear(self);
    next.res = self->res;
    *self = next;
    return 1;
fail:
    granny_model_resource_clear(&next);
    return 0;
}

void granny_animation_cache_init(struct C3dGrannyBoneRes *self)
{
    memset(self, 0, sizeof *self);
}

void granny_animation_cache_clear(struct C3dGrannyBoneRes *self)
{
    int job, animation;
    for (job = 0; job < GRANNY_BONE_JOB_COUNT; job++)
        for (animation = 0; animation < GRANNY_BONE_ANIMATION_COUNT; animation++) {
            struct GrannyAnimationEntry *entry = &self->entries[job][animation];
            if (entry->file) _GrannyFreeFile(entry->file);
            entry->file = NULL;
            entry->animation = NULL;
        }
}

BYTE granny_animation_cache_load(struct C3dGrannyBoneRes *self, const char *name, int job, int part)
{
    struct File source;
    struct GrannyAnimationEntry *entry;
    granny_file *file;
    granny_file_info *info;
    if (!self || !name || (unsigned)job >= GRANNY_BONE_JOB_COUNT ||
        (unsigned)part >= GRANNY_BONE_ANIMATION_COUNT)
        return 0;
    entry = &self->entries[job][part];
    if (entry->animation)
        return 1;
    if (!_GrannyVersionsMatch_(2, 1, 0, 5)) {
        error_msg("doesn't match the .h file used during compilation", 0);
        return 0;
    }
    file_construct(&source);
    if (!file_open(&source, name, 0, 1, 0)) {
        file_destruct(&source);
        return 0;
    }
    file = _GrannyReadEntireFileFromMemory(source.size, source.buffer);
    file_destruct(&source);
    if (!file)
        return 0;
    info = _GrannyGetFileInfo(file);
    if (!info || info->AnimationCount != 1 || !info->Animations || !info->Animations[0]) {
        if (info && info->AnimationCount)
            error_msg("This sample only supports single-animation files", 0);
        _GrannyFreeFile(file);
        return 0;
    }
    entry->file = file;
    entry->animation = info->Animations[0];
    return 1;
}

static granny_animation *bone_animation(struct C3dGrannyBoneRes *bones, int job, int part)
{
    struct GrannyAnimationEntry *entry;
    char name[264];
    int length;
    if (!bones || (unsigned)job >= GRANNY_BONE_JOB_COUNT || (unsigned)part >= GRANNY_BONE_ANIMATION_COUNT)
        return NULL;
    entry = &bones->entries[job][part];
    if (entry->animation)
        return entry->animation;
    if (part == 0) {
        const char *model = g_grannyJobModelName ? g_grannyJobModelName(g_grannyBoneJobIds[job]) : NULL;
        if (!model)
            return NULL;
        length = snprintf(name, sizeof name, "model\\3dmob\\%s", model);
    } else {
        length = snprintf(name, sizeof name, "model\\3dmob_bone\\%d_%s.gr2", job, g_grannyBonePartNames[part]);
    }
    if (length <= 0 || (size_t)length >= sizeof name || !file_make_file_name(name, 0))
        return NULL;
    granny_animation_cache_load(bones, name, job, part);
    return entry->animation;
}

granny_animation *granny_animation_cache_get(struct C3dGrannyBoneRes *self, int job, int part)
{
    return bone_animation(self, job, part);
}

static void xform(const float *m, float x, float y, float z, float *out)
{
    out[0] = x * m[0] + y * m[3] + z * m[6] + m[9];
    out[1] = x * m[1] + y * m[4] + z * m[7] + m[10];
    out[2] = x * m[2] + y * m[5] + z * m[8] + m[11];
}

BYTE granny_bounds_screen_box(const struct GrannyBounds *bounds, struct ScreenBounds *box,
                                            const float *view, const float *r, const float *eye)
{
    union { unsigned u; float f; } pi = { 0x40490fd8 };
    float m[12], p[3], q[3], d, k, a = bounds->half_width, b = bounds->half_height, s1, s2, inv, dy;
    const struct Renderer *R;
    DWORD w, h;

    mat_set_rotate_z(m, pi.f);
    xform(m, bounds->center[0], bounds->center[1], bounds->center[2], p);
    mat_multiply(m, r, view);
    xform(m, p[0], p[1], p[2], q);
    dy = eye[1] - p[1];
    d = float_sqrt(dy * dy + (eye[0] - p[0]) * (eye[0] - p[0]) + (eye[2] - p[2]) * (eye[2] - p[2]));
    k = (float)fabs((1.0f / d) * dy);
    k = 1.0f < k ? 1.0f : k;
    s2 = b - k * (b - a);
    s1 = float_sqrt(a * a * 0.5f);
    s2 = float_sqrt(s2 * s2 * 0.5f);
    inv = 1.0f / q[2];
    box->z = inv;
    R = g_renderer;
    box->left = (q[0] - s1) * R->proj[0] * inv + (float)R->halfW;
    box->top = (q[1] - s2) * R->proj[1] * inv + (float)R->halfH;
    box->right = (s1 + q[0]) * R->proj[0] * inv + (float)R->halfW;
    box->bottom = (s2 + q[1]) * R->proj[1] * inv + (float)R->halfH;
    w = (DWORD)R->width;
    if (number_truncate_low32(box->left) <= w || number_truncate_low32(box->right) <= w)
        return 1;
    h = (DWORD)R->height;
    if (number_truncate_low32(box->top) <= h || number_truncate_low32(box->bottom) <= h)
        return 1;
    return 0;
}

static void granny_model_resource_destroy(struct Res *resource)
{
    struct C3dGrannyModelRes *self = (struct C3dGrannyModelRes *)resource;
    granny_model_resource_finish(self);
    free(self);
}
static int granny_model_resource_load(struct Res *resource, const char *name)
{
    return c3d_granny_model_res_load((struct C3dGrannyModelRes *)resource, name);
}
const struct ResourceOps granny_model_resource_ops = {
    .destroy = granny_model_resource_destroy,
    .load = granny_model_resource_load,
};
