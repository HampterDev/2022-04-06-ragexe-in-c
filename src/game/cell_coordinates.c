#include "game/gamemode.h"
#include "game/cells.h"
#include <math.h>

void game_mode_cell_to_world(struct GameMode *self, int x, int y, float *world_x, float *world_z)
{
    const struct C3dAttr *attr = self->world->attr;
    float zoom = (float)attr->zoom;
    float half_cell = zoom * 8.0f * 0.0625f;

    *world_x = ((float)x - (float)attr->width * 0.5f) * zoom + half_cell;
    *world_z = ((float)y - (float)attr->height * 0.5f) * zoom + half_cell;
}

void game_mode_sub_cell_to_world(struct GameMode *self, int x, int y, int sub_x, int sub_y,
                                        float *world_x, float *world_z)
{
    const struct C3dAttr *attr = self->world->attr;
    float zoom = (float)attr->zoom;

    *world_x = ((float)x - (float)attr->width * 0.5f) * zoom + (float)sub_x * zoom * 0.0625f;
    *world_z = ((float)y - (float)attr->height * 0.5f) * zoom + (float)sub_y * zoom * 0.0625f;
}

void game_mode_world_to_cell(struct GameMode *self, float world_x, float world_z, int *x, int *y)
{
    const struct C3dAttr *attr = self->world->attr;
    float zoom = (float)attr->zoom;

    *x = (int)(float)floor((double)((float)(attr->width / 2) + world_x / zoom));
    *y = (int)(float)floor((double)((float)(attr->height / 2) + world_z / zoom));
}

void game_mode_world_to_sub_cell(struct GameMode *self, float world_x, float world_z,
                                        int *x, int *y, int *sub_x, int *sub_y)
{
    const struct C3dAttr *attr = self->world->attr;
    float zoom = (float)attr->zoom;
    float grid_x = world_x / zoom;
    float grid_z = world_z / zoom;
    float half_width = (float)(attr->width / 2);
    float half_height = (float)(attr->height / 2);

    *x = (int)(float)floor((double)(half_width + grid_x));
    *y = (int)(float)floor((double)(half_height + grid_z));
    *sub_x = (int)(float)floor((double)((half_width + grid_x - (float)*x) * 16.0f)) % 16;
    *sub_y = (int)(float)floor((double)((half_height + grid_z - (float)*y) * 16.0f)) % 16;
}
