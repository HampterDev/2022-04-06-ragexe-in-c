#include "navi/snapshot_records.h"
#include "navi/navi_manager.h"
#include "navi/navi_search.h"
#include "ui/quests/navigation.h"
#include "replay/replay.h"
#include <limits.h>
#include <string.h>

enum {
    NAVI_STATE_START = 0x1f40,
    NAVI_STATE_END,
    NAVI_STATE_ROUTE_OPTION_1,
    NAVI_STATE_ROUTE_OPTION_2,
    NAVI_STATE_ROUTE_OPTION_3,
    NAVI_STATE_ROUTE_PR_NPC_STATE,
    NAVI_STATE_RESULT,
    NAVI_STATE_GUIDED,
    NAVI_STATE_DESTINATION,
    NAVI_STATE_WINDOW_X,
    NAVI_STATE_WINDOW_Y,
    NAVI_STATE_WINDOW_COLLAPSED,
    NAVI_STATE_GUIDE_STATE,
    NAVI_STATE_WINDOW_PAGE,
    NAVI_STATE_SEARCH_FILTER,
    NAVI_STATE_SEARCH_TEXT,
    NAVI_STATE_PICTURE_KIND,
    NAVI_STATE_SHOW_PATH,
    NAVI_STATE_WALK_DISTANCE,
    NAVI_STATE_ARRIVE_NEAR_TARGET,
    NAVI_STATE_ROUTE_STEP,
    NAVI_STATE_WALK_ENDS,
    NAVI_STATE_ROUTE_LINK,
    NAVI_STATE_WINDOW_HIDDEN
};

static void put_byte(struct SnapshotWriter *writer, unsigned short tag, unsigned char value)
{
    snapshot_write(writer, tag, &value, sizeof value);
}

static void put_int(struct SnapshotWriter *writer, unsigned short tag, int value)
{
    snapshot_write(writer, tag, &value, sizeof value);
}

int navi_state_save(const struct NaviManager *self, struct ReplayBuffer *out)
{
    struct ReplayBuffer bytes = {0};
    struct SnapshotWriter writer = {0};
    unsigned i;

    if (!self || !out) return 0;
    snapshot_write(&writer, NAVI_STATE_START, NULL, 0);
    put_byte(&writer, NAVI_STATE_ROUTE_OPTION_1, self->routeUseKafra);
    put_byte(&writer, NAVI_STATE_ROUTE_OPTION_2, self->routeUseScrolls);
    put_byte(&writer, NAVI_STATE_ROUTE_OPTION_3, self->routeUseAirship);
    put_int(&writer, NAVI_STATE_ROUTE_PR_NPC_STATE, self->routePrNpcState);
    for (i = 0; i < VEC_COUNT(&self->routeLinks, struct NaviLink *); i++) {
        struct ReplayBuffer link = {0};

        if (!navi_link_record_save(((struct NaviLink **)self->routeLinks.first)[i], &link)) { writer.failed = 1; break; }
        snapshot_write(&writer, NAVI_STATE_ROUTE_LINK, link.data, link.size);
        replay_buffer_destroy(&link);
    }
    put_int(&writer, NAVI_STATE_WINDOW_X, self->window_x);
    put_int(&writer, NAVI_STATE_WINDOW_Y, self->window_y);
    put_byte(&writer, NAVI_STATE_WINDOW_COLLAPSED, self->windowCollapsed);
    put_int(&writer, NAVI_STATE_WINDOW_PAGE, self->windowPage);
    put_int(&writer, NAVI_STATE_SEARCH_FILTER, self->searchFilter);
    put_int(&writer, NAVI_STATE_PICTURE_KIND, self->pictureKind);
    put_byte(&writer, NAVI_STATE_SHOW_PATH, self->showPath);
    put_int(&writer, NAVI_STATE_WALK_DISTANCE, self->walkDistance);
    put_byte(&writer, NAVI_STATE_ARRIVE_NEAR_TARGET, self->arriveNearTarget);
    put_int(&writer, NAVI_STATE_ROUTE_STEP, self->routeStep);
    put_byte(&writer, NAVI_STATE_WINDOW_HIDDEN, self->windowHidden);
    put_int(&writer, NAVI_STATE_GUIDE_STATE, self->guideState);
    if (self->searchText.size)
        snapshot_write(&writer, NAVI_STATE_SEARCH_TEXT, string_cstr(&self->searchText), self->searchText.size);

    for (i = 0; i < VEC_COUNT(&self->results, struct SearchResult); i++) {
        if (!navi_result_record_save(&((struct SearchResult *)self->results.first)[i], &bytes)) { writer.failed = 1; break; }
        if (bytes.size)
            snapshot_write(&writer, NAVI_STATE_RESULT, bytes.data, bytes.size);
    }
    if (!navi_result_record_save(&self->guided, &bytes)) writer.failed = 1;
    if (bytes.size)
        snapshot_write(&writer, NAVI_STATE_GUIDED, bytes.data, bytes.size);
    if (!navi_result_record_save(&self->destination, &bytes)) writer.failed = 1;
    if (bytes.size)
        snapshot_write(&writer, NAVI_STATE_DESTINATION, bytes.data, bytes.size);
    {
        int count = ((int)self->path.count);

        if (count > 1) {
            const struct PathCell *cells = self->path.cells;
            int ends[4];

            ends[0] = cells[0].x;
            ends[1] = cells[0].y;
            ends[2] = cells[count - 1].x;
            ends[3] = cells[count - 1].y;
            snapshot_write(&writer, NAVI_STATE_WALK_ENDS, ends, 0x10);
        }
    }
    snapshot_write(&writer, NAVI_STATE_END, NULL, 0);
    replay_buffer_destroy(&bytes);
    return snapshot_finish(&writer, out);
}

