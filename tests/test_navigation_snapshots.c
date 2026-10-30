#include <assert.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "navi/navi_manager.h"
#include "navi/navi_search.h"
#include "replay/replay.h"
#include "replay/event.h"
#include "ui/quests/navigation.h"
#include "core/fatal.h"

static struct Allocation { unsigned char *base; size_t size; } allocations[256];
static int live, attempts, fail_at;
static int allocation_index(void *p)
{
    int i;
    for (i = 0; i < 256; ++i) if (allocations[i].base && allocations[i].base + 8 == p) return i;
    abort();
}
static void *allocate(size_t size)
{
    int i;
    if (++attempts == fail_at) return NULL;
    for (i = 0; i < 256 && allocations[i].base; ++i) {}
    assert(i < 256 && size < SIZE_MAX - 16);
    allocations[i].base = malloc(size + 16); assert(allocations[i].base);
    allocations[i].size = size;
    memset(allocations[i].base, 0xa5, size + 16); ++live;
    return allocations[i].base + 8;
}
static void release(void *p)
{
    int i, j;
    if (!p) return;
    i = allocation_index(p);
    for (j = 0; j < 8; ++j) {
        assert(allocations[i].base[j] == 0xa5);
        assert(allocations[i].base[8 + allocations[i].size + j] == 0xa5);
    }
    free(allocations[i].base); allocations[i].base = NULL; --live;
}
static void *resize(void *p, size_t size)
{
    void *next = allocate(size);
    if (next && p) {
        size_t old_size = allocations[allocation_index(p)].size;
        memcpy(next, p, size < old_size ? size : old_size); release(p);
    }
    return next;
}
void *mem_alloc(size_t size) { void *p = allocate(size); assert(p); return p; }
void mem_free(void *p)
{  release(p); }
__declspec(noreturn) void fatal_error(const char *message) { (void)message; abort(); }


