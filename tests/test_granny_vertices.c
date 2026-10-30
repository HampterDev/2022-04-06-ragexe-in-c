#define GRANNY_IMPORT
#include <assert.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "resources/granny_res.h"

struct Allocation { unsigned char *data; size_t bytes; };
static struct Allocation allocations[16];
static int live_allocations, allocation_calls, fail_allocation;

static void *checked_calloc(size_t count, size_t size)
{
    unsigned char *data;
    int i;
    allocation_calls++;
    if (fail_allocation)
        return NULL;
    assert(count == 1 && size < 4096);
    for (i = 0; i < 16 && allocations[i].data; i++) {}
    assert(i < 16);
    data = calloc(1, size + 16);
    assert(data);
    memset(data + size, 0xa7, 16);
    allocations[i].data = data;
    allocations[i].bytes = size;
    live_allocations++;
    return data;
}

static void checked_free(void *ptr)
{
    int i, j;
    if (!ptr)
        return;
    for (i = 0; i < 16 && allocations[i].data != ptr; i++) {}
    assert(i < 16);
    for (j = 0; j < 16; j++)
        assert(allocations[i].data[allocations[i].bytes + j] == 0xa7);
    memset(ptr, 0xdd, allocations[i].bytes);
    free(ptr);
    allocations[i].data = NULL;
    live_allocations--;
}

#define calloc checked_calloc
#define free checked_free
#include "../src/resources/granny_mesh.c"
#undef calloc
#undef free

static granny_mesh source_mesh;
static int vertex_count, index_count, material_group[3], bone_indices[1];
static int rigid, missing_groups, deform_calls;
static struct GrannyMeshVertex source_vertices[8];
static float default_composite[16], alternate_composite[16];
static granny_world_pose *default_pose = (granny_world_pose *)1;
static granny_world_pose *alternate_pose = (granny_world_pose *)2;
static granny_world_pose *last_pose;

int __stdcall _GrannyGetMeshVertexCount(granny_mesh *mesh)
{ assert(mesh == &source_mesh); return vertex_count; }
int __stdcall _GrannyGetMeshIndexCount(granny_mesh *mesh)
{ assert(mesh == &source_mesh); return index_count; }
BYTE __stdcall _GrannyMeshIsRigid(granny_mesh *mesh)
{ assert(mesh == &source_mesh); return (BYTE)rigid; }
int *__stdcall _GrannyGetMeshTriangleGroups(granny_mesh *mesh)
{ assert(mesh == &source_mesh); return missing_groups ? NULL : material_group; }
int *__stdcall _GrannyGetMeshBindingToBoneIndices(granny_mesh_binding *binding)
{ assert(binding == (granny_mesh_binding *)3); return bone_indices; }
float *__stdcall _GrannyGetWorldPoseComposite4x4Array(granny_world_pose *pose)
{
    assert(pose == default_pose || pose == alternate_pose);
    last_pose = pose;
    return pose == default_pose ? default_composite : alternate_composite;
}
float *__stdcall _GrannyGetWorldPoseComposite4x4(granny_world_pose *pose, int bone)
{ assert((pose == default_pose || pose == alternate_pose) && bone == 5); return pose == default_pose ? default_composite : alternate_composite; }
void *__stdcall _GrannyGetMeshVertices(granny_mesh *mesh)
{ assert(mesh == &source_mesh); return source_vertices; }
int __stdcall _GrannyGetMeshBytesPerIndex(granny_mesh *mesh)
{ assert(mesh == &source_mesh); return 2; }
void *__stdcall _GrannyGetMeshIndices(granny_mesh *mesh)
{ assert(mesh == &source_mesh); return NULL; }
void __stdcall _GrannyDeformVertices(granny_mesh_deformer *deformer, const int *bones,
                                    const float *composite, int count, const void *from, void *to)
{
    struct GrannyMeshVertex *out = to;
    int i;
    assert(deformer == (granny_mesh_deformer *)4 && bones == bone_indices);
    assert(from == source_vertices && count == vertex_count && count <= 8);
    memcpy(out, from, (size_t)count * sizeof *out);
    for (i = 0; i < count; i++)
        out[i].pos.x += composite[12];
    deform_calls++;
}

