#ifndef WORLD_LIFECYCLE_H
#define WORLD_LIFECYCLE_H

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
struct World;

struct World *world_construct(struct World *world);
void world_clear_map(struct World *world);
void world_delete(struct World *world);

#endif
