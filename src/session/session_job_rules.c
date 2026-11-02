#include "session/csession.h"
#include "session/session_jobs.h"
#include "session/session_lua.h"
#include "session/session_names.h"

enum {
    LV_ONE,
    LV_TO_4,
    LV_TO_2,
    LV_NONE
};
static const BYTE high_case[5] = { LV_TO_4, LV_ONE, LV_TO_2, LV_NONE, LV_NONE };

static const BYTE low_jobs[0x18] = {
    0, 1, 0, 0, 2, 2, 2, 1, 2, 0, 2, 0, 2, 3, 1, 3, 0, 2, 2, 0,
    0, 3, 3, 0
};
static const BYTE high_jobs[0xfa] = {
    0, 1, 1, 2, 2, 2, 0, 2, 1, 2, 1, 2, 4, 0, 4, 1, 2, 2, 1, 1,
    4, 1, 0, 1, 1, 2, 2, 2, 0, 2, 1, 2, 1, 2, 4, 0, 4, 1, 2, 2,
    1, 1, 4, 1, 3, 3, 3, 3, 3, 3, 3, 4, 0, 1, 1, 2, 2, 2, 0, 1,
    1, 2, 2, 2, 0, 1, 1, 1, 4, 2, 2, 0, 1, 1, 1, 4, 2, 2, 4, 4,
    4, 4, 3, 3, 3, 3, 4, 4, 4, 4, 4, 4, 4, 4, 0, 1, 1, 2, 2, 2,
    0, 1, 1, 1, 4, 2, 2, 4, 4, 3, 3, 4, 4, 4, 4, 4, 4, 4, 4, 4,
    4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4,
    4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4,
    4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4,
    4, 4, 4, 4, 4, 4, 4, 4, 1, 1, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4,
    4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 0, 4, 0, 4,
    4, 4, 4, 3, 3, 3, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 3, 3, 3, 3,
    3, 3, 3, 4, 4, 4, 4, 4, 4, 4,
};
static const BYTE low_jobs_2000[18] = {
    0, 1, 1, 1, 1, 1, 1, 1, 0, 0, 1, 1, 1, 0, 0, 0, 0, 0
};
static const BYTE high_jobs_2000[0xca] = {
    0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0,
    0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 1,
};

static BYTE level_rule(int rule, int level)
{
    switch (rule) {
    case LV_ONE:  return level == 1;
    case LV_TO_4: return (unsigned)(level - 1) <= 3;
    case LV_TO_2: return (unsigned)(level - 1) <= 1;
    default:      return 0;
    }
}

BYTE session_job_allows_level(struct Session *self, int job, int level)
{
    if (level > 2000) {
        if (job > 0xfcc) {
            if ((unsigned)(job - 0xfd2) > 0xc9)
                return 1;
            return high_jobs_2000[job - 0xfd2];
        }
        if (job == 0xfcc)
            return 0;
        if (job > 0xfae)
            return job != 0xfb6 && job != 0xfc4;
        if (job == 0xfae)
            return 0;
        if ((unsigned)(job - 0xd) > 0x11)
            return 1;
        return low_jobs_2000[job - 0xd];
    }
    if (job > 0xfa1) {
        if ((unsigned)(job - 0xfa2) > 0xf9)
            return 0;
        return level_rule(high_case[high_jobs[job - 0xfa2]], level);
    }
    if (job == 0xfa1)
        return level == 1;
    if ((unsigned)job > 0x17)
        return 0;
    return level_rule(low_jobs[job], level);
}

#define B(n) (1u << (n))
#define SET_A   (B(1) | B(2) | B(6) | B(8) | B(10) | B(23))
#define SET_B   (B(1) | B(2) | B(3) | B(4) | B(5) | B(6) | B(7) | B(8) | B(9))
#define SET_C   (B(1) | B(10) | B(23))
#define SET_D   (B(1) | B(11))
#define SET_E   (B(8) | B(10) | B(23))
#define SET_F   (B(1) | B(2) | B(6) | B(7) | B(8))
#define SET_G   (B(1) | B(2) | B(11))
#define SET_H   (B(8) | B(10) | B(15) | B(23))
#define SET_J   (B(1) | B(2) | B(6) | B(16) | B(25) | B(26) | B(27) | B(28) | B(29) | B(30))
#define SET_K   (B(1) | B(2) | B(3) | B(4) | B(5) | B(6) | B(7) | B(8))
#define SET_L   (B(8) | B(10) | B(12) | B(23))
#define SET_M   (B(1) | B(2) | B(10) | B(15) | B(23))
#define SET_N   (B(1) | B(2) | B(6) | B(7) | B(8) | B(10) | B(23))
#define SET_O   (B(1) | B(11) | B(13))
#define SET_P   B(1)
#define SET_Q   (B(17) | B(18) | B(19) | B(20) | B(21))
#define SET_S   B(22)
#define SET_T   B(15)
#define SET_ALL 0xffffffffu

