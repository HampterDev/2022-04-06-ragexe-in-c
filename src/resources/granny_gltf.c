#include "platform/win_types.h"
#ifndef PLATFORM_RAD_DLLS
#include "resources/granny_sdk.h"
#include "fileio/file_api.h"
#include "platform/clock.h"
#include "platform/system.h"
#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <zlib.h>
#include "stb_image.h"
#define CGLTF_IMPLEMENTATION
#include "cgltf.h"

#define BONE_NAME_SIZE 64
#define LOOP_FOREVER_LEFT 1e30f

struct GltfBone {
    char name[BONE_NAME_SIZE];
    int parent;
    float t[3], r[4], s[3];
    float inverse_world[16];
};

struct GltfVertex {
    float p[3], n[3], uv[2], w[4];
    int j[4];
};

struct GltfTexture {
    granny_texture pub;
    char *name;
    unsigned char *rgba;
    int alpha;
};

struct granny_material {
    granny_texture *diffuse;
};

struct GltfMesh {
    granny_mesh pub;
    char *name;
    struct GltfVertex *verts;
    int vertex_count;
    unsigned short *indices;
    int index_count;
    int *groups;
    int group_count;
    int rigid, rigid_bone;
};

enum { PATH_TRANSLATION, PATH_ROTATION, PATH_SCALE };

struct GltfTrack {
    char bone[BONE_NAME_SIZE];
    int path, count, width;
    float *times, *values;
};

struct granny_animation {
    float duration;
    int track_count;
    struct GltfTrack *tracks;
};

struct granny_file {
    granny_file_info info;
    granny_skeleton skeleton;
    granny_skeleton *skeletons[1];
    struct GltfBone *bones;
    granny_model model;
    granny_model *models[1];
    granny_model_mesh_binding *mesh_bindings;
    struct GltfMesh *meshes;
    granny_mesh **mesh_list;
    struct GltfTexture *textures;
    granny_texture **texture_list;
    struct granny_material *materials;
    granny_material **material_list;
    granny_material_binding *material_bindings;
    struct granny_animation *animations;
    granny_animation **animation_list;
};

struct GltfTransform {
    float t[3], r[4], s[3];
};

struct granny_local_pose {
    int bones;
    struct GltfTransform x[];
};

struct granny_world_pose {
    int bones;
    float *world, *composite;
};

struct granny_control {
    granny_model_instance *instance;
    granny_animation *animation;
    float start;
    int loops, active;
    int *track_bone;
    granny_control *next;
};

struct granny_model_instance {
    granny_model *model;
    float clock;
    granny_control *controls;
};

struct granny_mesh_binding {
    int count;
    int bones[];
};

struct granny_mesh_deformer {
    int deformation;
};

static const int g_vertexTypeTag, g_pixelLayoutTag, g_sourceVertexTag;
const granny_pixel_layout *GrannyRGBA8888PixelFormat = (const granny_pixel_layout *)&g_pixelLayoutTag;
const granny_data_type_definition *GrannyPNT332VertexType = (const granny_data_type_definition *)&g_vertexTypeTag;
static struct granny_mesh_deformer g_deformer;

/* ---- the clock ---- */

GrannyClock _GrannyGetSystemSeconds(void)
{
    GrannyClock clock = {{0}};
    int64_t now = platform_perf_counter();
    memcpy(clock.bytes, &now, sizeof now);
    return clock;
}

float _GrannyGetSecondsElapsed(const GrannyClock *start, const GrannyClock *end)
{
    int64_t a, b, frequency = platform_perf_frequency();
    memcpy(&a, start->bytes, sizeof a);
    memcpy(&b, end->bytes, sizeof b);
    return frequency > 0 ? (float)((double)(b - a) / (double)frequency) : 0.0f;
}

BYTE _GrannyVersionsMatch_(int major, int minor, int customization, int build)
{
    (void)customization;
    (void)build;
    return major == 2 && minor == 1;
}

/* ---- loading a converted model ---- */

/* Names are stored as JSON escapes of their CP949 bytes; cgltf decodes them to UTF-8. */
static char *client_name(const char *utf8)
{
    size_t n = utf8 ? strlen(utf8) : 0, i, o = 0;
    char *out = malloc(n + 1);
    if (!out)
        return NULL;
    for (i = 0; i < n; i++) {
        unsigned char c = (unsigned char)utf8[i];
        if (c >= 0xc2 && c <= 0xc3 && i + 1 < n) {
            out[o++] = (char)(((c & 0x1f) << 6) | ((unsigned char)utf8[++i] & 0x3f));
        } else {
            out[o++] = (char)c;
        }
    }
    out[o] = 0;
    return out;
}

static void copy_bone_name(char *out, const char *utf8)
{
    char *name = client_name(utf8);
    snprintf(out, BONE_NAME_SIZE, "%s", name ? name : "");
    free(name);
}

/* The converter's own facts ride in "extras". cgltf keeps a copy for most objects; for meshes
   only the offsets into the JSON chunk. */
