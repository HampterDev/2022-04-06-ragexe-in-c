#include "skills/world_video.h"
#include "skills/bink_player.h"
#include "platform/bink_api.h"
#include "render/texture_cache.h"
#include "render/math3d.h"
#include "contents/sound.h"
#include "game/context.h"
#include "actor/types.h"
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#define WORLD_VIDEO_PREFIX "video\\"
#define WORLD_VIDEO_RANGE 150.0f
#define WORLD_VIDEO_FAR 250.0f

struct WorldVideo {
    struct WorldVideo *next;
    char *path;
    struct BinkPlayer player;
    CRITICAL_SECTION positions_lock;
    Vec3 *positions;
    size_t position_count, position_capacity;
    int width, height;
    volatile LONG active;
};

struct WorldVideoList {
    CRITICAL_SECTION lock;
    struct WorldVideo *first, *last;
};

static const char *world_video_name(const struct WorldVideo *video)
{
    return video->path + sizeof WORLD_VIDEO_PREFIX - 1;
}

static void world_video_free(struct WorldVideo *video)
{
    if (!video) return;
    bink_player_destruct(&video->player);
    free(video->positions);
    free(video->path);
    DeleteCriticalSection(&video->positions_lock);
    free(video);
}

static struct WorldVideo *world_video_new(const char *name, int loop)
{
    struct WorldVideo *video;
    size_t length = strlen(name), prefix = sizeof WORLD_VIDEO_PREFIX - 1;
    if (!length || length > SIZE_MAX - prefix - 1) return NULL;
    video = calloc(1, sizeof *video);
    if (!video) return NULL;
    if (!InitializeCriticalSectionAndSpinCount(&video->positions_lock, 0)) {
        free(video);
        return NULL;
    }
    bink_player_construct(&video->player);
    video->path = malloc(prefix + length + 1);
    if (!video->path) {
        world_video_free(video);
        return NULL;
    }
    memcpy(video->path, WORLD_VIDEO_PREFIX, prefix);
    memcpy(video->path + prefix, name, length + 1);
    if (!bink_player_open(&video->player, video->path, loop, 1)) {
        world_video_free(video);
        return NULL;
    }
    video->width = video->player.bink->width;
    video->height = video->player.bink->height;
    video->active = 1;
    return video;
}

struct WorldVideoList *world_video_list_create(void)
{
    struct WorldVideoList *list = calloc(1, sizeof *list);
    if (!list) return NULL;
    if (!InitializeCriticalSectionAndSpinCount(&list->lock, 0)) {
        free(list);
        return NULL;
    }
    return list;
}

void world_video_list_destroy(struct WorldVideoList *list)
{
    struct WorldVideo *video, *next;
    if (!list) return;
    for (video = list->first; video; video = next) {
        next = video->next;
        world_video_free(video);
    }
    DeleteCriticalSection(&list->lock);
    free(list);
}

static struct WorldVideo *world_video_find(struct WorldVideoList *list, const char *name)
{
    struct WorldVideo *video;
    for (video = list->first; video; video = video->next)
        if (!strcmp(world_video_name(video), name)) return video;
    return NULL;
}

struct WorldVideo *world_video_list_open(struct WorldVideoList *list,
                                       const char *name, int loop)
{
    struct WorldVideo *video, *candidate;
    if (!list || !name || !*name) return NULL;
    EnterCriticalSection(&list->lock);
    video = world_video_find(list, name);
    LeaveCriticalSection(&list->lock);
    if (video) return video;
    candidate = world_video_new(name, loop);
    if (!candidate) return NULL;
    EnterCriticalSection(&list->lock);
    video = world_video_find(list, name);
    if (!video) {
        video = candidate;
        if (list->last) list->last->next = video;
        else list->first = video;
        list->last = video;
    }
    LeaveCriticalSection(&list->lock);
    if (video != candidate) world_video_free(candidate);
    return video;
}

int world_video_add_position(struct WorldVideo *video, Vec3 position)
{
    Vec3 *positions;
    size_t capacity, limit = SIZE_MAX / sizeof *positions;
    if (!video) return 0;
    EnterCriticalSection(&video->positions_lock);
    if (video->position_count == video->position_capacity) {
        if (video->position_count == limit) goto failed;
        capacity = video->position_capacity ? video->position_capacity * 2 : 8;
        if (capacity < video->position_capacity || capacity > limit) capacity = limit;
        positions = realloc(video->positions, capacity * sizeof *positions);
        if (!positions) goto failed;
        video->positions = positions;
        video->position_capacity = capacity;
    }
    video->positions[video->position_count++] = position;
    LeaveCriticalSection(&video->positions_lock);
    return 1;
failed:
    LeaveCriticalSection(&video->positions_lock);
    return 0;
}

int world_video_dimensions(struct WorldVideo *video, int *width, int *height)
{
    *width = *height = 0;
    if (!video || !InterlockedCompareExchange(&video->active, 0, 0)) return 0;
    *width = video->width;
    *height = video->height;
    return 1;
}

static void world_video_update(struct WorldVideo *video)
{
    struct RenderObject *actor;
    struct Texture *texture;
    size_t i;
    float nearest = WORLD_VIDEO_FAR, dx, dy, dz, distance;
    int has_positions;
    if (bink_player_advance(&video->player) == 1) {
        texture = texture_cache_get(&g_textureCache, world_video_name(video), 0, 0);
        if (texture) bink_frame_to_texture(&video->player, texture);
    }
    if (!video->player.bink) {
        InterlockedExchange(&video->active, 0);
        return;
    }
    actor = (struct RenderObject *)game_current_player();
    if (!actor) return;
    EnterCriticalSection(&video->positions_lock);
    has_positions = video->position_count != 0;
    for (i = 0; i < video->position_count; ++i) {
        const Vec3 *position = &video->positions[i];
        dx = position->x - actor->pos.x;
        dy = position->y - actor->pos.y;
        dz = position->z - actor->pos.z;
        distance = float_sqrt(dy * dy + dx * dx + dz * dz);
        if (!(distance > nearest)) {
            dx = position->x - actor->pos.x;
            dy = position->y - actor->pos.y;
            dz = position->z - actor->pos.z;
            nearest = float_sqrt(dy * dy + dx * dx + dz * dz);
        }
    }
    LeaveCriticalSection(&video->positions_lock);
    if (!has_positions) return;
    if (WORLD_VIDEO_RANGE > nearest)
        _BinkSetVolume(video->player.bink, 0,
            (int)((float)sound_effect_volume() / WORLD_VIDEO_RANGE * (WORLD_VIDEO_RANGE - nearest)) << 8);
    else
        _BinkSetVolume(video->player.bink, 0, 0);
}

void world_video_list_update(struct WorldVideoList *list)
{
    struct WorldVideo *video;
    if (!list) return;
    EnterCriticalSection(&list->lock);
    video = list->first;
    LeaveCriticalSection(&list->lock);
    while (video) {
        world_video_update(video);
        EnterCriticalSection(&list->lock);
        video = video->next;
        LeaveCriticalSection(&list->lock);
    }
}
