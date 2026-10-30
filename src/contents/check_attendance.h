#ifndef CONTENTS_CHECK_ATTENDANCE_H
#define CONTENTS_CHECK_ATTENDANCE_H
#include "std/std.h"

struct AttendanceReward {
    int item_id;
    int count;
};
struct AttendanceDayReward {
    int day;
    struct AttendanceReward reward;
};
struct CheckAttendanceMgr {
    int start_date;
    int end_date;
    struct Tree rewards;
};
struct AttendanceItem {
    int item_id;
    int count;
};
extern struct CheckAttendanceMgr *g_attendanceTable;

char check_attendance_create(void);
void check_attendance_destroy(void);
struct AttendanceItem *check_attendance_reward(struct CheckAttendanceMgr *self,
                                               struct AttendanceItem *out, int day);
#endif
