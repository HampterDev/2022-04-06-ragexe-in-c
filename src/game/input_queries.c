#include "core/compiler.h"
#include "game/gamemode.h"
#include "game/input.h"
#include "world/model_types.h"
#include "render/projection.h"
#include "render/renderer.h"
#include "world/view_types.h"
#include "world/ground_api.h"
#include "resources/attr_queries.h"
#include "world/pick_node.h"
#include "contents/mouse.h"
#include "world/ray_picker.h"

/* What the actors and the world's picking read of the game input: the shift key and the
   ground cell under the cursor. Apart from game/input.c, whose handling of the input reaches
   the windows, the popup menu and the game mode's messages, so that they do not. */

int g_shiftDown;

#define NO_HIT 999999.0f

RAGE_NOINLINE char game_mode_pick_ground_cell(struct GameMode *self, int *cell_x, int *cell_y)
{
    struct RayPicker blocks;
    struct Ray3 ray;
    Vec3 eye;
    size_t index;
    float best = NO_HIT, distance;
    char hit;
    int x, y;

    ray_picker_construct(&blocks);
    eye = (Vec3){self->view->from[0], self->view->from[1], self->view->from[2]};
    renderer_cursor_ray(g_renderer, g_mouse.x, g_mouse.y, &eye, self->view->invViewMatrix,
                        &ray);
    ray_picker_pick_ray(&blocks, &ray, &self->world->root);
    for (index = 0; index < blocks.count; ++index) {
        struct SceneGraphNode *block = blocks.hits[index];
        struct C3dAttr *ground = block->attr;
        for (y = block->attrCell[1]; y <= block->attrCell[3]; y++) {
            for (x = block->attrCell[0]; x <= block->attrCell[2]; x++) {
                distance = ground_ray_distance(ground, &ray, x, y);
                if (best > distance) {
                    best = distance;
                    *cell_x = x;
                    *cell_y = y;
                }
            }
        }
    }
    hit = NO_HIT > best;
    return hit;
}