static const DWORD low_job_sets[0x1a] = {
    SET_A, SET_B, SET_C, SET_D, SET_E, SET_F, SET_G, SET_B, SET_H, SET_C,
    SET_F, SET_D, SET_J, SET_B, SET_K, SET_L, SET_M, SET_G, SET_N, SET_O,
    SET_P, SET_K, SET_P, SET_A, SET_Q, SET_S,
};
static const DWORD high_job_sets[20] = {
    SET_B, SET_C, SET_D, SET_E, SET_F, SET_G, SET_H, SET_F, SET_J, SET_K,
    SET_L, SET_M, SET_N, SET_O, SET_A, SET_T, SET_ALL, SET_S, SET_Q, SET_P,
};
static const BYTE high_job_case[0xfa] = {
    0, 1, 2, 3, 4, 5, 0, 6, 1, 7, 2, 8, 0, 9, 10, 11, 5, 12, 13, 19,
    9, 14, 0, 1, 2, 3, 4, 5, 0, 6, 1, 7, 2, 8, 0, 9, 10, 11, 5, 12,
    13, 19, 9, 14, 19, 15, 15, 1, 16, 16, 16, 19, 0, 1, 2, 6, 7, 8, 0, 1,
    2, 6, 7, 8, 9, 11, 13, 19, 10, 12, 5, 9, 11, 13, 19, 10, 12, 5, 0, 0,
    9, 9, 2, 2, 7, 7, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 2, 6, 7, 8,
    9, 11, 13, 19, 10, 12, 5, 0, 9, 2, 7, 19, 19, 19, 19, 19, 19, 19, 19, 19,
    19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19,
    19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19,
    19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19,
    19, 19, 19, 19, 19, 19, 19, 19, 14, 14, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19,
    19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19,
    17, 19, 19, 19, 15, 1, 18, 19, 19, 19, 19, 19, 19, 19, 19, 19, 15, 15, 1, 15,
    1, 15, 15, 19, 19, 19, 19, 19, 19, 19,
};

BYTE session_job_accepts_value(struct Session *self, int job,
                                         unsigned __int64 value)
{
    DWORD set;

    if (job > 0xfa1)
        set = (unsigned)(job - 0xfa2) > 0xf9 ? SET_P : high_job_sets[high_job_case[job - 0xfa2]];
    else if (job == 0xfa1)
        set = SET_A;
    else if ((unsigned)job > 0x19)
        set = SET_P;
    else
        set = low_job_sets[job];
    if (set == SET_ALL)
        return 1;
    return value < 32 && ((set >> (unsigned)value) & 1);
}

BYTE session_get_pet_acc_act_name(struct Session *self, int job)
{
    if (job >= 0x2d && job < 0x3e8)
        return 1;
    return (unsigned)(job - 0x2711) <= 0x270d;
}

#define IS_ONE_HAND_SWORD(id) (((id) >= 0x44c && (id) < 0x47e) || ((id) >= 0x3458 && (id) < 0x34bc) \
                               || (unsigned)((id) - 0x7a120) <= 0x270f)
#define IS_DAGGER(id)         (((id) >= 0x4b0 && (id) < 0x4e2) || ((id) >= 0x32c8 && (id) < 0x332c) \
                               || ((id) >= 0x701c && (id) < 0x70e4) || (unsigned)((id) - 0x7c830) <= 0x270f)
#define IS_ONE_HAND_AXE(id)   (((id) >= 0x514 && (id) < 0x546) || (unsigned)((id) - 0x7ef40) <= 0x270f)

static int dual_wield_type(int first, int second)
{
    int type = 0;

    if (!first) {
        if (!second)
            return 0;
        first = second;
        second = 0;
    }
    if (IS_ONE_HAND_SWORD(first)) {
        type = 2;
        if (IS_ONE_HAND_SWORD(second))
            return 0x1a;
        if (IS_DAGGER(second))
            return 0x1c;
        if (IS_ONE_HAND_AXE(second))
            return 0x1e;
    } else if (IS_DAGGER(first)) {
        type = 1;
        if (IS_ONE_HAND_SWORD(second))
            return 0x1c;
        if (IS_DAGGER(second))
            return 0x19;
        if (IS_ONE_HAND_AXE(second))
            return 0x1d;
    } else if (IS_ONE_HAND_AXE(first)) {
        type = 6;
        if (IS_ONE_HAND_AXE(second))
            return 0x1b;
    }
    return type;
}

