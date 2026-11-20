#ifndef ACTOR_SPRITE_DRAW_H
#define ACTOR_SPRITE_DRAW_H
struct RenderObject;
void render_object_render_shadow(struct RenderObject *object, const float *matrix, float zoom);
void render_object_render_sprite(struct RenderObject *object, const float *matrix, unsigned char look, float extra);
#endif
