#include <windows.h>
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "owned_storage_test_allocator.h"
#include "platform/bink_api.h"
#include "skills/bink_player.h"
#include "skills/world_video.h"
#include "fileio/file.h"
#include "render/texture_cache.h"
#include "actor/types.h"

static CRITICAL_SECTION allocation_lock;
static void *guard_malloc(size_t size)
{
    void *p;
    EnterCriticalSection(&allocation_lock); p = checked_malloc(size);
    LeaveCriticalSection(&allocation_lock); return p;
}
static void guard_free(void *p)
{
    EnterCriticalSection(&allocation_lock); checked_free(p);
    LeaveCriticalSection(&allocation_lock);
}
static void *guard_calloc(size_t count, size_t size)
{
    void *p;
    assert(!size || count <= SIZE_MAX / size);
    p = guard_malloc(count * size);
    if (p) memset(p, 0, count * size);
    return p;
}
static void *guard_realloc(void *old, size_t size)
{
    void *p;
    EnterCriticalSection(&allocation_lock);
    p = checked_malloc(size);
    if (p && old) {
        size_t previous = ((struct Allocation *)old - 1)->size;
        memcpy(p, old, previous < size ? previous : size);
        checked_free(old);
    }
    LeaveCriticalSection(&allocation_lock);
    return p;
}

static int fail_file, fail_decoder, waiting, has_actor, effect_volume = 127;
static LONG movies, errors, closed_files, open_arrivals;
static unsigned copies, volume_calls, lookups;
static int last_volume;
static char last_path[4096], drawn_names[16][2048];
static unsigned char encoded[] = "fixture movie";
static HANDLE open_gate;
static struct WorldVideoList *active_list;
static struct RenderObject actor;
static struct Texture texture;
struct TexCache g_textureCache;

static struct BinkHandle *fake_open(const char *data, uint32_t flags)
{
    struct BinkHandle *handle;
    assert(data == (const char *)encoded && flags == 0x4104000);
    if (fail_decoder) return NULL;
    handle = guard_calloc(1, sizeof *handle);
    if (!handle) return NULL;
    handle->width = 32; handle->height = 16;
    handle->frames = 4; handle->frame_num = 1;
    InterlockedIncrement(&movies);
    if (open_gate) {
        if (InterlockedIncrement(&open_arrivals) == 4) SetEvent(open_gate);
        assert(WaitForSingleObject(open_gate, 5000) == WAIT_OBJECT_0);
    }
    return handle;
}
static void fake_close(struct BinkHandle *handle)
{ assert(handle); InterlockedDecrement(&movies); guard_free(handle); }
static const char *fake_error(void) { return "fixture movie failure"; }
static int32_t fake_wait(struct BinkHandle *handle) { assert(handle); return waiting; }
static int32_t fake_frame(struct BinkHandle *handle) { assert(handle); return 0; }
static int32_t fake_skip(struct BinkHandle *handle) { assert(handle); return 0; }
static void fake_next(struct BinkHandle *handle) { assert(handle); ++handle->frame_num; }
static void fake_volume(struct BinkHandle *handle, uint32_t track, int32_t volume)
{ assert(handle && !track); ++volume_calls; last_volume = volume; }

#define _BinkOpen fake_open
#define _BinkClose fake_close
#define _BinkGetError fake_error
#define _BinkWait fake_wait
#define _BinkDoFrame fake_frame
#define _BinkShouldSkip fake_skip
#define _BinkNextFrame fake_next
#define _BinkSetVolume fake_volume
#define malloc guard_malloc
#define calloc guard_calloc
#define realloc guard_realloc
#define free guard_free
#include "../src/skills/bink_decoder.c"
#include "../src/skills/bink_player.c"
#undef free
#undef realloc
#undef calloc
#undef malloc

