#include "resources/anim_model_res.h"
#include "resources/resource_ops.h"
#include "fileio/res_mgr.h"
#include "fileio/file.h"
#include "render/texture_cache.h"
#include <limits.h>
#include <stdlib.h>
#include <string.h>

#define RSX_NODE_MARKER 0x12345678

static void animated_node_destroy(struct AnimatedModelNode *node)
{
    size_t i;
    if (!node) return;
    for (i = 0; i < node->frameCount; ++i) {
        free(node->frames[i].vertices);
        free(node->frames[i].textureVertices);
    }
    for (i = 0; i < node->textureCount; ++i) {
        if (node->textures[i]) {
            texture_unlock(node->textures[i]);
            ui_tex_cache_release(&g_textureCache, node->textures[i]);
        }
    }
    free(node->frames);
    free(node->faces);
    free(node->textures);
    free(node->children);
    owned_text_clear(&node->name);
    owned_text_clear(&node->parentName);
    free(node);
}

static void animated_model_clear(struct AnimatedModelResource *model)
{
    size_t i;
    struct Res base = model->res;
    for (i = 0; i < model->nodeCount; ++i) animated_node_destroy(model->nodes[i]);
    free(model->nodes);
    owned_text_clear(&model->mainNode);
    memset(model, 0, sizeof *model);
    model->res = base;
    model->headerValue0 = 2;
    model->version12Scalar = 1.0f;
    model->version13Value = 255;
}

static int read_count(struct File *file, size_t element_size, size_t *count)
{
    int value;
    if (!file_read(file, &value, sizeof value) || value < 0 ||
        (size_t)value > INT_MAX / element_size) return 0;
    *count = (size_t)value;
    return 1;
}

static int read_name(struct File *file, struct OwnedText *name)
{
    char bytes[41];
    if (!file_read(file, bytes, 40)) return 0;
    bytes[40] = 0;
    return owned_text_assign(name, text_view(bytes, strlen(bytes)));
}

static int read_records(struct File *file, void *data, size_t count, size_t size)
{
    return !count || (data && file_read(file, data, (int)(count * size)));
}

static int read_node(struct File *file, struct AnimatedModelNode *node,
                     size_t frame_count, const struct OwnedText *textures, size_t texture_count)
{
    int marker;
    size_t i, count;
    if (!file_read(file, &marker, sizeof marker) || marker != RSX_NODE_MARKER ||
        !read_name(file, &node->name) || !read_name(file, &node->parentName)) return 0;
    if (frame_count) {
        node->frames = calloc(frame_count, sizeof *node->frames);
        if (!node->frames) return 0;
        node->frameCount = frame_count;
    }
    if (!read_count(file, sizeof *node->textures, &count)) return 0;
    if (count) {
        node->textures = calloc(count, sizeof *node->textures);
        if (!node->textures) return 0;
        node->textureCount = count;
    }
    for (i = 0; i < count; ++i) {
        int index;
        if (!file_read(file, &index, sizeof index) || index < 0 || (size_t)index >= texture_count) return 0;
        node->textures[i] = texture_cache_get(&g_textureCache, owned_text_cstr(&textures[index]), 0, 0);
        if (!node->textures[i]) return 0;
        texture_lock(node->textures[i]);
    }
    if (!read_count(file, sizeof *node->frames[0].vertices, &node->vertexCount)) return 0;
    for (i = 0; i < frame_count; ++i) {
        if (node->vertexCount) node->frames[i].vertices = malloc(node->vertexCount * sizeof *node->frames[i].vertices);
        if (!read_records(file, node->frames[i].vertices, node->vertexCount, sizeof *node->frames[i].vertices)) return 0;
    }
    if (!read_count(file, sizeof(struct RsxTextureVertex), &node->textureVertexCount) ||
        !file_read(file, &node->sharedTextureVertices, sizeof node->sharedTextureVertices)) return 0;
    count = node->sharedTextureVertices ? 1 : frame_count;
    if (count && !frame_count) return 0;
    for (i = 0; i < count; ++i) {
        if (node->textureVertexCount) node->frames[i].textureVertices = malloc(node->textureVertexCount * sizeof *node->frames[i].textureVertices);
        if (!read_records(file, node->frames[i].textureVertices, node->textureVertexCount, sizeof *node->frames[i].textureVertices)) return 0;
    }
    if (!read_count(file, sizeof *node->faces, &node->faceCount)) return 0;
    if (node->faceCount) node->faces = malloc(node->faceCount * sizeof *node->faces);
    return read_records(file, node->faces, node->faceCount, sizeof *node->faces) &&
           file_read(file, &marker, sizeof marker) && marker == RSX_NODE_MARKER;
}