static const char *extras_text(const cgltf_data *g, const cgltf_extras *extras, char *buf, size_t size)
{
    size_t n;
    if (extras->data)
        return extras->data;
    if (!g->json || extras->end_offset <= extras->start_offset)
        return "";
    n = extras->end_offset - extras->start_offset;
    if (n >= size)
        n = size - 1;
    memcpy(buf, g->json + extras->start_offset, n);
    buf[n] = 0;
    return buf;
}

static int extras_flag(const char *extras, const char *key)
{
    char pattern[64];
    snprintf(pattern, sizeof pattern, "\"%s\":true", key);
    return strstr(extras, pattern) != NULL;
}

static int extras_int_list(const char *extras, const char *key, int *out, int max)
{
    char pattern[64];
    const char *p;
    int count = 0;
    snprintf(pattern, sizeof pattern, "\"%s\":[", key);
    if (!(p = strstr(extras, pattern)))
        return 0;
    p += strlen(pattern);
    while (*p && *p != ']' && count < max) {
        out[count++] = (int)strtol(p, (char **)&p, 10);
        while (*p == ',' || *p == ' ')
            p++;
    }
    return count;
}

static float extras_float(const char *extras, const char *key)
{
    char pattern[64];
    const char *p;
    snprintf(pattern, sizeof pattern, "\"%s\":", key);
    return (p = strstr(extras, pattern)) ? strtof(p + strlen(pattern), NULL) : 0.0f;
}

static int load_textures(struct granny_file *f, cgltf_data *g)
{
    size_t i;
    f->info.TextureCount = (int)g->images_count;
    if (!g->images_count)
        return 1;
    f->textures = calloc(g->images_count, sizeof *f->textures);
    f->texture_list = calloc(g->images_count, sizeof *f->texture_list);
    if (!f->textures || !f->texture_list)
        return 0;
    for (i = 0; i < g->images_count; i++) {
        cgltf_image *image = &g->images[i];
        struct GltfTexture *t = &f->textures[i];
        const unsigned char *png;
        char extras[256];
        int w, h, channels;
        if (!image->buffer_view || !image->buffer_view->buffer->data)
            return 0;
        png = (const unsigned char *)image->buffer_view->buffer->data + image->buffer_view->offset;
        t->rgba = stbi_load_from_memory(png, (int)image->buffer_view->size, &w, &h, &channels, 4);
        if (!t->rgba)
            return 0;
        t->name = client_name(image->name);
        t->alpha = extras_flag(extras_text(g, &image->extras, extras, sizeof extras), "alpha");
        t->pub.FromFileName = t->name;
        t->pub.TextureType = (int)extras_float(extras_text(g, &image->extras, extras, sizeof extras), "textureType");
        t->pub.Width = w;
        t->pub.Height = h;
        t->pub.ImageCount = 1;
        f->texture_list[i] = &t->pub;
    }
    f->info.Textures = f->texture_list;
    return 1;
}

static int load_materials(struct granny_file *f, cgltf_data *g)
{
    size_t i;
    f->info.MaterialCount = (int)g->materials_count;
    if (!g->materials_count)
        return 1;
    f->materials = calloc(g->materials_count, sizeof *f->materials);
    f->material_list = calloc(g->materials_count, sizeof *f->material_list);
    if (!f->materials || !f->material_list)
        return 0;
    for (i = 0; i < g->materials_count; i++) {
        cgltf_texture *texture = g->materials[i].pbr_metallic_roughness.base_color_texture.texture;
        if (texture && texture->image)
            f->materials[i].diffuse = f->texture_list[texture->image - g->images];
        f->material_list[i] = &f->materials[i];
    }
    f->info.Materials = f->material_list;
    return 1;
}

static int load_skeleton(struct granny_file *f, cgltf_data *g)
{
    size_t joints = g->skins_count ? g->skins[0].joints_count : 0, i, k;
    cgltf_node **order;
    if (!joints) {
        /* An animation file: its bones are the nodes without a mesh, in file order. */
        for (i = 0; i < g->nodes_count; i++)
            if (!g->nodes[i].mesh)
                joints++;
    }
    order = calloc(joints + 1, sizeof *order);
    f->bones = calloc(joints + 1, sizeof *f->bones);
    if (!order || !f->bones) {
        free(order);
        return 0;
    }
    if (g->skins_count) {
        for (i = 0; i < joints; i++)
            order[i] = g->skins[0].joints[i];
    } else {
        for (i = 0, k = 0; i < g->nodes_count; i++)
            if (!g->nodes[i].mesh)
                order[k++] = &g->nodes[i];
    }
    for (i = 0; i < joints; i++) {
        struct GltfBone *b = &f->bones[i];
        cgltf_node *node = order[i];
        copy_bone_name(b->name, node->name);
        b->parent = -1;
        for (k = 0; k < joints; k++)
            if (order[k] == node->parent)
                b->parent = (int)k;
        memcpy(b->t, node->translation, sizeof b->t);
        memcpy(b->r, node->rotation, sizeof b->r);
        memcpy(b->s, node->scale, sizeof b->s);
        if (!node->has_rotation)
            b->r[3] = 1.0f;
        if (!node->has_scale)
            b->s[0] = b->s[1] = b->s[2] = 1.0f;
        if (g->skins_count && g->skins[0].inverse_bind_matrices)
            cgltf_accessor_read_float(g->skins[0].inverse_bind_matrices, i, b->inverse_world, 16);
        else
            b->inverse_world[0] = b->inverse_world[5] = b->inverse_world[10] = b->inverse_world[15] = 1.0f;
    }
    free(order);
    f->skeleton.Name = "";
    f->skeleton.BoneCount = (int)joints;
    f->skeleton.Bones = f->bones;
    f->skeletons[0] = &f->skeleton;
    f->info.SkeletonCount = 1;
    f->info.Skeletons = f->skeletons;
    return 1;
}