void error_msg(const char *message, int flags)
{ assert(message && !flags); InterlockedIncrement(&errors); }
struct File *file_construct(struct File *file)
{ memset(file, 0, sizeof *file); return file; }
unsigned char file_open(struct File *file, const char *path, int mode,
                        unsigned char archive, unsigned char buffered)
{
    assert(file && path && !mode && archive && !buffered);
    EnterCriticalSection(&allocation_lock);
    assert(strlen(path) < sizeof last_path); strcpy(last_path, path);
    LeaveCriticalSection(&allocation_lock);
    if (fail_file) return 0;
    file->buffer = encoded; file->size = sizeof encoded; return 1;
}
void file_destruct(struct File *file)
{ assert(file); InterlockedIncrement(&closed_files); memset(file, 0, sizeof *file); }
struct Player *game_current_player(void)
{ return has_actor ? (struct Player *)&actor : NULL; }
int sound_effect_volume(void) { return effect_volume; }
float float_sqrt(float value) { return sqrtf(value); }
struct Texture *texture_cache_get(struct TexCache *cache, const char *name, int keyed, int full)
{
    assert(cache == &g_textureCache && !keyed && !full);
    assert(world_video_list_open(active_list, name, 1));
    if (lookups < 16) { assert(strlen(name) < sizeof drawn_names[0]); strcpy(drawn_names[lookups], name); }
    ++lookups; return &texture;
}
unsigned char bink_frame_to_texture(struct BinkPlayer *player, void *target)
{ assert(player && player->bink && target == &texture); ++copies; return 1; }

static void finish(void)
{
    world_video_list_destroy(active_list); active_list = NULL;
    world_video_list_destroy(NULL); world_video_list_update(NULL);
    assert(!movies && !outstanding);
}

static void opening_and_failures(void)
{
    struct WorldVideo *kept, *video;
    unsigned step, held, calls;
    int width = 1, height = 1;
    char name[1024];
    fail_at = allocation_calls + 1;
    assert(!world_video_list_create()); fail_at = 0;
    active_list = world_video_list_create(); assert(active_list);
    assert(!world_video_list_open(NULL, "a.bik", 1));
    assert(!world_video_list_open(active_list, NULL, 1));
    assert(!world_video_list_open(active_list, "", 1));
    assert(!world_video_dimensions(NULL, &width, &height) && !width && !height);
    kept = world_video_list_open(active_list, "영화.bik", 1); assert(kept);
    assert(!strcmp(last_path, "video\\영화.bik"));
    calls = allocation_calls;
    assert(world_video_list_open(active_list, "영화.bik", 0) == kept && calls == allocation_calls);
    assert(world_video_dimensions(kept, &width, &height) && width == 32 && height == 16);
    held = outstanding;
    for (step = 1; step <= 4; ++step) {
        fail_at = allocation_calls + step;
        assert(!world_video_list_open(active_list, "failed.bik", 1));
        fail_at = 0; assert(outstanding == held && movies == 1);
        assert(world_video_list_open(active_list, "영화.bik", 1) == kept);
    }
    fail_file = 1; assert(!world_video_list_open(active_list, "file.bik", 1)); fail_file = 0;
    fail_decoder = 1; assert(!world_video_list_open(active_list, "decoder.bik", 1)); fail_decoder = 0;
    assert(outstanding == held && movies == 1);
    video = world_video_list_open(active_list, "Movie.bik", 1); assert(video);
    assert(world_video_list_open(active_list, "movie.bik", 1) != video);
    memset(name, 'x', sizeof name); strcpy(name + sizeof name - 5, ".bik");
    video = world_video_list_open(active_list, name, 1); assert(video);
    assert(strlen(last_path) == strlen(name) + 6 && !strcmp(last_path + 6, name));
    name[0] = 'y';
    assert(world_video_list_open(active_list, last_path + 6, 1) == video);
    finish();
}

static int reference_volume(const Vec3 *positions, size_t count)
{
    float nearest = 250.0f;
    size_t i;
    for (i = 0; i < count; ++i) {
        float dx = positions[i].x - actor.pos.x;
        float dy = positions[i].y - actor.pos.y;
        float dz = positions[i].z - actor.pos.z;
        float distance = sqrtf(dy * dy + dx * dx + dz * dz);
        if (!(distance > nearest)) {
            dx = positions[i].x - actor.pos.x;
            dy = positions[i].y - actor.pos.y;
            dz = positions[i].z - actor.pos.z;
            nearest = sqrtf(dy * dy + dx * dx + dz * dz);
        }
    }
    return 150.0f > nearest ? (int)((float)effect_volume / 150.0f * (150.0f - nearest)) << 8 : 0;
}

