#include <string.h>
#include "core/fatal.h"
#include "contents/client_res.h"
#include "contents/renew_quest.h"
#include "core/msg_str.h"
#include "ui/wndmgr/wndmgr.h"

struct ConditionName {
    unsigned bit;
    int message;
    char drop_last_two;
};

static const struct ConditionName condition_names[] = {
    { 0x00020000, 0x8d7, 1 }, { 0x00040000, 0x8da, 1 }, { 0x00080000, 0x8d8, 1 },
    { 0x00100000, 0x8d9, 1 }, { 0x00200000, 0x8dd, 1 }, { 0x00400000, 0x8dc, 1 },
    { 0x00800000, 0x8db, 1 }, { 0x01000000, 0x8de, 1 }, { 0x02000000, 0x8e0, 1 },
    { 0x04000000, 0x8df, 1 },
    { 0x00000010, 0x8e3, 0 }, { 0x00000020, 0x8e1, 0 }, { 0x00000040, 0x8e2, 0 },
    { 0x00000080, 0x8ea, 0 }, { 0x00000100, 0x8e5, 0 }, { 0x00000200, 0x8e7, 0 },
    { 0x00000400, 0x8e8, 0 }, { 0x00000800, 0x8e6, 0 }, { 0x00001000, 0x8e9, 0 },
    { 0x00002000, 0x8eb, 0 }, { 0x00004000, 0x8e4, 0 }, { 0x00008000, 0x8ed, 0 },
    { 0x00010000, 0x8ec, 0 },
};

#define LEVEL_UP_MARK   "▲"
#define LEVEL_DOWN_MARK "▼"

struct String *renew_quest_condition_text(struct RenewQuestMgr *self, struct String *out,
                                                        unsigned name_mask, int min_level, int max_level)
{
    struct String number, other;
    size_t i;

    (void)self;

    string_init(out);

    if (name_mask) {
        for (i = 0; i < sizeof condition_names / sizeof condition_names[0]; i++) {
            const struct ConditionName *name = &condition_names[i];
            struct String text;
            const char *message;

            if (!(name_mask & name->bit))
                continue;
            message = msg_str(name->message);
            string_init(&text);
            string_assign_cstr(&text, message);
            if (name->drop_last_two) {
                if (text.size < 2)
                    fatal_error("invalid string position");
                string_truncate(&text, text.size - 2);
            }
            string_append(out, string_cstr(&text), text.size);
            string_append_cstr(out, " ");
            string_free(&text);
        }
    }

    if (min_level == 0) {
        if (max_level > 0) {
            string_init(&number);
            string_format(&number, "%d", max_level);
            string_append_cstr(out, " Lv.");
            string_append(out, string_cstr(&number), number.size);
            string_append_cstr(out, LEVEL_DOWN_MARK);
            string_free(&number);
        }
    } else if (min_level > 0) {
        if (max_level == 0) {
            string_init(&number);
            string_format(&number, "%d", min_level);
            string_append_cstr(out, " Lv.");
            string_append(out, string_cstr(&number), number.size);
            string_append_cstr(out, LEVEL_UP_MARK);
            string_free(&number);
        } else if (min_level < max_level) {
            string_init(&other);
            string_format(&other, "%d", max_level);
            string_init(&number);
            string_format(&number, "%d", min_level);
            string_append_cstr(out, " Lv.");
            string_append(out, string_cstr(&number), number.size);
            string_append_cstr(out, "~Lv.");
            string_append(out, string_cstr(&other), other.size);
            string_free(&number);
            string_free(&other);
        }
    }
    return out;
}
