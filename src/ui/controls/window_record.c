#include "ui/controls/edit.h"
#include "replay/event.h"
#include <stdlib.h>
#include "ui/window.h"
#include "ui/draw.h"
#include "ui/skin.h"
#include "ui/window_mgr.h"
#include "ui/controls/controls.h"
#include "replay/replay.h"
#include "game/drag_info.h"
#include <string.h>

#define RECORD_PACKET   0x08ae
#define RECORD_COMMAND  0x0384
#define RECORD_DRAG     0x0385
#define RECORD_EDITS    0x0386
#define RECORD_TEXT     0x038b
#define RECORD_MSG      0x0391
#define RECORD_TEXT_MAX 100

#define UI_WNDFLAG_RECORD_DRAG 5

static int recording(void)
{
    return replay_recorder_instance()->state == 2;
}

static void write_record(const void *rec, unsigned short length, int time_mode)
{
    replay_recorder_write_packet(replay_recorder_instance(), length, rec, time_mode, 0);
}

#pragma pack(push, 1)
struct RecordHead {
    unsigned short packet;
    unsigned short type;
    unsigned short length;
    int window_id;
};

struct CommandRecord {
    struct RecordHead head;
    int val[4];
};

struct MsgRecord {
    struct RecordHead head;
    int val[4];
    int msg;
};

struct TextRecord {
    struct RecordHead head;
    int val;
    char text[RECORD_TEXT_MAX];
};

struct Text2Record {
    struct RecordHead head;
    int val1, val2;
    char text[RECORD_TEXT_MAX];
};

struct DragRecord {
    struct RecordHead head;
    int val1, val2;
    int fields[8];
    char identified;
    char display_name[RECORD_TEXT_MAX];
    char name[RECORD_TEXT_MAX];
    char name2[RECORD_TEXT_MAX];
    int level;
    int slots[4];
    char named;
    int option_count;
    unsigned char options[25];
};
#pragma pack(pop)
SIZE_ASSERT(CommandRecord, 0x1a);
SIZE_ASSERT(MsgRecord, 0x1e);
SIZE_ASSERT(TextRecord, 0x72);
SIZE_ASSERT(Text2Record, 0x76);
SIZE_ASSERT(DragRecord, 0x191);

static void head(struct RecordHead *h, int type, int length, int window_id)
{
    h->packet = RECORD_PACKET;
    h->type = (unsigned short)type;
    h->length = (unsigned short)length;
    h->window_id = window_id;
}

void ui_window_record_command(struct UIWindow *self, int val1, int val2, int val3,
                                       int val4)
{
    struct CommandRecord rec;

    if (!recording())
        return;
    head(&rec.head, RECORD_COMMAND, sizeof rec, self->id);
    rec.val[0] = val1;
    rec.val[1] = val2;
    rec.val[2] = val3;
    rec.val[3] = val4;
    write_record(&rec, sizeof rec, REPLAY_TIME_NOW);
}

static void copy_text(char *dst, const struct String *s)
{
    int n = (int)s->size;

    if (n > RECORD_TEXT_MAX)
        n = RECORD_TEXT_MAX;
    memcpy(dst, string_cstr(s), n);
}

void ui_window_record_drag(struct UIWindow *self, int val1, int val2,
                                    const struct DragInfo *drag, int window_id)
{
    struct DragRecord rec;

    if (!recording())
        return;
    if (!ui_window_mgr_get_window_flag(&g_windowMgr, UI_WNDFLAG_RECORD_DRAG, self->id))
        return;
    memset(&rec, 0, sizeof rec);
    head(&rec.head, RECORD_DRAG, sizeof rec, window_id == -1 ? self->id : window_id);
    rec.val1 = val1;
    rec.val2 = val2;
    memcpy(rec.fields, &drag->source, sizeof rec.fields);
    rec.identified = drag->identified;
    rec.level = drag->level;
    rec.named = drag->named;
    copy_text(rec.display_name, &drag->display_name);
    copy_text(rec.name, &drag->name);
    copy_text(rec.name2, &drag->name2);
    rec.option_count = drag->option_count;
    memcpy(rec.slots, drag->slots, sizeof rec.slots);
    memcpy(rec.options, drag->options, sizeof rec.options);
    write_record(&rec, rec.head.length, REPLAY_TIME_NOW);
}