static void resume_guide(struct NaviManager *self)
{
    if (self->guideState == NAVI_GUIDE_IDLE) {
        navi_manager_update_window(self);
    } else {
        if (self->guideState != NAVI_GUIDE_ACTIVE)
            return;
        navi_manager_report_status(self, navi_manager_start_guide(self), NAVI_REPORT_CHAT);
        if (self->windowHidden)
            return;
    }
    ui_window_open(NAVI_WINDOW_ID);
}

static int state_valid(const void *data, size_t size)
{
    struct SnapshotReader reader;
    struct SnapshotRecord record;
    if (!snapshot_open(&reader, data, size, NAVI_STATE_START)) return 0;
    while (snapshot_next(&reader, &record) == 1) {
        switch (record.tag) {
        case NAVI_STATE_START: return 0;
        case NAVI_STATE_END: return !record.value.size && reader.offset == size;
        case NAVI_STATE_ROUTE_OPTION_1: case NAVI_STATE_ROUTE_OPTION_2:
        case NAVI_STATE_ROUTE_OPTION_3: case NAVI_STATE_WINDOW_COLLAPSED:
        case NAVI_STATE_SHOW_PATH: case NAVI_STATE_ARRIVE_NEAR_TARGET:
        case NAVI_STATE_WINDOW_HIDDEN:
            if (record.value.size != 1) return 0;
            break;
        case NAVI_STATE_ROUTE_PR_NPC_STATE: case NAVI_STATE_WINDOW_X:
        case NAVI_STATE_WINDOW_Y: case NAVI_STATE_GUIDE_STATE:
        case NAVI_STATE_WINDOW_PAGE: case NAVI_STATE_SEARCH_FILTER:
        case NAVI_STATE_PICTURE_KIND: case NAVI_STATE_WALK_DISTANCE:
        case NAVI_STATE_ROUTE_STEP:
            if (record.value.size != 4) return 0;
            break;
        case NAVI_STATE_WALK_ENDS:
            if (record.value.size != 16) return 0;
            break;
        case NAVI_STATE_RESULT: case NAVI_STATE_GUIDED: case NAVI_STATE_DESTINATION:
            if (!navi_result_record_valid(record.value)) return 0;
            break;
        case NAVI_STATE_ROUTE_LINK:
            if (!navi_link_record_valid(record.value)) return 0;
            break;
        }
    }
    return 0;
}

static void restore_walk(struct NaviManager *self, const unsigned char *data)
{
    struct NaviObject *object = self->destination.obj;
    int ends[4];
    memcpy(ends, data, sizeof ends);
    if (object) {
        switch (self->destination.kind) {
        case NAVI_RESULT_NPC: {
            struct NaviPoint position;
            navi_object_position(object, &position);
            navi_manager_set_target(self, navi_object_map(object), position.x, position.y);
            break;
        }
        case NAVI_RESULT_MOB: navi_manager_set_target(self, navi_object_map(object), -1, -1); break;
        case NAVI_RESULT_MAP: navi_manager_set_target(self, self->destination.obj, -1, -1); break;
        case NAVI_RESULT_PLACE: navi_manager_set_target(self, self->destination.obj, ends[2], ends[3]); break;
        }
    }
    navi_manager_bind_world(self);
}

