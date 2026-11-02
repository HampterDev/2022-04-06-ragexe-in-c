#include "ui/resource/resource_paths.h"
#include "game/context.h"
#include "session/csession.h"
#include "session/session_lua.h"
#include "session/session_names.h"

#include <string.h>
#include "actor/types.h"
#include "platform/clock.h"

struct String *session_get_accessory_name(struct Session *self,
                                                     struct String *out, int accessory)
{
    const char *name = NULL;

    string_init(out);
    if (accessory > 0 &&
        client_lua_call(self->luaState, "ReqAccName", "d>s", accessory, &name) && name)
        string_assign_cstr(out, name);
    return out;
}

struct String *session_get_job_sprite_name(struct Session *self,
                                                     struct String *out, int job)
{
    string_init(out);
    string_assign_cstr(out, ((const char **)self->jobSpriteNames.first)[job]);
    return out;
}

struct String *session_get_sex_sprite_name(struct Session *self,
                                                     struct String *out, int sex)
{
    string_init(out);
    string_assign_cstr(out, ((const char **)self->sexSpriteNames.first)[sex]);
    return out;
}

struct String *session_get_weapon_sprite_name(struct Session *self,
                                                        struct String *out, int weapon)
{
    int real = session_get_real_weapon_id(weapon);

    string_init(out);
    string_assign_cstr(out, ((const char **)self->weaponSpriteNames.first)[real]);
    return out;
}

struct ItemInfo *session_get_viewed_costume(struct Session *self,
                                                   struct ItemInfo *out, int slot)
{
    if (slot == 15)
        item_info_construct_with_arg(out);
    else if ((unsigned)slot <= 9)
        item_info_copy(out, &self->viewedCostumes[slot]);
    else
        item_info_construct_with_arg(out);
    return out;
}

int session_get_transform_job(struct Session *self)
{
    int value = self->own_transform_job0 != -1 ? self->own_transform_job0 : -1;

    if (self->own_transform_job1 != -1)
        value = self->own_transform_job1;
    if (self->own_transform_job2 != -1)
        value = self->own_transform_job2;
    return value;
}

void session_set_transform_job(struct Session *self, int value, int index)
{
    if (index == 0)
        self->own_transform_job0 = value;
    else if (index == 1)
        self->own_transform_job1 = value;
    else if (index == 2)
        self->own_transform_job2 = value;
}

struct String *session_get_shield_sprite_name(struct Session *self,
                                                        struct String *out, int shield)
{
    string_init(out);
    string_assign_cstr(out, ((const char **)self->shieldSpriteNames.first)[shield]);
    return out;
}

struct String *session_get_robe_act_name(struct String *out,
                                                   int a, int b, int c, int d)
{
    resource_path_robe(out, 0, a, b, c, d);
    return out;
}

struct String *session_get_robe_spr_name(struct String *out,
                                                   int a, int b, int c, int d)
{
    resource_path_robe(out, 1, a, b, c, d);
    return out;
}

struct String *session_get_weapon_act_name(struct String *out,
                                                     int a, int b, int c, int d)
{
    resource_path_weapon(out, 0, a, b, c, d);
    return out;
}

struct String *session_get_weapon_spr_name(struct String *out,
                                                     int a, int b, int c, int d)
{
    resource_path_weapon(out, 1, a, b, c, d);
    return out;
}

struct String *session_get_glow_weapon_act_name(struct String *out, int a, int b, int c, int d, int e)
{
    resource_path_weapon_glow(out, 0, a, b, c, d, e);
    return out;
}

struct String *session_get_glow_weapon_spr_name(struct String *out, int a, int b, int c, int d, int e)
{
    resource_path_weapon_glow(out, 1, a, b, c, d, e);
    return out;
}

int session_get_motion_stepping(void *master, void *slave, int action, int block, DWORD start_time,
                                struct MotionStepping *out)
{
    int masterCount, slaveCount;