enum {
    WT_4_5,
    WT_1,
    WT_NOT_11,
    WT_11,
    WT_15,
    WT_1_10_23,
    WT_2_6_8,
    WT_16_25_30,
    WT_0_12,
    WT_5_10_15_23,
    WT_BY_MODE,
    WT_22,
    WT_18_21,
    WT_NONE
};

static const BYTE low_job_weapons[0x1a] = {
    WT_BY_MODE, WT_4_5, WT_1, WT_NOT_11, WT_NONE, WT_1, WT_11, WT_4_5, WT_15, WT_1_10_23,
    WT_2_6_8, WT_11, WT_16_25_30, WT_4_5, WT_4_5, WT_0_12, WT_5_10_15_23, WT_11, WT_2_6_8, WT_11,
    WT_11, WT_4_5, WT_NONE, WT_BY_MODE, WT_18_21, WT_22,
};
static const BYTE high_job_weapons[0xfa] = {
    WT_4_5, WT_1, WT_NOT_11, WT_NONE, WT_1, WT_11, WT_4_5, WT_15,
    WT_1_10_23, WT_2_6_8, WT_11, WT_16_25_30, WT_4_5, WT_4_5, WT_0_12, WT_5_10_15_23,
    WT_11, WT_2_6_8, WT_11, WT_11, WT_4_5, WT_BY_MODE, WT_4_5, WT_1,
    WT_NOT_11, WT_NONE, WT_1, WT_11, WT_4_5, WT_15, WT_1_10_23, WT_2_6_8,
    WT_11, WT_16_25_30, WT_4_5, WT_4_5, WT_0_12, WT_5_10_15_23, WT_11, WT_2_6_8,
    WT_11, WT_11, WT_4_5, WT_BY_MODE, WT_NONE, WT_NONE, WT_NONE, WT_1_10_23,
    WT_NONE, WT_NONE, WT_NONE, WT_NONE, WT_4_5, WT_1_10_23, WT_11, WT_15,
    WT_2_6_8, WT_16_25_30, WT_4_5, WT_1_10_23, WT_11, WT_15, WT_2_6_8, WT_16_25_30,
    WT_4_5, WT_5_10_15_23, WT_11, WT_11, WT_0_12, WT_2_6_8, WT_11, WT_4_5,
    WT_5_10_15_23, WT_11, WT_11, WT_0_12, WT_2_6_8, WT_11, WT_4_5, WT_4_5,
    WT_4_5, WT_4_5, WT_11, WT_11, WT_2_6_8, WT_2_6_8, WT_4_5, WT_4_5,
    WT_4_5, WT_4_5, WT_4_5, WT_4_5, WT_4_5, WT_4_5, WT_4_5, WT_1_10_23,
    WT_11, WT_15, WT_2_6_8, WT_16_25_30, WT_4_5, WT_5_10_15_23, WT_11, WT_11,
    WT_0_12, WT_2_6_8, WT_11, WT_4_5, WT_4_5, WT_11, WT_2_6_8, WT_NONE,
    WT_NONE, WT_NONE, WT_NONE, WT_NONE, WT_NONE, WT_NONE, WT_NONE, WT_NONE,
    WT_NONE, WT_NONE, WT_NONE, WT_NONE, WT_NONE, WT_NONE, WT_NONE, WT_NONE,
    WT_NONE, WT_NONE, WT_NONE, WT_NONE, WT_NONE, WT_NONE, WT_NONE, WT_NONE,
    WT_NONE, WT_NONE, WT_NONE, WT_NONE, WT_NONE, WT_NONE, WT_NONE, WT_NONE,
    WT_NONE, WT_NONE, WT_NONE, WT_NONE, WT_NONE, WT_NONE, WT_NONE, WT_NONE,
    WT_NONE, WT_NONE, WT_NONE, WT_NONE, WT_NONE, WT_NONE, WT_NONE, WT_NONE,
    WT_NONE, WT_NONE, WT_NONE, WT_NONE, WT_NONE, WT_NONE, WT_NONE, WT_NONE,
    WT_NONE, WT_NONE, WT_NONE, WT_NONE, WT_NONE, WT_NONE, WT_NONE, WT_NONE,
    WT_NONE, WT_NONE, WT_NONE, WT_NONE, WT_NONE, WT_NONE, WT_NONE, WT_NONE,
    WT_NONE, WT_NONE, WT_NONE, WT_NONE, WT_BY_MODE, WT_BY_MODE, WT_NONE, WT_NONE,
    WT_NONE, WT_NONE, WT_NONE, WT_NONE, WT_NONE, WT_NONE, WT_NONE, WT_NONE,
    WT_NONE, WT_NONE, WT_NONE, WT_NONE, WT_NONE, WT_NONE, WT_NONE, WT_NONE,
    WT_NONE, WT_22, WT_22, WT_NONE, WT_NONE, WT_18_21, WT_NONE, WT_NONE,
    WT_NONE, WT_NONE, WT_NONE, WT_NONE, WT_22, WT_22, WT_22, WT_NONE,
    WT_NONE, WT_1_10_23, WT_18_21, WT_18_21, WT_NONE, WT_NONE, WT_NONE, WT_NONE,
    WT_NONE, WT_NONE, WT_NONE, WT_NONE, WT_NONE, WT_NONE, WT_1_10_23, WT_NONE,
    WT_1_10_23, WT_NONE, WT_NONE, WT_NONE, WT_NONE, WT_NONE, WT_NONE, WT_NONE,
    WT_NONE, WT_NONE,
};

