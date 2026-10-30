#include "core/memory.h"
#include "contents/check_attendance.h"
#include "contents/lua_data.h"
#include "std/std.h"
#include "core/text_util.h"

struct CheckAttendanceMgr *g_attendanceTable;

static struct CheckAttendanceMgr *check_attendance_construct(struct CheckAttendanceMgr *self)
{
    self->start_date = 0;
    self->end_date = 0;
    tree_init(&self->rewards);
    return self;
}

static void check_attendance_destruct(struct CheckAttendanceMgr *self)
{
    tree_free(&self->rewards, NULL);
}

static char insert_reward(struct CheckAttendanceMgr *self, int day, int item_id, int count)
{
    struct AttendanceDayReward entry;
    int added;

    entry.day = day;
    entry.reward.item_id = item_id;
    entry.reward.count = count;
    tree_add(&self->rewards, &entry, sizeof entry, tree_less_int, &added);
    return (char)added;
}

struct AttendanceItem *check_attendance_reward(struct CheckAttendanceMgr *self,
                                                          struct AttendanceItem *out, int day)
{
    struct AttendanceDayReward *node = tree_find(&self->rewards, &day, tree_less_int);

    if (!node) {
        out->item_id = 0;
        out->count = 0;
    } else {
        out->item_id = node->reward.item_id;
        out->count = node->reward.count;
    }
    return out;
}

static int lua_insert_config(lua_State *L)
{
    int start_date, end_date;

    if (!lua_isnumber(L, -2))
        return lua_data_reply(L, 0, "[InsertCheckAttendanceConfig] argument 1st is must number");
    if (!lua_isnumber(L, -1))
        return lua_data_reply(L, 0, "[InsertCheckAttendanceConfig] argument 2nd is must number");
    start_date = (int)lua_tonumber(L, -2);
    end_date = (int)lua_tonumber(L, -1);
    g_attendanceTable->start_date = start_date;
    g_attendanceTable->end_date = end_date;
    return lua_data_reply(L, 1, "good");
}

static int reply_formatted_failure(lua_State *L, struct String *text)
{
    lua_pushstring(L, string_cstr(text));
    string_free(text);
    return 2;
}

static int lua_insert_reward(lua_State *L)
{
    struct String text;
    lua_Number day;
    int item_id, count;

    if (!lua_isnumber(L, -3))
        return lua_data_reply(L, 0, "[InsertCheckAttendanceReward] argument 1st is must number");
    if (!lua_isstring(L, -2))
        return lua_data_reply(L, 0, "[InsertCheckAttendanceReward] argument 2nd is must string");
    if (!lua_isnumber(L, -1))
        return lua_data_reply(L, 0, "[InsertCheckAttendanceReward] argument 3rd is must number");
    day = lua_tonumber(L, -3);
    item_id = (int)lua_tonumber(L, -2);
    if (item_id == 0) {
        lua_pushboolean(L, 0);
        string_init(&text);
        string_format(&text, "[InsertCheckAttendanceReward] Check ItemID : %d", item_id);
        return reply_formatted_failure(L, &text);
    }
    count = (int)lua_tonumber(L, -1);
    if (count <= 0) {
        lua_pushboolean(L, 0);
        string_init(&text);
        string_format(&text, "[InsertCheckAttendanceReward] Check ItemCnt : [%d] %d", item_id, count);
        return reply_formatted_failure(L, &text);
    }
    if (!insert_reward(g_attendanceTable, (int)day, item_id, count)) {
        lua_pushboolean(L, 0);
        string_init(&text);
        string_format(&text, "[InsertCheckAttendanceReward] Check RewardInfo : %d [%d] %d",
                   (int)day, item_id, count);
        return reply_formatted_failure(L, &text);
    }
    return lua_data_reply(L, 1, "good");
}

static const struct LuaDataFunction check_attendance_functions[] = {
    { "InsertCheckAttendanceConfig", lua_insert_config },
    { "InsertCheckAttendanceReward", lua_insert_reward },
};

static char load_file(void)
{
    return lua_data_run("System\\CheckAttendance.lub", "CCheckAttendanceMgr",
                        check_attendance_functions,
                        sizeof check_attendance_functions / sizeof check_attendance_functions[0],
                        LUA_DATA_ERROR_STRING);
}

char check_attendance_create(void)
{
    struct CheckAttendanceMgr *mgr;

    if (g_attendanceTable)
        return 0;
    mgr = (struct CheckAttendanceMgr *)mem_try_alloc(sizeof *mgr);
    if (mgr)
        check_attendance_construct(mgr);
    g_attendanceTable = mgr;
    if (!mgr)
        return 0;
    if (load_file())
        return 1;
    check_attendance_destroy();
    return 0;
}

void check_attendance_destroy(void)
{
    if (!g_attendanceTable)
        return;
    check_attendance_destruct(g_attendanceTable);
    mem_free(g_attendanceTable);
    g_attendanceTable = 0;
}