    if (!master || !slave)
        return 0;
    masterCount = (int)actres_frame_count(master, action);
    slaveCount = (int)actres_frame_count(slave, action);
    if (masterCount <= 0 || slaveCount <= 0 || masterCount >= slaveCount)
        return 0;
    if (slaveCount % masterCount != 0)
        return 0;
    out->ratio = slaveCount / masterCount;
    out->delay = (int)act_res_get_delay(slave, action);
    if (out->delay == 0)
        out->delay = 4;
    out->block = block;
    out->start_time = start_time;
    return 1;
}

int motion_stepping_frame(const struct MotionStepping *stepping)
{
    DWORD elapsed = platform_ticks_ms() - stepping->start_time;
    double wall = (double)elapsed;

    return (int)((float)wall / 24.0f / (float)stepping->delay) % stepping->ratio + stepping->ratio * stepping->block;
}

void *session_get_stepped_motion(void *master, void *slave,
                                          int action, int block, DWORD startTime, int *frame)
{
    struct MotionStepping stepping;

    if (!session_get_motion_stepping(master, slave, action, block, startTime, &stepping))
        return NULL;
    *frame = motion_stepping_frame(&stepping);
    return act_res_get_motion(slave, action, *frame);
}

int session_get_mounted_job(int job)
{
    switch (job) {
    case 0x0: return 0x101c;  case 0x1: return 0x1014;  case 0x2: return 0x1022;
    case 0x3: return 0x101a;  case 0x4: return 0x101e;  case 0x5: return 0x1017;
    case 0x6: return 0x102d;  case 0x7: return 0x1067;  case 0x8: return 0x103c;
    case 0x9: return 0x1026;  case 0xa: return 0x102a;  case 0xb: return 0x103a;
    case 0xc: return 0x1031;  case 0xe: return 0x106b;  case 0xf: return 0x101d;
    case 0x10: return 0x1023;  case 0x11: return 0x102e;  case 0x12: return 0x1029;
    case 0x13: return 0x1035;  case 0x14: return 0x1033;  case 0x17: return 0x1020;
    case 0x18: return 0x1013;  case 0x19: return 0x1012;  case 0xfa1: return 0x1057;
    case 0xfa2: return 0x1058;  case 0xfa3: return 0x1059;  case 0xfa4: return 0x105a;
    case 0xfa5: return 0x105b;  case 0xfa6: return 0x105c;  case 0xfa7: return 0x105d;
    case 0xfa8: return 0x1068;  case 0xfa9: return 0x103d;  case 0xfaa: return 0x1028;
    case 0xfab: return 0x1016;  case 0xfac: return 0x1036;  case 0xfad: return 0x1032;
    case 0xfaf: return 0x106c;  case 0xfb0: return 0x102b;  case 0xfb1: return 0x1027;
    case 0xfb2: return 0x1030;  case 0xfb3: return 0x1019;  case 0xfb4: return 0x1039;
    case 0xfb5: return 0x1038;  case 0xfb7: return 0x103e;  case 0xfb8: return 0x103f;
    case 0xfb9: return 0x1040;  case 0xfba: return 0x1041;  case 0xfbb: return 0x1042;
    case 0xfbc: return 0x1043;  case 0xfbd: return 0x1055;  case 0xfbe: return 0x106f;
    case 0xfbf: return 0x1062;  case 0xfc0: return 0x1063;  case 0xfc1: return 0x1064;
    case 0xfc2: return 0x1044;  case 0xfc3: return 0x1045;  case 0xfc5: return 0x1072;
    case 0xfc6: return 0x1046;  case 0xfc7: return 0x1047;  case 0xfc8: return 0x1048;
    case 0xfc9: return 0x1049;  case 0xfca: return 0x104a;  case 0xfcb: return 0x104b;
    case 0xfcd: return 0x104c;  case 0xfce: return 0x103b;  case 0xfcf: case 0xfd0: return 0x101b;
    case 0xfd1: return 0x1015;  case 0xfd6: return 0x106a;  case 0xfd7: return 0x1025;
    case 0xfd8: return 0x1066;  case 0xfd9: return 0x1021;  case 0xfda: return 0x1065;
    case 0xfdb: return 0x102c;  case 0xfdc: return 0x106a;  case 0xfdd: return 0x1025;
    case 0xfde: return 0x1066;  case 0xfdf: return 0x1021;  case 0xfe0: return 0x1065;
    case 0xfe1: return 0x102c;  case 0xfe2: return 0x1069;  case 0xfe3: return 0x1024;
    case 0xfe4: return 0x1034;  case 0xfe5: return 0x1037;  case 0xfe6: return 0x101f;
    case 0xfe7: return 0x1018;  case 0xfe8: return 0x102f;  case 0xfe9: return 0x1069;
    case 0xfea: return 0x1024;  case 0xfeb: return 0x1034;  case 0xfec: return 0x1037;
    case 0xfed: return 0x101f;  case 0xfee: return 0x1018;  case 0xfef: return 0x102f;
    case 0x1000: return 0x1071;  case 0x1001: return 0x104d;  case 0x1002: return 0x106e;
    case 0x1003: return 0x104e;  case 0x1004: return 0x106d;  case 0x1005: return 0x104f;
    case 0x1006: return 0x1070;  case 0x1007: return 0x1050;  case 0x1008: return 0x1051;
    case 0x1009: return 0x1052;  case 0x100a: return 0x1053;  case 0x100b: return 0x1054;
    case 0x100c: return 0x1056;  case 0x105e: return 0x1060;  case 0x105f: return 0x1061;
    case 0x1073: return 0x1075;  case 0x1074: return 0x1076;  case 0x1077: return 0x1078;
    case 0x107a: return 0x107b;  case 0x107c: return 0x107d;  case 0x107e: return 0x1086;
    case 0x107f: return 0x1087;  case 0x1080: return 0x1088;  case 0x1081: return 0x1089;
    case 0x1082: return 0x108a;  case 0x1083: return 0x108b;  case 0x1084: return 0x108c;
    case 0x1085: return 0x108d;  case 0x108e: return 0x108a;  case 0x108f: return 0x1095;
    case 0x1090: return 0x1096;  case 0x1091: return 0x1097;  case 0x1092: return 0x1098;
    case 0x1093: return 0x1095;  case 0x1094: return 0x1097;  case 0x109c: return 0x10a9;
    case 0x109d: return 0x10aa;  case 0x109e: return 0x10ab;  case 0x109f: return 0x10ac;
    case 0x10a0: return 0x10ad;  case 0x10a1: return 0x10ae;  case 0x10a2: return 0x10af;
    case 0x10a3: return 0x10b0;  case 0x10a4: return 0x10b1;  case 0x10a5: return 0x10b2;
    case 0x10a6: return 0x10b3;  case 0x10a7: return 0x10b4;  case 0x10a8: return 0x10b5;
    case 0x10ce: return 0x10d5;  case 0x10cf: return 0x10d6;  case 0x10d0: return 0x10d7;
    case 0x10d1: return 0x10d8;  case 0x10d2: return 0x10d9;  case 0x10d3: return 0x10da;
    case 0x10dc: return 0x10d5;    default:
        return job == 0x10d4 ? 0x10db : job;
    }
}

int session_get_item_view_id(int id)
{
    return item_table_get(id)->class_num;
}

int session_get_weapon_view_id(int id)
{
    int value = item_table_get(id)->class_num;
    if (id > 0x834 && id < 0x898)
        return -1;
    if ((unsigned)(id - 0x70e5) <= 0x61)
        return -1;
    if ((unsigned)(id - 0x704e0) <= 0x270f)
        return -1;
    return value;
}