static void frames_and_positions(void)
{
    struct WorldVideo *video;
    Vec3 positions[40], near_position = {10, 0, 0};
    unsigned i, before;
    int width, height;
    active_list = world_video_list_create(); assert(active_list);
    video = world_video_list_open(active_list, "loop.bik", 1); assert(video);
    has_actor = 1; memset(&actor.pos, 0, sizeof actor.pos);
    for (i = 0; i < 8; ++i) {
        positions[i].x = 300 + (float)i; positions[i].y = positions[i].z = 0;
        assert(world_video_add_position(video, positions[i]));
    }
    before = outstanding; fail_at = allocation_calls + 1;
    assert(!world_video_add_position(video, near_position)); fail_at = 0; assert(outstanding == before);
    world_video_list_update(active_list); assert(last_volume == 0);
    assert(world_video_add_position(video, near_position)); positions[8] = near_position;
    for (i = 9; i < 40; ++i) {
        positions[i].x = (float)i * 1.125f; positions[i].y = (float)i * -0.875f;
        positions[i].z = (float)i * 0.0625f;
        assert(world_video_add_position(video, positions[i]));
    }
    for (i = 0; i < 600; ++i) {
        actor.pos.x = (float)(i % 97) - 30.5f;
        actor.pos.y = (float)(i % 79) - 20.25f;
        actor.pos.z = (float)(i % 31) - 10.125f;
        effect_volume = (int)(i % 128);
        world_video_list_update(active_list);
        assert(last_volume == reference_volume(positions, 40));
    }
    waiting = 1; before = copies; world_video_list_update(active_list); assert(copies == before); waiting = 0;
    has_actor = 0; before = volume_calls; world_video_list_update(active_list); assert(volume_calls == before);
    finish();
    active_list = world_video_list_create(); assert(active_list);
    video = world_video_list_open(active_list, "once.bik", 0); assert(video);
    assert(world_video_add_position(video, near_position)); has_actor = 1;
    for (i = 0; i < 4; ++i) world_video_list_update(active_list);
    assert(!world_video_dimensions(video, &width, &height) && !width && !height && !movies);
    before = volume_calls; world_video_list_update(active_list); assert(volume_calls == before);
    finish();
    active_list = world_video_list_create(); assert(active_list); lookups = 0;
    assert(world_video_list_open(active_list, "first.bik", 1));
    assert(world_video_list_open(active_list, "second.bik", 1));
    world_video_list_update(active_list);
    assert(lookups == 2 && !strcmp(drawn_names[0], "first.bik") && !strcmp(drawn_names[1], "second.bik"));
    finish();
}

static struct WorldVideo *thread_videos[4];
static DWORD WINAPI add_on_worker(void *context)
{
    unsigned i, index = (unsigned)(uintptr_t)context;
    struct WorldVideo *video = world_video_list_open(active_list, "parallel.bik", 1);
    assert(video); thread_videos[index] = video;
    for (i = 0; i < 2000; ++i) {
        Vec3 position = {300.0f + (float)i, 0, 0};
        assert(world_video_add_position(video, position));
    }
    return 0;
}

static void workers(void)
{
    HANDLE threads[4]; unsigned i;
    active_list = world_video_list_create(); assert(active_list);
    open_arrivals = 0; open_gate = CreateEvent(NULL, TRUE, FALSE, NULL); assert(open_gate);
    for (i = 0; i < 4; ++i) {
        threads[i] = CreateThread(NULL, 0, add_on_worker, (void *)(uintptr_t)i, 0, NULL);
        assert(threads[i]);
    }
    while (WaitForMultipleObjects(4, threads, TRUE, 1) == WAIT_TIMEOUT)
        world_video_list_update(active_list);
    for (i = 0; i < 4; ++i) { assert(thread_videos[i] == thread_videos[0]); CloseHandle(threads[i]); }
    CloseHandle(open_gate); open_gate = NULL;
    assert(movies == 1 && open_arrivals == 4);
    assert(thread_videos[0]->position_count == 8000);
    world_video_list_update(active_list); assert(last_volume == 0);
    finish();
}

int main(void)
{
    InitializeCriticalSection(&allocation_lock);
    opening_and_failures(); frames_and_positions(); workers();
    assert(errors && closed_files && copies && volume_calls && !outstanding && !movies);
    DeleteCriticalSection(&allocation_lock);
    puts("world video: C ownership, every allocation failure, copied names, 600 volume references, frame order/end, concurrent publication and 8000 worker positions passed");
    return 0;
}