static int load_mesh(struct GltfMesh *out, cgltf_mesh *mesh, cgltf_data *g, struct granny_file *f,
                     granny_material_binding *bindings, int *binding_count)
{
    cgltf_primitive *first = mesh->primitives_count ? &mesh->primitives[0] : NULL;
    const cgltf_accessor *pos = NULL, *nrm = NULL, *uv = NULL, *joints = NULL, *weights = NULL;
    char extras_buf[1024];
    const char *extras = extras_text(g, &mesh->extras, extras_buf, sizeof extras_buf);
    int materials[64], material_count = extras_int_list(extras, "materials", materials, 64);
    size_t i, k, written = 0;
    if (!first)
        return 0;
    for (i = 0; i < first->attributes_count; i++) {
        cgltf_attribute *a = &first->attributes[i];
        if (a->type == cgltf_attribute_type_position) pos = a->data;
        else if (a->type == cgltf_attribute_type_normal) nrm = a->data;
        else if (a->type == cgltf_attribute_type_texcoord && a->index == 0) uv = a->data;
        else if (a->type == cgltf_attribute_type_joints && a->index == 0) joints = a->data;
        else if (a->type == cgltf_attribute_type_weights && a->index == 0) weights = a->data;
    }
    if (!pos)
        return 0;
    out->name = client_name(mesh->name);
    out->vertex_count = (int)pos->count;
    out->verts = calloc(pos->count + 1, sizeof *out->verts);
    out->rigid = extras_flag(extras, "rigid");
    for (i = 0; i < mesh->primitives_count; i++)
        out->index_count += mesh->primitives[i].indices ? (int)mesh->primitives[i].indices->count : 0;
    out->indices = calloc((size_t)out->index_count + 1, sizeof *out->indices);
    out->group_count = (int)mesh->primitives_count;
    out->groups = calloc((size_t)out->group_count * 3 + 3, sizeof *out->groups);
    if (!out->verts || !out->indices || !out->groups || !out->name)
        return 0;
    for (i = 0; i < pos->count; i++) {
        struct GltfVertex *v = &out->verts[i];
        cgltf_uint j[4] = {0, 0, 0, 0};
        cgltf_accessor_read_float(pos, i, v->p, 3);
        if (nrm)
            cgltf_accessor_read_float(nrm, i, v->n, 3);
        if (uv)
            cgltf_accessor_read_float(uv, i, v->uv, 2);
        if (joints)
            cgltf_accessor_read_uint(joints, i, j, 4);
        if (weights)
            cgltf_accessor_read_float(weights, i, v->w, 4);
        else
            v->w[0] = 1.0f;
        for (k = 0; k < 4; k++)
            v->j[k] = (int)j[k] < f->skeleton.BoneCount ? (int)j[k] : 0;
    }
    out->rigid_bone = out->vertex_count ? out->verts[0].j[0] : 0;
    for (i = 0; i < mesh->primitives_count; i++) {
        cgltf_primitive *p = &mesh->primitives[i];
        int local = 0;
        if (p->material) {
            int global = (int)(p->material - g->materials);
            for (k = 0; k < (size_t)material_count; k++)
                if (materials[k] == global)
                    local = (int)k;
        }
        out->groups[i * 3] = local;
        out->groups[i * 3 + 1] = (int)(written / 3);
        out->groups[i * 3 + 2] = p->indices ? (int)(p->indices->count / 3) : 0;
        for (k = 0; p->indices && k < p->indices->count; k++)
            out->indices[written++] = (unsigned short)cgltf_accessor_read_index(p->indices, k);
    }
    for (k = 0; k < (size_t)material_count; k++)
        bindings[k].Material = materials[k] >= 0 && materials[k] < f->info.MaterialCount ? f->material_list[materials[k]]
                                                                                          : NULL;
    *binding_count = material_count;
    out->pub.Name = out->name;
    out->pub.MaterialBindingCount = material_count;
    out->pub.MaterialBindings = bindings;
    out->pub.BoneBindingCount = out->rigid ? 1 : f->skeleton.BoneCount;
    return 1;
}