struct NaviManager g_navi;
struct UIWindowMgr g_windowMgr;
static struct ReplayRecorder recorder;
static struct NaviMap maps[2];
static struct NaviMap *map_rows[2];
static struct NaviObject npc, mob;
static struct NaviObject *npc_rows[1], *mob_rows[1];
static struct NaviLink link;
static struct NaviLink *link_rows[1];
static struct PathCell path[2];
static int resets, closes, opens, updates, starts, reports, binds, refreshes, read_ok = 1;
static const void *chunk_data;
static size_t chunk_size;
static unsigned char event_bytes[65536];
static size_t event_size;
#ifdef NAVI_SNAPSHOT_ORACLE
static int object_type(struct NaviObject *object) { return object->type; }
static int object_sprite(struct NaviObject *object) { return object->sprite; }
static const char *object_name(struct NaviObject *object) { return string_cstr(&object->name); }
static const char *object_alias(struct NaviObject *object) { return string_cstr(&object->alias); }
static struct NaviMap *object_map(struct NaviObject *object) { return object->map; }
static struct NaviPoint *object_position(struct NaviObject *object, struct NaviPoint *out)
{ out->x = object->x; out->y = object->y; return out; }
static const struct NaviObjectVtbl object_table = {
    .get_type = object_type, .GetSprite = object_sprite, .GetName = object_name,
    .GetAlias = object_alias, .GetMap = object_map, .GetPos = object_position
};
#endif
void navi_manager_reset(struct NaviManager *manager, unsigned char all)
{
    assert(all); ++resets;
    search_results_clear(&manager->results);
    manager->routeLinks.last = manager->routeLinks.first;
    search_result_destroy(&manager->guided); search_result_init(&manager->guided, -1);
    search_result_destroy(&manager->destination); search_result_init(&manager->destination, -1);
    string_clear(&manager->searchText);
    manager->guideState = 0;
}
BYTE ui_window_mgr_post_quit1(struct UIWindowMgr *manager, int id)
{ assert(manager == &g_windowMgr && (id == NAVI_WINDOW_ID || id == 0x1a)); ++closes; return 1; }
struct UIWindow *ui_window_open(int id) { assert(id == NAVI_WINDOW_ID); ++opens; return NULL; }
unsigned char navi_manager_update_window(struct NaviManager *manager) { assert(manager == &g_navi); ++updates; return 1; }
int navi_manager_start_guide(struct NaviManager *manager) { assert(manager == &g_navi); ++starts; return 7; }
void navi_manager_report_status(struct NaviManager *manager, int status, int how)
{ assert(manager == &g_navi && status == 7 && how == NAVI_REPORT_CHAT); ++reports; }
unsigned char navi_manager_bind_world(struct NaviManager *manager) { assert(manager == &g_navi); ++binds; return 1; }
unsigned char navi_manager_set_target(struct NaviManager *manager, struct NaviMap *map, int x, int y)
{ manager->base.targetMap = map; manager->base.targetX = x; manager->base.targetY = y; return 1; }
void ui_navigation_v4_wnd_fill_results(struct UINavigationV4Wnd *window) { assert(window); ++refreshes; }
#ifndef NAVI_SNAPSHOT_ORACLE
void ui_navigation_refresh_results(void)
{ if (g_windowMgr.navigation_v4_wnd) ui_navigation_v4_wnd_fill_results((void *)g_windowMgr.navigation_v4_wnd); }
int navi_navigate(struct NaviManager *manager, const char *name, int kind, int flags, unsigned char silent, int x, int y, int id)
{ (void)manager; (void)name; (void)kind; (void)flags; (void)silent; (void)x; (void)y; (void)id; abort(); }
int navi_show_map(struct NaviManager *manager, const char *name, int x, int y)
{ (void)manager; (void)name; (void)x; (void)y; abort(); }
#endif
struct ReplayRecorder *replay_recorder_instance(void) { return &recorder; }
BYTE replay_recorder_write_packet(struct ReplayRecorder *owner, unsigned short size, const void *packet, int mode, DWORD time)
{
    assert(owner == &recorder && mode == REPLAY_TIME_ZERO && time == 0);
    assert(size <= sizeof event_bytes); memcpy(event_bytes, packet, size); event_size = size; return 1;
}

#ifndef NAVI_SNAPSHOT_ORACLE
#define malloc allocate
#define realloc resize
#define free release
#include "../src/replay/snapshot.c"
#include "../src/navi/snapshot_records.c"
#include "../src/navi/result_snapshot.c"
#include "../src/navi/navi_state.c"
#include "../src/game/replay_navigation.c"
#include "../src/ui/controls/window_record.c"
#undef malloc
#undef realloc
#undef free
int replay_read_chunk(struct ReplayRecorder *owner, int slot, struct ReplayBuffer *out)
{
    assert(owner == &recorder && slot == 0x17);
    if (!read_ok) return 0;
    if (chunk_size) {
        out->data = allocate(chunk_size); if (!out->data) return 0;
        memcpy(out->data, chunk_data, chunk_size); out->size = chunk_size;
    }
    return 1;
}
#else
#include "navigation_oracle_support.h"
#endif

