#include "game/gamemode.h"
#include "game/quest_packets.h"
#include "game/tips.h"
#include "contents/quest.h"
#include "session/state.h"
#include "ui/window_manager_state.h"
#include "ui/window_access.h"
#include "world_state_memory.h"
#include <stdarg.h>
#include <stdio.h>
#include <time.h>

struct RenewQuestMgr *g_questTable = (struct RenewQuestMgr *)1;
struct Session g_session1;
struct UIWindowMgr g_windowMgr;
static struct { unsigned id; struct RenewQuestB value; unsigned guard[4]; } quests[8];
static struct RenewQuestB empty;
static unsigned count, events, lines_shown;

static void check_guards(void)
{
    unsigned i, j;
    for (i = 0; i < count; ++i) for (j = 0; j < 4; ++j)
        assert(quests[i].guard[j] == 0xa5a5a5a5);
}
static void reset(void)
{
    check_guards();
    while (count) quest_hunt_destroy(&quests[--count].value);
    memset(quests, 0xa5, sizeof quests);
    events = lines_shown = 0;
}
struct RenewQuestB *renew_quest_ongoing_info(struct RenewQuestMgr *mgr, unsigned id)
{
    unsigned i;
    assert(mgr == g_questTable); ++events;
    for (i = 0; i < count; ++i) if (quests[i].id == id) return &quests[i].value;
    return &empty;
}
struct RenewQuestB *renew_quest_get_or_add(struct RenewQuestMgr *mgr, unsigned id)
{
    struct RenewQuestB *found = renew_quest_ongoing_info(mgr, id);
    if (found != &empty) return found;
    assert(count < 8); quests[count].id = id;
    return quest_hunt_construct(&quests[count++].value);
}
char renew_quest_set_active(struct RenewQuestMgr *mgr, unsigned id, char active)
{
    struct RenewQuestB *found = renew_quest_ongoing_info(mgr, id);
    if (found == &empty) return 0;
    found->base.active = active; return 1;
}
void renew_quest_read_ongoing(struct RenewQuestMgr *mgr, unsigned id, struct RenewQuestB *value)
{
    value->cool_time_quest = 29; value->cool_time_quest_high = -1;
    string_assign_cstr(&value->base.text[0], "A catalog quest with owned title");
    quest_hunt_assign(renew_quest_get_or_add(mgr, id), value);
}
void renew_quest_remove(struct RenewQuestMgr *mgr, unsigned id)
{
    struct RenewQuestB *found = renew_quest_ongoing_info(mgr, id);
    assert(found != &empty); quest_hunt_destroy(found); quest_hunt_construct(found);
}
void renew_quest_refresh_windows(void) { ++events; }
struct UIWindow *ui_window_open(int id) { ++events; return NULL; }
struct UIWindow *ui_window_find(int id) { ++events; return NULL; }
intptr_t ui_window_send(struct UIWindow *window, struct UIWindow *sender, int message, intptr_t a, intptr_t b, intptr_t c, intptr_t d) { abort(); }
intptr_t ui_window_mgr_send_msg_(struct UIWindowMgr *mgr, int msg, intptr_t a, intptr_t b, intptr_t c, intptr_t d) { ++events; return 0; }
char *msg_str(int id) { return id == 0x64a ? "%s %s %d %d" : "%s"; }
const char *session_get_job_name(struct Session *s, int job, int max_len) { return "monster"; }
const char *session_map_display_name(const char *name)
{ assert(strlen(name) <= 28); return "map"; }
int crt_snprintf_s(char *out, size_t size, size_t limit, const char *format, ...)
{
    int n; va_list args; va_start(args, format); n = vsnprintf(out, size, format, args); va_end(args); return n;
}
struct String *renew_quest_condition_text(struct RenewQuestMgr *mgr, struct String *out,
                                            unsigned flags, int min, int max)
{ string_init(out); string_assign_cstr(out, "condition"); return out; }
struct String *quest_hunt_get_line(struct String *out, const struct RenewQuestB *q, int slot)
{ assert(slot >= 0 && slot < q->plain.hunt_count); string_init(out); string_assign_cstr(out, "hunt"); return out; }
void game_mode_show_quest_hunt_list(struct GameMode *mode, const struct Vec *lines)
{ lines_shown += (unsigned)((const struct String *)lines->last - (const struct String *)lines->first); }

static unsigned char packet[2048];
static size_t used;
static void at16(size_t at, unsigned value) { packet[at] = (unsigned char)value; packet[at + 1] = (unsigned char)(value >> 8); }
static void put16(unsigned value) { at16(used, value); used += 2; }
static void put32(unsigned value) { put16(value); put16(value >> 16); }
static void put_id(int wide, unsigned low) { put32(low); if (wide) put32(0x1234); }
static void hunt(int wide, int add, int slot)
{
    put_id(wide, 0xf0000000u + slot); put32(10 + slot); put32(2000 + slot);
    put16(1); put16(99); put16(2 + slot); if (!add) put16(20 + slot);
    memset(packet + used, 'A' + slot, 24); used += 24;
}
static void quest(unsigned id, int hunts, int wide)
{
    int i;
    put32(id); packet[used++] = 1; put32(0); put32(1577934245); put16(hunts);
    for (i = 0; i < hunts; ++i) hunt(wide, 0, i);
}
static void list_packet(int wide, int hunts)
{
    used = 0; put16(wide ? 0xaff : 0x9f8); put16(0); put32(2);
    quest(101, hunts, wide); quest(202, hunts, wide); at16(2, (unsigned)used);
}
static void added_packet(int wide, int hunts)
{
    int i;
    used = 0; put16(wide ? 0xb0c : 0x9f9); put32(303); packet[used++] = 1;
    put32(0); put32(1577934245); put16(hunts);
    for (i = 0; i < 3; ++i) hunt(wide, 1, i);
}
typedef void (*Handler)(struct GameMode *, const void *, size_t);
static void rejected(Handler fn, size_t size)
{
    unsigned before = events, old_count = count, shown = lines_shown;
    struct RenewQuestB values[8]; unsigned i;
    for (i = 0; i < count; ++i) values[i] = quests[i].value;
    fn(NULL, packet, size);
    assert(before == events && old_count == count && shown == lines_shown);
    for (i = 0; i < count; ++i) assert(!memcmp(&values[i], &quests[i].value, sizeof values[i]));
    check_guards();
}