static int load_model(struct granny_file *f, cgltf_data *g)
{
    size_t i, bindings = 0;
    int used;
    if (!g->meshes_count)
        return 1;
    f->meshes = calloc(g->meshes_count, sizeof *f->meshes);
    f->mesh_list = calloc(g->meshes_count, sizeof *f->mesh_list);
    f->mesh_bindings = calloc(g->meshes_count, sizeof *f->mesh_bindings);
    f->material_bindings = calloc(g->meshes_count * 64 + 1, sizeof *f->material_bindings);
    if (!f->meshes || !f->mesh_list || !f->mesh_bindings || !f->material_bindings)
        return 0;
    for (i = 0; i < g->meshes_count; i++) {
        if (!load_mesh(&f->meshes[i], &g->meshes[i], g, f, f->material_bindings + bindings, &used))
            return 0;
        bindings += (size_t)used;
        f->mesh_list[i] = &f->meshes[i].pub;
        f->mesh_bindings[i].Mesh = &f->meshes[i].pub;
    }
    f->info.MeshCount = (int)g->meshes_count;
    f->info.Meshes = f->mesh_list;
    f->model.Name = "";
    f->model.Skeleton = &f->skeleton;
    f->model.MeshBindingCount = (int)g->meshes_count;
    f->model.MeshBindings = f->mesh_bindings;
    f->models[0] = &f->model;
    f->info.ModelCount = 1;
    f->info.Models = f->models;
    return 1;
}

static int load_animations(struct granny_file *f, cgltf_data *g)
{
    size_t a, c;
    if (!g->animations_count)
        return 1;
    f->animations = calloc(g->animations_count, sizeof *f->animations);
    f->animation_list = calloc(g->animations_count, sizeof *f->animation_list);
    if (!f->animations || !f->animation_list)
        return 0;
    for (a = 0; a < g->animations_count; a++) {
        cgltf_animation *source = &g->animations[a];
        struct granny_animation *anim = &f->animations[a];
        char extras[256];
        anim->duration = extras_float(extras_text(g, &source->extras, extras, sizeof extras), "duration");
        anim->tracks = calloc(source->channels_count + 1, sizeof *anim->tracks);
        if (!anim->tracks)
            return 0;
        for (c = 0; c < source->channels_count; c++) {
            cgltf_animation_channel *ch = &source->channels[c];
            struct GltfTrack *t = &anim->tracks[anim->track_count];
            cgltf_accessor *in = ch->sampler->input, *out = ch->sampler->output;
            if (!ch->target_node || (ch->target_path != cgltf_animation_path_type_translation &&
                                     ch->target_path != cgltf_animation_path_type_rotation &&
                                     ch->target_path != cgltf_animation_path_type_scale))
                continue;
            copy_bone_name(t->bone, ch->target_node->name);
            t->path = ch->target_path == cgltf_animation_path_type_translation ? PATH_TRANSLATION
                    : ch->target_path == cgltf_animation_path_type_rotation ? PATH_ROTATION : PATH_SCALE;
            t->width = t->path == PATH_ROTATION ? 4 : 3;
            t->count = (int)in->count;
            t->times = malloc(sizeof(float) * (in->count + 1));
            t->values = malloc(sizeof(float) * (out->count * t->width + 1));
            if (!t->times || !t->values || out->count != in->count)
                return 0;
            cgltf_accessor_unpack_floats(in, t->times, in->count);
            cgltf_accessor_unpack_floats(out, t->values, out->count * t->width);
            if (t->count && t->times[t->count - 1] > anim->duration)
                anim->duration = t->times[t->count - 1];
            anim->track_count++;
        }
        f->animation_list[a] = anim;
    }
    f->info.AnimationCount = (int)g->animations_count;
    f->info.Animations = f->animation_list;
    return 1;
}

static void free_contents(struct granny_file *f)
{
    int i, k;
    for (i = 0; f->textures && i < f->info.TextureCount; i++) {
        stbi_image_free(f->textures[i].rgba);
        free(f->textures[i].name);
    }
    for (i = 0; f->meshes && i < f->info.MeshCount; i++) {
        free(f->meshes[i].name);
        free(f->meshes[i].verts);
        free(f->meshes[i].indices);
        free(f->meshes[i].groups);
    }
    for (i = 0; f->animations && i < f->info.AnimationCount; i++) {
        for (k = 0; k < f->animations[i].track_count; k++) {
            free(f->animations[i].tracks[k].times);
            free(f->animations[i].tracks[k].values);
        }
        free(f->animations[i].tracks);
    }
    free(f->bones);
    free(f->mesh_bindings);
    free(f->meshes);
    free(f->mesh_list);
    free(f->textures);
    free(f->texture_list);
    free(f->materials);
    free(f->material_list);
    free(f->material_bindings);
    free(f->animations);
    free(f->animation_list);
}

/* The client hands over the .gr2 bytes; the converted model is found by their CRC-32 under
   model\gltf\ (tools/granny_export writes it there from the same file). */