static int link_nodes(struct AnimatedModelResource *model)
{
    size_t i, j, depth;
    for (i = 0; i < model->nodeCount; ++i) {
        struct AnimatedModelNode *node = model->nodes[i];
        if (!node->parentName.size) continue;
        for (j = 0; j < model->nodeCount; ++j) {
            struct AnimatedModelNode *parent = model->nodes[j];
            if (parent->name.size == node->parentName.size &&
                !memcmp(owned_text_cstr(&parent->name), owned_text_cstr(&node->parentName), parent->name.size)) {
                node->parent = parent;
                ++parent->childCount;
                break;
            }
        }
        if (!node->parent) return 0;
    }
    for (i = 0; i < model->nodeCount; ++i) {
        struct AnimatedModelNode *node = model->nodes[i], *parent = node;
        for (depth = 0; parent && depth <= model->nodeCount; ++depth) parent = parent->parent;
        if (parent) return 0;
        if (node->childCount) {
            node->children = calloc(node->childCount, sizeof *node->children);
            if (!node->children) return 0;
            node->childCount = 0;
        }
    }
    for (i = 0; i < model->nodeCount; ++i) {
        struct AnimatedModelNode *node = model->nodes[i];
        if (node->parent) node->parent->children[node->parent->childCount++] = node;
    }
    return 1;
}

static int read_model(struct File *file, struct AnimatedModelResource *model)
{
    unsigned char magic[4], major, minor, skipped[16];
    struct OwnedText *textures = NULL;
    size_t texture_count = 0, count, i;
    int ok = 0;
    if (!file_read(file, magic, sizeof magic) || !file_read(file, &major, 1) ||
        !file_read(file, &minor, 1)) goto done;
    if (major == 1 && minor >= 2 &&
        (!file_read(file, &model->version12Scalar, 4) || !file_read(file, model->version12Values, 12))) goto done;
    if (major == 1 && minor == 3 && !file_read(file, &model->version13Value, 4)) goto done;
    if (!file_read(file, &model->headerValue0, 4) ||
        !read_count(file, sizeof(struct AnimatedModelFrame), &model->frameCount) ||
        !file_read(file, &model->headerValue1, 4) || !file_read(file, skipped, sizeof skipped) ||
        !read_count(file, sizeof *textures, &count)) goto done;
    if (count) {
        textures = calloc(count, sizeof *textures);
        if (!textures) goto done;
        texture_count = count;
    }
    for (i = 0; i < texture_count; ++i) {
        if (!read_name(file, &textures[i])) goto done;
        texture_cache_get(&g_textureCache, owned_text_cstr(&textures[i]), 0, 0);
    }
    if (!read_name(file, &model->mainNode) || !read_count(file, sizeof *model->nodes, &count)) goto done;
    if (count) {
        model->nodes = calloc(count, sizeof *model->nodes);
        if (!model->nodes) goto done;
        model->nodeCount = count;
    }
    for (i = 0; i < model->nodeCount; ++i) {
        model->nodes[i] = calloc(1, sizeof *model->nodes[i]);
        if (!model->nodes[i] || !read_node(file, model->nodes[i], model->frameCount, textures, texture_count)) goto done;
    }
    ok = link_nodes(model);
done:
    for (i = 0; i < texture_count; ++i) owned_text_clear(&textures[i]);
    free(textures);
    return ok;
}

static int animated_model_read(struct AnimatedModelResource *model, const char *name)
{
    struct AnimatedModelResource next = {0};
    struct File file;
    int opened, ok = 0;
    animated_model_clear(&next);
    file_construct(&file);
    opened = file_open(&file, name, 0, 1, 0);
    if (opened) {
        ok = read_model(&file, &next);
        if (!ok) error_msg("data corrupt", 0);
        file_close(&file);
    }
    file_destruct(&file);
    if (ok) {
        next.res = model->res;
        animated_model_clear(model);
        *model = next;
    } else animated_model_clear(&next);
    return ok;
}

static void animated_model_resource_destroy(struct Res *resource)
{
    struct AnimatedModelResource *model = (struct AnimatedModelResource *)resource;
    animated_model_clear(model);
    res_destruct(resource);
    free(model);
}
static int animated_model_resource_load(struct Res *resource, const char *name)
{
    return animated_model_read((struct AnimatedModelResource *)resource, name);
}
const struct ResourceOps animated_model_resource_ops = {
    .destroy = animated_model_resource_destroy,
    .load = animated_model_resource_load,
};

struct Res *animated_model_resource_create(void)
{
    struct AnimatedModelResource *model = calloc(1, sizeof *model);
    if (!model) return NULL;
    res_construct(&model->res);
    model->res.ops = &animated_model_resource_ops;
    animated_model_clear(model);
    return &model->res;
}
