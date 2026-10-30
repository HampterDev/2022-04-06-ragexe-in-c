#include "ui/wndmgr/wndmgr.h"
#include "replay/replay.h"

static int made[16], made_count, delivered[16], delivered_count, missing_window;
static struct UIWindow restored_window;
static struct UIWindowOps restored_ops;
static struct ReplayBuffer restore_chunks[3];
static int read_count, fail_read;

struct GameMode *game_current_mode(void) { abort(); }

static int receive_window_records(struct UIWindow *window, struct UIWindow *sender, int msg, int a, int b,
                                  int c, int d)
{
    (void)sender;
    (void)c;
    (void)d;
    assert(msg == UI_MSG_LOAD_RECORDS && b > 0);
    assert(ui_window_records_validate((void *)a, (size_t)b) == b);
    delivered[delivered_count++] = window->id;
    memset((void *)a, 0xcc, (size_t)b);
    return 0;
}

struct UIWindow *ui_window_mgr_make_window(struct UIWindowMgr *manager, int id)
{
    assert(manager && made_count < 16);
    made[made_count++] = id;
    if (id == missing_window)
        return NULL;
    restored_ops.send_msg = receive_window_records;
    restored_window.ops = &restored_ops;
    restored_window.id = id;
    return &restored_window;
}

int replay_read_chunk(struct ReplayRecorder *recorder, int id, struct ReplayBuffer *out)
{
    static const int slots[] = {0x13, 0x12, 0x14};
    const struct ReplayBuffer *source;
    assert(recorder && read_count < 3 && id == slots[read_count]);
    source = &restore_chunks[read_count++];
    if (source->size) {
        out->data = window_allocate(source->size);
        if (!out->data)
            return 0;
        memcpy(out->data, source->data, source->size);
        out->size = source->size;
    }
    return read_count != fail_read;
}

static void reset_window_events(void)
{
    made_count = delivered_count = read_count = 0;
    missing_window = 7;
}

static void check_record_field(uint16_t begin, uint16_t tag, uint16_t end, const void *value, size_t size,
                               int valid)
{
    struct SnapshotWriter writer = {0};
    snapshot_write(&writer, begin, NULL, 0);
    snapshot_write(&writer, tag, value, size);
    snapshot_write(&writer, end, NULL, 0);
    assert((ui_window_records_validate(writer.buffer.data, writer.buffer.size) >= 0) == valid);
    snapshot_discard(&writer);
}