int ui_window_record_bytes(struct UIWindow *self, int tag, const void *bytes, size_t size)
{
    struct ReplayRecorder *recorder = replay_recorder_instance();
    struct ReplayEventHeader header;
    unsigned char *packet;
    int written;
    (void)self;
    if (recorder->state != 2 || !size) return 0;
    if (!bytes || tag < 0 || tag > UINT16_MAX || size > UINT16_MAX - sizeof header) {
        recorder->writeFailed = 1;
        return -1;
    }
    packet = malloc(sizeof header + size);
    if (!packet) { recorder->writeFailed = 1; return -1; }
    header.id = RECORD_PACKET;
    header.kind = (uint16_t)tag;
    header.size = (uint16_t)(sizeof header + size);
    memcpy(packet, &header, sizeof header);
    memcpy(packet + sizeof header, bytes, size);
    written = replay_recorder_write_packet(recorder, header.size, packet, REPLAY_TIME_ZERO, 0);
    free(packet);
    return written ? header.size : -1;
}

void ui_window_record_msg(struct UIWindow *self, int msg, int val1, int val2, int val3,
                                   int val4)
{
    struct MsgRecord rec;

    if (g_replayPlaying || !recording())
        return;
    head(&rec.head, RECORD_MSG, sizeof rec, self->id);
    rec.msg = msg;
    rec.val[0] = val1;
    rec.val[1] = val2;
    rec.val[2] = val3;
    rec.val[3] = val4;
    write_record(&rec, sizeof rec, REPLAY_TIME_NOW);
}

void ui_window_record_text(struct UIWindow *self, int val, const char *text)
{
    struct TextRecord rec;
    int n;

    if (!recording())
        return;
    memset(rec.text, 0, sizeof rec.text);
    head(&rec.head, RECORD_TEXT, sizeof rec, self->id);
    rec.val = val;
    n = (int)strlen(text);
    memcpy(rec.text, text, n > RECORD_TEXT_MAX ? RECORD_TEXT_MAX : n);
    write_record(&rec, rec.head.length, REPLAY_TIME_NOW);
}

#define EDITS_BUFFER 0x1000
#define EDITS_COUNT  0xa
#define EDITS_DATA   0xe

int ui_window_send_edit_fields(struct UIWindow *self)
{
    unsigned char buf[EDITS_BUFFER];
    struct ListNode *node, *end;
    int count = 0, off;

    if (!recording())
        return 0;
    if (!ui_window_mgr_get_window_flag(&g_windowMgr, UI_WNDFLAG_SEND_EDITS, self->id))
        return 0;
    memset(buf, 0, sizeof buf);
    end = self->children.head;
    for (node = end->next; node != end; node = node->next)
        if ((*LIST_VALUE(node, struct UIWindow *))->ops->type == UI_TYPE_EDIT)
            count++;
    memcpy(buf + EDITS_COUNT, &count, 4);
    off = EDITS_DATA;
    if (count == 0)
        return off;
    for (node = end->next; node != end; node = node->next) {
        struct UIWindow *edit = *LIST_VALUE(node, struct UIWindow *);
        int len;

        if (edit->ops->type != UI_TYPE_EDIT)
            continue;
        len = ui_edit_ctrl_get_text_size(edit);
        memcpy(buf + off, &len, 4);
        off += 4;
        if (len) {
            memcpy(buf + off, ui_edit_ctrl_get_text(edit), len);
            off += len;
        }
    }
    head((struct RecordHead *)buf, RECORD_EDITS, off, self->id);
    write_record(buf, (unsigned short)off, REPLAY_TIME_ZERO);
    return off;
}

int ui_window_read_edit_fields(struct UIWindow *self, const void *data)
{
    const unsigned char *p = data;
    char text[0x104];
    struct ListNode *node, *end;
    int off = 4;

    if (*(const int *)p == 0)
        return 0;
    end = self->children.head;
    memset(text, 0, sizeof text);
    for (node = end->next; node != end; node = node->next) {
        struct UIWindow *edit = *LIST_VALUE(node, struct UIWindow *);
        int len;

        if (edit->ops->type != UI_TYPE_EDIT)
            continue;
        len = *(const int *)(p + off);
        off += 4;
        if (!len)
            continue;
        memset(text, 0, sizeof text);
        memcpy(text, p + off, len);
        off += len;
        ui_edit_ctrl_set_text((struct UIEditCtrl *)edit, text);
        ui_window_invalidate(edit);
    }
    return off;
}