int main(void)
{
    int wide, hunts, kind; size_t i, size; Handler fn;
    quest_hunt_construct(&empty); assert(!_putenv_s("TZ", "GMT0")); _tzset();
    for (wide = 0; wide < 2; ++wide) for (hunts = 0; hunts <= 3; ++hunts) {
        reset(); list_packet(wide, hunts);
        fn = wide ? game_mode_all_quest_list4 : game_mode_all_quest_list3;
        fn(NULL, packet, used); assert(count == 2 && quests[1].id == 202);
        assert(quests[0].value.plain.hunt_count == hunts);
        assert(quests[0].value.plain.deadline.year == 2020 && quests[0].value.plain.deadline.minute == 4);
        if (hunts) {
            assert(strlen(quests[0].value.plain.hunt_name[0]) == 24);
            assert(quests[0].value.plain.hunt_id[0] == (wide ? 0x1234f0000000LL : (int)0xf0000000u));
        }
        for (i = 0; i < used; ++i) rejected(fn, i);
        at16(21, 4); rejected(fn, used); at16(21, hunts);
        at16(4, 0xffff); rejected(fn, used); at16(4, 2);
        at16(2, (unsigned)used + 1); rejected(fn, used + 1); at16(2, (unsigned)used);
        quest_hunt_destroy(&quests[--count].value);
        fn(NULL, packet, used); assert(count == 2 && quests[1].id == 202);
        added_packet(wide, hunts); size = used;
        assert(size == (wide ? 155 : 143));
        fn = wide ? game_mode_zc_add_quest_ex2 : game_mode_zc_add_quest_ex;
        for (i = 0; i < size; ++i) rejected(fn, i);
        rejected(fn, size + 1); at16(15, 4); rejected(fn, size); at16(15, 0xffff); rejected(fn, size);
        at16(15, hunts); fn(NULL, packet, size); assert(count == 3 && quests[2].id == 303);
        if (hunts) assert(quests[2].value.plain.hunt_goal[0] == 0);
        check_guards();
    }
    for (kind = 0; kind < 3; ++kind) {
        reset(); wide = kind == 1; list_packet(wide, 1);
        (wide ? game_mode_all_quest_list4 : game_mode_all_quest_list3)(NULL, packet, used);
        used = 0; put16(kind == 2 ? 0x2b3 : wide ? 0xafe : 0x9fa); put16(0);
        if (kind != 2) put16(2);
        put32(101); if (kind == 2) put32(2000); else put_id(wide, 0xf0000000u); put16(99); put16(7);
        put32(202); if (kind == 2) put32(2000); else put_id(wide, 0xf0000000u); put16(88); put16(6);
        at16(2, (unsigned)used);
        fn = kind == 2 ? game_mode_zc_hunting_quest_info : wide ? game_mode_zc_update_mission_hunt_ex2 : game_mode_zc_update_mission_hunt_ex;
        for (i = 0; i < used; ++i) rejected(fn, i);
        at16(2, (unsigned)used - 1); rejected(fn, used); at16(2, (unsigned)used);
        if (kind != 2) { at16(4, 3); rejected(fn, used); at16(4, 0xffff); rejected(fn, used); at16(4, 2); }
        fn(NULL, packet, used);
        assert(quests[0].value.plain.hunt_done[0] == 7 && quests[1].value.plain.hunt_done[0] == 6);
        assert(quests[1].value.plain.hunt_goal[0] == 88);
        assert(lines_shown == (kind == 2 ? 0 : 2));
        quests[0].value.plain.hunt_count = 0; quests[0].value.plain.hunt_done[0] = 3;
        fn(NULL, packet, used); assert(quests[0].value.plain.hunt_done[0] == 3);
    }
    used = 0; put16(0x2b7); put32(101); packet[used++] = 0;
    for (i = 0; i < used; ++i) { unsigned before = events; game_mode_zc_active_quest(packet, i); assert(before == events); }
    game_mode_zc_active_quest(packet, used); assert(!quests[0].value.base.active);
    used = 0; put16(0x2b4); put32(101);
    for (i = 0; i < used; ++i) rejected(game_mode_del_quest, i);
    game_mode_del_quest(NULL, packet, used); assert(!quests[0].value.base.text[0].size);
    reset(); quest_hunt_destroy(&empty); assert(!live_blocks);
    puts("Quest packets: both list/add variants, actual spans, every-byte truncation, bounded hunt counts/names, existing quests, 32/64-bit updates and active slots passed");
    return 0;
}
