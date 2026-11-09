#include "game/context.h"
#include "world/model.h"
#include "world/model_node_util.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>

DWORD g_nodeCornerColours[3];
struct Texture g_whiteTexture;
static struct World *current_world;
static int samples;

struct World *game_current_world(void) { return current_world; }

void c3d_ground15_get_cell_colour(struct C3dGround15 *ground, float x, float z, int *rgb)
{
    assert(ground == (struct C3dGround15 *)current_world->ground);
    assert(x == 10.0f && z == 30.0f);
    ++samples;
    rgb[0] = 200;
    rgb[1] = 20;
    rgb[2] = 30;
}

/* the corners' colours under the ground's light, as face_colours makes them */
static void expect_rgb(const int *light, const struct FaceInfo *info, int r, int g, int b)
{
    int i;
    for (i = 0; i < 3; ++i) {
        assert(ground_lit(light[i * 3], BYTE_R(info->colour[i])) == r);
        assert(ground_lit(light[i * 3 + 1], BYTE_G(info->colour[i])) == g);
        assert(ground_lit(light[i * 3 + 2], BYTE_B(info->colour[i])) == b);
    }
}

static void expect_ground(const int *light, int r, int g, int b)
{
    int i;
    for (i = 0; i < 3; ++i)
        assert(light[i * 3] == r && light[i * 3 + 1] == g && light[i * 3 + 2] == b);
}

int main(void)
{
    struct C3dNode node = {0};
    struct C3dActor actor = {0};
    struct AnimMesh mesh = {0};
    struct AnimMeshFace face = {0};
    struct AnimMeshTexVertex uv = {0};
    struct FaceInfo info = {0};
    struct Texture texture = {0};
    struct Texture *textures[] = {&texture};
    struct World world = {0};
    Vec3 normal[3] = {{0, 1, 0}, {0, 1, 0}, {0, 1, 0}};
    Vec3 vertex[1] = {{0, 0, 0}};
    const float direction[3] = {0, 1, 0};
    const float diffuse[3] = {0.25f, 0.5f, 0.0f};
    const float ambient[3] = {0.125f, 0.125f, 0.25f};
    const float black[3] = {0, 0, 0};
    int light[9], ground_tag;

    memset(light, 0xff, sizeof light);
    node.mesh = &mesh;
    node.actor = &actor;
    node.alpha = 1.0f;
    node.textures = textures;
    node.textureCount = 1;
    node.faceInfo = &info;
    node.groundLight = light;
    assert(run_texture(&node, 0) == &texture && run_texture(&node, 7) == &texture);
    node.textureCount = 0;
    assert(run_texture(&node, 0) == &g_whiteTexture);
    node.textureCount = 1; textures[0] = NULL;
    assert(run_texture(&node, 0) == &g_whiteTexture);
    textures[0] = &texture;
    memcpy(node.matrix, g_identityMatrix.m, sizeof node.matrix);
    mesh.positions = vertex;
    mesh.vertexCount = 1;
    mesh.faces = &face;
    mesh.faceCount = 1;
    mesh.texVerts = &uv;
    mesh.texVertCount = 1;
    mesh.faceNormals = normal;
    mesh.vertexNormals = normal;
    texture.base.width = texture.base.height = texture.reqWidth = texture.reqHeight = 1;
    actor.pos[0] = 10;
    actor.pos[1] = 20;
    actor.pos[2] = 30;

    c3d_node_light(&node, g_identityMatrix.m, direction, diffuse, ambient, 1);
    expect_rgb(light, &info, 95, 159, 63);
    expect_ground(light, 0, 0, 0);
    assert(samples == 0 && info.colour[0] == 0xff5f9f3f);

    c3d_node_light(&node, g_identityMatrix.m, direction, diffuse, ambient, 2);
    expect_rgb(light, &info, 95, 159, 63);
    uv.colour = 128; uv.u = 0.25f; uv.v = 0.75f;
    c3d_node_light(&node, g_identityMatrix.m, direction, diffuse, ambient, 2);
    expect_rgb(light, &info, 48, 80, 32);
    assert(info.uv[2][0] == 0.25f && info.uv[2][1] == 0.75f);
    mesh.texVertCount = 0; mesh.texVerts = NULL;
    c3d_node_light(&node, g_identityMatrix.m, direction, diffuse, ambient, 0);
    expect_rgb(light, &info, 255, 255, 255);
    assert(info.uv[2][0] == 1.0f && info.uv[2][1] == 1.0f);
    c3d_node_light(&node, g_identityMatrix.m, direction, diffuse, ambient, 1);
    expect_rgb(light, &info, 95, 159, 63);
    c3d_node_light(&node, g_identityMatrix.m, direction, diffuse, ambient, 2);
    expect_rgb(light, &info, 95, 159, 63);
    mesh.texVertCount = 1; mesh.texVerts = &uv; uv.colour = 0;

    current_world = &world;
    c3d_node_light(&node, g_identityMatrix.m, direction, black, ambient, 1);
    expect_rgb(light, &info, 31, 31, 63);
    assert(samples == 0);

    world.ground = (struct C3dGround15 *)&ground_tag;
    c3d_node_light(&node, g_identityMatrix.m, direction, diffuse, ambient, 1);
    expect_rgb(light, &info, 255, 179, 93);
    expect_ground(light, 200, 20, 30);
    assert(samples == 3);

    current_world = NULL;
    c3d_node_light(&node, g_identityMatrix.m, direction, black, black, 1);
    expect_rgb(light, &info, 0, 0, 0);
    assert(samples == 3);
    mesh.faceCount = mesh.vertexCount = mesh.texVertCount = 0;
    mesh.faces = NULL; mesh.positions = NULL; mesh.texVerts = NULL;
    node.faceInfo = NULL; node.groundLight = NULL;
    c3d_node_light(&node, g_identityMatrix.m, direction, diffuse, ambient, 1);
    c3d_node_light(&node, g_identityMatrix.m, direction, diffuse, ambient, 2);
    assert(samples == 3);
    puts("model lighting: absent ground, current base colors, clamping and teardown passed");
    return 0;
}
