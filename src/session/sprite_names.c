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
    case 0xfd1: return 0x1015;  case 0x107e: return 0x1086;  case 0x1081: return 0x1089;
    case 0x1082: return 0x108a;  case 0x1083: return 0x108b;  case 0x1084: return 0x108c;
    case 0x108e: return 0x108a;
    default:
        return job;
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
