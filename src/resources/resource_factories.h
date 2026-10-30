#ifndef RESOURCES_RESOURCE_FACTORIES_H
#define RESOURCES_RESOURCE_FACTORIES_H

struct Res;

struct Res *granny_model_resource_create(void);
struct Res *model_resource_v1_create(void);
struct Res *model_resource_v2_create(void);
struct Res *world_resource_create(void);
struct Res *animation_resource_create(void);
struct Res *bitmap_resource_create(void);
struct Res *effect_resource_create(void);
struct Res *emblem_resource_create(void);
struct Res *gif_resource_create(void);
struct Res *ground_resource_create(void);
struct Res *imf_resource_create(void);
struct Res *palette_resource_create(void);
struct Res *rma_resource_create(void);
struct Res *sprite_resource_create(void);
struct Res *wave_resource_create(void);

#endif
