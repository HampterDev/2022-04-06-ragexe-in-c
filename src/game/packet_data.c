#include "core/compiler.h"
#include "contents/player_names.h"
#include "skills/skill_bundle.h"
#include "contents/guild_info.h"
#include "session/state.h"
#include "world/mode_manager.h"
#include "game/context.h"
#include "game/packet_data.h"
#include "skills/skill_info.h"
#include "game/client.h"
#include "session/session_party.h"
#include "session/session_skills.h"
#include "ui/ui_std.h"

void packet_decode_move_data(const unsigned char *p, int *x0, int *y0, int *x1, int *y1, int *sx, int *sy)
{
    *x0 = (p[0] << 2) | (p[1] >> 6);
    *y0 = ((p[1] & 0x3f) << 4) | (p[2] >> 4);
    *x1 = ((p[2] & 0xf) << 6) | (p[3] >> 2);
    *y1 = ((p[3] & 3) << 8) | p[4];
    *sx = p[5] >> 4;
    *sy = p[5] & 0xf;
}

int job_get_base_job(int job)
{
    switch (job) {
    case 4001: case 4023:
        return 0;
    case 4002: case 4024:
        return 1;
    case 4003: case 4025:
        return 2;
    case 4004: case 4026:
        return 3;
    case 4005: case 4027:
        return 4;
    case 4006: case 4028:
        return 5;
    case 4007: case 4029:
        return 6;
    case 4008: case 4030: case 4054: case 4060: case 4096: case 4252:
        return 7;
    case 4009: case 4031: case 4057: case 4063: case 4099: case 4256:
        return 8;
    case 4010: case 4032: case 4055: case 4061: case 4097: case 4255:
        return 9;
    case 4011: case 4033: case 4058: case 4064: case 4100: case 4253:
        return 10;
    case 4012: case 4034: case 4056: case 4062: case 4098: case 4257:
        return 11;
    case 4013: case 4035: case 4059: case 4065: case 4101: case 4254:
        return 12;
    case 4015: case 4037: case 4066: case 4073: case 4102: case 4258:
        return 14;
    case 4016: case 4038: case 4070: case 4077: case 4106: case 4262:
        return 15;
    case 4017: case 4039: case 4067: case 4074: case 4103: case 4261:
        return 16;
    case 4018: case 4040: case 4072: case 4079: case 4108: case 4260:
        return 17;
    case 4019: case 4041: case 4071: case 4078: case 4107: case 4259:
        return 18;
    case 4020: case 4042: case 4068: case 4075: case 4104: case 4263:
        return 19;
    case 4021: case 4043: case 4069: case 4076: case 4105: case 4264:
        return 20;
    case 4045: case 4190: case 4191: case 4307:
        return 23;
    case 4215: case 4228: case 4229: case 4306:
        return 24;
    case 4211: case 4212: case 4222: case 4223: case 4224: case 4304: case 4305:
        return 25;
    case 4225:
        return 4046;
    case 4226: case 4239: case 4241: case 4302:
        return 4047;
    case 4227: case 4240: case 4242: case 4303:
        return 4049;
    case 4220: case 4308:
        return 4218;
    default:
        return job;
    }
}

static void int_map_set(struct Tree *m, int key, int value)
{
    struct IntIntPair *pair = tree_get(m, &key, sizeof key, sizeof *pair, tree_less_int);

    pair->value = value;
}

struct Tree *state_ids_by_value(struct Tree *out)
{
    tree_init(out);
    int_map_set(out, 0, 0);
    int_map_set(out, 1, 0x36b);
    int_map_set(out, 2, 0x36c);
    int_map_set(out, 3, 0x36d);
    int_map_set(out, 4, 0x36e);
    int_map_set(out, 5, 0);
    int_map_set(out, 6, 0x370);
    int_map_set(out, 7, 0x371);
    int_map_set(out, 8, 0x372);
    return out;
}

struct Tree *state_ids_by_flag(struct Tree *out)
{
    tree_init(out);
    int_map_set(out, 1, 0x373);
    int_map_set(out, 2, 0x374);
    int_map_set(out, 4, 0x375);
    int_map_set(out, 8, 0x376);
    int_map_set(out, 0x10, 0x377);
    int_map_set(out, 0x20, 0x378);
    int_map_set(out, 0x40, 0x379);
    int_map_set(out, 0x80, 0x37a);
    int_map_set(out, 0x100, 0x37b);
    return out;
}

void client_add_skill(unsigned short id, short level, int type, unsigned char upgradable, short sp_cost,
                               short range, short level2)
{
    struct SkillInfo info;
    skill_info_construct(&info);
    info.type = type;
    info.id = id;
    info.level = level;
    info.upgradable = upgradable;
    info.id_name = skill_info_get_id_name(&info);
    info.sp_cost = sp_cost;
    info.range = range;
    info.level2 = level2;
    info.ap_amount = skill_info_get_ap_amount(&info, level);
    player_skill_bundle_note_level_raise(&g_session1.skills, &info);
    if (info.id == 0x150)
        g_session1.married = 1;
    session_put_skill(&g_session1, &info, 1, 0);
    skill_info_destruct(&info);
}

RAGE_NOINLINE void game_mode_drop_aid_entry(struct GameMode *self, unsigned aid,
                                                        const struct String *text)
{
    struct AwaitingName *entry = tree_find(&self->awaiting_name, &aid, tree_less_uint);

    if (!entry)
        return;
    mode_send_msg(self, 0x8c, (int)string_cstr(text), entry->request, 0, 0);
    tree_erase(&self->awaiting_name, TREE_NODE_OF(entry), NULL);
}

void client_post_aid_text(int aid, struct String text)
{
    player_names_set(aid, text_view(string_cstr(&text), text.size));
    game_mode_drop_aid_entry(game_current_mode(), aid, &text);
    guild_info_resolve_name(&g_guildInfo, aid, string_cstr(&text));
    session_on_pending_name(&g_session1, aid, string_cstr(&text));
    session_rename_friend(&g_session1, aid, string_cstr(&text));
    string_free(&text);
}