granny_file *_GrannyReadEntireFileFromMemory(int size, void *memory)
{
    static int warned;
    char name[64];
    struct File file;
    struct granny_file *f;
    cgltf_options options;
    cgltf_data *g = NULL;
    int ok;
    if (size <= 0 || !memory)
        return NULL;
    snprintf(name, sizeof name, "model\\gltf\\%08lx.glb", (unsigned long)crc32(0, memory, (uInt)size));
    file_construct(&file);
    if (!file_open(&file, name, 0, 1, 0)) {
        if (!warned) {
            char text[160];
            snprintf(text, sizeof text,
                     "Granny models need converting: data\\%s is missing (run tools/granny_export).\n", name);
            platform_debug_output(text);
            warned = 1;
        }
        file_destruct(&file);
        return NULL;
    }
    memset(&options, 0, sizeof options);
    f = calloc(1, sizeof *f);
    ok = f && cgltf_parse(&options, file.buffer, (cgltf_size)file.size, &g) == cgltf_result_success &&
         cgltf_load_buffers(&options, g, NULL) == cgltf_result_success && load_textures(f, g) &&
         load_materials(f, g) && load_skeleton(f, g) && load_model(f, g) && load_animations(f, g);
    cgltf_free(g);
    file_destruct(&file);
    if (!ok) {
        if (f)
            free_contents(f);
        free(f);
        return NULL;
    }
    f->info.FromFileName = "";
    return f;
}

granny_file_info *_GrannyGetFileInfo(granny_file *file)
{
    return file ? &file->info : NULL;
}

void _GrannyFreeFileSection(granny_file *file, int section)
{
    (void)file;
    (void)section;
}

void _GrannyFreeFile(granny_file *file)
{
    if (!file)
        return;
    free_contents(file);
    free(file);
}

/* ---- poses, the model instance and its animation controls ---- */

granny_local_pose *_GrannyNewLocalPose(int bones)
{
    granny_local_pose *pose = calloc(1, sizeof *pose + sizeof pose->x[0] * (size_t)(bones > 0 ? bones : 1));
    if (pose)
        pose->bones = bones;
    return pose;
}

void _GrannyFreeLocalPose(granny_local_pose *pose)
{
    free(pose);
}

granny_world_pose *_GrannyNewWorldPose(int bones)
{
    granny_world_pose *pose = calloc(1, sizeof *pose);
    if (!pose)
        return NULL;
    pose->bones = bones;
    pose->world = calloc((size_t)(bones > 0 ? bones : 1) * 16, sizeof(float));
    pose->composite = calloc((size_t)(bones > 0 ? bones : 1) * 16, sizeof(float));
    if (!pose->world || !pose->composite) {
        free(pose->world);
        free(pose->composite);
        free(pose);
        return NULL;
    }
    return pose;
}

void _GrannyFreeWorldPose(granny_world_pose *pose)
{
    if (!pose)
        return;
    free(pose->world);
    free(pose->composite);
    free(pose);
}

granny_model_instance *_GrannyInstantiateModel(granny_model *model)
{
    granny_model_instance *instance;
    if (!model)
        return NULL;
    instance = calloc(1, sizeof *instance);
    if (instance)
        instance->model = model;
    return instance;
}

void _GrannyFreeModelInstance(granny_model_instance *instance)
{
    if (!instance)
        return;
    while (instance->controls)
        _GrannyFreeControl(instance->controls);
    free(instance);
}

granny_skeleton *_GrannyGetSourceSkeleton(granny_model_instance *instance)
{
    return instance ? instance->model->Skeleton : NULL;
}

static int bone_index(const granny_skeleton *skeleton, const char *name)
{
    const struct GltfBone *bones = skeleton->Bones;
    int i;
    for (i = 0; i < skeleton->BoneCount; i++)
        if (!strcmp(bones[i].name, name))
            return i;
    return -1;
}

granny_control *_GrannyPlayControlledAnimation(float start, granny_animation *animation, granny_model_instance *instance)
{
    granny_control *control;
    int i;
    if (!animation || !instance)
        return NULL;
    control = calloc(1, sizeof *control);
    if (!control)
        return NULL;
    control->track_bone = calloc((size_t)animation->track_count + 1, sizeof *control->track_bone);
    if (!control->track_bone) {
        free(control);
        return NULL;
    }
    for (i = 0; i < animation->track_count; i++)
        control->track_bone[i] = bone_index(instance->model->Skeleton, animation->tracks[i].bone);
    control->instance = instance;
    control->animation = animation;
    control->start = start;
    control->loops = 1;
    control->active = 1;
    control->next = instance->controls;
    instance->controls = control;
    return control;
}

void _GrannySetControlLoopCount(granny_control *control, int count)
{
    if (control)
        control->loops = count;
}

void _GrannySetControlActive(granny_control *control, BYTE active)
{
    if (control)
        control->active = active != 0;
}

