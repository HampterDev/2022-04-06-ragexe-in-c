#ifndef ACTOR_GRANNY_PROJECTION_H
#define ACTOR_GRANNY_PROJECTION_H

struct GrannyProjection;
struct TexDrawVertex;
struct Texture;
struct Renderer;

struct GrannyProjection *granny_projection_create(int triangles);
void granny_projection_destroy(struct GrannyProjection *projection);
struct TexDrawVertex *granny_projection_vertices(struct GrannyProjection *projection, int triangles);
void granny_projection_submit(struct Renderer *renderer, struct GrannyProjection *projection,
                             struct Texture *texture, int triangles);

#endif