int main(void)
{
    struct C3dGrannyModelRes resource = {0};
    struct GrannyResModel model = {0};
    struct GrannyResMesh mesh = {0};
    struct GrannyResTexture texture = {0};
    struct GrannyResTexture *materials[] = {&texture};
    struct GrannyHairPivot hair = {0};
    struct GrannyMeshPose pose = {0}, alternate = {0};
    struct GrannyVertexBuffer *first, *second;
    const struct GrannyMeshVertex *vertices;
    struct Texture *out_texture;
    unsigned short indices[] = {0, 1, 2}, *out_indices;
    float matrix[12];
    int triangles, i, calls;
    unsigned uv_bits = 0x3f123456;

    resource.model = &model;
    model.meshCount = 1;
    model.meshes = &mesh;
    pose.world = default_pose; pose.binding = (granny_mesh_binding *)3;
    alternate.world = alternate_pose; alternate.binding = (granny_mesh_binding *)3;
    mesh.mesh = &source_mesh;
    mesh.deformer = (granny_mesh_deformer *)4;
    mesh.indices = indices;
    mesh.materialCount = 1;
    mesh.textures = materials;
    texture.texture = (struct Texture *)&texture;
    source_mesh.Name = "body";
    vertex_count = 8;
    index_count = 3;
    bone_indices[0] = 5;
    for (i = 0; i < 16; i++) {
        default_composite[i] = (float)i;
        alternate_composite[i] = (float)(i + 20);
    }
    for (i = 0; i < 8; i++) {
        source_vertices[i].pos.x = (float)i;
        source_vertices[i].ny = 1.0f;
        memcpy(&source_vertices[i].u, &uv_bits, sizeof uv_bits);
    }
    first = granny_vertex_buffer_create(&resource, 0);
    second = granny_vertex_buffer_create(&resource, 0);
    assert(first && second && first != second && first->count == 8 && second->count == 8);
    for (i = 0; i < 8; i++) {
        struct GrannyMeshVertex zero = {0};
        assert(!memcmp(&first->vertices[i], &zero, sizeof zero));
    }
    vertices = c3d_granny_model_res_mesh_to_draw(&resource, 0, &out_texture, &out_indices,
                                           &triangles, matrix, &pose, first);
    assert(vertices == first->vertices && out_texture == texture.texture);
    assert(triangles == 1 && out_indices == indices && last_pose == default_pose);
    assert(!memcmp(matrix, &g_identityMatrix, sizeof matrix));
    assert(vertices[7].pos.x == 19.0f && !memcmp(&vertices[7].u, &uv_bits, sizeof uv_bits));
    vertices = c3d_granny_model_res_mesh_to_draw(&resource, 0, &out_texture, &out_indices,
                                           &triangles, matrix, &alternate, second);
    assert(vertices == second->vertices && vertices[7].pos.x == 39.0f);
    assert(first->vertices[7].pos.x == 19.0f && last_pose == alternate_pose);

    calls = deform_calls;
    for (i = 0; i < 3; i++) {
        struct GrannyMeshPose missing = pose;
        if (i == 0) missing.world = NULL;
        if (i == 1) missing.binding = NULL;
        assert(!c3d_granny_model_res_mesh_to_draw(&resource, 0, &out_texture, &out_indices,
                                           &triangles, matrix, i == 2 ? NULL : &missing, first));
        assert(!out_texture && !out_indices && !triangles && deform_calls == calls);
    }
    first->count = 7;
    assert(!c3d_granny_model_res_mesh_to_draw(&resource, 0, &out_texture, &out_indices,
                                       &triangles, matrix, &pose, first));
    assert(!out_texture && !out_indices && !triangles && deform_calls == calls);
    first->count = 8;
    assert(!c3d_granny_model_res_mesh_to_draw(&resource, 0, &out_texture, &out_indices,
                                       &triangles, matrix, &pose, NULL));
    mesh.deformer = NULL;
    assert(!c3d_granny_model_res_mesh_to_draw(&resource, 0, &out_texture, &out_indices,
                                       &triangles, matrix, &pose, first));
    assert(deform_calls == calls);
    mesh.deformer = (granny_mesh_deformer *)4;

    for (i = -1; i <= 1; i += 2) {
        material_group[0] = i;
        assert(c3d_granny_model_res_mesh_to_draw(&resource, 0, &out_texture, &out_indices,
                                          &triangles, matrix, &pose, first) == first->vertices);
        assert(!out_texture);
    }
    material_group[0] = 0;
    for (i = 0; i < 3; i++) {
        missing_groups = i == 0;
        mesh.textures = i == 1 ? NULL : materials;
        materials[0] = i == 2 ? NULL : &texture;
        assert(c3d_granny_model_res_mesh_to_draw(&resource, 0, &out_texture, &out_indices,
                                          &triangles, matrix, &pose, first));
        assert(!out_texture);
    }
    missing_groups = 0;
    mesh.textures = materials;
    materials[0] = &texture;

    rigid = 1;
    mesh.rigidVerts = source_vertices;
    source_mesh.Name = "hair_pivot";
    alternate.hair = &hair;
    vertices = c3d_granny_model_res_mesh_to_draw(&resource, 0, &out_texture, &out_indices,
                                           &triangles, matrix, &alternate, NULL);
    assert(vertices == source_vertices && out_texture == texture.texture);
    assert(c3d_granny_model_res_rigid_mesh_vertices(&resource, 0) == source_vertices);
    assert(!c3d_granny_model_res_rigid_mesh_vertices(&resource, -1));
    assert(!c3d_granny_model_res_rigid_mesh_vertices(&resource, 1));
    assert(!c3d_granny_model_res_rigid_mesh_vertices(NULL, 0));
    for (i = 0; i < 12; i++)
        assert(matrix[i] == alternate_composite[(i / 3) * 4 + i % 3]);
    assert(!memcmp(hair.matrix, matrix, sizeof matrix));
    rigid = 0;
    assert(!c3d_granny_model_res_rigid_mesh_vertices(&resource, 0));

    {
        struct GrannyMeshVertex anchors[65] = {0};
        Vec3 point = {-1.0f, -2.0f, -3.0f};
        anchors[64].pos.x = 12.5f;
        anchors[64].pos.y = -4.25f;
        anchors[64].pos.z = 99.0f;
        vertex_count = 65;
        assert(granny_mesh_vertex_position(&resource, 0, anchors, 64, &point));
        assert(!memcmp(&point, &anchors[64].pos, sizeof point));
        vertex_count = 64;
        assert(!granny_mesh_vertex_position(&resource, 0, anchors, 64, &point));
        assert(!granny_mesh_vertex_position(&resource, 0, anchors, -1, &point));
        assert(!granny_mesh_vertex_position(&resource, -1, anchors, 0, &point));
        assert(!granny_mesh_vertex_position(NULL, 0, anchors, 0, &point));
        assert(!granny_mesh_vertex_position(&resource, 0, NULL, 0, &point));
        assert(!memcmp(&point, &anchors[64].pos, sizeof point));
        vertex_count = 8;
    }

    for (i = -1; i <= 1; i += 2) {
        assert(!granny_vertex_buffer_create(&resource, i));
        assert(!c3d_granny_model_res_mesh_to_draw(&resource, i, &out_texture, &out_indices,
                                           &triangles, matrix, &pose, first));
        assert(!out_texture && !out_indices && !triangles);
    }
    assert(!granny_vertex_buffer_create(NULL, 0));
    assert(!c3d_granny_model_res_mesh_to_draw(NULL, 0, &out_texture, &out_indices,
                                       &triangles, matrix, &pose, first));
    model.meshCount = 0;
    assert(!granny_vertex_buffer_create(&resource, 0));
    model.meshCount = 1;
    calls = allocation_calls;
    vertex_count = -1; assert(!granny_vertex_buffer_create(&resource, 0));
    vertex_count = 0; assert(!granny_vertex_buffer_create(&resource, 0));
    vertex_count = INT_MAX; assert(!granny_vertex_buffer_create(&resource, 0));
    assert(allocation_calls == calls);
    vertex_count = 8;
    fail_allocation = 1;
    assert(!granny_vertex_buffer_create(&resource, 0));
    fail_allocation = 0;
    granny_vertex_buffer_destroy(first);
    granny_vertex_buffer_destroy(second);
    granny_vertex_buffer_destroy(NULL);
    assert(live_allocations == 0);
    puts("Granny vertices: actual vertex counts, independent actor buffers, bounded writes, rigid/deformed output, material bounds, effect anchor indexing and allocation failures passed.");
    return 0;
}