static void test_window_record_readers(void)
{
    struct UIWindowMgr manager = {0};
    struct SnapshotWriter writer = {0};
    struct ReplayBuffer nested = {0}, original = {0}, options = {0}, empty = {0};
    size_t i, prefix;
    int mask, step, baseline;

    assert(ui_window_records_validate(NULL, 0) == 0);
    assert(ui_window_records_validate(NULL, 6) < 0);
    assert(ui_window_records_validate("x", (size_t)INT_MAX + 1) < 0);
    {
        char name[UI_RECORD_TEXT_CAPACITY];
        unsigned char packet[] = {0x77, 1, 6, 0, 0xff, 0xff};
        memset(name, 'n', sizeof name);
        check_record_field(JOIN_FRIEND_REC_BEGIN, JOIN_FRIEND_REC_NAME, JOIN_FRIEND_REC_END, name,
                           sizeof name - 1, 1);
        check_record_field(JOIN_FRIEND_REC_BEGIN, JOIN_FRIEND_REC_NAME, JOIN_FRIEND_REC_END, name,
                           sizeof name, 0);
        check_record_field(REC_IDENTIFY_BEGIN, REC_IDENTIFY_PACKET, REC_IDENTIFY_END, packet, sizeof packet,
                           1);
        packet[0] = 0x7b;
        check_record_field(REC_IDENTIFY_BEGIN, REC_IDENTIFY_PACKET, REC_IDENTIFY_END, packet, sizeof packet,
                           0);
        packet[0] = 0x77;
        packet[2] = 4;
        check_record_field(REC_IDENTIFY_BEGIN, REC_IDENTIFY_PACKET, REC_IDENTIFY_END, packet, sizeof packet,
                           0);
        packet[2] = 5;
        check_record_field(REC_IDENTIFY_BEGIN, REC_IDENTIFY_PACKET, REC_IDENTIFY_END, packet, 5, 0);
        check_record_field(REC_IDENTIFY_BEGIN, REC_IDENTIFY_PACKET, REC_IDENTIFY_END, packet, 3, 0);
    }
    for (i = 0; i < sizeof expected_window_writers / sizeof expected_window_writers[0]; ++i) {
        const unsigned char *data = expected_window_writers[i].data;
        size_t size = expected_window_writers[i].size;
        struct SnapshotReader reader = {{data, size}, 0};
        struct SnapshotRecord record;
        for (prefix = 1; prefix < size; ++prefix)
            assert(ui_window_records_validate(data, prefix) < 0);
        while (reader.offset < size) {
            size_t header = reader.offset;
            unsigned char changed[512];
            assert(size <= sizeof changed);
            memcpy(changed, data, size);
            memset(changed + header + 2, 0xff, 4);
            assert(ui_window_records_validate(changed, size) < 0);
            assert(snapshot_next(&reader, &record) == 1);
        }
    }

    snapshot_write(&writer, REC_WINDOWS_BEGIN, NULL, 0);
    snapshot_write(&writer, 0x7fff, "unknown", 7);
    snapshot_write(&writer, REC_WINDOW_BASE + 3, window_wire_1, sizeof window_wire_1);
    snapshot_write(&writer, REC_WINDOW_BASE + 7, window_wire_24, sizeof window_wire_24);
    snapshot_write(&writer, REC_WINDOW_BASE + 8, NULL, 0);
    snapshot_write(&writer, REC_WINDOW_BASE + 9, window_wire_24, sizeof window_wire_24);
    snapshot_write(&writer, REC_WINDOWS_END, NULL, 0);
    assert(snapshot_finish(&writer, &nested));
    original.data = window_allocate(nested.size);
    original.size = nested.size;
    memcpy(original.data, nested.data, nested.size);

    reset_window_events();
    assert(ui_window_mgr_load_window_records(&manager, nested.data, nested.size) == (int)nested.size);
    assert(made_count == 4 && made[0] == 3 && made[1] == 7 && made[2] == 8 && made[3] == 9);
    assert(delivered_count == 2 && delivered[0] == 3 && delivered[1] == 9);
    assert(!memcmp(nested.data, original.data, nested.size));
    baseline = allocations;
    for (prefix = 0; prefix < nested.size; ++prefix) {
        reset_window_events();
        assert(ui_window_mgr_load_window_records(&manager, nested.data, prefix) < 0);
        assert(!made_count && !delivered_count && allocations == baseline);
    }
    reset_window_events();
    attempts = 0;
    fail_at = 1;
    assert(ui_window_mgr_load_window_records(&manager, nested.data, nested.size) < 0);
    fail_at = 0;
    assert(!made_count && allocations == baseline);

    assert(test_write_UIWindowMgr_GetOptionRecords(0, &options));
    assert(test_write_UIWindowMgr_GetEmptyRecords(0, &empty));
    for (prefix = 0; prefix < options.size; ++prefix) {
        manager.drag_all = -123;
        assert(ui_window_mgr_load_option_records(&manager, options.data, prefix) < 0);
        assert(manager.drag_all == -123);
    }
    for (mask = 0; mask < 8; ++mask) {
        restore_chunks[0] = options;
        restore_chunks[1] = nested;
        restore_chunks[2] = empty;
        for (step = 0; step < 3; ++step)
            if (!(mask & (1 << step)))
                memset(&restore_chunks[step], 0, sizeof restore_chunks[step]);
        reset_window_events();
        manager.drag_all = -123;
        assert(ui_window_snapshots_restore(&manager, (struct ReplayRecorder *)1));
        assert(read_count == 3 && made_count == ((mask & 2) ? 4 : 0));
        assert(manager.drag_all == ((mask & 1) ? 0x12345678 : -123));
    }
    baseline = allocations;
    reset_window_events();
    attempts = 0;
    assert(ui_window_snapshots_restore(&manager, (struct ReplayRecorder *)1));
    {
        int count = attempts;
        for (step = 1; step <= count; ++step) {
            reset_window_events();
            attempts = 0;
            fail_at = step;
            assert(!ui_window_snapshots_restore(&manager, (struct ReplayRecorder *)1));
            fail_at = 0;
            assert(allocations == baseline);
        }
    }
    --restore_chunks[1].size;
    reset_window_events();
    assert(!ui_window_snapshots_restore(&manager, (struct ReplayRecorder *)1));
    assert(read_count == 2 && !made_count && allocations == baseline);
    ++restore_chunks[1].size;
    for (step = 1; step <= 3; ++step) {
        reset_window_events();
        fail_read = step;
        assert(!ui_window_snapshots_restore(&manager, (struct ReplayRecorder *)1));
        fail_read = 0;
        assert(read_count == step && allocations == baseline);
    }
    snapshot_write(&writer, REC_WINDOWS_BEGIN, NULL, 0);
    snapshot_write(&writer, REC_WINDOW_BASE + 3, nested.data, nested.size);
    snapshot_write(&writer, REC_WINDOWS_END, NULL, 0);
    assert(ui_window_records_validate(writer.buffer.data, writer.buffer.size) < 0);
    snapshot_discard(&writer);
    snapshot_write(&writer, REC_OPTIONS_BEGIN, NULL, 0);
    snapshot_write(&writer, REC_DRAG_ALL, "x", 1);
    snapshot_write(&writer, REC_OPTIONS_END, NULL, 0);
    manager.drag_all = -123;
    assert(ui_window_mgr_load_option_records(&manager, writer.buffer.data, writer.buffer.size) < 0);
    assert(manager.drag_all == -123);
    snapshot_discard(&writer);
    replay_buffer_destroy(&nested);
    replay_buffer_destroy(&original);
    replay_buffer_destroy(&options);
    replay_buffer_destroy(&empty);
    assert(!allocations);
    puts("window readers: truncation, field framing, nested validation, unknown and unavailable windows, "
         "immutable input, allocation failure and replay restore passed");
}