static void clear_state(struct NaviManager *self)
{
    ui_window_mgr_post_quit1(&g_windowMgr, NAVI_WINDOW_ID);
    navi_manager_reset(self, 1);
}

int navi_state_load(struct NaviManager *self, const void *data, size_t size)
{
    struct SnapshotReader reader;
    struct SnapshotRecord record;
    if (!self || !state_valid(data, size)) return -1;
    clear_state(self);
    snapshot_open(&reader, data, size, NAVI_STATE_START);
    while (snapshot_next(&reader, &record) == 1) {
#define READ_FIELD(field) memcpy(&self->field, record.value.data, sizeof self->field)
        switch (record.tag) {
        case NAVI_STATE_END: resume_guide(self); return (int)size;
        case NAVI_STATE_ROUTE_OPTION_1: READ_FIELD(routeUseScrolls); break;
        case NAVI_STATE_ROUTE_OPTION_2: READ_FIELD(routeUseAirship); break;
        case NAVI_STATE_ROUTE_OPTION_3: READ_FIELD(routeUseKafra); break;
        case NAVI_STATE_ROUTE_PR_NPC_STATE: READ_FIELD(routePrNpcState); break;
        case NAVI_STATE_WINDOW_X: READ_FIELD(window_x); break;
        case NAVI_STATE_WINDOW_Y: READ_FIELD(window_y); break;
        case NAVI_STATE_WINDOW_COLLAPSED: READ_FIELD(windowCollapsed); break;
        case NAVI_STATE_GUIDE_STATE: READ_FIELD(guideState); break;
        case NAVI_STATE_WINDOW_PAGE: READ_FIELD(windowPage); break;
        case NAVI_STATE_SEARCH_FILTER: READ_FIELD(searchFilter); break;
        case NAVI_STATE_PICTURE_KIND: READ_FIELD(pictureKind); break;
        case NAVI_STATE_SHOW_PATH: READ_FIELD(showPath); break;
        case NAVI_STATE_WALK_DISTANCE: READ_FIELD(walkDistance); break;
        case NAVI_STATE_ARRIVE_NEAR_TARGET: READ_FIELD(arriveNearTarget); break;
        case NAVI_STATE_ROUTE_STEP: READ_FIELD(routeStep); break;
        case NAVI_STATE_WINDOW_HIDDEN: READ_FIELD(windowHidden); break;
        case NAVI_STATE_SEARCH_TEXT: {
            const unsigned char *end = memchr(record.value.data, 0, record.value.size);
            size_t length = end ? (size_t)(end - record.value.data) : record.value.size;
            string_assign(&self->searchText, (const char *)record.value.data, (unsigned)length);
            break;
        }
        case NAVI_STATE_RESULT: case NAVI_STATE_GUIDED: case NAVI_STATE_DESTINATION: {
            struct SearchResult result;
            navi_result_record_load(self, record.value, &result);
            if (record.tag == NAVI_STATE_RESULT) {
                if (result.obj) search_results_push_copy(&self->results, &result);
            } else {
                search_result_assign(record.tag == NAVI_STATE_GUIDED ? &self->guided : &self->destination, &result);
            }
            search_result_destroy(&result);
            break;
        }
        case NAVI_STATE_ROUTE_LINK: {
            struct NaviLink *link;
            navi_link_record_load(self, record.value, &link);
            if (!link) break;
            vec_push(&self->routeLinks, &link, sizeof link);
            break;
        }
        case NAVI_STATE_WALK_ENDS: restore_walk(self, record.value.data); break;
        }
#undef READ_FIELD
    }
    return -1;
}

int navi_state_restore(struct NaviManager *self, struct ReplayRecorder *recorder)
{
    struct ReplayBuffer chunk = {0};
    int loaded;
    if (!self || !recorder) return 0;
    loaded = replay_read_chunk(recorder, 0x17, &chunk);
    if (loaded) {
        if (!chunk.size) clear_state(self);
        else loaded = navi_state_load(self, chunk.data, chunk.size) >= 0;
    }
    replay_buffer_destroy(&chunk);
    return loaded;
}