void _GrannyFreeControl(granny_control *control)
{
    granny_control **link;
    if (!control)
        return;
    for (link = &control->instance->controls; *link; link = &(*link)->next)
        if (*link == control) {
            *link = control->next;
            break;
        }
    free(control->track_bone);
    free(control);
}

float _GrannyGetControlDurationLeft(granny_control *control)
{
    float left;
    if (!control)
        return 0.0f;
    if (control->loops <= 0)
        return LOOP_FOREVER_LEFT;
    left = control->start + control->animation->duration * (float)control->loops - control->instance->clock;
    return left > 0.0f ? left : 0.0f;
}

void _GrannyFreeCompletedModelControls(granny_model_instance *instance)
{
    granny_control *control, *next;
    if (!instance)
        return;
    for (control = instance->controls; control; control = next) {
        next = control->next;
        if (control->loops > 0 && !(_GrannyGetControlDurationLeft(control) > 0.0f))
            _GrannyFreeControl(control);
    }
}

void _GrannySetModelClock(granny_model_instance *instance, float t)
{
    if (instance)
        instance->clock = t;
}

static float control_time(const granny_control *control)
{
    float duration = control->animation->duration, t = control->instance->clock - control->start;
    if (duration <= 0.0f)
        return 0.0f;
    if (control->loops > 0 && t >= duration * (float)control->loops)
        return duration;
    if (t < 0.0f)
        return control->loops > 0 ? 0.0f : t - floorf(t / duration) * duration;
    return t - floorf(t / duration) * duration;
}

static void sample_track(const struct GltfTrack *track, float t, float *out)
{
    int lo = 0, hi = track->count - 1, k, w = track->width;
    const float *a, *b;
    float f, dot = 0.0f;
    if (track->count <= 0)
        return;
    if (t <= track->times[0] || track->count == 1) {
        memcpy(out, track->values, sizeof(float) * w);
        return;
    }
    if (t >= track->times[hi]) {
        memcpy(out, track->values + hi * w, sizeof(float) * w);
        return;
    }
    while (hi - lo > 1) {
        int mid = (lo + hi) / 2;
        if (track->times[mid] <= t)
            lo = mid;
        else
            hi = mid;
    }
    f = (t - track->times[lo]) / (track->times[hi] - track->times[lo]);
    a = track->values + lo * w;
    b = track->values + hi * w;
    for (k = 0; k < w; k++)
        out[k] = a[k] + (b[k] - a[k]) * f;
    if (w == 4) {
        for (k = 0; k < 4; k++)
            dot += out[k] * out[k];
        dot = dot > 0.0f ? 1.0f / sqrtf(dot) : 1.0f;
        for (k = 0; k < 4; k++)
            out[k] *= dot;
    }
}

void _GrannySampleModelAnimations(granny_model_instance *instance, int first, int count, granny_local_pose *pose)
{
    const granny_skeleton *skeleton;
    const struct GltfBone *bones;
    struct GltfTransform sum[256];
    float weight[256];
    granny_control *c;
    int b, i, k;
    if (!instance || !pose)
        return;
    skeleton = instance->model->Skeleton;
    bones = skeleton->Bones;
    if (count > pose->bones)
        count = pose->bones;
    if (first < 0 || first + count > skeleton->BoneCount || first + count > 256)
        return;
    memset(sum, 0, sizeof sum);
    memset(weight, 0, sizeof weight);
    for (c = instance->controls; c; c = c->next) {
        float t, value[4];
        struct GltfTransform x[256];
        int touched[256];
        if (!c->active)
            continue;
        t = control_time(c);
        for (b = first; b < first + count; b++) {
            memcpy(x[b].t, bones[b].t, sizeof x[b].t);
            memcpy(x[b].r, bones[b].r, sizeof x[b].r);
            memcpy(x[b].s, bones[b].s, sizeof x[b].s);
            touched[b] = 0;
        }
        for (i = 0; i < c->animation->track_count; i++) {
            const struct GltfTrack *track = &c->animation->tracks[i];
            b = c->track_bone[i];
            if (b < first || b >= first + count)
                continue;
            sample_track(track, t, value);
            memcpy(track->path == PATH_TRANSLATION ? x[b].t : track->path == PATH_ROTATION ? x[b].r : x[b].s, value,
                   sizeof(float) * track->width);
            touched[b] = 1;
        }
        for (b = first; b < first + count; b++) {
            float sign = 1.0f;
            if (!touched[b])
                continue;
            if (weight[b] > 0.0f &&
                sum[b].r[0] * x[b].r[0] + sum[b].r[1] * x[b].r[1] + sum[b].r[2] * x[b].r[2] + sum[b].r[3] * x[b].r[3] < 0)
                sign = -1.0f;
            for (k = 0; k < 3; k++) {
                sum[b].t[k] += x[b].t[k];
                sum[b].s[k] += x[b].s[k];
            }
            for (k = 0; k < 4; k++)
                sum[b].r[k] += sign * x[b].r[k];
            weight[b] += 1.0f;
        }
    }
    for (b = first; b < first + count; b++) {
        struct GltfTransform *out = &pose->x[b];
        if (weight[b] > 0.0f) {
            float len = 0.0f;
            for (k = 0; k < 3; k++) {
                out->t[k] = sum[b].t[k] / weight[b];
                out->s[k] = sum[b].s[k] / weight[b];
            }
            for (k = 0; k < 4; k++)
                len += sum[b].r[k] * sum[b].r[k];
            len = len > 0.0f ? 1.0f / sqrtf(len) : 1.0f;
            for (k = 0; k < 4; k++)
                out->r[k] = sum[b].r[k] * len;
        } else {
            memcpy(out->t, bones[b].t, sizeof out->t);
            memcpy(out->r, bones[b].r, sizeof out->r);
            memcpy(out->s, bones[b].s, sizeof out->s);
        }
    }
}

