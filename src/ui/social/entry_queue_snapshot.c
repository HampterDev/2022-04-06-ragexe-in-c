#include "ui/social/entry_queue_snapshot.h"
#include "ui/social/entry_queue_state.h"
#include "ui/social/entry_queue_windows.h"
#include "ui/window_access.h"
#include "core/object_type.h"
#include <string.h>

enum { QUEUE_BEGIN = 0x1964, QUEUE_END = 0x1965, QUEUE_ID = 0x196f, QUEUE_FLAG = 0x1970,
       QUEUE_NAME = 0x1971, QUEUE_REPLY = 0x1972, QUEUE_SELECTED = 0x1973,
       QUEUE_STATUS = 0x1974, QUEUE_ARG1 = 0x1975, QUEUE_ARG2 = 0x1976 };
struct QueueSnapshot {
    int id, selected, arg1, arg2;
    unsigned char in_queue;
    unsigned present;
    struct SnapshotSpan name, reply, status;
};
static size_t text_length(const char *text, size_t capacity)
{
    const char *zero = memchr(text, 0, capacity);
    return zero ? (size_t)(zero - text) : capacity;
}
int entry_queue_snapshot_save(const struct EntryQueueMgr *manager, struct ReplayBuffer *out)
{
    struct SnapshotWriter writer = {0};
    struct UIEntryQueueWnd *window;
    struct UIEntryQueueStandByWnd *standby;
    size_t length;
    if (!manager) return 0;
    snapshot_write(&writer, QUEUE_BEGIN, NULL, 0);
    if (manager->current) snapshot_write(&writer, QUEUE_ID, &manager->current->id, sizeof manager->current->id);
    snapshot_write(&writer, QUEUE_FLAG, &manager->in_queue, sizeof manager->in_queue);
    length = text_length(manager->queue_name, sizeof manager->queue_name);
    if (length) snapshot_write(&writer, QUEUE_NAME, manager->queue_name, length);
    length = text_length(manager->lobby_name, sizeof manager->lobby_name);
    if (length) snapshot_write(&writer, QUEUE_REPLY, manager->lobby_name, length);
    window = object_as_type(ui_window_find(ENTRY_QUEUE_WND_ID), OBJECT_ENTRY_QUEUE_WINDOW);
    if (window) {
        int selected = ui_entry_queue_wnd_get_selected(window);
        snapshot_write(&writer, QUEUE_SELECTED, &selected, sizeof selected);
    }
    standby = object_as_type(ui_window_find(ENTRY_QUEUE_STANDBY_WND_ID), OBJECT_ENTRY_QUEUE_STANDBY_WINDOW);
    if (standby) {
        struct EntryQueueStatusView status;
        entry_queue_standby_status(standby, &status);
        snapshot_write(&writer, QUEUE_ARG1, &status.arg1, sizeof status.arg1);
        snapshot_write(&writer, QUEUE_ARG2, &status.arg2, sizeof status.arg2);
        snapshot_write(&writer, QUEUE_STATUS, status.text, status.size);
    }
    snapshot_write(&writer, QUEUE_END, NULL, 0);
    return snapshot_finish(&writer, out);
}
static int parse_queue(struct QueueSnapshot *out, const void *data, size_t size)
{
    struct SnapshotReader reader;
    struct SnapshotRecord record;
    if (!snapshot_open(&reader, data, size, QUEUE_BEGIN)) return -1;
    while (snapshot_next(&reader, &record) == 1) {
        int *number = NULL;
        if (record.tag == QUEUE_END) return record.value.size ? -1 : (int)reader.offset;
        if (record.tag == QUEUE_BEGIN) return -1;
        switch (record.tag) {
        case QUEUE_ID: number = &out->id; break;
        case QUEUE_SELECTED: number = &out->selected; break;
        case QUEUE_ARG1: number = &out->arg1; break;
        case QUEUE_ARG2: number = &out->arg2; break;
        case QUEUE_FLAG:
            if (record.value.size != 1) return -1;
            out->in_queue = record.value.data[0]; break;
        case QUEUE_NAME: case QUEUE_REPLY:
            if (record.value.size > 24) return -1;
            if (record.tag == QUEUE_NAME) out->name = record.value;
            else out->reply = record.value;
            break;
        case QUEUE_STATUS: out->status = record.value; break;
        default: continue;
        }
        if (number) {
            if (record.value.size != sizeof *number) return -1;
            memcpy(number, record.value.data, sizeof *number);
        }
        out->present |= 1u << (record.tag - QUEUE_ID);
    }
    return -1;
}
#define PRESENT(tag) (parsed.present & (1u << ((tag) - QUEUE_ID)))
int entry_queue_snapshot_load(struct EntryQueueMgr *manager, const void *data, size_t size)
{
    struct QueueSnapshot parsed = {0};
    int used = parse_queue(&parsed, data, size);
    if (!manager || used < 0) return -1;
    if (PRESENT(QUEUE_ID)) {
        struct EntryQueueInfo *queue = entry_queue_find((unsigned)parsed.id);
        if (queue) manager->current = queue;
    }
    if (PRESENT(QUEUE_FLAG)) manager->in_queue = parsed.in_queue;
    if (PRESENT(QUEUE_NAME)) {
        memset(manager->queue_name, 0, sizeof manager->queue_name);
        memcpy(manager->queue_name, parsed.name.data, parsed.name.size);
    }
    if (PRESENT(QUEUE_REPLY)) {
        memset(manager->lobby_name, 0, sizeof manager->lobby_name);
        memcpy(manager->lobby_name, parsed.reply.data, parsed.reply.size);
    }
    if (PRESENT(QUEUE_SELECTED)) {
        struct UIWindow *window = object_as_type(ui_window_find(ENTRY_QUEUE_WND_ID), OBJECT_ENTRY_QUEUE_WINDOW);
        if (window) ui_window_send(window, NULL, 0x87, 0, parsed.selected, 0, 0);
    }
    if (PRESENT(QUEUE_STATUS)) {
        struct UIEntryQueueStandByWnd *window = object_as_type(ui_window_find(ENTRY_QUEUE_STANDBY_WND_ID), OBJECT_ENTRY_QUEUE_STANDBY_WINDOW);
        if (window) entry_queue_standby_restore(window, (const char *)parsed.status.data, parsed.status.size, parsed.arg1, parsed.arg2);
    }
    return used;
}
