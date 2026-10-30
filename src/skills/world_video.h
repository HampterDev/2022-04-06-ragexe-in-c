#ifndef SKILLS_WORLD_VIDEO_H
#define SKILLS_WORLD_VIDEO_H
#include "render/vec3.h"

struct WorldVideo;
struct WorldVideoList;

struct WorldVideoList *world_video_list_create(void);
void world_video_list_destroy(struct WorldVideoList *list);
struct WorldVideo *world_video_list_open(struct WorldVideoList *list,
                                       const char *name, int loop);
int world_video_add_position(struct WorldVideo *video, Vec3 position);
int world_video_dimensions(struct WorldVideo *video, int *width, int *height);
void world_video_list_update(struct WorldVideoList *list);
#endif