static int weapon_rule(int rule, int type, int mode)
{
    switch (rule) {
    case WT_4_5:        return (unsigned)(type - 4) <= 1;
    case WT_1:          return type == 1;
    case WT_NOT_11:     return type != 0xb;
    case WT_11:         return type == 0xb;
    case WT_15:         return type == 0xf;
    case WT_1_10_23:
        if (type == 1)
            return mode == 1;
        if (type == 0xa || type == 0x17)
            return mode == 0;
        return 0;
    case WT_2_6_8:      return type == 2 || (unsigned)(type - 6) <= 2;
    case WT_16_25_30:   return type == 0x10 || (unsigned)(type - 0x19) <= 5;
    case WT_0_12:       return type == 0 || type == 0xc;
    case WT_5_10_15_23: return type == 5 || type == 0xa || type == 0xf || type == 0x17;
    case WT_BY_MODE:
        if (mode == 0)
            return type == 1;
        if (mode == 1)
            return (unsigned)(type - 2) <= 0x15
                   && (type == 2 || type == 3 || (unsigned)(type - 6) <= 4 || type == 0x17);
        return 0;
    case WT_22:         return type == 0x16;
    case WT_18_21:      return (unsigned)(type - 0x12) <= 3;
    default:            return 0;
    }
}

BYTE session_job_accepts_weapon(struct Session *self, int job, int mode,
                                          unsigned __int64 item)
{
    int lo = (int)(DWORD)item, hi = (int)(DWORD)(item >> 32);
    int id = lo, type, rule;

    if (hi > 0 || (hi == 0 && (unsigned)lo >= 0x69)) {
        if ((job == 0xc || job == 0xfad || job == 0xfc3 || job == 0xfdb || job == 0xfe1
             || job == 0x1005)
            && session_get_weapon_view_id(hi) > 0)
            id = dual_wield_type(lo, hi);
        else
            id = session_get_weapon_view_id(lo);
    }
    type = session_get_real_weapon_id(id);
    if (job > 0xfa1) {
        if ((unsigned)(job - 0xfa2) > 0xf9)
            return 0;
        rule = high_job_weapons[job - 0xfa2];
    } else if (job == 0xfa1) {
        rule = WT_BY_MODE;
    } else if ((unsigned)job > 0x19) {
        return 0;
    } else {
        rule = low_job_weapons[job];
    }
    return (BYTE)weapon_rule(rule, type, mode);
}

struct JobRange { WORD first, last; };

static int in_ranges(int job, const struct JobRange *r, int n)
{
    int i;
    for (i = 0; i < n; i++)
        if (job >= r[i].first && job <= r[i].last)
            return 1;
    return 0;
}

int session_is_third_job(struct Session *self, int job)
{
    static const struct JobRange jobs[] = {
        { 0x1018, 0x1018 }, { 0x101f, 0x101f }, { 0x1021, 0x1021 }, { 0x1024, 0x1025 },
        { 0x102c, 0x102c }, { 0x102f, 0x102f }, { 0x1034, 0x1034 }, { 0x1037, 0x1037 },
        { 0x104d, 0x1054 }, { 0x1056, 0x1056 }, { 0x1065, 0x1066 }, { 0x1069, 0x106a },
        { 0x106d, 0x106e }, { 0x1070, 0x1071 },
    };
    if ((unsigned)(job - 0xfd6) <= 0x3a)
        return 1;
    return in_ranges(job, jobs, sizeof jobs / sizeof jobs[0]);
}

int session_is_pc_job(struct Session *self, int job)
{
    return (unsigned)(job - 0xfa1) <= 0x7ce || (unsigned)job <= 0x1e;
}
