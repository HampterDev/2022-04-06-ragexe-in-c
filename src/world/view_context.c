#include "game/context.h"
#include "world/camera.h"
#include "world/actor_lookup.h"
#include <string.h>

int game_copy_view_matrix(float matrix[12])
{
    const struct View *view = game_current_view();
    if (!view)
        return 0;
    memcpy(matrix, view->viewMatrix, sizeof view->viewMatrix);
    return 1;
}

int game_copy_view_matrix_at(float matrix[12], float heading)
{
    struct View *view = game_current_view();
    if (!view)
        return 0;
    view_get_view_matrix_at(view, matrix, heading);
    return 1;
}

struct WorldVideoList *game_current_video_list(void)
{
    struct World *world = game_current_world();
    return world ? world->videoList : 0;
}

struct C3dGround15 *game_current_ground(void)
{
    struct World *world = game_current_world();
    return world ? world->ground : 0;
}

int game_copy_camera_position(float position[3])
{
    const struct View *view = game_current_view();
    if (!view)
        return 0;
    memcpy(position, view->from, sizeof view->from);
    return 1;
}

struct List *game_current_object_list(void)
{
    struct World *world = game_current_world();
    return world ? &world->gameObjects : 0;
}

struct SceneGraphNode *game_current_scene_root(void)
{
    struct World *world = game_current_world();
    int i;
    if (!world)
        return NULL;
    for (i = 0; i < 4; ++i)
        if (!world->root.child[i])
            return NULL;
    return &world->root;
}

struct GameActor *game_find_actor(int aid)
{
    struct World *world = game_current_world();
    return world ? world_get_game_actor_by_aid(world, (unsigned)aid) : 0;
}

const struct List *game_current_actor_list(void)
{
    struct World *world = game_current_world();
    return world ? &world->actors : 0;
}

void game_remove_object(void *object)
{
    struct World *world = game_current_world();
    if (world && object)
        world_remove_object(world, object);
}
