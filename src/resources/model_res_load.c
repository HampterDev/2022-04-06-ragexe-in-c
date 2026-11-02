#include <limits.h>
#include <stdlib.h>
#include <string.h>
#include "fileio/file.h"
#include "render/keyframes.h"
#include "render/texture_cache.h"
#include "resources/model_res.h"
#include "resources/model_mesh.h"

static int read_model_count(struct File *file, size_t *count, size_t size)
{
    int value;
    if (!file_read(file, &value, sizeof value) || value < 0 ||
        (size_t)value > SIZE_MAX / size) return 0;
    *count = (size_t)value;
    return 1;
}

static int read_model_name(struct File *file, struct OwnedText *name, int fixed)
{
    size_t size = 40;
    char *text;
    if (!fixed && !read_model_count(file, &size, 1)) return 0;
    if (size > INT_MAX || size == SIZE_MAX) return 0;
    text = size ? malloc(size + 1) : NULL;
    if (size && (!text || !file_read(file, text, (int)size))) {
        free(text);
        return 0;
    }
    if (text) text[size] = '\0';
    owned_text_clear(name);
    name->data = text;
    name->size = text ? strlen(text) : 0;
    return 1;
}

static int read_model_names(struct File *file, struct TextArray *names, int fixed)
{
    size_t count, i;
    if (!read_model_count(file, &count, sizeof *names->items) || !text_array_resize(names, count)) return 0;
    for (i = 0; i < count; ++i)
        if (!read_model_name(file, &names->items[i], fixed)) return 0;
    return 1;
}

static int read_node_textures(struct File *file, struct ModelResNode *node,
                              const struct TextArray *names, int own_names)
{
    size_t count, i;
    struct OwnedText name = {0};
    if (!read_model_count(file, &count, sizeof *node->textures)) return 0;
    node->textures = count ? calloc(count, sizeof *node->textures) : NULL;
    if (count && !node->textures) return 0;
    node->textureCount = count;
    for (i = 0; i < count; ++i) {
        const char *path;
        if (own_names) {
            if (!read_model_name(file, &name, 0)) goto fail;
            path = owned_text_cstr(&name);
        } else {
            int index;
            if (!file_read(file, &index, 4) || index < 0 || (size_t)index >= names->size) goto fail;
            path = owned_text_cstr(&names->items[index]);
        }
        node->textures[i] = texture_cache_get(&g_textureCache, path, 0, 0);
        if (node->textures[i]) texture_lock(node->textures[i]);
    }
    owned_text_clear(&name);
    return 1;
fail:
    owned_text_clear(&name);
    return 0;
}

static int link_model_nodes(struct C3dModelRes *model, int family)
{
    size_t i, depth;
    for (i = 0; i < model->nodeCount; ++i) {
        struct ModelResNode *node = model->nodes[i];
        if (!node->parentName.size) continue;
        node->parent = model_res_find_node(model, owned_text_cstr(&node->parentName));
        if (!node->parent) {
            if (family == 1) return 0;
            continue;
        }
        ++node->parent->childCount;
    }
    for (i = 0; i < model->nodeCount; ++i) {
        struct ModelResNode *node = model->nodes[i], *ancestor = node;
        for (depth = 0; ancestor && depth <= model->nodeCount; ++depth) ancestor = ancestor->parent;
        if (ancestor) return 0;
        if (node->childCount) {
            node->children = calloc(node->childCount, sizeof *node->children);
            if (!node->children) return 0;
        }
        node->childCount = 0;
    }
    for (i = 0; i < model->nodeCount; ++i) {
        struct ModelResNode *node = model->nodes[i];
        if (node->parent) node->parent->children[node->parent->childCount++] = node;
    }
    return 1;
}

static int read_model_blocks(struct File *file, struct C3dModelRes *model, int family, int minor)
{
    struct {
        float size[3], pos[3], rot[3];
        int flags;
    } record = {0};
    int value = 0;
    size_t count, i;
    if (family == 1 && minor < 2) {
        int ignored;
        file_read(file, &ignored, 4);
        return 1;
    }
    file_read(file, &value, sizeof value);
    if (value <= 0)
        return 1;
    if ((size_t)value > SIZE_MAX / sizeof *model->blocks) return 0;
    count = (size_t)value;
    model->blocks = calloc(count, sizeof *model->blocks);
    if (!model->blocks) return 0;
    model->blockCount = count;
    for (i = 0; i < count; ++i) {
        struct VolumeBox *box = &model->blocks[i];
        if (family == 2) {
            struct VolumeBox read = {0};
            if (file_read(file, &read, sizeof read))
                *box = read;
        } else {
            int flags = record.flags;
            memset(&record, 0, sizeof record);
            record.flags = flags;
            if (!file_read(file, &record, minor >= 3 ? (int)sizeof record : 0x24)) {
                memset(&record, 0, 0x24);
                record.flags = flags;
            }
            memcpy(box->size, record.size, sizeof record.size);
            memcpy(box->pos, record.pos, sizeof record.pos);
            memcpy(box->rot, record.rot, sizeof record.rot);
            if (minor >= 3) {
                box->flags = record.flags;
                if (!box->flags) { box->size[0] += 3.0f; box->size[2] += 3.0f; }
            }
        }
    }
    return 1;
}