static void set_text(struct String *out, const char *text)
{ string_init(out); string_assign(out, text, (unsigned)strlen(text)); }
static void setup_object(struct NaviObject *object, int type, const char *name, const char *alias, int x, int y)
{
    memset(object, 0, sizeof *object);
#ifdef NAVI_SNAPSHOT_ORACLE
    object->ops = &object_table;
#else
    object->kind = NAVI_OBJECT_ENTITY;
#endif
    object->type = type; object->sprite = 456; object->map = &maps[0]; object->x = x; object->y = y;
    set_text(&object->name, name); set_text(&object->alias, alias);
}
static void append_result(int kind, void *object, const char *name, int x, int y)
{
    struct SearchResult result;
    search_result_init(&result, kind); result.obj = object; result.x = x; result.y = y;
    string_assign(&result.name, name, (unsigned)strlen(name));
    search_results_push_copy(&g_navi.results, &result); search_result_destroy(&result);
}
static void setup(void)
{
    memset(&g_navi, 0, sizeof g_navi);
    memset(maps, 0, sizeof maps);
    set_text(&maps[0].name, "prontera"); set_text(&maps[1].name, "izlude");
    map_rows[0] = &maps[0]; map_rows[1] = &maps[1];
    g_navi.base.maps.first = map_rows; g_navi.base.maps.last = g_navi.base.maps.end = map_rows + 2;
    setup_object(&npc, 101, "Kafra service", "Storage and transport", 32, 45);
    setup_object(&mob, 300, "Poring", "Slime", 70, 80);
    setup_object(&link.base, 201, "Gate", "East gate", 90, 91);
#ifndef NAVI_SNAPSHOT_ORACLE
    link.base.kind = NAVI_OBJECT_LINK;
#endif
    link.destMap = &maps[1]; link.destX = 10; link.destY = 11;
    npc_rows[0] = &npc; mob_rows[0] = &mob; link_rows[0] = &link;
    maps[0].npcs.first = npc_rows; maps[0].npcs.last = maps[0].npcs.end = npc_rows + 1;
    maps[0].mobs.first = mob_rows; maps[0].mobs.last = maps[0].mobs.end = mob_rows + 1;
    maps[0].links.first = link_rows; maps[0].links.last = maps[0].links.end = link_rows + 1;
    vec_push(&g_navi.routeLinks, link_rows, sizeof *link_rows);
    search_result_init(&g_navi.guided, NAVI_RESULT_ROUTE_LINK); g_navi.guided.obj = &link;
    search_result_init(&g_navi.destination, NAVI_RESULT_NPC); g_navi.destination.obj = &npc;
    set_text(&g_navi.searchText, "prontera Kafra service");
    g_navi.routeUseKafra = 1; g_navi.routeUseScrolls = 2; g_navi.routeUseAirship = 3;
    g_navi.routePrNpcState = 4; g_navi.window_x = -1; g_navi.window_y = 20;
    g_navi.windowCollapsed = 1; g_navi.windowPage = 2; g_navi.searchFilter = 3;
    g_navi.pictureKind = 4; g_navi.showPath = 1; g_navi.walkDistance = 27;
    g_navi.arriveNearTarget = 1; g_navi.routeStep = 2; g_navi.guideState = NAVI_GUIDE_IDLE;
    path[0].x = 12; path[0].y = 13; path[1].x = 28; path[1].y = 29;
    g_navi.path.cells = path; g_navi.path.count = g_navi.path.capacity = 2;
    append_result(NAVI_RESULT_PLACE, &maps[0], "A named place", 17, 23);
    append_result(NAVI_RESULT_NPC, &npc, "An NPC", 32, 45);
    append_result(NAVI_RESULT_MOB, &mob, "A monster", -1, -1);
    closes = resets = opens = updates = starts = reports = binds = refreshes = 0;
}
static void cleanup(void)
{
    search_results_free(&g_navi.results); vec_free(&g_navi.routeLinks);
    search_result_destroy(&g_navi.guided); search_result_destroy(&g_navi.destination);
    string_free(&g_navi.searchText);
    string_free(&maps[0].name); string_free(&maps[1].name);
    string_free(&npc.name); string_free(&npc.alias);
    string_free(&mob.name); string_free(&mob.alias);
    string_free(&link.base.name); string_free(&link.base.alias);
    assert(!live);
}

