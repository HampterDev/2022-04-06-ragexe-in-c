#include <stdlib.h>
#include "ui/record_tags.h"
#include "replay/replay.h"
#include "ui/window_access.h"
#include "ui/wndmgr/wndmgr.h"
#include <string.h>

#define WM_FLAG_RECORDED 7

int ui_window_mgr_get_option_records(struct UIWindowMgr *self, struct ReplayBuffer *out)
{
    struct SnapshotWriter writer = {0};
    snapshot_write(&writer, REC_OPTIONS_BEGIN, NULL, 0);
    snapshot_write(&writer, REC_DRAG_ALL, &self->drag_all, sizeof self->drag_all);
    snapshot_write(&writer, REC_OPTIONS_END, NULL, 0);
    return snapshot_finish(&writer, out);
}

int ui_window_mgr_load_option_records(struct UIWindowMgr *self, const void *data, size_t size)
{
    struct SnapshotReader reader;
    struct SnapshotRecord record;
    int used = ui_window_records_open(&reader, data, size, REC_OPTIONS_BEGIN);
    if (!self || used < 0) return -1;
    while (snapshot_next(&reader, &record) == 1 && record.tag != REC_OPTIONS_END)
        if (record.tag == REC_DRAG_ALL)
            memcpy(&self->drag_all, record.value.data, sizeof self->drag_all);
    return used;
}

int ui_window_mgr_get_empty_records(struct UIWindowMgr *self, struct ReplayBuffer *out)
{
    (void)self;
    struct SnapshotWriter writer = {0};
    snapshot_write(&writer, REC_EMPTY_BEGIN, NULL, 0);
    snapshot_write(&writer, REC_EMPTY_END, NULL, 0);
    return snapshot_finish(&writer, out);
}

int ui_window_mgr_skip_empty_records(struct UIWindowMgr *self, const void *data, size_t size)
{
    struct SnapshotReader reader;
    (void)self;
    return ui_window_records_open(&reader, data, size, REC_EMPTY_BEGIN);
}

int ui_window_mgr_get_window_records(struct UIWindowMgr *self, struct ReplayBuffer *out)
{
    struct SnapshotWriter writer = {0};
    struct ListNode *node;
    snapshot_write(&writer, REC_WINDOWS_BEGIN, NULL, 0);
    for (node = self->windows.head->next; node != self->windows.head; node = node->next) {
        struct UIWindow *window = *LIST_VALUE(node, struct UIWindow *);
        struct ReplayBuffer records = {0};
        if ((unsigned)window->id > 0x165 || !self->window_flags[window->id][WM_FLAG_RECORDED])
            continue;
        if (!ui_window_get_records(window, &records) ||
            ui_window_records_validate(records.data, records.size) != (int)records.size) writer.failed = 1;
        else snapshot_write(&writer, (uint16_t)(REC_WINDOW_BASE + window->id), records.data, records.size);
        replay_buffer_destroy(&records);
        if (writer.failed) break;
    }
    snapshot_write(&writer, REC_WINDOWS_END, NULL, 0);
    return snapshot_finish(&writer, out);
}

int ui_window_mgr_load_window_records(struct UIWindowMgr *self, const void *data, size_t size)
{
    struct SnapshotReader reader;
    struct SnapshotRecord record;
    unsigned char *copy = NULL;
    size_t largest = 0;
    int used = ui_window_records_open(&reader, data, size, REC_WINDOWS_BEGIN);
    if (!self || used < 0) return -1;
    while (snapshot_next(&reader, &record) == 1 && record.tag != REC_WINDOWS_END)
        if ((unsigned)(record.tag - REC_WINDOW_BASE) < REC_WINDOW_IDS && record.value.size > largest)
            largest = record.value.size;
    if (largest && !(copy = malloc(largest))) return -1;
    snapshot_open(&reader, data, (size_t)used, REC_WINDOWS_BEGIN);
    while (snapshot_next(&reader, &record) == 1 && record.tag != REC_WINDOWS_END) {
        struct UIWindow *window;
        unsigned id = (unsigned)(record.tag - REC_WINDOW_BASE);
        if (id >= REC_WINDOW_IDS) continue;
        window = ui_window_mgr_make_window(self, (int)id);
        if (!window || !record.value.size) continue;
        memcpy(copy, record.value.data, record.value.size);
        ui_window_send_msg(window, NULL, UI_MSG_LOAD_RECORDS, (int)copy, (int)record.value.size, 0, 0);
    }
    free(copy);
    return used;
}

int ui_window_snapshots_restore(struct UIWindowMgr *self, struct ReplayRecorder *recorder)
{
    typedef int (*Loader)(struct UIWindowMgr *, const void *, size_t);
    static const struct { int slot; Loader load; } components[] = {
        {0x13, ui_window_mgr_load_option_records},
        {0x12, ui_window_mgr_load_window_records},
        {0x14, ui_window_mgr_skip_empty_records}
    };
    size_t i;
    if (!self || !recorder) return 0;
    for (i = 0; i < sizeof components / sizeof components[0]; ++i) {
        struct ReplayBuffer chunk = {0};
        int loaded = replay_read_chunk(recorder, components[i].slot, &chunk) &&
            (!chunk.size || components[i].load(self, chunk.data, chunk.size) >= 0);
        replay_buffer_destroy(&chunk);
        if (!loaded) return 0;
    }
    return 1;
}

void ui_window_mgr_reset_shown_infos(struct UIWindowMgr *self, const BYTE *opts)
{
    self->equip_info.show = 0;
    self->status_info.show = 0;
    self->quest_info.show = 0;
    self->item_info.show = 0;
    self->messenger_group_info.show = 0;
    self->skill_list_info.show = 0;
    self->option_info.show = 0;
    self->shorten_item_info.show = 0;
    self->shortcut_info.show = opts[7];
    self->chat_wnd_show = opts[6];
}

#define REPLAY_UI_PACKET 0x08ae
#define REPLAY_WINDOW_OPENED 0x038d

static struct ReplayRecorder *recording(void)
{
    struct ReplayRecorder *rec = replay_recorder_instance();

    if (rec->state != WM_REPLAY_RECORDING || !rec->info.options[2] || !rec->info.options[0]
        || !rec->info.options[8])
        return NULL;
    return rec;
}

static void note(struct ReplayRecorder *rec, WORD event, int window_id)
{
#pragma pack(push, 1)
    struct {
        WORD packet, event, len;
        int window_id;
    } record;
#pragma pack(pop)

    record.packet = REPLAY_UI_PACKET;
    record.event = event;
    record.len = sizeof record;
    record.window_id = window_id;
    replay_recorder_write_packet(rec, sizeof record, &record, 0x2a, 0);
}

void ui_window_mgr_note_window_opened(struct UIWindowMgr *self, int window_id)
{
    struct ReplayRecorder *rec = recording();

    (void)self;
    if (rec)
        note(rec, REPLAY_WINDOW_OPENED, window_id);
}
