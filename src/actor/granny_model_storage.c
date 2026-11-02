#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include "actor/granny_model_storage.h"
#include "fileio/resource_api.h"

struct GrannyModelState *granny_model_state_create(struct C3dGrannyModelRes *resource,
                                                   struct C3dGrannyModelRes *base)
{
    struct GrannyModelState *model;
    int count, i;

    if (!resource || !resource->model || !resource->skeleton ||
        (base && !base->skeleton))
        return NULL;
    count = resource->model->meshCount;
    if (count < 0 || (count && !resource->model->meshes) ||
        (size_t)count > (SIZE_MAX - sizeof *model) / sizeof model->meshes[0])
        return NULL;
    model = calloc(1, sizeof *model + (size_t)count * sizeof model->meshes[0]);
    if (!model)
        return NULL;
    model->resource = resource;
    model->bounds = resource->bounds;
    model->base_resource = base;
    model->mesh_count = count;
    res_lock(&resource->res);
    if (base)
        res_lock(&base->res);
    else {
        model->playback = granny_playback_create(resource);
        if (!model->playback)
            goto fail;
    }
    for (i = 0; i < count; i++) {
        struct GrannyResMesh *source = &resource->model->meshes[i];
        struct GrannyMeshState *mesh = &model->meshes[i];
        int vertices, indices;

        if (!source->mesh)
            goto fail;
        vertices = _GrannyGetMeshVertexCount(source->mesh);
        indices = _GrannyGetMeshIndexCount(source->mesh);
        if (vertices < 0 || indices < 0 || (indices && (!vertices || !source->indices)))
            goto fail;
        if (!vertices)
            continue;
        mesh->binding = _GrannyNewMeshBinding(source->mesh, resource->skeleton,
                                              base ? base->skeleton : resource->skeleton);
        if (!mesh->binding)
            goto fail;
        if (indices >= 3) {
            mesh->projection = granny_projection_create(indices / 3);
            if (!mesh->projection)
                goto fail;
        }
        if (!_GrannyMeshIsRigid(source->mesh)) {
            mesh->vertices = granny_vertex_buffer_create(resource, i);
            if (!mesh->vertices)
                goto fail;
        }
    }
    return model;

fail:
    granny_model_state_destroy(model);
    return NULL;
}

void granny_model_state_destroy(struct GrannyModelState *model)
{
    int i;

    if (!model)
        return;
    for (i = 0; i < model->mesh_count; i++) {
        struct GrannyMeshState *mesh = &model->meshes[i];
        granny_projection_destroy(mesh->projection);
        granny_vertex_buffer_destroy(mesh->vertices);
        if (mesh->binding)
            _GrannyFreeMeshBinding(mesh->binding);
    }
    granny_playback_destroy(model->playback);
    if (model->base_resource)
        res_unlock(&model->base_resource->res);
    res_unlock(&model->resource->res);
    free(model);
}

const struct GrannyMeshVertex *granny_model_prepare_mesh(struct GrannyModelState *model, int index,
                                                        granny_world_pose *pose,
                                                        struct GrannyHairPivot *hair)
{
    struct GrannyMeshState *mesh;
    struct GrannyMeshPose input;

    if (!model || index < 0 || index >= model->mesh_count)
        return NULL;
    mesh = &model->meshes[index];
    input.world = pose ? pose : granny_playback_world_pose(model->playback);
    input.binding = mesh->binding;
    input.hair = hair;
    return c3d_granny_model_res_mesh_to_draw(model->resource, index, &mesh->texture,
                                       &mesh->indices, &mesh->triangles, mesh->matrix,
                                       &input, mesh->vertices);
}

const struct GrannyMeshVertex *granny_model_cached_vertices(struct GrannyModelState *model, int index)
{
    const struct GrannyMeshVertex *vertices;
    struct GrannyMeshState *mesh;

    if (!model || index < 0 || index >= model->mesh_count)
        return NULL;
    mesh = &model->meshes[index];
    vertices = c3d_granny_model_res_rigid_mesh_vertices(model->resource, index);
    return vertices ? vertices : mesh->vertices ? mesh->vertices->vertices : NULL;
}

int granny_model_find_hair_pivot(const struct GrannyModelState *model)
{
    int i;

    if (!model)
        return -1;
    for (i = 0; i < model->mesh_count; i++) {
        const char *name = model->resource->model->meshes[i].mesh->Name;
        if (name && strstr(name, "hair_pivot"))
            return i;
    }
    return -1;
}