/* Matrices are Granny's: row vectors, v' = v * M, translation in the last row. */
static void compose(const struct GltfTransform *x, float *m)
{
    float qx = x->r[0], qy = x->r[1], qz = x->r[2], qw = x->r[3];
    float r[9] = {
        1 - 2 * (qy * qy + qz * qz), 2 * (qx * qy + qw * qz), 2 * (qx * qz - qw * qy),
        2 * (qx * qy - qw * qz), 1 - 2 * (qx * qx + qz * qz), 2 * (qy * qz + qw * qx),
        2 * (qx * qz + qw * qy), 2 * (qy * qz - qw * qx), 1 - 2 * (qx * qx + qy * qy),
    };
    int i;
    for (i = 0; i < 3; i++) {
        m[i * 4] = x->s[i] * r[i * 3];
        m[i * 4 + 1] = x->s[i] * r[i * 3 + 1];
        m[i * 4 + 2] = x->s[i] * r[i * 3 + 2];
        m[i * 4 + 3] = 0.0f;
    }
    m[12] = x->t[0];
    m[13] = x->t[1];
    m[14] = x->t[2];
    m[15] = 1.0f;
}

static void multiply(const float *a, const float *b, float *out)
{
    float m[16];
    int i, k;
    for (i = 0; i < 4; i++)
        for (k = 0; k < 4; k++)
            m[i * 4 + k] = a[i * 4] * b[k] + a[i * 4 + 1] * b[4 + k] + a[i * 4 + 2] * b[8 + k] + a[i * 4 + 3] * b[12 + k];
    memcpy(out, m, sizeof m);
}

void _GrannyBuildWorldPose(granny_skeleton *skeleton, int first, int count, granny_local_pose *localPose,
                           const float *offset, granny_world_pose *worldPose)
{
    const struct GltfBone *bones;
    int b;
    if (!skeleton || !localPose || !worldPose)
        return;
    bones = skeleton->Bones;
    if (count > worldPose->bones)
        count = worldPose->bones;
    for (b = first; b < first + count && b < skeleton->BoneCount; b++) {
        float local[16], *world = worldPose->world + b * 16;
        compose(&localPose->x[b], local);
        if (bones[b].parent >= 0 && bones[b].parent < b)
            multiply(local, worldPose->world + bones[b].parent * 16, world);
        else if (offset)
            multiply(local, offset, world);
        else
            memcpy(world, local, sizeof local);
        multiply(bones[b].inverse_world, world, worldPose->composite + b * 16);
    }
}

float *_GrannyGetWorldPoseComposite4x4Array(granny_world_pose *worldPose)
{
    return worldPose ? worldPose->composite : NULL;
}

float *_GrannyGetWorldPoseComposite4x4(granny_world_pose *worldPose, int bone)
{
    return worldPose && bone >= 0 && bone < worldPose->bones ? worldPose->composite + bone * 16 : NULL;
}

/* ---- meshes ---- */

static struct GltfMesh *gltf_mesh(granny_mesh *mesh)
{
    return (struct GltfMesh *)mesh;
}

granny_mesh_binding *_GrannyNewMeshBinding(granny_mesh *mesh, granny_skeleton *from, granny_skeleton *to)
{
    struct GltfMesh *m = gltf_mesh(mesh);
    int count = m->rigid ? 1 : to->BoneCount, i;
    granny_mesh_binding *binding = calloc(1, sizeof *binding + sizeof binding->bones[0] * (size_t)(count + 1));
    (void)from;
    if (!binding)
        return NULL;
    binding->count = count;
    if (m->rigid)
        binding->bones[0] = m->rigid_bone;
    else
        for (i = 0; i < count; i++)
            binding->bones[i] = i;
    return binding;
}

void _GrannyFreeMeshBinding(granny_mesh_binding *binding)
{
    free(binding);
}

int *_GrannyGetMeshBindingToBoneIndices(granny_mesh_binding *binding)
{
    return binding ? binding->bones : NULL;
}

int _GrannyGetMeshIndexCount(granny_mesh *mesh)
{
    return mesh ? gltf_mesh(mesh)->index_count : 0;
}

