#include "model_mesh_test_support.h"

static void expect_unit(Vec3 v, float x, float y, float z)
{
    assert(fabsf(v.x-x) < 0.000001f && fabsf(v.y-y) < 0.000001f && fabsf(v.z-z) < 0.000001f);
}

static void check_formats(void)
{
    const float matrix[12] = {2,0,0, 0,2,0, 0,0,2, 3,4,5};
    int major, minor, textures;
    for (major = 1; major <= 2; ++major) {
        for (minor = 1; minor <= (major == 1 ? 5 : 3); ++minor) {
            for (textures = 0; textures <= 1; ++textures) {
                struct File file;
                struct AnimMesh *mesh;
                unsigned calls, failure;
                fixture(major, minor, textures, 3);
                file = stream(used);
                allocation_calls = 0;
                mesh = model_mesh_read(&file, major, minor, major == 1 ? matrix : NULL);
                assert(mesh && file.position == used);
                calls = allocation_calls;
                assert(mesh->vertexCount == 4 && mesh->faceCount == 3 && mesh->texVertCount == textures);
                assert(mesh->faces[0].texture == 0 && mesh->faces[1].texture == 1 && mesh->faces[2].texture == 2);
                assert(mesh->faces[0].groupCount == (major == 2 && minor >= 2 ? 3u : 1u));
                assert(mesh->faces[0].groups[0] == (major == 1 && minor == 1 ? 0 : 7));
                assert(mesh->positions[1].x == (major == 1 ? 5 : 1));
                if (textures) {
                    assert(mesh->texVerts[0].colour == (major == 1 && minor == 1 ? 0xffffffffu : 0x12345678u));
                    assert(mesh->texVerts[0].u == .25f && mesh->texVerts[0].v == .5f);
                }
                expect_unit(mesh->faceNormals[0], -1, 0, 0);
                expect_unit(mesh->faceNormals[1], 0, -1, 0);
                expect_unit(mesh->faceNormals[2], 0, 0, -1);
                expect_unit(mesh->vertexNormals[0], -.577350269f, -.577350269f, -.577350269f);
                anim_mesh_destroy(mesh);
                assert(!outstanding);
                for (failure = 1; failure <= calls; ++failure) {
                    file = stream(used); allocation_calls = 0; fail_at = failure;
                    assert(!model_mesh_read(&file, major, minor, major == 1 ? matrix : NULL));
                    assert(!outstanding);
                }
                fail_at = 0;
                for (file.size = 0; file.size < used; ++file.size) {
                    file.buffer = input; file.position = 0;
                    assert(!model_mesh_read(&file, major, minor, major == 1 ? matrix : NULL));
                    assert(!outstanding);
                }
            }
        }
    }
}

static void check_invalid_and_empty(void)
{
    struct File file;
    struct AnimMesh *mesh;
    int value;
    const int invalid[] = {-1, INT_MAX};
    size_t i;
    for (i = 0; i < sizeof invalid / sizeof invalid[0]; ++i) {
        used = 0; integer(invalid[i]); file = stream(used);
        assert(!model_mesh_read(&file, 2, 2, NULL) && !outstanding);
        fixture(2, 2, 1, 2);
        memcpy(input + count_offset, &invalid[i], 4); file = stream(used);
        assert(!model_mesh_read(&file, 2, 2, NULL) && !outstanding);
    }
    for (value = -1; value < 20; ++value) {
        fixture(2, 2, 1, 2); memcpy(input + face_offset, &value, 4); file = stream(used);
        assert(!model_mesh_read(&file, 2, 2, NULL) && !outstanding);
    }
    fixture(2, 2, 1, 2); input[face_offset + 4] = 4; file = stream(used);
    assert(!model_mesh_read(&file, 2, 2, NULL) && !outstanding);
    fixture(2, 2, 1, 2); input[face_offset + 10] = 1; file = stream(used);
    assert(!model_mesh_read(&file, 2, 2, NULL) && !outstanding);
    used = 0; integer(0); integer(0); integer(0); file = stream(used);
    mesh = model_mesh_read(&file, 2, 2, NULL);
    assert(mesh && !mesh->positions && !mesh->faces && !mesh->vertexNormals);
    anim_mesh_destroy(mesh); anim_mesh_destroy(NULL); assert(!outstanding);
    fixture(2, 2, 0, 0); file = stream(used);
    mesh = model_mesh_read(&file, 2, 2, NULL);
    assert(mesh && !mesh->faces[0].groups && !mesh->faces[0].groupCount);
    assert(isnan(mesh->vertexNormals[0].x));
    anim_mesh_destroy(mesh); assert(!outstanding);
}

static void check_rebuild_failure(void)
{
    struct File file;
    struct AnimMesh *mesh;
    Vec3 *faces, *corners, saved[9];
    unsigned failure, count;
    fixture(2, 2, 1, 2); file = stream(used);
    mesh = model_mesh_read(&file, 2, 2, NULL); assert(mesh);
    memcpy(saved, mesh->vertexNormals, sizeof saved);
    faces = mesh->faceNormals; corners = mesh->vertexNormals; count = outstanding;
    for (failure = 1; failure <= 4; ++failure) {
        allocation_calls = 0; fail_at = failure;
        assert(!anim_mesh_smooth_normals(mesh));
        assert(outstanding == count && mesh->faceNormals == faces && mesh->vertexNormals == corners);
        assert(!memcmp(saved, corners, sizeof saved));
    }
    fail_at = 0;
    assert(anim_mesh_smooth_normals(mesh));
    assert(!memcmp(saved, mesh->vertexNormals, sizeof saved));
    anim_mesh_destroy(mesh); assert(!outstanding);
}

int main(void)
{
    check_formats();
    check_invalid_and_empty();
    check_rebuild_failure();
    puts("model mesh: V1/V2 formats, transforms, normals, allocation failures and truncation cleanup pass");
    return 0;
}
