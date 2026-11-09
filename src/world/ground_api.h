#ifndef WORLD_GROUND_API_H
#define WORLD_GROUND_API_H

#define WIN32_LEAN_AND_MEAN
#include "render/math3d.h"
#include "platform/win_types.h"
struct C3dGround15;
struct C3dAttr;
struct GndRes;
struct Texture;
struct GndCell;

struct GroundWaterSettings {
    float level;
    int type;
    float waveHeight, waveSpeed, wavePitch;
    int frameDelay;
};

struct C3dGround15 *ground_new(void);
void ground_free(struct C3dGround15 *ground);
void ground_set_attribute_map(struct C3dGround15 *ground, struct C3dAttr *attr);
void c3d_ground_get_cell_pos(const struct C3dGround15 *ground, float x, float z, int *cx, int *cy);

int c3d_ground15_create(struct C3dGround15 *self, int width, int height);
void c3d_ground15_clear(struct C3dGround15 *self);
void c3d_ground15_get_cell_colour(struct C3dGround15 *self, float x, float z, int *rgb);
BYTE c3d_ground15_get_attr_quad(struct C3dGround15 *self, int ax, int ay, float *quad);
BYTE c3d_ground15_get_attr_quad_half(struct C3dGround15 *self, int ax, int ay, float *quad);
void c3d_ground15_get_height_min_max(struct C3dGround15 *self, const int *cells,
    float *min, float *max);
void c3d_ground15_get_cell_height_min_max(struct C3dGround15 *self, int x, int y,
    float *min, float *max);
int c3d_ground15_get_vertex_colour(struct C3dGround15 *self, int ax, int ay,
    float *r, float *g, float *b);
void c3d_ground15_get_lightmap_colour(struct C3dGround15 *self, int ax, int ay,
    BYTE *c0, BYTE *c1, BYTE *c2, BYTE *c3);
void c3d_ground15_update_water(struct C3dGround15 *self);
void c3d_ground15_render(struct C3dGround15 *self, const float *view, const int *cells);
/* custom: only the shadow casters of render's meshes (render/rp_pool.h) */
void c3d_ground15_cast(struct C3dGround15 *self, const float *view, const int *cells);
void c3d_ground15_draw_attr_quad(struct C3dGround15 *self, const float *view, int ax,
    int ay, DWORD colour, struct Texture *texture, const float *uv);
void c3d_ground15_render_attr_tile(struct C3dGround15 *self, const float *view,
    int ax, int ay, DWORD colour);
void c3d_ground15_draw_attr_quad_raised(struct C3dGround15 *self, const float *view,
    int ax, int ay, DWORD colour, struct Texture *texture,
    const float *uv);
void c3d_ground15_draw_attr_marker(struct C3dGround15 *self, const float *view,
    int ax, int ay, DWORD colour, struct Texture *texture);
int c3d_ground15_load_water(struct C3dGround15 *self, const struct GndRes *gnd,
    const struct GroundWaterSettings *scene);
void c3d_ground15_build_vertex_colours(struct C3dGround15 *self);
int c3d_ground15_load(struct C3dGround15 *self, const struct GndRes *gnd,
    const float *lightDir, const float *diffuse, const float *ambient);
int c3d_ground15_get_block_index(struct C3dGround15 *self, int x, int y);

#endif