#ifdef NAVI_SNAPSHOT_ORACLE
static void emit(const char *name, struct Vec *bytes)
{
    char path_name[260]; FILE *file;
    sprintf(path_name, "%s.bin", name); file = fopen(path_name, "wb"); assert(file);
    assert(fwrite(bytes->first, 1, (char *)bytes->last - (char *)bytes->first, file) == (size_t)((char *)bytes->last - (char *)bytes->first));
    assert(!fclose(file)); vec_free(bytes);
}
int main(void)
{
    struct Vec bytes;
    struct SearchResult result;
    setup();
    NaviManager_ObjectToRecord(&g_navi, &bytes, &npc); emit("navigation_object", &bytes);
    NaviManager_LinkToRecord(&g_navi, &bytes, &link); emit("navigation_link", &bytes);
    search_result_init(&result, NAVI_RESULT_NPC); result.obj = &npc;
    NaviManager_ResultToRecord(&g_navi, &bytes, result); emit("navigation_result", &bytes);
    NaviManager_Save(&g_navi, &bytes); emit("navigation_state", &bytes);
    search_results_clear(&g_navi.results);
    append_result(NAVI_RESULT_PLACE, &maps[0], "A place name long enough for owned storage", 17, 23);
    append_result(NAVI_RESULT_MAP, &maps[1], "Another map", -1, -1);
    NaviManager_ResultsToBytes(&g_navi, &bytes); emit("navigation_results", &bytes);
    cleanup(); puts("archived navigation writer fixtures generated"); return 0;
}
#else
#include "fixtures/navigation_snapshots_wire.h"
static void equals(const struct ReplayBuffer *buffer, const unsigned char *expected, size_t size)
{ assert(buffer->size == size && !memcmp(buffer->data, expected, size)); }
static void primitives(void)
{
    struct ReplayBuffer bytes = {0}; struct SearchResult result;
    struct NaviLink *found; size_t i;
    assert(navi_link_record_save(&link, &bytes)); equals(&bytes, navigation_link, sizeof navigation_link);
    for (i = 1; i < bytes.size; ++i) assert(!navi_link_record_valid((struct SnapshotSpan){bytes.data, i}));
    assert(navi_link_record_load(&g_navi, (struct SnapshotSpan){bytes.data, bytes.size}, &found) && found == &link);
    search_result_init(&result, NAVI_RESULT_NPC); result.obj = &npc;
    assert(navi_result_record_save(&result, &bytes)); equals(&bytes, navigation_result, sizeof navigation_result);
    search_result_destroy(&result);
    for (i = 0; i < bytes.size; ++i) assert(!navi_result_record_valid((struct SnapshotSpan){bytes.data, i}));
    assert(navi_result_record_load(&g_navi, (struct SnapshotSpan){bytes.data, bytes.size}, &result));
    assert(result.obj == &npc && result.x == -1 && result.y == -1 && !result.name.size); search_result_destroy(&result);
    bytes.data[20] = 0xff; bytes.data[21] = 0xff;
    assert(!navi_result_record_valid((struct SnapshotSpan){bytes.data, bytes.size}));
    replay_buffer_destroy(&bytes);
}
static void state_records(void)
{
    struct ReplayBuffer bytes = {0}, old = {0};
    struct NaviManager unchanged; size_t i; int count, baseline;
    assert(navi_state_save(&g_navi, &bytes)); equals(&bytes, navigation_state, sizeof navigation_state);
    unchanged = g_navi;
    for (i = 0; i < bytes.size; ++i) {
        assert(navi_state_load(&g_navi, bytes.data, i) == -1);
        assert(!memcmp(&g_navi, &unchanged, sizeof unchanged) && !closes && !resets);
    }
    old.data = allocate(3); old.size = 3; memcpy(old.data, "old", 3);
    count = attempts; assert(navi_state_save(&g_navi, &old)); count = attempts - count;
    replay_buffer_destroy(&old);
    for (i = 1; i <= (size_t)count; ++i) {
        old.data = allocate(3); old.size = 3; memcpy(old.data, "old", 3); baseline = live;
        fail_at = attempts + (int)i;
        assert(!navi_state_save(&g_navi, &old)); fail_at = 0;
        assert(old.size == 3 && !memcmp(old.data, "old", 3) && live == baseline); replay_buffer_destroy(&old);
    }
    assert(navi_state_load(&g_navi, bytes.data, bytes.size) == (int)bytes.size);
    assert(closes == 1 && resets == 1 && updates == 1 && opens == 1 && binds == 1);
    assert(g_navi.routeUseScrolls == 1 && g_navi.routeUseAirship == 2 && g_navi.routeUseKafra == 3);
    assert(VEC_COUNT(&g_navi.results, struct SearchResult) == 3);
    assert(g_navi.guided.kind == NAVI_RESULT_ROUTE_LINK && !g_navi.guided.obj);
    assert(g_navi.destination.obj == &npc && g_navi.base.targetX == npc.x && g_navi.base.targetY == npc.y);
    equals(&bytes, navigation_state, sizeof navigation_state);
    chunk_data = bytes.data; chunk_size = bytes.size; assert(navi_state_restore(&g_navi, &recorder));
    unchanged = g_navi; read_ok = 0; assert(!navi_state_restore(&g_navi, &recorder)); read_ok = 1;
    assert(!memcmp(&unchanged, &g_navi, sizeof unchanged));
    fail_at = attempts + 1; assert(!navi_state_restore(&g_navi, &recorder)); fail_at = 0;
    chunk_size = 1; assert(!navi_state_restore(&g_navi, &recorder));
    chunk_size = 0; assert(navi_state_restore(&g_navi, &recorder) && !g_navi.guided.obj && !g_navi.destination.obj);
    replay_buffer_destroy(&bytes);
}
static void result_lists(void)
{
    struct ReplayBuffer bytes = {0}; struct NaviManager unchanged; size_t i;
    search_results_clear(&g_navi.results);
    append_result(NAVI_RESULT_PLACE, &maps[0], "A place name long enough for owned storage", 17, 23);
    append_result(NAVI_RESULT_MAP, &maps[1], "Another map", -1, -1);
    assert(navi_results_save(&g_navi, &bytes)); equals(&bytes, navigation_results, sizeof navigation_results);
    unchanged = g_navi;
    for (i = 0; i < bytes.size; ++i) {
        assert(!navi_results_load(&g_navi, bytes.data, i)); assert(!memcmp(&unchanged, &g_navi, sizeof unchanged));
    }
    assert(navi_results_load(&g_navi, bytes.data, bytes.size));
    assert(!strcmp(string_cstr(&((struct SearchResult *)g_navi.results.first)[0].name), "A place name long enough for owned storage"));
    append_result(NAVI_RESULT_PLACE, NULL, "skipped", 0, 0);
    assert(navi_results_save(&g_navi, &bytes)); equals(&bytes, navigation_results, sizeof navigation_results);
    string_assign_cstr(&maps[0].name, "abcdefghijklmnopqrst");
    assert(navi_results_save(&g_navi, &bytes) && navi_results_load(&g_navi, bytes.data, bytes.size));
    string_assign_cstr(&maps[0].name, "abcdefghijklmnopqrstu");
    assert(!navi_results_save(&g_navi, &bytes));
    string_assign_cstr(&maps[0].name, "prontera");
    assert(navi_results_save(&g_navi, &bytes));
    recorder.state = 2; event_size = 0;
    assert(ui_window_record_bytes(NULL, 0x38f, bytes.data, bytes.size) == (int)(bytes.size + 6));
    assert(event_size == bytes.size + 6 && !memcmp(event_bytes + 6, bytes.data, bytes.size));
    g_windowMgr.navigation_v4_wnd = (void *)1;
    for (i = 0; i < event_size; ++i) assert(!game_replay_navigation_results(event_bytes, i));
    assert(game_replay_navigation_results(event_bytes, event_size) && refreshes == 1);
    recorder.writeFailed = 0; fail_at = attempts + 1;
    assert(ui_window_record_bytes(NULL, 0x38f, bytes.data, bytes.size) == -1 && recorder.writeFailed); fail_at = 0;
    assert(ui_window_record_bytes(NULL, 0x38f, bytes.data, UINT16_MAX) == -1);
    recorder.state = 0; assert(!ui_window_record_bytes(NULL, 0x38f, bytes.data, bytes.size));
    replay_buffer_destroy(&bytes);
}
static void malformed_records(void)
{
    struct SnapshotReader reader = {{navigation_state, sizeof navigation_state}, 0};
    struct SnapshotRecord record;
    struct NaviManager before = g_navi;
    while (snapshot_next(&reader, &record) == 1) {
        size_t prefix = (size_t)(record.value.data - navigation_state) - 6;
        size_t sizes[2], j;
        if (record.tag == NAVI_STATE_SEARCH_TEXT || record.tag == NAVI_STATE_RESULT ||
            record.tag == NAVI_STATE_GUIDED || record.tag == NAVI_STATE_DESTINATION || record.tag == NAVI_STATE_ROUTE_LINK) continue;
        sizes[0] = record.value.size ? record.value.size - 1 : 1;
        sizes[1] = record.value.size + 1;
        for (j = 0; j < 2; ++j) {
            unsigned char data[2048] = {0};
            uint32_t length = (uint32_t)sizes[j];
            size_t suffix = sizeof navigation_state - reader.offset;
            memcpy(data, navigation_state, prefix + 6);
            memcpy(data + prefix + 2, &length, 4);
            memcpy(data + prefix + 6, record.value.data, length < record.value.size ? length : record.value.size);
            memcpy(data + prefix + 6 + length, navigation_state + reader.offset, suffix);
            assert(navi_state_load(&g_navi, data, prefix + 6 + length + suffix) == -1);
            assert(!memcmp(&before, &g_navi, sizeof before));
        }
    }
    {
        unsigned char data[2048]; int32_t bad[] = {-1, INT_MAX}; size_t i;
        for (i = 0; i < 2; ++i) {
            memcpy(data, navigation_results, sizeof navigation_results); memcpy(data, &bad[i], 4);
            assert(!navi_results_load(&g_navi, data, sizeof navigation_results));
            memcpy(data, navigation_results, sizeof navigation_results); memcpy(data + 36, &bad[i], 4);
            assert(!navi_results_load(&g_navi, data, sizeof navigation_results));
            assert(!memcmp(&before, &g_navi, sizeof before));
        }
        memcpy(data, navigation_state, sizeof navigation_state); data[sizeof navigation_state] = 0;
        assert(navi_state_load(&g_navi, data, sizeof navigation_state + 1) == -1);
        memcpy(data, navigation_results, sizeof navigation_results); data[sizeof navigation_results] = 0;
        assert(!navi_results_load(&g_navi, data, sizeof navigation_results + 1));
    }
}
static void lookup_and_limits(void)
{
    struct ReplayBuffer bytes = {0}; struct SearchResult source, resolved;
    struct NaviLink *found; int before; char *long_name = malloc(INT16_MAX + 2);
    assert(long_name); memset(long_name, 'x', INT16_MAX + 1); long_name[INT16_MAX + 1] = 0;
    string_assign(&npc.name, long_name, INT16_MAX);
    before = attempts;
    search_result_init(&source, NAVI_RESULT_NPC); source.obj = &npc;
    assert(navi_result_record_save(&source, &bytes));
    assert(attempts == before + 1); before = attempts;
    assert(navi_result_record_load(&g_navi, (struct SnapshotSpan){bytes.data, bytes.size}, &resolved) && resolved.obj == &npc);
    search_result_destroy(&resolved); assert(attempts == before);
    string_assign(&npc.name, long_name, INT16_MAX + 1);
    assert(!navi_result_record_save(&source, &bytes));
    string_assign_cstr(&npc.name, "Kafra service"); free(long_name);
    source.kind = NAVI_RESULT_MOB; source.obj = &mob;
    assert(navi_result_record_save(&source, &bytes)); before = attempts;
    assert(navi_result_record_load(&g_navi, (struct SnapshotSpan){bytes.data, bytes.size}, &resolved) && resolved.obj == &mob);
    search_result_destroy(&resolved); assert(attempts == before);
    { int32_t type = 105; memcpy(bytes.data + 4, &type, 4); }
    assert(navi_result_record_load(&g_navi, (struct SnapshotSpan){bytes.data, bytes.size}, &resolved) && resolved.obj == &mob);
    search_result_destroy(&resolved);
    bytes.data[bytes.size - 1] = '?';
    assert(navi_result_record_load(&g_navi, (struct SnapshotSpan){bytes.data, bytes.size}, &resolved) && !resolved.obj);
    search_result_destroy(&resolved);
    assert(navi_link_record_save(&link, &bytes)); before = attempts;
    assert(navi_link_record_load(&g_navi, (struct SnapshotSpan){bytes.data, bytes.size}, &found) && found == &link && attempts == before);
    { int32_t type = 400; memcpy(bytes.data, &type, 4); }
    assert(navi_link_record_load(&g_navi, (struct SnapshotSpan){bytes.data, bytes.size}, &found) && !found);
    replay_buffer_destroy(&bytes); search_result_destroy(&source);
}
static void state_order_and_resume(void)
{
    int kinds[] = {NAVI_RESULT_PLACE, NAVI_RESULT_MAP, NAVI_RESULT_NPC, NAVI_RESULT_MOB};
    size_t i;
    for (i = 0; i < 4; ++i) {
        struct SnapshotWriter writer = {0}; struct ReplayBuffer bytes = {0}, result_bytes = {0};
        struct SearchResult destination; int ends[4] = {1, 2, 55, 66};
        int opened = opens, started = starts, bound = binds;
        search_result_init(&destination, kinds[i]); destination.obj = i < 2 ? (void *)&maps[0] : i == 2 ? (void *)&npc : (void *)&mob;
        assert(navi_result_record_save(&destination, &result_bytes));
        snapshot_write(&writer, NAVI_STATE_START, NULL, 0);
        put_int(&writer, NAVI_STATE_GUIDE_STATE, NAVI_GUIDE_ACTIVE);
        put_byte(&writer, NAVI_STATE_WINDOW_HIDDEN, (unsigned char)(i % 2));
        snapshot_write(&writer, 0x7000, "unknown", 7);
        put_int(&writer, NAVI_STATE_WINDOW_PAGE, 7); put_int(&writer, NAVI_STATE_WINDOW_PAGE, 9);
        snapshot_write(&writer, NAVI_STATE_DESTINATION, result_bytes.data, result_bytes.size);
        snapshot_write(&writer, NAVI_STATE_WALK_ENDS, ends, sizeof ends);
        snapshot_write(&writer, NAVI_STATE_END, NULL, 0); assert(snapshot_finish(&writer, &bytes));
        assert(navi_state_load(&g_navi, bytes.data, bytes.size) == (int)bytes.size);
        assert(g_navi.windowPage == 9 && starts == started + 1 && binds == bound + 1 && opens == opened + (i % 2 ? 0 : 1));
        assert(g_navi.base.targetMap == &maps[0]);
        assert(g_navi.base.targetX == (i == 0 ? 55 : i == 2 ? npc.x : -1));
        assert(g_navi.base.targetY == (i == 0 ? 66 : i == 2 ? npc.y : -1));
        replay_buffer_destroy(&bytes); replay_buffer_destroy(&result_bytes); search_result_destroy(&destination);
    }
}
int main(void)
{
    setup(); primitives(); state_records(); result_lists(); malformed_records(); lookup_and_limits(); state_order_and_resume(); cleanup();
    puts("navigation snapshots: archived wire bytes, bounded records, borrowed lookups, counts, full-width names, allocation rollback, immutable input and replay restoration passed");
    return 0;
}
#endif
