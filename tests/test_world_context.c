#include "game/context.h"
#include "world/camera.h"
#include "world/actor_lookup.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>

static struct World *world;
static struct View *view;
static int lookup_calls, remove_calls, object;
static struct GameActor *found;

struct GameActor *world_get_game_actor_by_aid(struct World *owner, unsigned aid)
{
    assert(owner == world && aid == 42);
    ++lookup_calls;
    return found;
}

void world_remove_object(struct World *owner, void *value)
{
    assert(owner == world && value == &object);
    ++remove_calls;
}

struct World *game_current_world(void) { return world; }
struct View *game_current_view(void) { return view; }

int main(void)
{
    static struct World map;
    static struct View camera;
    float matrix[12], saved[12];
    float eye[3] = {10, 20, 30};
    int i;

    for (i = 0; i < 12; ++i)
        matrix[i] = saved[i] = (float)(i * 3 - 9);
    assert(!game_current_object_list());
    assert(!game_current_scene_root());
    assert(!game_current_actor_list());
    assert(!game_current_ground());
    assert(!game_find_actor(42) && lookup_calls == 0);
    game_remove_object(&object);
    assert(remove_calls == 0);
    assert(!game_copy_camera_position(eye) && eye[0] == 10 && eye[1] == 20 && eye[2] == 30);
    assert(!game_copy_view_matrix(matrix));
    assert(memcmp(matrix, saved, sizeof matrix) == 0);

    world = &map;
    assert(!game_current_scene_root());
    for (i = 0; i < 3; ++i)
        map.root.child[i] = &map.root;
    assert(!game_current_scene_root());
    map.root.child[3] = &map.root;
    assert(game_current_scene_root() == &map.root);
    assert(!game_current_ground());
    map.ground = (struct C3dGround15 *)&object;
    assert(game_current_ground() == (struct C3dGround15 *)&object);
    assert(game_current_object_list() == &map.gameObjects);
    assert(game_current_actor_list() == &map.actors);
    assert(!game_find_actor(42) && lookup_calls == 1);
    found = (struct GameActor *)&object;
    assert(game_find_actor(42) == found && lookup_calls == 2);
    game_remove_object(&object);
    game_remove_object(NULL);
    assert(remove_calls == 1);
    view = &camera;
    camera.from[0] = 1;
    camera.from[1] = 2;
    camera.from[2] = 3;
    assert(game_copy_camera_position(eye));
    assert(memcmp(eye, camera.from, sizeof eye) == 0);
    for (i = 0; i < 12; ++i)
        camera.viewMatrix[i] = (float)(i * 7 + 1);
    assert(game_copy_view_matrix(matrix));
    assert(memcmp(matrix, camera.viewMatrix, sizeof matrix) == 0);
    camera.viewMatrix[4] = -17.0f;
    assert(game_copy_view_matrix(matrix) && matrix[4] == -17.0f);

    world = NULL;
    assert(!game_current_scene_root());
    view = NULL;
    assert(!game_find_actor(42) && lookup_calls == 2);
    game_remove_object(&object);
    assert(remove_calls == 1 && !game_current_object_list());
    assert(!game_current_actor_list());
    assert(!game_copy_camera_position(eye) && eye[0] == 1 && eye[1] == 2 && eye[2] == 3);
    memcpy(saved, matrix, sizeof saved);
    assert(!game_copy_view_matrix(matrix));
    assert(memcmp(matrix, saved, sizeof matrix) == 0);
    puts("world context: absent world/camera, object lists and current view matrix passed");
    return 0;
}
