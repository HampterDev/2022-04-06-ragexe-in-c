#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "render/anim_mesh.h"

static uint32_t random_state = 0x17a4932b;
static uint32_t next_value(void)
{
    random_state = random_state * 1664525u + 1013904223u;
    return random_state;
}

int main(int argc, char **argv)
{
    FILE *expected;
    unsigned scenario;
    assert(argc == 2);
#ifdef LEGACY_MESH_REFERENCE
    expected = fopen(argv[1], "wb");
#else
    expected = fopen(argv[1], "rb");
#endif
    assert(expected);
    for (scenario = 0; scenario < 128; ++scenario) {
        struct AnimMesh mesh = {0};
        Vec3 result[3 * 79], faceResult[79];
        int f, v, k;
        mesh.vertexCount = 7 + scenario % 31;
        mesh.faceCount = 1 + scenario % 79;
        mesh.positions = calloc((size_t)mesh.vertexCount, sizeof *mesh.positions);
        mesh.faces = calloc((size_t)mesh.faceCount, sizeof *mesh.faces);
        assert(mesh.positions && mesh.faces);
        for (v = 0; v < mesh.vertexCount; ++v) {
            mesh.positions[v].x = ((int)(next_value() % 2001) - 1000) * .0078125f;
            mesh.positions[v].y = ((int)(next_value() % 2001) - 1000) * .00390625f;
            mesh.positions[v].z = ((int)(next_value() % 2001) - 1000) * .015625f;
        }
        for (f = 0; f < mesh.faceCount; ++f) {
            struct AnimMeshFace *face = &mesh.faces[f];
            unsigned count = next_value() % 8;
            int *groups = count ? malloc(count * sizeof *groups) : NULL;
            for (k = 0; k < 3; ++k) face->v[k] = (uint16_t)(next_value() % mesh.vertexCount);
            for (k = 0; k < (int)count; ++k) groups[k] = (int)(next_value() % 5) - 2;
#ifdef LEGACY_MESH_REFERENCE
            face->groups.first = groups;
            face->groups.last = face->groups.end = count ? groups + count : NULL;
#else
            face->groups = groups;
            face->groupCount = count;
#endif
        }
#ifdef LEGACY_MESH_REFERENCE
        anim_mesh_smooth_normals(&mesh);
        assert(fwrite(mesh.faceNormals, sizeof *faceResult, mesh.faceCount, expected) == (size_t)mesh.faceCount);
        assert(fwrite(mesh.vertexNormals, sizeof *result, mesh.faceCount * 3, expected) == (size_t)mesh.faceCount * 3);
#else
        assert(anim_mesh_smooth_normals(&mesh));
        assert(fread(faceResult, sizeof *faceResult, mesh.faceCount, expected) == (size_t)mesh.faceCount);
        assert(fread(result, sizeof *result, mesh.faceCount * 3, expected) == (size_t)mesh.faceCount * 3);
        assert(!memcmp(faceResult, mesh.faceNormals, mesh.faceCount * sizeof *faceResult));
        assert(!memcmp(result, mesh.vertexNormals, mesh.faceCount * 3 * sizeof *result));
#endif
        for (f = 0; f < mesh.faceCount; ++f) {
#ifdef LEGACY_MESH_REFERENCE
            free(mesh.faces[f].groups.first);
#else
            free(mesh.faces[f].groups);
#endif
        }
        free(mesh.faces); free(mesh.positions); free(mesh.faceNormals); free(mesh.vertexNormals);
    }
#ifndef LEGACY_MESH_REFERENCE
    assert(fgetc(expected) == EOF);
#endif
    fclose(expected);
    puts("model mesh normals: 128 reference meshes match bit for bit, including duplicate corners/groups and NaNs");
    return 0;
}
