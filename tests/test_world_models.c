#include "world/state.h"
#include "world/scene_objects.h"
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static size_t live_storage, allocations, fail_at;
static int fail_thread, thread_calls, priority_calls;
static HANDLE entered, release_factory;
static volatile LONG live_models, finished_factory;
static int reject_id, insertion_ids[128], insertions, made, destroyed;
static struct WorldLighting expected_light;

struct Guard { size_t bytes; unsigned magic; };
struct FakeModel { int id; unsigned magic; };

static void *storage_malloc(size_t bytes)
{
    struct Guard *guard;
    if (++allocations == fail_at) return NULL;
    guard = malloc(sizeof *guard + bytes + 8);
    assert(guard);
    guard->bytes = bytes; guard->magic = 0x12345678;
    memset((char *)(guard + 1) + bytes, 0x5a, 8);
    ++live_storage;
    return guard + 1;
}
static void storage_free(void *p)
{
    struct Guard *guard;
    size_t i;
    if (!p) return;
    guard = (struct Guard *)p - 1;
    assert(guard->magic == 0x12345678 && live_storage);
    for (i = 0; i < 8; ++i) assert(((unsigned char *)p)[guard->bytes + i] == 0x5a);
    guard->magic = 0; --live_storage; free(guard);
}
static void *storage_calloc(size_t count, size_t size)
{
    void *p = storage_malloc(count * size);
    if (p) memset(p, 0, count * size);
    return p;
}
static void *storage_realloc(void *p, size_t size)
{
    void *next = storage_malloc(size);
    if (!next) return NULL;
    if (p) {
        size_t previous = ((struct Guard *)p - 1)->bytes;
        memcpy(next, p, previous < size ? previous : size);
        storage_free(p);
    }
    return next;
}
static HANDLE WINAPI observed_thread(LPSECURITY_ATTRIBUTES security, SIZE_T stack,
    LPTHREAD_START_ROUTINE entry, LPVOID context, DWORD flags, LPDWORD id)
{
    ++thread_calls;
    return fail_thread ? NULL : CreateThread(security, stack, entry, context, flags, id);
}
static BOOL WINAPI observed_priority(HANDLE thread, int priority)
{
    ++priority_calls;
    return SetThreadPriority(thread, priority);
}
#define malloc storage_malloc
#define calloc storage_calloc
#define realloc storage_realloc
#define free storage_free
#define CreateThread observed_thread
#define SetThreadPriority observed_priority
#include "../src/world/models.c"
#undef SetThreadPriority
#undef CreateThread
#undef free
#undef realloc
#undef calloc
#undef malloc