void _GrannyCopyMeshIndices(granny_mesh *mesh, int bytesPerIndex, void *out)
{
    struct GltfMesh *m = gltf_mesh(mesh);
    int i;
    if (bytesPerIndex == 2) {
        memcpy(out, m->indices, sizeof *m->indices * (size_t)m->index_count);
        return;
    }
    for (i = 0; i < m->index_count; i++)
        ((uint32_t *)out)[i] = m->indices[i];
}

int _GrannyGetMeshVertexCount(granny_mesh *mesh)
{
    return mesh ? gltf_mesh(mesh)->vertex_count : 0;
}

BYTE _GrannyMeshIsRigid(granny_mesh *mesh)
{
    return mesh && gltf_mesh(mesh)->rigid;
}

void _GrannyCopyMeshVertices(granny_mesh *mesh, const granny_data_type_definition *type, void *out)
{
    struct GltfMesh *m = gltf_mesh(mesh);
    float *o = out;
    int i;
    (void)type;
    for (i = 0; i < m->vertex_count; i++, o += 8) {
        memcpy(o, m->verts[i].p, sizeof(float) * 3);
        memcpy(o + 3, m->verts[i].n, sizeof(float) * 3);
        memcpy(o + 6, m->verts[i].uv, sizeof(float) * 2);
    }
}

const granny_data_type_definition *_GrannyGetMeshVertexType(granny_mesh *mesh)
{
    (void)mesh;
    return (const granny_data_type_definition *)&g_sourceVertexTag;
}

granny_mesh_deformer *_GrannyNewMeshDeformer(const granny_data_type_definition *from,
                                             const granny_data_type_definition *to, int deformation)
{
    (void)from;
    (void)to;
    g_deformer.deformation = deformation;
    return &g_deformer;
}

void _GrannyFreeMeshDeformer(granny_mesh_deformer *deformer)
{
    (void)deformer;
}

int *_GrannyGetMeshTriangleGroups(granny_mesh *mesh)
{
    return mesh ? gltf_mesh(mesh)->groups : NULL;
}

void *_GrannyGetMeshVertices(granny_mesh *mesh)
{
    return mesh ? gltf_mesh(mesh)->verts : NULL;
}

/* Position and normal skinned by up to four composite bone matrices; texture coordinates copied. */
void _GrannyDeformVertices(granny_mesh_deformer *deformer, const int *bones, const float *composite, int count,
                           const void *from, void *to)
{
    const struct GltfVertex *v = from;
    float *o = to;
    int i, k;
    (void)deformer;
    for (i = 0; i < count; i++, v++, o += 8) {
        float p[3] = {0, 0, 0}, n[3] = {0, 0, 0};
        for (k = 0; k < 4; k++) {
            const float *m;
            float w = v->w[k];
            if (w == 0.0f)
                continue;
            m = composite + bones[v->j[k]] * 16;
            p[0] += w * (v->p[0] * m[0] + v->p[1] * m[4] + v->p[2] * m[8] + m[12]);
            p[1] += w * (v->p[0] * m[1] + v->p[1] * m[5] + v->p[2] * m[9] + m[13]);
            p[2] += w * (v->p[0] * m[2] + v->p[1] * m[6] + v->p[2] * m[10] + m[14]);
            n[0] += w * (v->n[0] * m[0] + v->n[1] * m[4] + v->n[2] * m[8]);
            n[1] += w * (v->n[0] * m[1] + v->n[1] * m[5] + v->n[2] * m[9]);
            n[2] += w * (v->n[0] * m[2] + v->n[1] * m[6] + v->n[2] * m[10]);
        }
        memcpy(o, p, sizeof p);
        memcpy(o + 3, n, sizeof n);
        memcpy(o + 6, v->uv, sizeof v->uv);
    }
}

int _GrannyGetMeshBytesPerIndex(granny_mesh *mesh)
{
    (void)mesh;
    return 2;
}

void *_GrannyGetMeshIndices(granny_mesh *mesh)
{
    return mesh ? gltf_mesh(mesh)->indices : NULL;
}

/* ---- materials and textures ---- */

granny_texture *_GrannyGetMaterialTextureByType(granny_material *material, int type)
{
    (void)type;
    return material ? material->diffuse : NULL;
}

BYTE _GrannyTextureHasAlpha(granny_texture *texture)
{
    return texture && ((struct GltfTexture *)texture)->alpha;
}

void _GrannyCopyTextureImage(granny_texture *texture, int image, int mip, const granny_pixel_layout *layout,
                             int width, int height, int stride, void *pixels)
{
    const struct GltfTexture *t = (const struct GltfTexture *)texture;
    int y;
    (void)image;
    (void)mip;
    (void)layout;
    if (!t || width != t->pub.Width || height != t->pub.Height)
        return;
    for (y = 0; y < height; y++)
        memcpy((unsigned char *)pixels + (size_t)y * stride, t->rgba + (size_t)y * width * 4, (size_t)width * 4);
}
#endif
