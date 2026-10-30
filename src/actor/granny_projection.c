#include <limits.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include "actor/granny_projection.h"
#include "render/rp_pool.h"

struct GrannyProjection {
    int capacity;
    struct TexDrawVertex vertices[];
};

struct GrannyProjection *granny_projection_create(int triangles)
{
    struct GrannyProjection *projection;
    size_t count;
    int i;

    if (triangles <= 0 || triangles > INT_MAX / 3)
        return NULL;
    count = (size_t)triangles * 3;
    if (count > (SIZE_MAX - sizeof *projection) / sizeof projection->vertices[0])
        return NULL;
    projection = malloc(sizeof *projection + count * sizeof projection->vertices[0]);
    if (!projection)
        return NULL;
    projection->capacity = (int)count;
    for (i = 0; i < projection->capacity; i++)
        tex_draw_vertex_construct(&projection->vertices[i]);
    return projection;
}

void granny_projection_destroy(struct GrannyProjection *projection)
{
    free(projection);
}

struct TexDrawVertex *granny_projection_vertices(struct GrannyProjection *projection, int triangles)
{
    if (!projection || triangles <= 0 || triangles > projection->capacity / 3)
        return NULL;
    return projection->vertices;
}

void granny_projection_submit(struct Renderer *renderer, struct GrannyProjection *projection,
                             struct Texture *texture, int triangles)
{
    const struct TexDrawVertex *vertices = granny_projection_vertices(projection, triangles);
    struct RPQuad *primitive;
    if (!vertices || !texture)
        return;
    primitive = rp_quad_construct(renderer_new_quad(renderer));
    primitive->nverts = triangles * 3;
    primitive->verts = renderer_new_tex_draw_vertices(renderer, primitive->nverts);
    memcpy(primitive->verts, vertices, (size_t)primitive->nverts * sizeof *vertices);
    primitive->texture = texture;
    renderer_add_prim(renderer, primitive, 0);
}