static int read_model_body(struct C3dModelRes *model, struct File *file, int family, int minor)
{
    struct TextArray textures = {0};
    size_t count, i;
    unsigned char skipped[16];
    int success = 0;
    if (!file_read(file, &model->frameCount, 4) || !file_read(file, &model->shadeType, 4)) goto done;
    model->alpha = 255;
    if ((family == 2 || minor >= 4) && !file_read(file, &model->alpha, 1)) goto done;
    if (family == 2) {
        if (!file_read(file, &model->fps, 4)) goto done;
    } else if (!file_read(file, skipped, sizeof skipped)) goto done;
    if ((family == 1 || minor < 3) && !read_model_names(file, &textures, family == 1)) goto done;
    if (family == 1) {
        for (i = 0; i < textures.size; ++i) {
            if (strstr(owned_text_cstr(&textures.items[i]), ".bik")) {
                size_t next = model->videoNames.size;
                if (!text_array_resize(&model->videoNames, next + 1) ||
                    !owned_text_assign(&model->videoNames.items[next], owned_text_view(&textures.items[i]))) goto done;
            }
        }
        if (!text_array_resize(&model->mainNodes, 1) || !read_model_name(file, &model->mainNodes.items[0], 1)) goto done;
    } else if (!read_model_names(file, &model->mainNodes, 0)) goto done;
    if (!read_model_count(file, &count, sizeof *model->nodes)) goto done;
    model->nodes = count ? calloc(count, sizeof *model->nodes) : NULL;
    if (count && !model->nodes) goto done;
    model->nodeCount = count;
    for (i = 0; i < count; ++i) {
        struct ModelResNode *node = model_res_node_create(family);
        float matrix[12];
        if (!node) goto done;
        model->nodes[i] = node;
        node->model = model;
        if (family == 1) node->alpha = model->alpha;
        if (!read_model_name(file, &node->name, family == 1) ||
            !read_model_name(file, &node->parentName, family == 1) ||
            !read_node_textures(file, node, &textures, family == 2 && minor >= 3)) goto done;
        if (family == 1) {
            struct ModelResNodeV1 *v1 = (struct ModelResNodeV1 *)node;
            if (!file_read(file, matrix, sizeof matrix) || !file_read(file, v1->pos, sizeof v1->pos) ||
                !file_read(file, &v1->angle, sizeof v1->angle) || !file_read(file, v1->axis, sizeof v1->axis) ||
                !file_read(file, v1->scale, sizeof v1->scale)) goto done;
            node->mesh = model_mesh_read(file, family, minor, matrix);
            if (!node->mesh) goto done;
            if (!key_quat_read(file, &v1->rotKeys, &v1->rotKeyCount)) goto done;
        } else {
            struct ModelResNodeV2 *v2 = (struct ModelResNodeV2 *)node;
            if (!file_read(file, v2->matrix, sizeof v2->matrix)) goto done;
            node->mesh = model_mesh_read(file, family, minor, NULL);
            if (!node->mesh) goto done;
            if (!key_vec_read(file, &v2->scaleKeys, &v2->scaleKeyCount) ||
                !key_quat_read(file, &v2->rotKeys, &v2->rotKeyCount) ||
                !key_vec_read(file, &v2->posKeys, &v2->posKeyCount) ||
                (minor >= 3 && !key_uv_read(file, &v2->uvKeys))) goto done;
        }
    }
    if (!link_model_nodes(model, family)) goto done;
    if (family == 2)
        for (i = 0; i < model->mainNodes.size; ++i)
            if (!model_res_find_node(model, owned_text_cstr(&model->mainNodes.items[i]))) goto done;
    success = read_model_blocks(file, model, family, minor);
done:
    text_array_clear(&textures);
    return success;
}

static BYTE load_model_resource(struct C3dModelRes *model, const char *name, int family)
{
    struct File file;
    unsigned char major, minor;
    char magic[4];
    struct OwnedText path = {0};
    int success = 0;
    if (!owned_text_assign(&path, text_view_cstr(name))) return 0;
    model_res_clear(model);
    file_construct(&file);
    if (!file_open(&file, owned_text_cstr(&path), 0, 1, 0)) goto done;
    if (!file_read(&file, magic, sizeof magic) || memcmp(magic, "GRSM", 4)) {
        error_msg(g_model_res_not_a_model, 0);
        goto close;
    }
    if (!file_read(&file, &major, 1) || !file_read(&file, &minor, 1)) goto close;
    model->version = major;
    if (family == 1 && (major != 1 || minor > 5)) {
        error_msg("지원되지 않는 버젼입니다. 내용물이 깨져있을 수 있습니다", 0);
        goto close;
    }
    if (family == 2 && (minor < 1 || minor > 3)) {
        success = 1;
        goto close;
    }
    if (family == 2 && major != 2) goto close;
    success = read_model_body(model, &file, family, minor);
close:
    file_close(&file);
done:
    file_destruct(&file);
    if (success) model->fileName = path;
    else {
        model_res_clear(model);
        owned_text_clear(&path);
    }
    return (BYTE)success;
}

BYTE model_resource_load_v1(struct C3dModelRes *model, const char *name)
{ return load_model_resource(model, name, 1); }
BYTE model_resource_load_v2(struct C3dModelRes *model, const char *name)
{ return load_model_resource(model, name, 2); }