static struct C3dActor *make_model(int id)
{
    struct FakeModel *model = malloc(sizeof *model);
    assert(model);
    model->id = id; model->magic = 0x12345678;
    InterlockedIncrement(&live_models);
    return (struct C3dActor *)model;
}
struct C3dActor *world_model_create(const struct RswModel *record,
    const struct WorldLighting *lighting, int deferred)
{
    assert(deferred == 1);
    assert(memcmp(lighting, &expected_light, sizeof *lighting) == 0);
    assert(record->pos[0] == (float)record->animType);
    assert(strcmp(record->fileName, "copied model") == 0);
    ++made;
    if (record->animType < 0) return NULL;
    if (record->animType == 99 && entered) {
        SetEvent(entered);
        assert(WaitForSingleObject(release_factory, 5000) == WAIT_OBJECT_0);
        InterlockedExchange(&finished_factory, 1);
    }
    return make_model(record->animType);
}
void world_model_destroy(struct C3dActor *model)
{
    struct FakeModel *value = (struct FakeModel *)model;
    assert(value && value->magic == 0x12345678);
    value->magic = 0;
    ++destroyed;
    InterlockedDecrement(&live_models);
    free(value);
}
int scene_graph_node_insert(struct SceneGraphNode *root, struct C3dActor *actor, int level)
{
    struct FakeModel *value = (struct FakeModel *)actor;
    assert(root && level == 0 && value && value->magic == 0x12345678);
    if (value->id == reject_id) return 0;
    assert(insertions < 128);
    insertion_ids[insertions++] = value->id;
    return 1;
}
static void queue(struct World *world, int id, float distance, size_t index)
{
    struct RswModel record = {0};
    record.animType = id; record.pos[0] = (float)id;
    strcpy(record.fileName, "copied model");
    assert(world_models_queue(world, &record, distance, index));
    memset(&record, 0xa5, sizeof record);
}
static void empty(const struct World *world)
{
    assert(!world->models.data && !world->models.count && !world->models.capacity);
    assert(!world->models.inserted && !world->models.pending && !world->modelLoader);
    assert(!live_storage && !live_models);
}
static void allocation_failures(void)
{
    struct World world = {0};
    struct RswModel record = {0};
    size_t i;
    for (i = 1; i <= 2; ++i) {
        allocations = 0; fail_at = i;
        assert(!world_models_prepare(&world, 1, &expected_light));
        empty(&world);
    }
    fail_at = 0;
    assert(!world_models_prepare(&world, SIZE_MAX, &expected_light));
    assert(!world_models_prepare(&world, (size_t)LONG_MAX + 1, &expected_light));
    assert(!world_models_prepare(&world, 1, NULL));
    empty(&world);
    assert(world_models_prepare(&world, 17, &expected_light));
    assert(!world_models_prepare(&world, 1, &expected_light));
    fail_at = allocations + 1;
    assert(!world_models_queue(&world, &record, 300, 0));
    fail_at = 0;
    for (i = 0; i < 16; ++i) queue(&world, (int)i, 300, i);
    fail_at = allocations + 1;
    assert(!world_models_queue(&world, &record, 300, 16));
    assert(world.modelLoader->count == 16);
    fail_at = 0;
    queue(&world, 16, 300, 16);
    assert(!world_models_queue(&world, &record, 300, 17));
    assert(!world_models_add(&world, NULL));
    world_models_clear(&world);
    world_models_clear(&world); empty(&world);
}
static void synchronous_fallback(void)
{
    struct World world = {0};
    struct WorldLighting light = expected_light;
    struct RswModel record = {0};
    union { uint32_t bits; float value; } nan = {0x7fc00000};
    int old_priority = priority_calls;
    assert(world_models_prepare(&world, 6, &light));
    assert(!world_models_queue(&world, &record, nan.value, 0));
    assert(world_models_add(&world, make_model(10)));
    queue(&world, 4, 400, 3); queue(&world, 2, 300, 2);
    queue(&world, 1, 300, 1); queue(&world, -1, 500, 4);
    queue(&world, 99, 600, 5);
    assert(!world_models_queue(&world, &record, 700, 6));
    memset(&light, 0xa5, sizeof light);
    fail_thread = 1;
    world_models_start(&world);
    assert(priority_calls == old_priority && !world.modelLoader);
    assert(world.models.count == 5 && world.models.inserted == 1 && world.models.pending == 4);
    assert(((struct FakeModel *)world.models.data[0])->id == 10);
    assert(!world_models_add(&world, (struct C3dActor *)&record));
    reject_id = 2; insertions = 0;
    world_models_process(&world);
    assert(insertions == 1 && insertion_ids[0] == 1);
    assert(world.models.inserted == 2 && world.models.pending == 3);
    world_models_wait(&world);
    assert(world.models.pending == 3);
    reject_id = -999;
    world_models_process(&world);
    assert(insertions == 4 && insertion_ids[1] == 2 && insertion_ids[2] == 4 && insertion_ids[3] == 99);
    assert(world.models.inserted == 5 && !world.models.pending);
    world_models_start(&world); world_models_process(&world);
    world_models_clear(&world); empty(&world);
    assert(world_models_prepare(&world, 0, &expected_light));
    old_priority = thread_calls;
    world_models_start(&world);
    assert(thread_calls == old_priority);
    world_models_clear(&world); empty(&world);
}
static DWORD WINAPI allow_factory(void *context)
{
    (void)context;
    assert(WaitForSingleObject(entered, 5000) == WAIT_OBJECT_0);
    SetEvent(release_factory);
    return 0;
}
static void asynchronous_load(void)
{
    struct World world = {0};
    HANDLE unblock;
    struct C3dActor **original;
    fail_thread = 0; insertions = 0; finished_factory = 0;
    entered = CreateEvent(NULL, TRUE, FALSE, NULL);
    release_factory = CreateEvent(NULL, TRUE, FALSE, NULL);
    assert(entered && release_factory);
    assert(world_models_prepare(&world, 3, &expected_light));
    original = world.models.data;
    queue(&world, 1, 300, 0); queue(&world, 99, 400, 1); queue(&world, 100, 500, 2);
    world_models_start(&world);
    assert(WaitForSingleObject(entered, 5000) == WAIT_OBJECT_0);
    assert(world.models.data == original);
    world_models_process(&world);
    assert(insertions == 1 && insertion_ids[0] == 1);
    assert(world.models.inserted == 1 && !world.models.pending);
    assert(!world_models_add(&world, (struct C3dActor *)original));
    unblock = CreateThread(NULL, 0, allow_factory, NULL, 0, NULL);
    assert(unblock);
    world_models_wait(&world);
    assert(finished_factory && world.models.count == 3 && world.models.pending == 2);
    assert(world.models.data == original);
    world_models_process(&world);
    assert(insertions == 3 && insertion_ids[1] == 99 && insertion_ids[2] == 100);
    world_models_clear(&world); empty(&world);
    assert(WaitForSingleObject(unblock, 5000) == WAIT_OBJECT_0);
    CloseHandle(unblock); CloseHandle(entered); CloseHandle(release_factory);
    entered = release_factory = NULL;
}
static void clear_joins(void)
{
    struct World world = {0};
    HANDLE unblock;
    finished_factory = 0;
    entered = CreateEvent(NULL, TRUE, FALSE, NULL);
    release_factory = CreateEvent(NULL, TRUE, FALSE, NULL);
    assert(entered && release_factory);
    assert(world_models_prepare(&world, 1, &expected_light));
    queue(&world, 99, 300, 0); world_models_start(&world);
    assert(WaitForSingleObject(entered, 5000) == WAIT_OBJECT_0);
    unblock = CreateThread(NULL, 0, allow_factory, NULL, 0, NULL); assert(unblock);
    world_models_clear(&world);
    assert(finished_factory); empty(&world);
    assert(WaitForSingleObject(unblock, 5000) == WAIT_OBJECT_0);
    CloseHandle(unblock); CloseHandle(entered); CloseHandle(release_factory);
    entered = release_factory = NULL;
}
int main(void)
{
    expected_light.direction[0] = 1; expected_light.diffuse[1] = 2;
    expected_light.ambient[2] = 3; reject_id = -999;
    allocation_failures(); synchronous_fallback(); asynchronous_load(); clear_joins();
    assert(made == 9 && destroyed == 9 && priority_calls == 2);
    puts("world models: owned copies, bounded storage, allocation failures, ordered publication, fallback, retry and joined cleanup passed");
    return 0;
}
