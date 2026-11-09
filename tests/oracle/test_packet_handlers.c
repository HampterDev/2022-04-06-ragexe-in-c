#include "skill_oracle.h"
#include "game/packet_handlers.h"
#include "game/effect_notify.h"
#include "game/world_objects.h"
#include "game/packet_data.h"
#include "web/name_card_api.h"
#include "ui/window_messages.h"
#include "ui/window_access.h"
#include "ui/shops/merchant_shop.h"
#include "effects/effect_controls.h"
#include "effects/skills/emitter_effect.h"
#include "contents/client_lua_calls.h"
#include "contents/connection_api.h"
#include "session/session_items.h"
#include "session/session_lua.h"
#include "session/session_windows.h"
#include "session/abr.h"
#include "std/string.h"
#include "std/tree.h"
#include "std/integer_map.h"
#include "platform/error_message.h"
#include "game/actor_entry.h"
#include "game/entry_packet_handlers.h"
#include "game/actor_marks.h"
#include "contents/champion_monster_effects.h"
#include "contents/player_names.h"
#include "world/world.h"
#include "session/session_jobs.h"
#include "session/session_talk.h"
#include "session/character_name.h"
#include "platform/clientinfo.h"
#include "game/display_settings.h"
#include "game/character_stats.h"
#include "game/status_values.h"
#include "session/session_state.h"
#include "ui/character/gauge.h"
#include "ui/character/status_wnd.h"
#include "iteminfo/item_api.h"
#include "game/item_info.h"
#include "contents/pet_evolution.h"
#include "session/item_predicates.h"
#include "ui/shops/storage.h"
#include "ui/wndmgr/wndmgr.h"
#include "core/unknown_packet.h"
#include "packet_handlers.inc"

#define ORIG_SESSION          0x1133c50u
#define ORIG_SESSION_SIZE     0x83b0u
#define ORIG_EFFECT_ON        0x1138cb4u
#define ORIG_SERVICE_TYPE     0x1226950u
#define ORIG_INSTANT_VARS     0x113b620u
#define ORIG_PLAYER_TARGET    0x4fc
#define ORIG_MODE_NAMES       0x160
#define ORIG_MODE_DIALOG_OPEN 0x24c
#define ORIG_MODE_PLAYER_DEAD 0x250
#define ORIG_NAMES_ERASE      0x8af760u
#define ORIG_NAMES_SET_ALL    0x8b0480u
#define ORIG_ERASE_ABR        0x874db0u
#define ORIG_WORLD_REMOVE     0x65e390u
#define ORIG_JOB_IS_MONSTER   0x74bef0u
#define ORIG_HOMUN_OR_MERC    0x883dd0u
#define ORIG_IS_HOMUNCULUS    0x883d20u
#define ORIG_LAUNCH_DUMMY     0x779bc0u
#define ORIG_PET_HUNGER       0x8774c0u
#define ORIG_PET_INTIMACY     0x877360u
#define ORIG_PET_EMOTION      0x877280u
#define ORIG_PET_TALK         0x8787d0u
#define ORIG_NOTIFY_PLATES    0x86c3b0u
#define ORIG_MESSAGE_BOX      0x62e260u
#define ORIG_POST_QUIT        0x62b6c0u
#define ORIG_MAKE_WINDOW      0x6349a0u
#define ORIG_CLEAR_PURCHASE   0x84fc60u
#define ORIG_ACTOR_SET_JOB    0x75f300u
#define ORIG_SESSION_SET_JOB  0x899b50u
#define ORIG_WEAPON_VIEW      0x87f2b0u
#define ORIG_SET_COMBO_DELAY  0x88b9b0u
#define ORIG_FORCE_ANIM_END   0x75eee0u
#define ORIG_SESSION_ACTOR    0x875cd0u
#define ORIG_SET_ARGB         0x75cf20u
#define ORIG_CONN_INSTANCE    0x90bac0u
#define ORIG_CLEAR_HELD       0x90b640u
#define ORIG_TITLE_STRING     0x87eff0u
#define ORIG_EMITTER_SPEED    0x994520u
#define ORIG_EMITTER_COUNT    0x9944f0u
#define ORIG_STOP_AURA        0x749ca0u
#define ORIG_LEVEL_AURA       0x7497d0u
#define ORIG_REMOVE_TYPE      0x96b3d0u
#define ORIG_REROLL           0x84f5f0u
#define ORIG_REFRESH_BASIC    0x867510u
#define ORIG_IS_PC_JOB_ACTOR  0x74bf70u
#define ORIG_SPRINTF          0x4bcf10u
#define ORIG_SESSION_AID      0x1134c14u
#define ORIG_PET_ACCESSORY    0x1134c3cu
#define ORIG_HOMUN_AI_FLAG    0x1138e84u
#define ORIG_MERC_PRESENT     0x1138f28u
#define ORIG_PVP_FIRST_TIME   0x11343acu
#define ORIG_PVP_RULE         0x1138cc8u
#define ORIG_REPLAY_PLAYING   0x1119d38u
#define ORIG_MODE_SQUARE_MARKS   0x1b4
#define ORIG_MODE_TRIANGLE_MARKS 0x1bc
#define ORIG_WORLD_ACTORS     0x10
#define ORIG_PLAYER_PVP_RANK  0x474
#define ORIG_PLAYER_PVP_TOTAL 0x478
#define ORIG_ACTOR_BALLOON_FONT 0xfc
#define ORIG_ACTOR_MARK_LISTED  0x2e8
#define ORIG_ACTOR_IS_BOSS      0x308
#define ORIG_ACTOR_OBJECT_TYPE  0x318
#define ORIG_PC_SIZE          0x4d8
#define ORIG_OPERATOR_NEW     0xb461beu
#define ORIG_PC_CONSTRUCT     0x82e5e0u
#define ORIG_NEW_ACTOR_FOR_JOB 0x777890u
#define ORIG_LIST_BUY_NODE    0x659710u
#define ORIG_ACTOR_LIST_BUY_NODE 0x7979e0u
#define ORIG_OBJECT_LIST_PUSH 0x70bbb0u
#define ORIG_ACTOR_LIST_PUSH  0x7e3530u
#define ORIG_MARKS_FIND       0x797860u
#define ORIG_MAKE_EMBLEM_WND  0x7737e0u
#define ORIG_IS_SIEGE_MODE    0x884040u
#define ORIG_IS_PLAYER_JOB    0x884010u
#define ORIG_IS_HOMUNCULUS_AID 0x883ec0u
#define ORIG_IS_MERCENARY_AID 0x883ee0u
#define ORIG_PET_ACCESSORY_ACT 0x877120u
#define ORIG_LOAD_ACT_RES     0x75cc50u
#define ORIG_CHAMPION_EFFECTS 0x9238b0u
#define ORIG_PLAYER_NAMES_SET 0x8f1710u
#define ORIG_PLACE_NOWHERE    0x65ed90u
#define ORIG_PLACE_AT_CELL    0x65ec20u
#define ORIG_NAMES_SET_NAME   0x8b0da0u
#define ORIG_SYNC_SERVER_TIME 0x86dc90u
#define ORIG_CHARACTER_NAME   0x875550u
#define ORIG_IS_ADMIN_OPTION  0x8682a0u
#define ORIG_IS_ADMIN_AID     0xb05da0u
#define ORIG_INVALIDATE_WINDOWS 0x867610u
#define ORIG_IS_SUPER_NOVICE  0x884100u
#define ORIG_DRAWN_JOB        0x8539e0u
#define ORIG_BASIC_INFO_HEIGHT 0xaf8c50u
#define ORIG_BAR_SET_VALUES   0x568330u
#define ORIG_REFRESH_ACTOR_EFFECTS 0x772130u
#define ORIG_GET_WINDOW       0x6409d0u
#define ORIG_REFRESH_SKILL_WNDS 0x88af30u
#define ORIG_STATUS_RESTORE   0x57b6f0u
#define ORIG_END_EXCHANGE     0x84f3f0u
#define ORIG_FIND_ABR         0x875070u
#define ORIG_ABR_GET          0x7e6110u
#define ORIG_SET_ADVANCED_STATUS 0x7ae5e0u
#define ORIG_PET_TALK_DUE     0x8903b0u
#define ORIG_WND_STATUS       0xee8974u
#define ORIG_WND_BASIC_INFO   0xee898cu
#define ORIG_WND_GROUP        0xee8a78u
#define ORIG_WND_HEIGHT       0x18
#define ORIG_WND_BASE_BAR     0xd0
#define ORIG_WND_JOB_BAR      0xd4
#define ORIG_WINDOW_VT_SEND   (0x88 / 4)
#define ORIG_MODE_EXCHANGE_OPEN 0x114
#define ORIG_MODE_STATUS_PENDING 0x2e4
#define ORIG_ABR_HP           0x1134374u
#define ORIG_ABR_MAX_HP       0x113437cu
#define ORIG_HP               0x1138ddcu
#define ORIG_MAX_HP           0x1138de0u
#define ORIG_SP               0x1138de4u
#define ORIG_MAX_SP           0x1138de8u
#define ORIG_WALK_SPEED       0x11352e4u
#define ORIG_KARMA            0x11352f4u
#define ORIG_MANNER           0x11352e8u
#define ORIG_STATUS_POINTS    0x1135244u
#define ORIG_BASE_LEVEL       0x1135240u
#define ORIG_JOB_LEVEL        0x1135248u
#define ORIG_SKILL_POINTS     0x113524cu
#define ORIG_WEIGHT           0x11352f0u
#define ORIG_MAX_WEIGHT       0x11352ecu
#define ORIG_WEIGHT_PENALTY   0x113b5ecu
#define ORIG_STATS            0x1135274u
#define ORIG_STAT_COSTS       0x113528cu
#define ORIG_STAT_BONUS       0x113525cu
#define ORIG_TRAIT_BONUS      0x113b638u
#define ORIG_ATK              0x11352a8u
#define ORIG_REFINE_ATK       0x11352bcu
#define ORIG_MATK_MAX         0x11352c0u
#define ORIG_MATK_MIN         0x11352c4u
#define ORIG_DEF              0x11352b4u
#define ORIG_PLUS_DEF         0x11352b8u
#define ORIG_MDEF             0x11352acu
#define ORIG_PLUS_MDEF        0x11352c8u
#define ORIG_HIT              0x11352ccu
#define ORIG_FLEE             0x11352d0u
#define ORIG_PLUS_FLEE        0x11352d8u
#define ORIG_CRITICAL         0x11352d4u
#define ORIG_ASPD             0x11352a4u
#define ORIG_PLUS_ASPD        0x11352b0u
#define ORIG_BASE_EXP         0x1135220u
#define ORIG_NEXT_BASE_EXP    0x1135228u
#define ORIG_NEXT_JOB_EXP     0x1135230u
#define ORIG_JOB_EXP          0x1135238u
#define ORIG_ZENY             0x11352e0u
#define ORIG_PET_FULLNESS     0x1134c48u
#define ORIG_PET_INTIMACY_VAL 0x1134c50u
#define ORIG_PET_JOB          0x1134c40u
#define ORIG_HATCHED_EGG      0x1134c54u
#define ORIG_PET_AUTO_FEED    0x1135200u
#define ORIG_PACKET_BUFFER    0x1121a10u
#define ORIG_ITEM_CONSTRUCT   0x92c970u
#define ORIG_ITEM_SET_ID      0x9305f0u
#define ORIG_ADD_CART_ITEM    0x84e390u
#define ORIG_ADD_STORAGE_ITEM 0x84ef00u
#define ORIG_INVENTORY_UPDATE 0x86cd50u
#define ORIG_ADD_INVENTORY_ITEM 0x84dfd0u
#define ORIG_SET_WORN_ITEM    0x84dd10u
#define ORIG_SET_WORN_COSTUME 0x873c40u
#define ORIG_REMOVE_FILTER_TYPES 0x84faa0u
#define ORIG_REMOVE_OTHER_TYPES 0x84f900u
#define ORIG_CLEAR_WORN_ITEMS 0x84fa60u
#define ORIG_CLEAR_WORN_COSTUMES 0x874c40u
#define ORIG_CLEAR_INVENTORY  0x84fc30u
#define ORIG_CLEAR_CART       0x84fcc0u
#define ORIG_REFRESH_PARTY_UI 0x86c2d0u
#define ORIG_REFRESH_FRIEND_UI 0x86c3e0u
#define ORIG_REFRESH_ITEM_WNDS 0x640e00u
#define ORIG_IS_PET_EGG_ID    0x884020u
#define ORIG_IS_COSTUME_LOCATION 0x883a80u
#define ORIG_GET_INVENTORY_ITEM 0x8532e0u
#define ORIG_PET_AUTO_FEEDING 0x91b940u
#define ORIG_SEND_PET_AUTO_FEED 0x899bd0u
#define ORIG_SET_TEXT_ELLIPSIS 0x5380c0u
#define ORIG_STORAGE_SET_NAME 0x7e3670u
#define ORIG_WND_TITLE_CTRL   0x13c
#define OI_TYPE 0x00
#define OI_INDEX 0x04
#define OI_EQUIP_LOCATION 0x08
#define OI_WORN_LOCATION 0x0c
#define OI_COUNT 0x10
#define OI_PRICE 0x14
#define OI_CHARGED_PRICE 0x18
#define OI_SLOT 0x1c
#define OI_ID_TEXT 0x2c
#define OI_NAME 0x44
#define OI_IDENTIFIED 0x5c
#define OI_DAMAGED 0x5d
#define OI_REFINE 0x60
#define OI_BIND 0x64
#define OI_EXPIRE_HOURS 0x66
#define OI_EXPIRE 0x68
#define OI_WEIGHT 0x6c
#define OI_VIEW 0x70
#define OI_FAVORITE 0x74
#define OI_IS_NEW 0x75
#define OI_MOVE_PENDING 0x76
#define OI_BY_INDEX 0x77
#define OI_SALE_KIND 0x78
#define OI_SALE_START 0x7c
#define OI_SALE_END 0x80
#define OI_SALE_REMAINING 0x84
#define OI_GRADE 0x88
#define OI_OPTION_COUNT 0x8c
#define OI_OPTIONS 0x90
#define ORIG_STD_STRING       24
#define ORIG_STD_STRING_SSO   16

#define ORIG_ACTOR_VT_SET_HEAD        (0x58 / 4)
#define ORIG_ACTOR_VT_SET_WEAPON      (0x5c / 4)
#define ORIG_ACTOR_VT_SET_WEAPON_LOOK (0x60 / 4)
#define ORIG_ACTOR_VT_SET_ACCESSORY   (0x64 / 4)
#define ORIG_ACTOR_VT_SET_ACCESSORY2  (0x68 / 4)
#define ORIG_ACTOR_VT_SET_ACCESSORY3  (0x6c / 4)
#define ORIG_ACTOR_VT_SET_SHIELD      (0x70 / 4)
#define ORIG_ACTOR_VT_SET_APPEARANCE  (0x74 / 4)
#define ORIG_ACTOR_VT_SET_IMF_NAME    (0x78 / 4)
#define ORIG_ACTOR_VT_SET_HEAD_PAL    (0x7c / 4)
#define ORIG_ACTOR_VT_SET_BODY_PAL    (0x80 / 4)
#define ORIG_ACTOR_VT_SET_BODY_PALETTE (0x84 / 4)
#define ORIG_ACTOR_VT_SET_HEAD_PALETTE (0x88 / 4)
#define ORIG_ACTOR_VT_SET_HONOR       (0xa0 / 4)
#define ORIG_ACTOR_VT_SET_ROBE        (0xac / 4)
#define ORIG_ACTOR_VT_SET_BODY_STYLE  (0xb0 / 4)
#define ORIG_ACTOR_VT_REFRESH_ACC3    (0xe8 / 4)
#define ORIG_ACTOR_VT_SET_GUILD       (0xc0 / 4)
#define ORIG_ACTOR_VT_GET_GUILD_ID    (0xc4 / 4)

#define AID_UNKNOWN 4242
#define PACKET_MAX 256
#define SWEEP_VALUES 16
#define MAX_REPORTED 40
#define FAKE_CONNECTION 0x5151

enum { FILL_ZERO, FILL_PATTERN, FILL_ONES, FILL_COUNT };
enum { PK_DEFAULT, PK_NAMES, PK_ENTRY, PK_PAR, PK_EXP, PK_LIST, PK_SPLIT_SET };

enum { R_SESSION, R_MODE, R_CASTER, R_TARGET, R_PLAYER, R_EFFECT, R_NEW, R_COUNT };
#define M(r) (1u << (r))
#define M_ACTORS (M(R_CASTER) | M(R_TARGET) | M(R_PLAYER) | M(R_NEW))

struct Region {
    const char *name;
    unsigned char *orig, *lift;
    unsigned orig_size, lift_size;
    unsigned char *orig_snap, *lift_snap, *orig_seen, *lift_seen;
};

struct Field {
    const char *name;
    unsigned mask;
    unsigned orig_off, lift_off, size;
    int quiet;
};

static struct Region regions[R_COUNT];
static struct IntInt64Pair lift_instant_var;
static long long orig_instant_var;
static int instant_var_set, lift_instant_var_got;
static unsigned char lift_connection[0x100];
static unsigned char lift_names_holder;
static struct UIWindow *lift_window;
static unsigned char orig_new_actor[0x1000];
static union { struct Pc pc; unsigned char raw[0x1000]; } lift_new_actor;
static int sweep_job, move_mask;
static unsigned orig_actor_list_head[3];
static unsigned fake_nodes[4][4];
static int fake_node_count;
static unsigned fake_tree_node[8];
static struct MemberMark fake_mark;
static int drawn_job, windows_present;
static unsigned char orig_status_wnd[0x400], orig_basic_info_wnd[0x400], orig_group_wnd[0x400], orig_found_wnd[0x400];
static struct UIWindow lift_status_wnd, lift_basic_info_wnd, lift_group_wnd, lift_found_wnd;
static void *orig_window_vt[64];
static long long fake_abr_value[2];
static int fake_abr_used[2];
static struct Int64Pair lift_abr_pair[2];
static struct IntIntPair lift_adv_pair;
static int adv_pending, adv_key, adv_value;
static unsigned char orig_storage_wnd[0x400];
static struct UIWindow lift_storage_wnd;
struct PetEvolutionMgr *g_petTable;
BYTE g_packetBuffer[0x10000];
void vec_free(struct Vec *v) { (void)v; }

static const char *window_name(const void *w)
{
    if (w == orig_status_wnd || w == &lift_status_wnd) return "status";
    if (w == orig_basic_info_wnd || w == &lift_basic_info_wnd) return "bi";
    if (w == orig_group_wnd || w == &lift_group_wnd) return "group";
    if (w == orig_found_wnd || w == &lift_found_wnd) return "found";
    if (w == orig_storage_wnd || w == &lift_storage_wnd) return "storage";
    return w ? "?" : "null";
}
static int is_super_novice(int job)
{
    return job == 0x17 || job == 0xfcd || job == 0x1020 || job == 0x104c || (job >= 0x105e && job <= 0x1061) || job == 0x10d3 || job == 0x10da;
}
static const char *orig_std_string(const unsigned char *s);
static int is_pet_egg(int id) { return (unsigned)(id - 9001) <= 0x1f2; }
static int is_costume_location(int flag) { return flag != 0 && flag % 3 == 1; }
static const char *item_fields_text(int index, const char *id, const char *name, int type, int eq, int worn, int count, int price,
                                    int charged, const int *slot, int identified, int damaged, int refine, int bind, int hours,
                                    int expire, int weight, int view, int favorite, int is_new, int move_pending, int by_index,
                                    int sale_kind, int sale_start, int sale_end, int sale_remaining, int grade, int option_count,
                                    const unsigned char *options)
{
    static char text[2][256];
    static int turn;
    char *out = text[turn ^= 1];
    int n, i;
    n = snprintf(out, 256, "i%d id'%s' n'%s' t%d eq%x w%x c%d p%d cp%d s%x,%x,%x,%x idf%d dmg%d r%d b%d eh%d x%x wt%d v%d f%d nw%d mp%d bi%d sk%d ss%d se%d sr%d g%d o%d",
                 index, id, name, type, eq, worn, count, price, charged, slot[0], slot[1], slot[2], slot[3], identified, damaged, refine,
                 bind, hours, expire, weight, view, favorite, is_new, move_pending, by_index, sale_kind, sale_start, sale_end, sale_remaining,
                 grade, option_count);
    for (i = 0; i < ITEM_OPTION_SLOTS; i++) {
        unsigned short oid = *(const unsigned short *)(options + 5 * i);
        short oval = *(const short *)(options + 5 * i + 2);
        unsigned char oparam = options[5 * i + 4];
        if (oid || oval || oparam)
            n += snprintf(out + n, 256 - n, " [%u %d %u]", oid, oval, oparam);
    }
    return out;
}
static const char *item_text(const struct ItemInfo *it)
{
    return item_fields_text(it->index, string_cstr(&it->id_text), string_cstr(&it->name), it->type, it->equip_location, it->worn_location,
                            it->count, it->price, it->charged_price, it->slot, it->identified, it->damaged, it->refine, it->bind_on_equip_type,
                            it->expire_in_hours, it->expire_time, it->weight, it->view, it->favorite, it->is_new, it->move_pending,
                            it->by_index, it->sale_kind, it->sale_start_time, it->sale_end_time, it->sale_remaining, it->grade,
                            it->option_count, (const unsigned char *)it->options);
}
#define OI32(b, off) (*(const int *)((b) + (off)))
#define OI16(b, off) (*(const unsigned short *)((b) + (off)))
#define OI8(b, off) ((b)[off])
static const char *orig_item_text(const unsigned char *b)
{
    return item_fields_text(OI32(b, OI_INDEX), orig_std_string(b + OI_ID_TEXT), orig_std_string(b + OI_NAME), OI32(b, OI_TYPE),
                            OI32(b, OI_EQUIP_LOCATION), OI32(b, OI_WORN_LOCATION), OI32(b, OI_COUNT), OI32(b, OI_PRICE),
                            OI32(b, OI_CHARGED_PRICE), (const int *)(b + OI_SLOT), OI8(b, OI_IDENTIFIED), OI8(b, OI_DAMAGED),
                            OI32(b, OI_REFINE), OI16(b, OI_BIND), OI8(b, OI_EXPIRE_HOURS), OI32(b, OI_EXPIRE), OI16(b, OI_WEIGHT),
                            OI32(b, OI_VIEW), OI8(b, OI_FAVORITE), OI8(b, OI_IS_NEW), OI8(b, OI_MOVE_PENDING), OI8(b, OI_BY_INDEX),
                            OI32(b, OI_SALE_KIND), OI32(b, OI_SALE_START), OI32(b, OI_SALE_END), OI32(b, OI_SALE_REMAINING),
                            *(const short *)(b + OI_GRADE), OI32(b, OI_OPTION_COUNT), b + OI_OPTIONS);
}
static int abr_slot(const void *tree)
{
    if (tree == &g_session1.abrHp || tree == oracle_addr(ORIG_ABR_HP)) return 0;
    if (tree == &g_session1.abrMaxHp || tree == oracle_addr(ORIG_ABR_MAX_HP)) return 1;
    return -1;
}

#define SF(f) offsetof(struct Session, f)
#define AF(f) offsetof(struct GameActor, f)
#define PF(f) offsetof(struct Player, f)
#define GF(f) offsetof(struct GameMode, f)
#define EF(f) offsetof(struct EmitterEffect, f)
#define S(a) ((a) - ORIG_SESSION)

static const struct Field fields[] = {
    { "hairStyle", M(R_SESSION), S(0x1134adcu), SF(hairStyle), 4 },
    { "weapon", M(R_SESSION), S(0x1134ae0u), SF(weapon), 8 },
    { "shield", M(R_SESSION), S(0x1134ae8u), SF(shield), 4 },
    { "clothesColor", M(R_SESSION), S(0x1134aecu), SF(clothesColor), 4 },
    { "hairColor", M(R_SESSION), S(0x1134af0u), SF(hairColor), 4 },
    { "headBottom", M(R_SESSION), S(0x1134af4u), SF(headBottom), 4 },
    { "headTop", M(R_SESSION), S(0x1134af8u), SF(headTop), 4 },
    { "headMid", M(R_SESSION), S(0x1134afcu), SF(headMid), 4 },
    { "robe", M(R_SESSION), S(0x1134b00u), SF(robe), 4 },
    { "body", M(R_SESSION), S(0x1134b08u), SF(body), 4 },
    { "game_over", M(R_SESSION), S(0x1134b68u), SF(game_over), 4 },
    { "hp", M(R_SESSION), S(0x1138ddcu), SF(hp), 4 },
    { "sp", M(R_SESSION), S(0x1138de4u), SF(sp), 4 },
    { "ownTitleId", M(R_SESSION), S(0x1139988u), SF(ownTitleId), 4 },
    { "target_aid", M(R_PLAYER), ORIG_PLAYER_TARGET, PF(target_aid), 4 },
    { "npc_dialog_open", M(R_MODE), ORIG_MODE_DIALOG_OPEN, GF(npc_dialog_open), 4 },
    { "player_dead", M(R_MODE), ORIG_MODE_PLAYER_DEAD, GF(player_dead), 4 },
    { "motion_held", M_ACTORS, 0x9e, AF(motion_held), 1 },
    { "argb_locked", M_ACTORS, 0xa3, AF(argb_locked), 1 },
    { "level", M_ACTORS, 0x214, AF(level), 2 },
    { "velocity_min", M(R_EFFECT), 0xc8, EF(velocity_min), 12 },
    { "velocity_max", M(R_EFFECT), 0xd4, EF(velocity_max), 12 },
    { "acceleration", M(R_EFFECT), 0xe0, EF(acceleration), 12 },
    { "spawn_box", M(R_EFFECT), 0xec, EF(spawn_box), 12 },
    { "size_min", M(R_EFFECT), 0x100, EF(size_min), 4 },
    { "size_max", M(R_EFFECT), 0x104, EF(size_max), 4 },
    { "life_min", M(R_EFFECT), 0x108, EF(life_min), 4 },
    { "life_max", M(R_EFFECT), 0x10c, EF(life_max), 4 },
    { "rate_min", M(R_EFFECT), 0x110, EF(rate_min), 4 },
    { "rate_max", M(R_EFFECT), 0x114, EF(rate_max), 4 },
    { "r", M(R_EFFECT), 0x120, EF(r), 4 },
    { "g", M(R_EFFECT), 0x124, EF(g), 4 },
    { "b", M(R_EFFECT), 0x128, EF(b), 4 },
    { "alpha", M(R_EFFECT), 0x12c, EF(alpha), 4 },
    { "src_blend", M(R_EFFECT), 0x148, EF(src_blend), 4 },
    { "dest_blend", M(R_EFFECT), 0x14c, EF(dest_blend), 4 },
    { "max_frames", M(R_EFFECT), 0x184, EF(max_frames), 4 },
    { "aid", M(R_SESSION), S(ORIG_SESSION_AID), SF(aid), 4 },
    { "petAccessory", M(R_SESSION), S(ORIG_PET_ACCESSORY), SF(petAccessory), 4 },
    { "homun_ai_flag", M(R_SESSION), S(ORIG_HOMUN_AI_FLAG), SF(homun_ai_flags), 4 },
    { "mercPresent", M(R_SESSION), S(ORIG_MERC_PRESENT), SF(mercPresent), 4 },
    { "pvpFirstPlaceTime", M(R_SESSION), S(ORIG_PVP_FIRST_TIME), SF(pvpFirstPlaceTime), 4 },
    { "pvp_rank", M(R_PLAYER), ORIG_PLAYER_PVP_RANK, PF(pvp_rank), 4 },
    { "pvp_total", M(R_PLAYER), ORIG_PLAYER_PVP_TOTAL, PF(pvp_total), 4 },
    { "balloon_font", M_ACTORS, ORIG_ACTOR_BALLOON_FONT, AF(balloon_font), 2 },
    { "mark_listed", M_ACTORS, ORIG_ACTOR_MARK_LISTED, AF(mark_listed), 4 },
    { "is_boss", M_ACTORS, ORIG_ACTOR_IS_BOSS, AF(is_boss), 1 },
    { "object_type", M_ACTORS, ORIG_ACTOR_OBJECT_TYPE, AF(object_type), 1 },
    { "vtable", M(R_NEW), 0, 0, 4, 1 },
    { "maxHp", M(R_SESSION), S(ORIG_MAX_HP), SF(maxHp), 4 },
    { "maxSp", M(R_SESSION), S(ORIG_MAX_SP), SF(maxSp), 4 },
    { "walkSpeed", M(R_SESSION), S(ORIG_WALK_SPEED), SF(walkSpeed), 4 },
    { "karma", M(R_SESSION), S(ORIG_KARMA), SF(karma), 4 },
    { "manner", M(R_SESSION), S(ORIG_MANNER), SF(manner), 4 },
    { "statusPoints", M(R_SESSION), S(ORIG_STATUS_POINTS), SF(statusPoints), 4 },
    { "charBaseLevel", M(R_SESSION), S(ORIG_BASE_LEVEL), SF(charBaseLevel), 4 },
    { "jobLevel", M(R_SESSION), S(ORIG_JOB_LEVEL), SF(jobLevel), 4 },
    { "skillPoints", M(R_SESSION), S(ORIG_SKILL_POINTS), SF(skillPoints), 4 },
    { "weight", M(R_SESSION), S(ORIG_WEIGHT), SF(weight), 4 },
    { "maxWeight", M(R_SESSION), S(ORIG_MAX_WEIGHT), SF(maxWeight), 4 },
    { "stats", M(R_SESSION), S(ORIG_STATS), SF(stats), 24 },
    { "statCosts", M(R_SESSION), S(ORIG_STAT_COSTS), SF(statCosts), 24 },
    { "statBonus", M(R_SESSION), S(ORIG_STAT_BONUS), SF(statBonus), 24 },
    { "traitBonus", M(R_SESSION), S(ORIG_TRAIT_BONUS), SF(traitBonus), 24 },
    { "atk", M(R_SESSION), S(ORIG_ATK), SF(atk), 4 },
    { "refineAtk", M(R_SESSION), S(ORIG_REFINE_ATK), SF(refineAtk), 4 },
    { "matkMax", M(R_SESSION), S(ORIG_MATK_MAX), SF(matkMax), 4 },
    { "matkMin", M(R_SESSION), S(ORIG_MATK_MIN), SF(matkMin), 4 },
    { "def", M(R_SESSION), S(ORIG_DEF), SF(def), 4 },
    { "plusDef", M(R_SESSION), S(ORIG_PLUS_DEF), SF(plusDef), 4 },
    { "mdef", M(R_SESSION), S(ORIG_MDEF), SF(mdef), 4 },
    { "plusMdef", M(R_SESSION), S(ORIG_PLUS_MDEF), SF(plusMdef), 4 },
    { "hit", M(R_SESSION), S(ORIG_HIT), SF(hit), 4 },
    { "flee", M(R_SESSION), S(ORIG_FLEE), SF(flee), 4 },
    { "plusFlee", M(R_SESSION), S(ORIG_PLUS_FLEE), SF(plusFlee), 4 },
    { "critical", M(R_SESSION), S(ORIG_CRITICAL), SF(critical), 4 },
    { "aspd", M(R_SESSION), S(ORIG_ASPD), SF(aspd), 4 },
    { "plusAspd", M(R_SESSION), S(ORIG_PLUS_ASPD), SF(plusAspd), 4 },
    { "baseExp", M(R_SESSION), S(ORIG_BASE_EXP), SF(baseExp), 8 },
    { "nextBaseExp", M(R_SESSION), S(ORIG_NEXT_BASE_EXP), SF(nextBaseExp), 8 },
    { "nextJobExp", M(R_SESSION), S(ORIG_NEXT_JOB_EXP), SF(nextJobExp), 8 },
    { "jobExp", M(R_SESSION), S(ORIG_JOB_EXP), SF(jobExp), 8 },
    { "zeny", M(R_SESSION), S(ORIG_ZENY), SF(zeny), 4 },
    { "exchange_open", M(R_MODE), ORIG_MODE_EXCHANGE_OPEN, GF(exchange_open), 4 },
    { "status_request_pending", M(R_MODE), ORIG_MODE_STATUS_PENDING, GF(status_request_pending), 4 },
    { "hatchedEggIndex", M(R_SESSION), S(ORIG_HATCHED_EGG), SF(hatchedEggIndex), 4 },
};

static const struct { unsigned mask; unsigned orig_off, lift_off; const char *name; } strings[] = {
    { M(R_EFFECT), 0x130, EF(texture), "texture" },
};

static const char *who_packets(const void *p)
{
    if (p == (const char *)oracle_addr(ORIG_INSTANT_VARS) - ORIG_ACTOR_COUNTERS) return "V";
    if (p == orig_new_actor || p == &lift_new_actor) return "N";
    return NULL;
}

static const char *hex_bytes(const unsigned char *p, unsigned size)
{
    static char text[2][64];
    static int turn;
    char *out = text[turn ^= 1];
    unsigned i;
    out[0] = 0;
    for (i = size; i-- > 0;)
        snprintf(out + strlen(out), 64 - strlen(out), "%02x", p[i]);
    return out;
}

static const char *orig_std_string(const unsigned char *s)
{
    unsigned cap = *(const unsigned *)(s + 20);
    return cap >= ORIG_STD_STRING_SSO ? (const char *)*(const char *const *)s : (const char *)s;
}

static void snapshot_regions(int side)
{
    int r;
    for (r = 0; r < R_COUNT; r++) {
        struct Region *g = &regions[r];
        if (side == 0) {
            memcpy(g->orig_snap, g->orig, g->orig_size);
            memset(g->orig_seen, 0, g->orig_size);
        } else {
            memcpy(g->lift_snap, g->lift, g->lift_size);
            memset(g->lift_seen, 0, g->lift_size);
        }
    }
}

static void report_changes(int side)
{
    unsigned i;
    int r;
    for (i = 0; i < sizeof fields / sizeof *fields; i++) {
        const struct Field *f = &fields[i];
        for (r = 0; r < R_COUNT; r++) {
            struct Region *g = &regions[r];
            unsigned off = side == 0 ? f->orig_off : f->lift_off;
            unsigned char *now = (side == 0 ? g->orig : g->lift) + off;
            unsigned char *was = (side == 0 ? g->orig_snap : g->lift_snap) + off;
            unsigned char *seen = (side == 0 ? g->orig_seen : g->lift_seen) + off;
            if (!(f->mask & M(r)))
                continue;
            memset(seen, 1, f->size);
            if (f->quiet)
                continue;
            if (memcmp(now, was, f->size) != 0)
                event("set %s.%s %s->%s", g->name, f->name, hex_bytes(was, f->size), hex_bytes(now, f->size));
        }
    }
    for (i = 0; i < sizeof strings / sizeof *strings; i++) {
        for (r = 0; r < R_COUNT; r++) {
            struct Region *g = &regions[r];
            unsigned off = side == 0 ? strings[i].orig_off : strings[i].lift_off;
            unsigned size = side == 0 ? ORIG_STD_STRING : sizeof(struct String);
            unsigned char *now = (side == 0 ? g->orig : g->lift) + off;
            unsigned char *was = (side == 0 ? g->orig_snap : g->lift_snap) + off;
            if (!(strings[i].mask & M(r)))
                continue;
            memset((side == 0 ? g->orig_seen : g->lift_seen) + off, 1, size);
            if (memcmp(now, was, size) != 0)
                event("set %s.%s '%s'", g->name, strings[i].name,
                      side == 0 ? orig_std_string(now) : string_cstr((const struct String *)now));
        }
    }
    for (r = 0; r < R_COUNT; r++) {
        struct Region *g = &regions[r];
        unsigned size = side == 0 ? g->orig_size : g->lift_size;
        unsigned char *now = side == 0 ? g->orig : g->lift, *was = side == 0 ? g->orig_snap : g->lift_snap;
        unsigned char *seen = side == 0 ? g->orig_seen : g->lift_seen;
        unsigned off;
        for (off = 0; off + 4 <= size; off += 4) {
            unsigned k, unseen_change = 0;
            for (k = 0; k < 4; k++)
                unseen_change |= !seen[off + k] && now[off + k] != was[off + k];
            if (unseen_change)
                event("set %s+0x%x ? %s->%s", g->name, off, hex_bytes(was + off, 4), hex_bytes(now + off, 4));
        }
    }
}

void name_card_holder_erase(struct ActorNameCardHolder *self, unsigned aid) { (void)self; event("names erase %u", aid); }
void name_card_holder_set_all(struct ActorNameCardHolder *self, unsigned aid, struct String name, struct String party,
                              struct String guild, struct String position, struct String title, struct String npc_title, int flag)
{
    (void)self;
    event("names set %u '%s' '%s' '%s' '%s' '%s' '%s' %d", aid, string_cstr(&name), string_cstr(&party), string_cstr(&guild),
          string_cstr(&position), string_cstr(&title), string_cstr(&npc_title), flag);
}
void session_erase_abr(struct Session *self, long long key) { (void)self; event("abr erase %lld", key); }
char world_remove_actor_by_aid(struct World *self, unsigned aid)
{ (void)self; event("remove %u", aid); return aid == AID_CASTER || aid == AID_TARGET; }
unsigned char job_is_monster(int job) { return job >= 1001 && job <= 3999; }
int session_is_homun_or_merc_job(struct Session *self, int id) { (void)self; return id >= 6001 && id <= 6052; }
int session_is_homunculus_job(struct Session *self, int id) { (void)self; return id >= 6001 && id <= 6016; }
unsigned long long game_mode_launch_dummy_effect(struct GameMode *mode, const Vec3 *at, int effect_id, int frames, int value)
{ (void)mode; event("dummy %x %d %d at %.0f %.0f %.0f", effect_id, frames, value, at->x, at->y, at->z); return 0; }
int session_pet_hunger_level(int fullness) { return fullness / 20; }
int session_pet_intimacy_level(int intimacy) { return intimacy / 250; }
int session_get_pet_emotion(struct Session *self, int hungerLevel, int intimacyLevel, int action)
{ (void)self; return hungerLevel * 100 + intimacyLevel * 10 + action; }
int session_make_pet_talk_id(int job, int action, int hunger) { return job * 1000 + action * 10 + hunger; }
void session_notify_messenger_group_and_plates(struct Session *self) { (void)self; event("notify plates"); }
int ui_window_mgr_message_box(struct UIWindowMgr *self, const char *text, int type, int arg3, int arg4, int arg5, const char *title,
                              int width, int height, int arg9)
{ (void)self; (void)title; event("msgbox '%s' %d %d %d %d %d %d %d", text, type, arg3, arg4, arg5, width, height, arg9); return 0; }
struct UIWindow *ui_window_open(int id) { event("open %x", id); return id == 0x21 ? &lift_storage_wnd : lift_window; }
unsigned ui_merchant_shop_seller(const struct UIWindow *wnd) { (void)wnd; return 0; }
void session_clear_purchase_lists(struct Session *self) { (void)self; event("clear purchase"); }
void session_set_job(struct Session *self, int job) { (void)self; event("session job %d", job); }
int session_get_weapon_view_id(int id) { return id % 3 == 0 ? -1 : id + 1000; }
void game_actor_set_job(struct GameActor *self, int job) { event("setjob %s %d", who(self), job); }
void *session_get_actor_by_aid(struct Session *self, DWORD aid) { (void)self; return world_get_game_actor_by_aid(NULL, aid); }
void render_object_set_argb(struct RenderObject *self, int a, int r, int g, int b) { event("argb %s %d %d %d %d", who(self), a, r, g, b); }
void *tree_get(struct Tree *tree, const void *key, size_t key_size, size_t size, TreeLessFn less)
{
    int slot = abr_slot(tree);
    (void)key_size; (void)size; (void)less;
    if (slot >= 0) {
        event("abr get %d %lld", slot, *(const long long *)key);
        fake_abr_used[slot] = 1;
        lift_abr_pair[slot].value = 0;
        return &lift_abr_pair[slot];
    }
    if (tree == &g_session1.statusValues) {
        adv_key = *(const int *)key;
        adv_pending = 1;
        lift_adv_pair.value = 0;
        event("advstat get %d", adv_key);
        return &lift_adv_pair;
    }
    event("instvar get %d", *(const int *)key);
    lift_instant_var_got = 1;
    return &lift_instant_var;
}
int tree_less_int(const void *a, const void *b) { return *(const int *)a < *(const int *)b; }
struct RagConnection *rag_connection_instance(void) { return (struct RagConnection *)lift_connection; }
struct Connection *rag_connection_transport(struct RagConnection *connection) { return (struct Connection *)connection; }
void connection_clear_held(struct Connection *connection) { (void)connection; event("clear held"); }
void session_sync_server_time(struct Session *self, DWORD serverTime) { (void)self; event("sync %u", serverTime); }
struct String *session_get_title_string(struct Session *self, struct String *out, int id)
{ (void)self; string_init(out); event("title %d", id); return out; }
void session_set_combo_delay(struct Session *self, int key, int value) { (void)self; event("combo %d %d", key, value); }
void render_object_set_force_anim_end_tick(struct RenderObject *self, int ms) { event("animend %s %d", who(self), ms); }
void session_set_arrow(struct Session *self, int index) { (void)self; event("arrow %d", index); }
void emitter_effect_set_count(struct EmitterEffect *self, int count) { event("emitter count %s %d", who(self), count); }
void emitter_effect_set_particle_speed(struct EmitterEffect *self, float speed) { event("emitter speed %s %.2f", who(self), speed); }
void game_actor_stop_aura_effects(struct GameActor *self) { event("stop aura %s", who(self)); }
void game_actor_show_level_aura(struct GameActor *self) { event("level aura %s", who(self)); }
void session_reroll_values(struct Session *self) { (void)self; event("reroll"); }
void session_refresh_basic_info(struct Session *self) { (void)self; event("refresh basic"); }
void effect_mgr_remove_type(struct EffectMgr *self, int aid, int type) { (void)self; event("remove type %x aid %d", type, aid); }
int client_lua_call(struct ClientLua *lua, const char *function, const char *format, ...) { (void)lua; (void)format; event("lua %s", function); return 0; }
void warning_plane_effect_add_plane(struct Effect *effect, Vec3 pos) { event("plane %s %.0f %.0f %.0f", who(effect), pos.x, pos.y, pos.z); }
void warning_plane_effect_set_end_time(struct Effect *effect, DWORD end_time) { event("plane end %s %u", who(effect), end_time); }
int game_actor_is_pc_job(int job) { return job < 1000; }
void string_assign(struct String *s, const char *text, size_t len)
{
    char *copy = malloc(len + 1);
    memcpy(copy, text, len);
    copy[len] = 0;
    s->data = copy;
    s->size = len;
    s->capacity = len;
}
void string_assign_cstr(struct String *s, const char *text) { string_assign(s, text, strlen(text)); }
void string_copy(struct String *dst, const struct String *src) { string_init(dst); string_assign(dst, string_cstr(src), src->size); }
int ui_window_send(struct UIWindow *window, struct UIWindow *sender, int message, int a, int b, int c, int d)
{ (void)sender; event("wndsend %s %x %x %x %x %x", window_name(window), message, a, b, c, d); return 0; }
BYTE effect_state_bit6(DWORD state) { return (BYTE)((state >> 6) & 1); }
BYTE is_effect_state_hiding(DWORD state) { return (BYTE)((state >> 1) & 1); }
BYTE is_effect_state_cloaking(DWORD state) { return (BYTE)((state >> 2) & 1); }

int g_replayPlaying;
struct ChampionMonsterMgr *g_championMonster;

static int job_of(const void *actor)
{
    if (actor == orig_caster || actor == &lift_caster) return 46;
    if (actor == orig_target || actor == &lift_target) return 1002;
    if (actor == orig_new_actor || actor == &lift_new_actor) return sweep_job;
    return 0;
}
static int lift_job(const struct GameActor *actor) { return job_of(actor); }
static void *lift_alloc(size_t bytes) { return bytes == sizeof(struct Pc) ? (void *)&lift_new_actor : NULL; }
static const char *tree_name(const void *tree)
{
    if (tree == &lift_mode.square_marks || tree == orig_mode + ORIG_MODE_SQUARE_MARKS) return "sq";
    if (tree == &lift_mode.triangle_marks || tree == orig_mode + ORIG_MODE_TRIANGLE_MARKS) return "tri";
    return "?";
}
static const char *entry_text(long long msg, long long a)
{
    static char text[2][256];
    static int turn;
    char *out = text[turn ^= 1];
    int n;
    if (msg == ACTOR_MSG_ENTRY_OLD || msg == ACTOR_MSG_NEW_ENTRY_OLD || msg == ACTOR_MSG_NEW_ENTRY || msg == ACTOR_MSG_ENTRY) {
        const struct ActorEntryInfo *e = (const struct ActorEntryInfo *)(int)a;
        snprintf(out, 256, "%llx entry a%u j%d s%d at%d,%d,%d sp%d sz%d,%d h%d w%d,%d ac%d st%d bs%d hs%d es%x pk%d sh%d a2%d a3%d hp%d bp%d hd%d g%u ev%d ho%d v%d r%d c%d b%d",
                 msg, e->aid, e->job, e->sex, e->x, e->y, e->dir, e->speed, e->x_size, e->y_size, e->head, e->weapon[0], e->weapon[1],
                 e->accessory, e->state, e->body_state, e->health_state, e->effect_state, e->is_pk_mode, e->shield, e->accessory2,
                 e->accessory3, e->head_palette, e->body_palette, e->head_dir, e->guild_id, e->emblem_version, e->honor, e->virtue,
                 e->robe, e->char_id, e->body);
        return out;
    }
    if (msg == ACTOR_MSG_MOVE_ENTRY_OLD || msg == ACTOR_MSG_NEW_MOVE_ENTRY_OLD || msg == ACTOR_MSG_NEW_MOVE_ENTRY || msg == ACTOR_MSG_MOVE_ENTRY) {
        const struct ActorMoveEntryInfo *e = (const struct ActorMoveEntryInfo *)(int)a;
        n = snprintf(out, 256, "%llx move a%u j%d s%d t%u from%d,%d sub%d,%d to%d,%d sp%d h%d w%d,%d ac%d bs%d hs%d es%x pk%d a2%d a3%d hp%d bp%d hd%d g%u ev%d ho%d v%d",
                     msg, e->aid, e->job, e->sex, e->move_start_time, e->src_x, e->src_y, e->sub_x, e->sub_y, e->dst_x, e->dst_y, e->speed,
                     e->head, e->weapon[0], e->weapon[1], e->accessory, e->body_state, e->health_state, e->effect_state, e->is_pk_mode,
                     e->accessory2, e->accessory3, e->head_palette, e->body_palette, e->head_dir, e->guild_id,
                     e->emblem_version, e->honor, e->virtue);
        if (move_mask & 8) n += snprintf(out + n, 256 - n, " sh%d", e->shield);
        if (move_mask & 1) n += snprintf(out + n, 256 - n, " r%d", e->robe);
        if (move_mask & 2) n += snprintf(out + n, 256 - n, " c%u", e->char_id);
        if (move_mask & 4) n += snprintf(out + n, 256 - n, " b%d", e->body);
        return out;
    }
    return NULL;
}
static const char *msg_text_packets(long long msg, long long a, long long b, long long c, long long d)
{ (void)b; (void)c; (void)d; return readable(a) ? entry_text(msg, a) : NULL; }

int game_actor_get_guild_id(struct GameActor *actor) { return actor == &lift_target ? 77 : 0; }
void game_actor_set_guild(struct GameActor *actor, int value0, int value1) { event("guild %s %d %d", who(actor), value0, value1); }
void game_actor_set_mark_listed(struct GameActor *self, int value) { self->mark_listed = value; }
BYTE clientinfo_is_admin_aid(int aid) { return aid == AID_OWN; }
int session_is_siege_mode(struct Session *self) { return self->map_rule_flags[0]; }
int session_is_player_job(struct Session *self, int id) { (void)self; return id < 1000; }
int session_is_guardian(struct Session *self, int job) { (void)self; return job >= 1285 && job <= 1287; }
int session_is_own_aid(struct Session *self, int aid) { return aid != -1 && aid == self->aid; }
int session_is_homunculus_aid(struct Session *self, int aid) { (void)self; return aid == AID_UNKNOWN; }
int session_is_mercenary_aid(struct Session *self, int aid) { (void)self; return aid == AID_UNKNOWN; }
int session_is_admin_option(struct Session *self, int what) { return (what == 7 || what == 2) && self->map_rule_flags[0]; }
const char *session_character_name(const struct Session *session) { (void)session; return "Hero"; }
const char *session_get_pet_accessory_act(struct Session *self, int accessory)
{ static char name[32]; (void)self; snprintf(name, sizeof name, "pet%d.act", accessory); return name; }
void render_object_load_act_res(struct RenderObject *self, const char *name) { event("loadact %s %s", who(self), name); }
void game_mode_make_emblem_wnd(struct GameMode *self, struct GameActor *actor, int guild_id) { (void)self; event("emblem %s %d", who(actor), guild_id); }
struct Pc *pc_construct(struct Pc *self)
{ ((struct GameActor *)self)->ops = (const struct GameObjectOps *)&lift_actor_ops; event("pc construct %s", who(self)); return self; }
struct GameActor *game_mode_new_actor_for_job(struct GameMode *self, int job)
{
    (void)self;
    event("new actor %d", job);
    lift_new_actor.pc.ops = (const struct GameObjectOps *)&lift_actor_ops;
    return (struct GameActor *)&lift_new_actor;
}
void actor_list_push_back(struct List *list, void *actor) { list_push_back(list, &actor, sizeof actor); }
void *tree_find(const struct Tree *tree, const void *key, TreeLessFn less)
{ unsigned aid = *(const unsigned *)key; (void)less; event("marks %s %u", tree_name(tree), aid); return aid == AID_OWN ? &fake_mark : NULL; }
int tree_less_uint(const void *a, const void *b) { return *(const unsigned *)a < *(const unsigned *)b; }
void name_card_holder_set_name(struct ActorNameCardHolder *self, unsigned aid, struct String name)
{ (void)self; event("name %u '%s'", aid, string_cstr(&name)); }
int player_names_set(unsigned char_id, struct TextView name) { event("pname %u '%.*s'", char_id, (int)name.size, name.data); return 0; }
int champion_monster_apply_effects(struct ChampionMonsterMgr *manager, int job, struct GameActor *actor, int actor_id)
{ (void)manager; event("champion %d %s %d", job, who(actor), actor_id); return 0; }
void world_place_id_nowhere(struct World *self, int id, const float *pos) { (void)self; event("place nowhere %d %.0f %.0f", id, pos[0], pos[1]); }
void world_place_id_at_client_cell(struct World *self, int id, const int *cell) { (void)self; event("place cell %d %d %d", id, cell[0], cell[1]); }
void session_invalidate_windows(struct Session *self) { (void)self; event("invalidate"); }
BYTE session_is_any_super_novice_job(struct Session *self, int id) { (void)self; return (BYTE)is_super_novice(id); }
int session_get_drawn_job(struct Session *self) { (void)self; return drawn_job; }
void ui_basic_info_wnd_set_height(struct UIBasicInfoWnd *self, int height) { event("bi height %s %d", window_name(self), height); }
void ui_basic_info_wnd_set_base_exp(struct UIBasicInfoWnd *self, __int64 current, __int64 maximum) { (void)self; event("bar 1 %lld %lld", current, maximum); }
void ui_basic_info_wnd_set_job_exp(struct UIBasicInfoWnd *self, __int64 current, __int64 maximum) { (void)self; event("bar 2 %lld %lld", current, maximum); }
void game_mode_refresh_actor_effects(struct GameMode *self, char player_only) { (void)self; event("refresh effects %d", player_only); }
struct UIWindow *ui_window_find(int id) { event("find %x", id); return windows_present ? &lift_found_wnd : NULL; }
void session_refresh_skill_wnds(struct Session *self) { (void)self; event("refresh skills"); }
void ui_status_wnd_restore_frame(struct UIStatusWnd *self) { event("status restore %s", window_name(self)); }
void session_end_exchange2(struct Session *self) { (void)self; event("end exchange"); }
long long session_find_abr(struct Session *self, long long key) { (void)self; event("find abr %lld", key); return key == AID_UNKNOWN ? -1 : 0; }
BYTE session_pet_talk_due(struct Session *self, __int64 interval) { (void)self; event("talk due %lld", interval); return 1; }
int tree_less_int64(const void *a, const void *b) { return *(const long long *)a < *(const long long *)b; }
void string_append_cstr(struct String *s, const char *text)
{
    size_t len = strlen(text);
    char *copy = malloc(s->size + len + 1);
    if (s->size)
        memcpy(copy, s->data, s->size);
    memcpy(copy + s->size, text, len + 1);
    s->data = copy;
    s->size += len;
    s->capacity = s->size;
}
void string_format(struct String *s, const char *format, ...)
{
    char text[512];
    va_list ap;
    va_start(ap, format);
    vsnprintf(text, sizeof text, format, ap);
    va_end(ap);
    string_assign_cstr(s, text);
}
BYTE session_add_cart_item(struct Session *self, const struct ItemInfo *item) { (void)self; event("cart add %s", item_text(item)); return 1; }
BYTE session_add_storage_item(struct Session *self, const struct ItemInfo *item, BYTE notify) { (void)self; event("storage add %s %d", item_text(item), notify); return 1; }
void session_inventory_update(struct Session *self, const struct ItemInfo *item, BYTE notify) { (void)self; event("inv update %s %d", item_text(item), notify); }
BYTE session_add_inventory_item(struct Session *self, const struct ItemInfo *item, BYTE refresh) { (void)self; event("inv add %s %d", item_text(item), refresh); return 1; }
BYTE session_set_worn_item(struct Session *self, const struct ItemInfo *item) { (void)self; event("worn %s", item_text(item)); return 1; }
BYTE session_set_worn_costume(struct Session *self, const struct ItemInfo *item) { (void)self; event("costume %s", item_text(item)); return 1; }
void session_remove_inventory_filter_types(struct Session *self) { (void)self; event("remove filter types"); }
void session_remove_inventory_other_types(struct Session *self) { (void)self; event("remove other types"); }
void session_clear_worn_items(struct Session *self) { (void)self; event("clear worn"); }
void session_clear_worn_costumes(struct Session *self) { (void)self; event("clear costumes"); }
void session_clear_inventory(struct Session *self) { (void)self; event("clear inventory"); }
void session_clear_cart(struct Session *self) { (void)self; event("clear cart"); }
void session_refresh_party_ui(struct Session *self) { (void)self; event("refresh party"); }
void session_refresh_friend_ui(struct Session *self) { (void)self; event("refresh friend"); }
void ui_window_mgr_refresh_item_windows(struct UIWindowMgr *self) { (void)self; event("refresh item wnds"); }
int session_is_pet_egg_id(struct Session *self, int id) { (void)self; return is_pet_egg(id); }
BYTE session_is_costume_or_shadow_location(struct Session *self, int flag) { (void)self; return (BYTE)is_costume_location(flag); }
struct ItemInfo *session_get_inventory_item(struct Session *self, struct ItemInfo *out, int index)
{
    (void)self;
    event("inv get %d", index);
    item_info_construct(out);
    out->index = index & 1 ? index : 0;
    item_info_set_item_id(out, 9000 + index);
    return out;
}
char pet_evolution_is_auto_feeding(struct PetEvolutionMgr *self, int egg_id) { (void)self; return (char)(egg_id & 1); }
void session_send_pet_auto_feed(struct Session *self, BYTE on) { (void)self; event("autofeed %d", on); }
void ui_storage_set_title(struct UIWindow *wnd, const char *name) { event("storage title %s '%s'", window_name(wnd), name); }
void ui_storage_set_list_received(struct UIWindow *wnd) { event("storage received %s", window_name(wnd)); }

static int lift_get_effect_state(struct RenderObject *self) { (void)self; return 0; }
static void lift_set_head(struct GameActor *a, int v) { event("head %s %d", who(a), v); }
static void lift_set_weapon(struct GameActor *a, int64_t v) { event("weapon %s %lld", who(a), v); }
static void lift_set_weapon_look(struct GameActor *a, int64_t v) { event("weaponlook %s %lld", who(a), v); }
static void lift_set_accessory(struct GameActor *a, int v) { event("acc1 %s %d", who(a), v); }
static void lift_set_accessory2(struct GameActor *a, int v) { event("acc2 %s %d", who(a), v); }
static void lift_set_accessory3(struct GameActor *a, int v) { event("acc3 %s %d", who(a), v); }
static void lift_set_shield(struct GameActor *a, int v) { event("shield %s %d", who(a), v); }
static void lift_set_appearance(struct GameActor *a, int v) { event("appearance %s %d", who(a), v); }
static void lift_set_imf_name(struct GameActor *a) { event("imf %s", who(a)); }
static void lift_set_head_pal_name(struct GameActor *a) { event("headpalname %s", who(a)); }
static void lift_set_body_pal_name(struct GameActor *a) { event("bodypalname %s", who(a)); }
static void lift_set_body_palette(struct GameActor *a, int v) { event("bodypal %s %d", who(a), v); }
static void lift_set_head_palette(struct GameActor *a, int v) { event("headpal %s %d", who(a), v); }
static void lift_set_honor(struct GameActor *a, int v) { event("honor %s %d", who(a), v); }
static void lift_set_robe(struct GameActor *a, int v, unsigned char f) { event("robe %s %d %d", who(a), v, f); }
static void lift_set_body_style(struct GameActor *a, int v, unsigned char f) { event("bodystyle %s %d %d", who(a), v, f); }
static void lift_refresh_accessory3(struct GameActor *a) { event("refreshacc3 %s", who(a)); }

static void __fastcall o_names_erase(void *self, void *edx, unsigned aid) { (void)self; (void)edx; event("names erase %u", aid); }
struct OrigString { unsigned char raw[ORIG_STD_STRING]; };
static void __fastcall o_names_set_all(void *self, void *edx, unsigned aid, struct OrigString name, struct OrigString party,
                                       struct OrigString guild, struct OrigString position, struct OrigString title,
                                       struct OrigString npc_title, int flag)
{
    (void)self; (void)edx;
    event("names set %u '%s' '%s' '%s' '%s' '%s' '%s' %d", aid, orig_std_string(name.raw), orig_std_string(party.raw),
          orig_std_string(guild.raw), orig_std_string(position.raw), orig_std_string(title.raw), orig_std_string(npc_title.raw), flag);
}
static void __fastcall o_erase_abr(void *self, void *edx, unsigned lo, int hi) { (void)self; (void)edx; event("abr erase %lld", ((long long)hi << 32) | lo); }
static int __fastcall o_world_remove(void *self, void *edx, unsigned aid) { (void)self; (void)edx; return world_remove_actor_by_aid(NULL, aid); }
static int __fastcall o_job_is_monster(void *self, void *edx, int job) { (void)self; (void)edx; return job_is_monster(job); }
static int __fastcall o_homun_or_merc(void *self, void *edx, int job) { (void)self; (void)edx; return session_is_homun_or_merc_job(NULL, job); }
static int __fastcall o_is_homunculus(void *self, void *edx, int job) { (void)self; (void)edx; return session_is_homunculus_job(NULL, job); }
static void __fastcall o_launch_dummy(void *self, void *edx, const float *at, int effect, int frames, int value)
{ (void)self; (void)edx; event("dummy %x %d %d at %.0f %.0f %.0f", effect, frames, value, at[0], at[1], at[2]); }
static int __fastcall o_pet_hunger(void *self, void *edx, int fullness) { (void)self; (void)edx; return session_pet_hunger_level(fullness); }
static int __fastcall o_pet_intimacy(void *self, void *edx, int intimacy) { (void)self; (void)edx; return session_pet_intimacy_level(intimacy); }
static int __fastcall o_pet_emotion(void *self, void *edx, int hunger, int intimacy, int action) { (void)self; (void)edx; return session_get_pet_emotion(NULL, hunger, intimacy, action); }
static int __fastcall o_pet_talk(void *self, void *edx, int job, int action, int hunger) { (void)self; (void)edx; return session_make_pet_talk_id(job, action, hunger); }
static void __fastcall o_notify_plates(void *self, void *edx) { (void)self; (void)edx; event("notify plates"); }
static int __fastcall o_message_box(void *self, void *edx, const char *text, int type, int arg3, int arg4, int arg5, const char *title,
                                    int width, int height, int arg9)
{ (void)edx; return ui_window_mgr_message_box((struct UIWindowMgr *)self, text, type, arg3, arg4, arg5, title, width, height, arg9); }
static int __fastcall o_post_quit(void *self, void *edx, int id) { (void)self; (void)edx; event("quit1 %x", id); return 1; }
static void *__fastcall o_make_window(void *self, void *edx, int id) { (void)self; (void)edx; event("open %x", id); return id == 0x21 ? orig_storage_wnd : NULL; }
static void __fastcall o_clear_purchase(void *self, void *edx) { (void)self; (void)edx; event("clear purchase"); }
static void __fastcall o_actor_set_job(void *self, void *edx, int job) { (void)edx; event("setjob %s %d", who(self), job); }
static void __fastcall o_session_set_job(void *self, void *edx, int job) { (void)self; (void)edx; event("session job %d", job); }
static int __fastcall o_weapon_view(void *self, void *edx, int id) { (void)self; (void)edx; return session_get_weapon_view_id(id); }
static void __fastcall o_set_combo_delay(void *self, void *edx, int key, int value) { (void)self; (void)edx; event("combo %d %d", key, value); }
static void __fastcall o_force_anim_end(void *self, void *edx, int ms) { (void)edx; event("animend %s %d", who(self), ms); }
static void *__fastcall o_session_actor(void *self, void *edx, unsigned aid) { (void)self; (void)edx; return o_actor_by_aid(NULL, NULL, aid); }
static void __fastcall o_set_argb(void *self, void *edx, int a, int r, int g, int b) { (void)edx; event("argb %s %d %d %d %d", who(self), a, r, g, b); }
static void *__cdecl o_conn_instance(void) { return (void *)FAKE_CONNECTION; }
static void __fastcall o_clear_held(void *self, void *edx) { (void)self; (void)edx; event("clear held"); }
static unsigned *__fastcall o_title_string(void *self, void *edx, unsigned *out, int id)
{ (void)self; (void)edx; memset(out, 0, ORIG_STD_STRING); out[5] = ORIG_STD_STRING_SSO - 1; event("title %d", id); return out; }
static void __fastcall o_emitter_speed(void *self, void *edx, float speed) { (void)edx; event("emitter speed %s %.2f", who(self), speed); }
static void __fastcall o_emitter_count(void *self, void *edx, int count) { (void)edx; event("emitter count %s %d", who(self), count); }
static void __fastcall o_stop_aura(void *self, void *edx) { (void)edx; event("stop aura %s", who(self)); }
static void __fastcall o_level_aura(void *self, void *edx) { (void)edx; event("level aura %s", who(self)); }
static void __fastcall o_remove_type(void *self, void *edx, int aid, int type) { (void)self; (void)edx; event("remove type %x aid %d", type, aid); }
static void __fastcall o_reroll(void *self, void *edx) { (void)self; (void)edx; event("reroll"); }
static void __fastcall o_refresh_basic(void *self, void *edx) { (void)self; (void)edx; event("refresh basic"); }
static int __stdcall o_is_pc_job_actor(int job) { return game_actor_is_pc_job(job); }
static int __cdecl o_sprintf(char *out, const char *fmt, ...)
{
    va_list ap;
    int n;
    va_start(ap, fmt);
    n = vsprintf(out, fmt, ap);
    va_end(ap);
    return n;
}

static void __fastcall o_set_head(void *a, void *edx, int v) { (void)edx; event("head %s %d", who(a), v); }
static void __fastcall o_set_weapon(void *a, void *edx, unsigned lo, int hi) { (void)edx; event("weapon %s %lld", who(a), ((long long)hi << 32) | lo); }
static void __fastcall o_set_weapon_look(void *a, void *edx, unsigned lo, int hi) { (void)edx; event("weaponlook %s %lld", who(a), ((long long)hi << 32) | lo); }
static void __fastcall o_set_accessory(void *a, void *edx, int v) { (void)edx; event("acc1 %s %d", who(a), v); }
static void __fastcall o_set_accessory2(void *a, void *edx, int v) { (void)edx; event("acc2 %s %d", who(a), v); }
static void __fastcall o_set_accessory3(void *a, void *edx, int v) { (void)edx; event("acc3 %s %d", who(a), v); }
static void __fastcall o_set_shield(void *a, void *edx, int v) { (void)edx; event("shield %s %d", who(a), v); }
static void __fastcall o_set_appearance(void *a, void *edx, int v) { (void)edx; event("appearance %s %d", who(a), v); }
static void __fastcall o_set_imf_name(void *a, void *edx) { (void)edx; event("imf %s", who(a)); }
static void __fastcall o_set_head_pal_name(void *a, void *edx) { (void)edx; event("headpalname %s", who(a)); }
static void __fastcall o_set_body_pal_name(void *a, void *edx) { (void)edx; event("bodypalname %s", who(a)); }
static void __fastcall o_set_body_palette(void *a, void *edx, int v) { (void)edx; event("bodypal %s %d", who(a), v); }
static void __fastcall o_set_head_palette(void *a, void *edx, int v) { (void)edx; event("headpal %s %d", who(a), v); }
static void __fastcall o_set_honor(void *a, void *edx, int v) { (void)edx; event("honor %s %d", who(a), v); }
static void __fastcall o_set_robe(void *a, void *edx, int v, int f) { (void)edx; event("robe %s %d %d", who(a), v, f & 0xff); }
static void __fastcall o_set_body_style(void *a, void *edx, int v, int f) { (void)edx; event("bodystyle %s %d %d", who(a), v, f & 0xff); }
static void __fastcall o_refresh_accessory3(void *a, void *edx) { (void)edx; event("refreshacc3 %s", who(a)); }

static int __fastcall o_job_packets(void *self, void *edx) { (void)edx; return job_of(self); }
static void *__cdecl o_operator_new(unsigned size) { return size == ORIG_PC_SIZE ? (void *)orig_new_actor : malloc(size); }
static void *__fastcall o_pc_construct(void *self, void *edx) { (void)edx; *(void **)self = orig_actor_vt; event("pc construct %s", who(self)); return self; }
static void *__fastcall o_new_actor_for_job(void *self, void *edx, int job)
{ (void)self; (void)edx; event("new actor %d", job); *(void **)orig_new_actor = orig_actor_vt; return orig_new_actor; }
static void *__stdcall o_buy_node(void *next, void *prev, void *value)
{
    unsigned *node = fake_nodes[fake_node_count++ & 3];
    node[0] = (unsigned)(UINT_PTR)next;
    node[1] = (unsigned)(UINT_PTR)prev;
    node[2] = *(unsigned *)value;
    return node;
}
static void __fastcall o_list_push(void *list, void *edx, void *value) { (void)edx; (void)value; ((unsigned *)list)[1]++; }
static unsigned **__fastcall o_marks_find(void *tree, void *edx, unsigned **out, const unsigned *key)
{ (void)edx; event("marks %s %u", tree_name(tree), *key); *out = *key == AID_OWN ? fake_tree_node : *(unsigned **)tree; return out; }
static void __fastcall o_make_emblem_wnd(void *self, void *edx, void *actor, int guild_id, int emblem_version)
{ (void)self; (void)edx; (void)emblem_version; event("emblem %s %d", who(actor), guild_id); }
static int __fastcall o_is_siege_mode(void *self, void *edx) { (void)self; (void)edx; return *(int *)oracle_addr(ORIG_EFFECT_OPTION); }
static int __fastcall o_is_player_job(void *self, void *edx, int id) { (void)self; (void)edx; return id < 1000; }
static int __fastcall o_is_homunculus_aid(void *self, void *edx, int aid) { (void)self; (void)edx; return aid == AID_UNKNOWN; }
static int __fastcall o_is_mercenary_aid(void *self, void *edx, int aid) { (void)self; (void)edx; return aid == AID_UNKNOWN; }
static const char *__fastcall o_pet_accessory_act(void *self, void *edx, int accessory) { (void)self; (void)edx; return session_get_pet_accessory_act(NULL, accessory); }
static int __fastcall o_load_act_res(void *self, void *edx, const char *name) { (void)edx; event("loadact %s %s", who(self), name); return 0; }
static int __fastcall o_champion_effects(void *self, void *edx, int job, void *actor, int aid) { (void)self; (void)edx; event("champion %d %s %d", job, who(actor), aid); return 0; }
static void __fastcall o_player_names_set(void *self, void *edx, unsigned char_id, struct OrigString name)
{ (void)self; (void)edx; event("pname %u '%s'", char_id, orig_std_string(name.raw)); }
static int __fastcall o_place_nowhere(void *self, void *edx, int id, const float *pos) { (void)self; (void)edx; event("place nowhere %d %.0f %.0f", id, pos[0], pos[1]); return 0; }
static int __fastcall o_place_at_cell(void *self, void *edx, int id, const int *cell) { (void)self; (void)edx; event("place cell %d %d %d", id, cell[0], cell[1]); return 0; }
static void __fastcall o_names_set_name(void *self, void *edx, unsigned aid, struct OrigString name)
{ (void)self; (void)edx; event("name %u '%s'", aid, orig_std_string(name.raw)); }
static void __fastcall o_sync_server_time(void *self, void *edx, unsigned t) { (void)self; (void)edx; event("sync %u", t); }
static const char *__fastcall o_character_name(void *self, void *edx) { (void)self; (void)edx; return "Hero"; }
static int __fastcall o_is_admin_option(void *self, void *edx, int what) { (void)self; (void)edx; return (what == 7 || what == 2) && *(int *)oracle_addr(ORIG_EFFECT_OPTION); }
static int __cdecl o_is_admin_aid(int aid) { return aid == AID_OWN; }
static int __fastcall o_get_guild_id(void *self, void *edx) { (void)edx; return self == orig_target ? 77 : 0; }
static void __fastcall o_set_guild(void *self, void *edx, int value0, int value1) { (void)edx; event("guild %s %d %d", who(self), value0, value1); }
static void __cdecl o_invalidate_windows(void) { event("invalidate"); }
static int __fastcall o_is_super_novice(void *self, void *edx, int job) { (void)self; (void)edx; return is_super_novice(job); }
static int __fastcall o_drawn_job(void *self, void *edx) { (void)self; (void)edx; return drawn_job; }
static void __fastcall o_basic_info_height(void *self, void *edx, int height) { (void)edx; event("bi height %s %d", window_name(self), height); }
static void __fastcall o_bar_set_values(void *bar, void *edx, unsigned cur_lo, int cur_hi, unsigned max_lo, int max_hi)
{ (void)edx; event("bar %d %lld %lld", (int)(UINT_PTR)bar, ((long long)cur_hi << 32) | cur_lo, ((long long)max_hi << 32) | max_lo); }
static void __fastcall o_refresh_actor_effects(void *self, void *edx, int player_only) { (void)self; (void)edx; event("refresh effects %d", player_only & 0xff); }
static void *__fastcall o_get_window(void *self, void *edx, int id) { (void)self; (void)edx; event("find %x", id); return windows_present ? orig_found_wnd : NULL; }
static void __cdecl o_refresh_skill_wnds(void) { event("refresh skills"); }
static void __fastcall o_status_restore(void *self, void *edx) { (void)edx; event("status restore %s", window_name(self)); }
static void __fastcall o_end_exchange(void *self, void *edx) { (void)self; (void)edx; event("end exchange"); }
static long long __fastcall o_find_abr(void *self, void *edx, unsigned lo, int hi) { (void)self; (void)edx; return session_find_abr(NULL, ((long long)hi << 32) | lo); }
static long long *__fastcall o_abr_get(void *tree, void *edx, const long long *key)
{
    int slot = abr_slot(tree);
    (void)edx;
    event("abr get %d %lld", slot, *key);
    fake_abr_used[slot] = 1;
    fake_abr_value[slot] = 0;
    return &fake_abr_value[slot];
}
static void __fastcall o_set_advanced_status(void *self, void *edx, int status, int value)
{ (void)self; (void)edx; adv_key = status; adv_value = value; adv_pending = 1; event("advstat get %d", status); }
static int __fastcall o_pet_talk_due(void *self, void *edx, unsigned lo, int hi) { (void)self; (void)edx; return session_pet_talk_due(NULL, ((long long)hi << 32) | lo); }
static int __cdecl o_rand(void) { return rand(); }
static int __fastcall o_window_send(void *self, void *edx, void *sender, int msg, int a, int b, int c, int d)
{ (void)edx; return ui_window_send((struct UIWindow *)self, (struct UIWindow *)sender, msg, a, b, c, d); }
static int __fastcall o_add_cart_item(void *self, void *edx, const unsigned char *item) { (void)self; (void)edx; event("cart add %s", orig_item_text(item)); return 1; }
static int __fastcall o_add_storage_item(void *self, void *edx, const unsigned char *item, int notify) { (void)self; (void)edx; event("storage add %s %d", orig_item_text(item), notify & 0xff); return 1; }
static void __fastcall o_inventory_update(void *self, void *edx, const unsigned char *item, int notify) { (void)self; (void)edx; event("inv update %s %d", orig_item_text(item), notify & 0xff); }
static int __fastcall o_add_inventory_item(void *self, void *edx, const unsigned char *item, int refresh) { (void)self; (void)edx; event("inv add %s %d", orig_item_text(item), refresh & 0xff); return 1; }
static int __fastcall o_set_worn_item(void *self, void *edx, const unsigned char *item) { (void)self; (void)edx; event("worn %s", orig_item_text(item)); return 1; }
static int __fastcall o_set_worn_costume(void *self, void *edx, const unsigned char *item) { (void)self; (void)edx; event("costume %s", orig_item_text(item)); return 1; }
static void __fastcall o_remove_filter_types(void *self, void *edx) { (void)self; (void)edx; event("remove filter types"); }
static void __fastcall o_remove_other_types(void *self, void *edx) { (void)self; (void)edx; event("remove other types"); }
static void __fastcall o_clear_worn_items(void *self, void *edx) { (void)self; (void)edx; event("clear worn"); }
static void __fastcall o_clear_worn_costumes(void *self, void *edx) { (void)self; (void)edx; event("clear costumes"); }
static void __fastcall o_clear_inventory(void *self, void *edx) { (void)self; (void)edx; event("clear inventory"); }
static void __fastcall o_clear_cart(void *self, void *edx) { (void)self; (void)edx; event("clear cart"); }
static void __cdecl o_refresh_party_ui(void) { event("refresh party"); }
static void __cdecl o_refresh_friend_ui(void) { event("refresh friend"); }
static void __fastcall o_refresh_item_wnds(void *self, void *edx) { (void)self; (void)edx; event("refresh item wnds"); }
static int __fastcall o_is_pet_egg_id(void *self, void *edx, int id) { (void)self; (void)edx; return is_pet_egg(id); }
static int __fastcall o_is_costume_location(void *self, void *edx, int flag) { (void)self; (void)edx; return is_costume_location(flag); }
static unsigned char *__fastcall o_get_inventory_item(void *self, void *edx, unsigned char *out, int index)
{
    (void)self; (void)edx;
    event("inv get %d", index);
    ((void *(__fastcall *)(void *, void *))oracle_addr(ORIG_ITEM_CONSTRUCT))(out, NULL);
    *(int *)(out + OI_INDEX) = index & 1 ? index : 0;
    ((void (__fastcall *)(void *, void *, int))oracle_addr(ORIG_ITEM_SET_ID))(out, NULL, 9000 + index);
    return out;
}
static int __fastcall o_pet_auto_feeding(void *self, void *edx, int egg_id) { (void)self; (void)edx; return egg_id & 1; }
static void __fastcall o_send_pet_auto_feed(void *self, void *edx, int on) { (void)self; (void)edx; event("autofeed %d", on & 0xff); }
static void __fastcall o_set_text_ellipsis(void *ctrl, void *edx, const char *text) { (void)ctrl; (void)edx; event("storage title storage '%s'", text); }
static void __fastcall o_storage_set_name(void *wnd, void *edx, struct OrigString name) { (void)edx; event("storage title %s '%s'", window_name(wnd), orig_std_string(name.raw)); }

typedef void (*mode_pkt_fn)(struct GameMode *, const void *);
#define LIFT(fn) static void w_##fn(const unsigned char *pkt) { ((mode_pkt_fn)fn)(&lift_mode, pkt); }
LIFT(game_mode_zc_notify_vanish)
LIFT(game_mode_zc_sprite_change)
LIFT(game_mode_zc_sprite_change2)
LIFT(game_mode_zc_fast_move)
LIFT(game_mode_zc_blade_stop)
LIFT(game_mode_zc_notify_actor_init)
LIFT(game_mode_zc_par_change_user)
LIFT(game_mode_on_ack_req_name_all2)
LIFT(game_mode_zc_combo_delay)
LIFT(game_mode_on_notify_effect)
LIFT(game_mode_on_notify_effect2)
LIFT(game_mode_on_notify_effect3)
LIFT(game_mode_on_notify_effect4)
LIFT(game_mode_on_clear_effect)
LIFT(game_mode_on_congratulation)
LIFT(game_mode_on_npc_show_efst_update)
LIFT(game_mode_zc_devotion_list)
LIFT(game_mode_zc_dispel)
LIFT(game_mode_zc_notify_newentry)
LIFT(game_mode_zc_notify_newentry2)
LIFT(game_mode_zc_notify_newentry3)
LIFT(game_mode_zc_notify_newentry4)
LIFT(game_mode_zc_notify_standentry)
LIFT(game_mode_zc_notify_standentry_npc)
LIFT(game_mode_zc_notify_standentry2)
LIFT(game_mode_zc_notify_standentry3)
LIFT(game_mode_zc_notify_standentry4)
LIFT(game_mode_zc_notify_standentry5)
LIFT(game_mode_zc_notify_newentry5)
LIFT(game_mode_zc_notify_moveentry)
LIFT(game_mode_zc_notify_moveentry2)
LIFT(game_mode_zc_notify_moveentry3)
LIFT(game_mode_zc_notify_moveentry6)
LIFT(game_mode_zc_notify_ranking)
LIFT(game_mode_zc_notify_newentry7)
LIFT(game_mode_zc_notify_standentry8)
LIFT(game_mode_zc_notify_moveentry9)
LIFT(game_mode_zc_notify_moveentry10)
LIFT(game_mode_zc_notify_moveentry11)
LIFT(game_mode_zc_notify_newentry10)
LIFT(game_mode_zc_notify_newentry11)
LIFT(game_mode_zc_notify_standentry10)
LIFT(game_mode_zc_notify_standentry11)
LIFT(game_mode_zc_par_change)
LIFT(game_mode_on_long_par_change)
LIFT(game_mode_on_long_long_par_change)
LIFT(game_mode_zc_status)
LIFT(game_mode_zc_status_change_ack)
LIFT(game_mode_zc_couple_status)
LIFT(game_mode_zc_hp_info)
LIFT(game_mode_zc_hp_info_tiny)
LIFT(game_mode_zc_abr_init)
LIFT(game_mode_zc_abr_par_change)
LIFT(game_mode_on_notify_exp)
LIFT(game_mode_on_notify_exp2)
#define LIFT1(fn) static void w_##fn(const unsigned char *pkt) { ((void (*)(const void *))fn)(pkt); }
LIFT1(game_mode_ack_status_gm)
LIFT1(game_mode_cart_equipment_item_list2)
LIFT1(game_mode_cart_equipment_item_list3)
LIFT1(game_mode_cart_item_list_equip_v5)
LIFT1(game_mode_cart_item_list_equip_v6)
LIFT1(game_mode_cart_item_list_normal_v5)
LIFT1(game_mode_equipment_item_list2)
LIFT1(game_mode_equipment_item_list3)
LIFT1(game_mode_equipment_item_list4)
LIFT1(game_mode_equipment_item_list_v5)
LIFT1(game_mode_equipment_item_list_v6)
LIFT1(game_mode_equipment_item_list_v7)
LIFT1(game_mode_inventory_item_list_normal_v5)
LIFT1(game_mode_inventory_tab)
LIFT1(game_mode_store_item_list_equip_v5)
LIFT1(game_mode_store_item_list_equip_v6)
LIFT1(game_mode_store_item_list_normal_v5)
LIFT(game_mode_zc_store_equipment_item_list2)
LIFT(game_mode_zc_store_equipment_item_list3)
LIFT(game_mode_zc_store_item_list_equip_v2)
LIFT(game_mode_zc_store_item_list_equip_v5)
LIFT(game_mode_zc_store_item_list_equip_v6)
LIFT(game_mode_zc_store_item_list_normal_v5)
LIFT(game_mode_zc_split_send_item_list_equip_v8)
LIFT(game_mode_zc_split_send_item_list_set)

static const unsigned effect_ids[] = { 0x3fd, 0x5a, 0x5c, 0x5f, 0x238, 0x14b, 0x14c, 0x291, 0x294, 0x298, 0x299 };
static const unsigned actor_aids[] = { AID_CASTER, AID_TARGET, AID_OWN };
static const unsigned state_bits[] = { 0x40, 0x42, 0x44, 0x46 };
static const unsigned ranks[] = { 1, 10, 11, 0xfffffff0u };
static const unsigned entry_types[] = { 0, 1, 5, 6, 7, 8, 9, 10, 12, 13, 14 };
static const int entry_jobs[] = { 0, 19, 20, 45, 46, 999, 1000, 1001, 1285, 2000, 3999, 4000, 6001, 6017, 6053, 20021, 32000, 0x2d2 };
#define N_TYPES ((unsigned)(sizeof entry_types / sizeof *entry_types))
#define N_JOBS ((unsigned)(sizeof entry_jobs / sizeof *entry_jobs))
static const unsigned par_types[] = { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 0xb, 0xc, 13, 14, 15, 16, 17, 18, 0x18, 0x19, 0x1c, 41, 42, 43, 44, 45, 46, 47, 48,
                                      49, 50, 51, 52, 53, 54, 55, 0x3c, 0xdb, 0xe0, 0xe9, 0xea, 0xffff };
static const unsigned exp_types[] = { 0, 1, 2, 3, 20, 21, 22, 23, 0xffff };
static const unsigned ack_types[] = { 0, 13, 14, 15, 16, 17, 18, 19, 41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 51, 52, 53, 54, 55, 56, 100, 0xffff };
static const unsigned couple_types[] = { 0, 12, 13, 14, 15, 16, 17, 18, 19, 0xda, 0xdb, 0xdc, 0xdd, 0xde, 0xdf, 0xe0, 0xe1, 0xffff };
static const unsigned exp_amounts[] = { 0xfffffffbu, 100000, 0xffffffffu };
static const int par_values[12] = { 0, 1, -1, 50, 51, 100, 5000, 9000, 10000, 20000, -61, 123456 };
static const int drawn_jobs[5] = { 0x17, 0x108e, 0xfce, 0xfd2, 1 };
static const char *const list_names[4] = { "", "Kafra", "xxxxxxxxxxxxxxxxxxxxxxx", "Store" };
static const char long_list_name[] = "yyyyyyyyyyyyyyyyyyyyyyyyy";

struct ListLayout { unsigned char rec_size, flags_off, damaged_off, header; };
static const struct ListLayout lay_old34 = { 34, 0, 0xc, 4 };
static const struct ListLayout lay_old38 = { 38, 0, 0xc, 4 };
static const struct ListLayout lay_eq4 = { 37, 0x24, 0, 4 };
static const struct ListLayout lay_v5 = { 41, 0x28, 0, 4 };
static const struct ListLayout lay_v6 = { 67, 0x42, 0, 4 };
static const struct ListLayout lay_v7 = { 68, 0x43, 0, 4 };
static const struct ListLayout lay_n34 = { 34, 0, 0, 4 };
static const struct ListLayout lay_neq4 = { 37, 0x24, 0, 0x1c };
static const struct ListLayout lay_nv5 = { 41, 0x28, 0, 0x1c };
static const struct ListLayout lay_nv6 = { 67, 0x42, 0, 0x1c };
static const struct ListLayout lay_nn34 = { 34, 0, 0, 0x1c };
static const struct ListLayout lay_split8 = { 0x44, 0x43, 0, 5 };

struct EntryLayout { unsigned char type_off, aid_off, job_off, head_off, varlen; };
static const struct EntryLayout lay_new1 = { 0, 2, 0xe, 0x10, 0 };
static const struct EntryLayout lay_new4 = { 0, 2, 0x10, 0x12, 0 };
static const struct EntryLayout lay_stand = { 2, 3, 0xf, 0x11, 0 };
static const struct EntryLayout lay_npc = { 2, 3, 0x15, 0xf, 0 };
static const struct EntryLayout lay_move3 = { 2, 3, 0x11, 0x13, 0 };
static const struct EntryLayout lay_var = { 4, 5, 0x13, 0x15, 1 };
static const struct EntryLayout lay_var10 = { 4, 5, 0x17, 0x19, 1 };

struct Handler {
    const char *name;
    unsigned orig;
    unsigned short id;
    int length;
    void (*lift)(const unsigned char *pkt);
    int kind;
    const unsigned *extra;
    int extra_count;
    const struct EntryLayout *layout;
    int move_mask;
    int no_unknown;
    const struct ListLayout *list;
};
#define EXTRA(a) a, (int)(sizeof a / sizeof *a)

static const struct Handler handlers[] = {
    { "notify_vanish", 0x7cc580u, 0x80, 7, w_game_mode_zc_notify_vanish, PK_DEFAULT, NULL, 0 },
    { "sprite_change", 0x7d6650u, 0xc3, 8, w_game_mode_zc_sprite_change, PK_DEFAULT, NULL, 0 },
    { "sprite_change2", 0x7f5320u, 0x1d7, 15, w_game_mode_zc_sprite_change2, PK_DEFAULT, NULL, 0 },
    { "fast_move", 0x7fae70u, 0x8d2, 10, w_game_mode_zc_fast_move, PK_DEFAULT, NULL, 0 },
    { "blade_stop", 0x7f9bb0u, 0x1d1, 14, w_game_mode_zc_blade_stop, PK_DEFAULT, EXTRA(actor_aids) },
    { "notify_actor_init", 0x7f4d40u, 0xb1b, 2, w_game_mode_zc_notify_actor_init, PK_DEFAULT, NULL, 0 },
    { "par_change_user", 0x7ce200u, 0x1ab, 12, w_game_mode_zc_par_change_user, PK_DEFAULT, NULL, 0 },
    { "ack_req_name_all2", 0x7f4470u, 0xa30, 106, w_game_mode_on_ack_req_name_all2, PK_NAMES, NULL, 0 },
    { "combo_delay", 0x7ba170u, 0x1d2, 10, w_game_mode_zc_combo_delay, PK_DEFAULT, NULL, 0 },
    { "notify_effect", 0x7ff750u, 0x19b, 10, w_game_mode_on_notify_effect, PK_DEFAULT, NULL, 0 },
    { "notify_effect2", 0x7ff460u, 0x1f3, 10, w_game_mode_on_notify_effect2, PK_DEFAULT, EXTRA(effect_ids) },
    { "notify_effect3", 0x7ff730u, 0x284, 14, w_game_mode_on_notify_effect3, PK_DEFAULT, EXTRA(effect_ids) },
    { "notify_effect4", 0x7f4ed0u, 0xb69, 18, w_game_mode_on_notify_effect4, PK_DEFAULT, EXTRA(effect_ids) },
    { "clear_effect", 0x7fa8b0u, 0xb0d, 10, w_game_mode_on_clear_effect, PK_DEFAULT, NULL, 0 },
    { "congratulation", 0x7fac60u, 0x1ea, 6, w_game_mode_on_congratulation, PK_DEFAULT, NULL, 0 },
    { "npc_show_efst_update", 0x803b30u, 0x28a, 18, w_game_mode_on_npc_show_efst_update, PK_DEFAULT, EXTRA(state_bits) },
    { "devotion_list", 0x7ba9d0u, 0x1cf, 28, w_game_mode_zc_devotion_list, PK_DEFAULT, EXTRA(actor_aids) },
    { "dispel", 0x7bac20u, 0x1b9, 6, w_game_mode_zc_dispel, PK_DEFAULT, NULL, 0 },
    { "newentry", 0x7c9ce0u, 0x79, 0x35, w_game_mode_zc_notify_newentry, PK_ENTRY, NULL, 0, &lay_new1, 0 },
    { "newentry2", 0x7c8c50u, 0x1d9, 0x39, w_game_mode_zc_notify_newentry2, PK_ENTRY, NULL, 0, &lay_new1, 0 },
    { "newentry3", 0x7c91d0u, 0x22b, 0x3f, w_game_mode_zc_notify_newentry3, PK_ENTRY, NULL, 0, &lay_new4, 0 },
    { "newentry4", 0x7c9750u, 0x2ed, 0x3f, w_game_mode_zc_notify_newentry4, PK_ENTRY, NULL, 0, &lay_new4, 0 },
    { "standentry", 0x7cbbf0u, 0x78, 0x37, w_game_mode_zc_notify_standentry, PK_ENTRY, NULL, 0, &lay_stand, 0 },
    { "standentry_npc", 0x7cc180u, 0x7c, 0x2c, w_game_mode_zc_notify_standentry_npc, PK_ENTRY, NULL, 0, &lay_npc, 0 },
    { "standentry2", 0x7cacc0u, 0x1d8, 0x3a, w_game_mode_zc_notify_standentry2, PK_ENTRY, NULL, 0, &lay_new1, 0 },
    { "standentry3", 0x7cb1d0u, 0x22a, 0x40, w_game_mode_zc_notify_standentry3, PK_ENTRY, NULL, 0, &lay_new4, 0 },
    { "standentry4", 0x7cb6e0u, 0x2ee, 0x40, w_game_mode_zc_notify_standentry4, PK_ENTRY, NULL, 0, &lay_new4, 0 },
    { "standentry5", 0x7ca7b0u, 0x7f9, 0x45, w_game_mode_zc_notify_standentry5, PK_ENTRY, NULL, 0, &lay_var, 0 },
    { "newentry5", 0x7c87b0u, 0x7f8, 0x44, w_game_mode_zc_notify_newentry5, PK_ENTRY, NULL, 0, &lay_var, 0 },
    { "moveentry", 0x7c8280u, 0x7b, 0x3c, w_game_mode_zc_notify_moveentry, PK_ENTRY, NULL, 0, &lay_new1, 12 },
    { "moveentry2", 0x7c7150u, 0x1da, 0x40, w_game_mode_zc_notify_moveentry2, PK_ENTRY, NULL, 0, &lay_new1, 4 },
    { "moveentry3", 0x7c76d0u, 0x22c, 0x45, w_game_mode_zc_notify_moveentry3, PK_ENTRY, NULL, 0, &lay_move3, 4 },
    { "moveentry6", 0x7c7c70u, 0x2ec, 0x4b, w_game_mode_zc_notify_moveentry6, PK_ENTRY, NULL, 0, &lay_var, 5 },
    { "ranking", 0x7ca510u, 0x19a, 14, w_game_mode_zc_notify_ranking, PK_DEFAULT, EXTRA(ranks), NULL, 0, 1 },
    { "newentry7", 0x7f1ba0u, 0x90f, 0x4d, w_game_mode_zc_notify_newentry7, PK_ENTRY, NULL, 0, &lay_var, 0 },
    { "standentry8", 0x7f2a90u, 0x915, 0x4e, w_game_mode_zc_notify_standentry8, PK_ENTRY, NULL, 0, &lay_var, 0 },
    { "moveentry9", 0x7f0d40u, 0x914, 0x54, w_game_mode_zc_notify_moveentry9, PK_ENTRY, NULL, 0, &lay_var, 5 },
    { "moveentry10", 0x7f0a90u, 0x9db, 0x58, w_game_mode_zc_notify_moveentry10, PK_ENTRY, NULL, 0, &lay_var10, 7 },
    { "moveentry11", 0x7f0be0u, 0x9fd, 0x5a, w_game_mode_zc_notify_moveentry11, PK_ENTRY, NULL, 0, &lay_var10, 7 },
    { "newentry10", 0x7f1900u, 0x9dc, 0x51, w_game_mode_zc_notify_newentry10, PK_ENTRY, NULL, 0, &lay_var10, 0 },
    { "newentry11", 0x7f1a50u, 0x9fe, 0x53, w_game_mode_zc_notify_newentry11, PK_ENTRY, NULL, 0, &lay_var10, 0 },
    { "standentry10", 0x7f27e0u, 0x9dd, 0x52, w_game_mode_zc_notify_standentry10, PK_ENTRY, NULL, 0, &lay_var10, 0 },
    { "standentry11", 0x7f2930u, 0x9ff, 0x54, w_game_mode_zc_notify_standentry11, PK_ENTRY, NULL, 0, &lay_var10, 0 },
    { "par_change", 0x7cd5c0u, 0xb0, 8, w_game_mode_zc_par_change, PK_PAR, EXTRA(par_types) },
    { "long_par_change", 0x7c4530u, 0xb1, 8, w_game_mode_on_long_par_change, PK_PAR, EXTRA(exp_types) },
    { "long_long_par_change", 0x7fce30u, 0xacb, 12, w_game_mode_on_long_long_par_change, PK_PAR, EXTRA(exp_types) },
    { "status", 0x7d7110u, 0xbd, 44, w_game_mode_zc_status, PK_DEFAULT, NULL, 0 },
    { "status_change_ack", 0x7d7560u, 0xbc, 6, w_game_mode_zc_status_change_ack, PK_PAR, EXTRA(ack_types) },
    { "ack_status_gm", 0x7b33a0u, 0x214, 44, w_game_mode_ack_status_gm, PK_DEFAULT, NULL, 0 },
    { "couple_status", 0x7dd890u, 0x141, 14, w_game_mode_zc_couple_status, PK_PAR, EXTRA(couple_types) },
    { "hp_info", 0x7f09f0u, 0x977, 14, w_game_mode_zc_hp_info, PK_DEFAULT, NULL, 0 },
    { "hp_info_tiny", 0x7f0a40u, 0xa36, 7, w_game_mode_zc_hp_info_tiny, PK_DEFAULT, NULL, 0 },
    { "abr_init", 0x7f4290u, 0xb6b, 14, w_game_mode_zc_abr_init, PK_DEFAULT, NULL, 0 },
    { "abr_par_change", 0x7f4330u, 0xb6c, 12, w_game_mode_zc_abr_par_change, PK_DEFAULT, NULL, 0 },
    { "notify_exp", 0x7c6ba0u, 0x7f6, 14, w_game_mode_on_notify_exp, PK_EXP, EXTRA(exp_amounts) },
    { "notify_exp2", 0x7ff9b0u, 0xacc, 18, w_game_mode_on_notify_exp2, PK_EXP, EXTRA(exp_amounts) },
    { "cart_equip2", 0x7b84a0u, 0x297, 4, w_game_mode_cart_equipment_item_list2, PK_LIST, NULL, 0, NULL, 0, 0, &lay_old34 },
    { "cart_equip3", 0x7b8600u, 0x2d2, 4, w_game_mode_cart_equipment_item_list3, PK_LIST, NULL, 0, NULL, 0, 0, &lay_old38 },
    { "cart_equip_v5", 0x7b8a30u, 0x994, 4, w_game_mode_cart_item_list_equip_v5, PK_LIST, NULL, 0, NULL, 0, 0, &lay_v5 },
    { "cart_equip_v6", 0x7b8ce0u, 0xa0f, 4, w_game_mode_cart_item_list_equip_v6, PK_LIST, NULL, 0, NULL, 0, 0, &lay_v6 },
    { "cart_normal_v5", 0x7b9100u, 0x993, 4, w_game_mode_cart_item_list_normal_v5, PK_LIST, NULL, 0, NULL, 0, 0, &lay_n34 },
    { "equip2", 0x7bb600u, 0x295, 4, w_game_mode_equipment_item_list2, PK_LIST, NULL, 0, NULL, 0, 0, &lay_old34 },
    { "equip3", 0x7bb7e0u, 0x2d0, 4, w_game_mode_equipment_item_list3, PK_LIST, NULL, 0, NULL, 0, 0, &lay_old38 },
    { "equip4", 0x7bf850u, 0x901, 4, w_game_mode_equipment_item_list4, PK_LIST, NULL, 0, NULL, 0, 0, &lay_eq4 },
    { "equip_v5", 0x7bfa60u, 0x992, 4, w_game_mode_equipment_item_list_v5, PK_LIST, NULL, 0, NULL, 0, 0, &lay_v5 },
    { "equip_v6", 0x7bfe60u, 0xa0d, 4, w_game_mode_equipment_item_list_v6, PK_LIST, NULL, 0, NULL, 0, 0, &lay_v6 },
    { "equip_v7", 0x7c0290u, 0xb3c, 4, w_game_mode_equipment_item_list_v7, PK_LIST, NULL, 0, NULL, 0, 0, &lay_v7 },
    { "inventory_normal_v5", 0x7c0830u, 0x991, 4, w_game_mode_inventory_item_list_normal_v5, PK_LIST, NULL, 0, NULL, 0, 0, &lay_n34 },
    { "inventory_tab", 0x7c0990u, 0x908, 6, w_game_mode_inventory_tab, PK_DEFAULT, NULL, 0 },
    { "guild_equip_v5", 0x7be970u, 0x9d3, 0x1c, w_game_mode_store_item_list_equip_v5, PK_LIST, NULL, 0, NULL, 0, 0, &lay_nv5 },
    { "guild_equip_v6", 0x7beca0u, 0xa11, 0x1c, w_game_mode_store_item_list_equip_v6, PK_LIST, NULL, 0, NULL, 0, 0, &lay_nv6 },
    { "guild_normal_v5", 0x7bf030u, 0x9d2, 0x1c, w_game_mode_store_item_list_normal_v5, PK_LIST, NULL, 0, NULL, 0, 0, &lay_nn34 },
    { "store_equip2", 0x7d7850u, 0x296, 4, w_game_mode_zc_store_equipment_item_list2, PK_LIST, NULL, 0, NULL, 0, 0, &lay_old34 },
    { "store_equip3", 0x7d79c0u, 0x2d1, 4, w_game_mode_zc_store_equipment_item_list3, PK_LIST, NULL, 0, NULL, 0, 0, &lay_old38 },
    { "store_equip_v2", 0x7d7e20u, 0x976, 0x1c, w_game_mode_zc_store_item_list_equip_v2, PK_LIST, NULL, 0, NULL, 0, 0, &lay_neq4 },
    { "store_equip_v5", 0x7d8040u, 0x996, 0x1c, w_game_mode_zc_store_item_list_equip_v5, PK_LIST, NULL, 0, NULL, 0, 0, &lay_nv5 },
    { "store_equip_v6", 0x7d8370u, 0xa10, 0x1c, w_game_mode_zc_store_item_list_equip_v6, PK_LIST, NULL, 0, NULL, 0, 0, &lay_nv6 },
    { "store_normal_v5", 0x7d8a40u, 0x995, 0x1c, w_game_mode_zc_store_item_list_normal_v5, PK_LIST, NULL, 0, NULL, 0, 0, &lay_nn34 },
    { "split_equip_v8", 0x7d54c0u, 0xb39, 5, w_game_mode_zc_split_send_item_list_equip_v8, PK_LIST, NULL, 0, NULL, 0, 0, &lay_split8 },
    { "split_set", 0x7d6100u, 0xb08, 5, w_game_mode_zc_split_send_item_list_set, PK_SPLIT_SET, NULL, 0 },
};

static unsigned handler_values(const struct Handler *h)
{
    if (h->kind == PK_ENTRY)
        return (h->layout->type_off ? N_TYPES : 1) * N_JOBS * 2;
    if (h->kind == PK_PAR)
        return (unsigned)h->extra_count;
    if (h->kind == PK_LIST || h->kind == PK_SPLIT_SET)
        return 8;
    return SWEEP_VALUES + h->extra_count;
}

static void build_packet(unsigned char *pkt, const struct Handler *h, unsigned aid, unsigned value, int fill)
{
    int i;
    memset(pkt, 0, PACKET_MAX);
    for (i = 2; i < h->length; i++)
        pkt[i] = fill == FILL_ZERO ? 0 : fill == FILL_ONES ? 0xff : (unsigned char)(i * 7 + 3);
    *(unsigned short *)pkt = h->id;
    drawn_job = drawn_jobs[(value + fill) % 5];
    if (h->kind == PK_LIST || h->kind == PK_SPLIT_SET) {
        unsigned ai = aid == AID_CASTER ? 0 : aid == AID_TARGET ? 1 : aid == AID_OWN ? 2 : 3;
        const char *name = h->kind == PK_SPLIT_SET && (value & 4) ? long_list_name : list_names[ai];
        short total;
        memset(pkt + 2, 0, PACKET_MAX - 2);
        if (h->kind == PK_SPLIT_SET) {
            total = (short)(5 + strlen(name));
            pkt[4] = (unsigned char)(value & 3);
            memcpy(pkt + 5, name, strlen(name));
        } else {
            const struct ListLayout *l = h->list;
            unsigned records = value & 3, egg = value >> 2, r;
            total = (short)(l->header + records * l->rec_size);
            if (l->header == 0x1c)
                strncpy((char *)pkt + 4, name, 24);
            if (l->header == 5)
                pkt[4] = (unsigned char)ai;
            for (r = 0; r < records; r++) {
                unsigned char *rec = pkt + l->header + r * l->rec_size;
                short index = (short)(r + 1);
                int item_id = egg ? 9005 + (int)r : 1201 + 100 * fill + (int)r;
                for (i = 0; i < l->rec_size; i++)
                    rec[i] = fill == FILL_ZERO ? 0 : fill == FILL_ONES ? 0xff : (unsigned char)(i * 7 + 3 + r * 13);
                memcpy(rec, &index, 2);
                memcpy(rec + 2, &item_id, 4);
                if (egg && l->damaged_off)
                    rec[l->damaged_off] = 1;
                if (egg && l->flags_off)
                    rec[l->flags_off] |= 2;
            }
        }
        memcpy(pkt + 2, &total, 2);
        memcpy(g_packetBuffer, pkt, PACKET_MAX);
        memcpy(oracle_addr(ORIG_PACKET_BUFFER), pkt, PACKET_MAX);
        return;
    }
    if (h->kind == PK_PAR) {
        unsigned ai = aid == AID_CASTER ? 0 : aid == AID_TARGET ? 1 : aid == AID_OWN ? 2 : 3;
        int pv = par_values[ai * FILL_COUNT + fill];
        unsigned short type = (unsigned short)value;
        memcpy(pkt + 2, &type, 2);
        *(int *)oracle_addr(ORIG_SESSION_AID) = g_session1.aid = (int)aid;
        if (h->id == 0xbc) {
            pkt[4] = (unsigned char)(ai & 1);
            pkt[5] = (unsigned char)pv;
        } else if (h->id == 0x141) {
            int bonus = pv ^ 0x55;
            memcpy(pkt + 2, &value, 4);
            memcpy(pkt + 6, &pv, 4);
            memcpy(pkt + 10, &bonus, 4);
        } else if (h->length == 12) {
            long long wide = pv;
            memcpy(pkt + 4, &wide, 8);
        } else {
            memcpy(pkt + 4, &pv, 4);
        }
        return;
    }
    if (h->kind == PK_EXP) {
        unsigned short var_id = (unsigned short)fill, exp_type = (unsigned short)(value & 1);
        int var_off = h->id == 0xacc ? 14 : 10;
        memcpy(pkt + 2, &aid, 4);
        if (h->id == 0xacc) {
            long long wide = (int)value;
            memcpy(pkt + 6, &wide, 8);
        } else {
            memcpy(pkt + 6, &value, 4);
        }
        memcpy(pkt + var_off, &var_id, 2);
        memcpy(pkt + var_off + 2, &exp_type, 2);
        return;
    }
    if (h->kind == PK_ENTRY) {
        const struct EntryLayout *l = h->layout;
        unsigned types = l->type_off ? N_TYPES : 1;
        unsigned char type = (unsigned char)entry_types[value % types];
        unsigned pet = value / (types * N_JOBS);
        short job = (short)entry_jobs[(value / types) % N_JOBS];
        short head = 0x2b;
        int name_len = fill == FILL_ZERO ? 0 : fill == FILL_PATTERN ? 4 : 15;
        sweep_job = job;
        if (l->type_off)
            pkt[l->type_off] = type;
        memcpy(pkt + l->aid_off, &aid, 4);
        memcpy(pkt + l->job_off, &job, 2);
        if (pet)
            memcpy(pkt + l->head_off, &head, 2);
        if (l->varlen) {
            short total = (short)(h->length + name_len);
            memcpy(pkt + 2, &total, 2);
            memcpy(pkt + h->length, "Entry name here", name_len);
        }
        return;
    }
    if (h->length >= 6)
        memcpy(pkt + 2, &aid, 4);
    if (h->kind == PK_NAMES) {
        const char *names[5] = { "ann", "party1", "guild22", "pos", "x" };
        for (i = 0; i < 4; i++) {
            memset(pkt + 6 + i * 24, 0, 24);
            if (fill != FILL_ZERO)
                strcpy((char *)pkt + 6 + i * 24, names[i]);
        }
        memcpy(pkt + 102, &value, 4);
        return;
    }
    if (h->length >= 10)
        memcpy(pkt + 6, &value, 4);
    else if (h->length >= 8)
        memcpy(pkt + 6, &value, 2);
    else if (h->length >= 7)
        pkt[6] = (unsigned char)value;
}

static void seed_packets(int option)
{
    unsigned i;
    memset(oracle_addr(ORIG_SESSION), 0, ORIG_SESSION_SIZE);
    seed_all(option);
    *(int *)oracle_addr(ORIG_EFFECT_ON) = 1;
    g_session1.effectOn = 1;
    *(int *)oracle_addr(ORIG_SERVICE_TYPE) = g_serviceType;
    *(unsigned *)(orig_player + ORIG_PLAYER_TARGET) = AID_TARGET;
    for (i = 0; i < sizeof strings / sizeof *strings; i++)
        *(unsigned *)(regions[R_EFFECT].orig + strings[i].orig_off + 20) = ORIG_STD_STRING_SSO - 1;
    ((struct Player *)lift_player)->target_aid = AID_TARGET;
    instant_var_set = 0;
    lift_instant_var_got = 0;
    lift_instant_var.value = 0x55;
    *(int *)oracle_addr(ORIG_SESSION_AID) = AID_OWN;
    g_session1.aid = AID_OWN;
    *(int *)oracle_addr(ORIG_PVP_RULE) = option;
    g_session1.map_rule_flags[3] = option;
    *(int *)oracle_addr(ORIG_REPLAY_PLAYING) = 0;
    memset(orig_new_actor, 0, sizeof orig_new_actor);
    memset(&lift_new_actor, 0, sizeof lift_new_actor);
    orig_actor_list_head[0] = orig_actor_list_head[1] = (unsigned)(UINT_PTR)orig_actor_list_head;
    orig_actor_list_head[2] = 0;
    *(void **)(orig_world + ORIG_WORLD_ACTORS) = orig_actor_list_head;
    *(unsigned *)(orig_world + ORIG_WORLD_ACTORS + 4) = 0;
    fake_node_count = 0;
    windows_present = option;
    *(void **)oracle_addr(ORIG_WND_STATUS) = option ? orig_status_wnd : NULL;
    *(void **)oracle_addr(ORIG_WND_GROUP) = option ? orig_group_wnd : NULL;
    *(void **)oracle_addr(ORIG_WND_BASIC_INFO) = orig_basic_info_wnd;
    g_windowMgr.status_wnd = option ? &lift_status_wnd : NULL;
    g_windowMgr.messenger_group_wnd = option ? &lift_group_wnd : NULL;
    g_windowMgr.basic_info_wnd = &lift_basic_info_wnd;
    lift_basic_info_wnd.height = 77;
    *(int *)(orig_basic_info_wnd + ORIG_WND_HEIGHT) = 77;
    *(int *)(orig_basic_info_wnd + ORIG_WND_BASE_BAR) = 1;
    *(int *)(orig_basic_info_wnd + ORIG_WND_JOB_BAR) = 2;
    *(void **)orig_status_wnd = *(void **)orig_basic_info_wnd = *(void **)orig_group_wnd = *(void **)orig_found_wnd = orig_window_vt;
    *(int *)(orig_mode + ORIG_MODE_EXCHANGE_OPEN) = option;
    lift_mode.exchange_open = option;
    *(int *)(orig_mode + ORIG_MODE_STATUS_PENDING) = 1;
    lift_mode.status_request_pending = 1;
    fake_abr_used[0] = fake_abr_used[1] = 0;
    adv_pending = 0;
#define SEED(orig, field, v) (*(int *)oracle_addr(orig) = (v), g_session1.field = (v))
#define SEED64(orig, field, v) (*(long long *)oracle_addr(orig) = (v), g_session1.field = (v))
    SEED(ORIG_HP, hp, 100);
    SEED(ORIG_MAX_HP, maxHp, 1000);
    SEED(ORIG_SP, sp, 50);
    SEED(ORIG_MAX_SP, maxSp, 200);
    SEED(ORIG_WEIGHT, weight, option ? 950 : 600);
    SEED(ORIG_MAX_WEIGHT, maxWeight, 1000);
    SEED(ORIG_WEIGHT_PENALTY, weightPenaltyPercent, 50);
    SEED(ORIG_ZENY, zeny, 7);
    SEED(ORIG_PET_FULLNESS, petFullness, 60);
    SEED(ORIG_PET_INTIMACY_VAL, petIntimacy, 500);
    SEED(ORIG_PET_JOB, petJob, 1002);
    SEED(ORIG_BASE_LEVEL, charBaseLevel, 5);
    SEED(ORIG_JOB_LEVEL, jobLevel, 3);
    SEED64(ORIG_BASE_EXP, baseExp, 1000);
    SEED64(ORIG_NEXT_BASE_EXP, nextBaseExp, 5000);
    SEED64(ORIG_JOB_EXP, jobExp, 10);
    SEED64(ORIG_NEXT_JOB_EXP, nextJobExp, 300);
    for (i = 0; i < 6; i++)
        *(int *)oracle_addr(ORIG_STATS + 4 * i) = g_session1.stats[i] = (int)i + 1;
    SEED(ORIG_HATCHED_EGG, hatchedEggIndex, option ? 3 : 4);
    *(unsigned char *)oracle_addr(ORIG_PET_AUTO_FEED) = 1;
    g_session1.petAutoFeed = 1;
    *(void **)orig_storage_wnd = orig_window_vt;
    *(int *)(orig_storage_wnd + ORIG_WND_TITLE_CTRL) = 1;
#undef SEED
#undef SEED64
}

static void __fastcall o_set_counter_packets(void *counters, void *edx, int key, unsigned lo, int hi)
{
    (void)edx;
    if (counters == oracle_addr(ORIG_INSTANT_VARS)) {
        event("instvar get %d", key);
        orig_instant_var = ((long long)hi << 32) | lo;
        instant_var_set = 1;
        return;
    }
    event("counter %s %d %lld", who((char *)counters - ORIG_ACTOR_COUNTERS), key, ((long long)hi << 32) | lo);
}

static void after_call(int side)
{
    if (side == 0 && instant_var_set)
        event("instvar 5 = %lld", orig_instant_var);
    if (side == 1 && lift_instant_var_got)
        event("instvar 5 = %lld", lift_instant_var.value);
    {
        unsigned objects = side == 0 ? *(unsigned *)(orig_world + ORIG_WORLD_OBJECTS + 4) : lift_world.gameObjects.size;
        unsigned actors = side == 0 ? *(unsigned *)(orig_world + ORIG_WORLD_ACTORS + 4) : lift_world.actors.size;
        if (objects || actors)
            event("lists %u %u", objects, actors);
    }
    {
        int slot;
        for (slot = 0; slot < 2; slot++)
            if (fake_abr_used[slot]) {
                event("abr %d = %lld", slot, side == 0 ? fake_abr_value[slot] : lift_abr_pair[slot].value);
                fake_abr_used[slot] = 0;
            }
        if (adv_pending) {
            event("advstat %d = %d", adv_key, side == 0 ? adv_value : lift_adv_pair.value);
            adv_pending = 0;
        }
    }
    report_changes(side);
}

typedef void (__fastcall *handler_fn)(void *self, void *edx, void *pkt);

static int call_guarded(const struct Handler *h, unsigned char *pkt)
{
    __try {
        ((handler_fn)oracle_addr(h->orig))(orig_mode, NULL, pkt);
        return 1;
    } __except (note_fault(GetExceptionInformation())) {
        return 0;
    }
}

static int call_original(const struct Handler *h, unsigned char *pkt)
{
    jmp_buf env;
    int ok;
    trap_jump = &env;
    if (setjmp(env) != 0) {
        trap_jump = NULL;
        return 1;
    }
    ok = call_guarded(h, pkt);
    trap_jump = NULL;
    return ok;
}

static void setup_regions(void)
{
    static const struct { const char *name; int r; } names[R_COUNT] = {
        { "S", R_SESSION }, { "M", R_MODE }, { "C", R_CASTER }, { "T", R_TARGET }, { "P", R_PLAYER }, { "E", R_EFFECT }, { "N", R_NEW },
    };
    int r;
    for (r = 0; r < R_COUNT; r++)
        regions[names[r].r].name = names[r].name;
    regions[R_SESSION].orig = (unsigned char *)oracle_addr(ORIG_SESSION);
    regions[R_SESSION].orig_size = ORIG_SESSION_SIZE;
    regions[R_SESSION].lift = (unsigned char *)&g_session1;
    regions[R_SESSION].lift_size = sizeof g_session1;
    regions[R_MODE].orig = orig_mode;
    regions[R_MODE].orig_size = sizeof orig_mode;
    regions[R_MODE].lift = (unsigned char *)&lift_mode;
    regions[R_MODE].lift_size = sizeof lift_mode;
    regions[R_CASTER].orig = orig_caster;
    regions[R_CASTER].orig_size = sizeof orig_caster;
    regions[R_CASTER].lift = (unsigned char *)&lift_caster;
    regions[R_CASTER].lift_size = sizeof lift_caster;
    regions[R_TARGET].orig = orig_target;
    regions[R_TARGET].orig_size = sizeof orig_target;
    regions[R_TARGET].lift = (unsigned char *)&lift_target;
    regions[R_TARGET].lift_size = sizeof lift_target;
    regions[R_PLAYER].orig = orig_player;
    regions[R_PLAYER].orig_size = sizeof orig_player;
    regions[R_PLAYER].lift = lift_player;
    regions[R_PLAYER].lift_size = sizeof(struct Player);
    regions[R_EFFECT].orig = orig_effect;
    regions[R_EFFECT].orig_size = sizeof orig_effect;
    regions[R_EFFECT].lift = (unsigned char *)&lift_effect_storage;
    regions[R_EFFECT].lift_size = sizeof lift_effect_storage;
    regions[R_NEW].orig = orig_new_actor;
    regions[R_NEW].orig_size = sizeof orig_new_actor;
    regions[R_NEW].lift = (unsigned char *)&lift_new_actor;
    regions[R_NEW].lift_size = sizeof lift_new_actor;
    for (r = 0; r < R_COUNT; r++) {
        regions[r].orig_snap = calloc(1, regions[r].orig_size);
        regions[r].orig_seen = calloc(1, regions[r].orig_size);
        regions[r].lift_snap = calloc(1, regions[r].lift_size);
        regions[r].lift_seen = calloc(1, regions[r].lift_size);
    }
}

static void packet_oracle_setup(int verbose)
{
    int i;
    skill_oracle_setup(verbose);
    who_extra = who_packets;
    msg_str_named = 1;
    setup_regions();
    oracle_patch_jump(ORIG_NAMES_ERASE, (void *)o_names_erase);
    oracle_patch_jump(ORIG_NAMES_SET_ALL, (void *)o_names_set_all);
    oracle_patch_jump(ORIG_ERASE_ABR, (void *)o_erase_abr);
    oracle_patch_jump(ORIG_WORLD_REMOVE, (void *)o_world_remove);
    oracle_patch_jump(ORIG_JOB_IS_MONSTER, (void *)o_job_is_monster);
    oracle_patch_jump(ORIG_HOMUN_OR_MERC, (void *)o_homun_or_merc);
    oracle_patch_jump(ORIG_IS_HOMUNCULUS, (void *)o_is_homunculus);
    oracle_patch_jump(ORIG_LAUNCH_DUMMY, (void *)o_launch_dummy);
    oracle_patch_jump(ORIG_PET_HUNGER, (void *)o_pet_hunger);
    oracle_patch_jump(ORIG_PET_INTIMACY, (void *)o_pet_intimacy);
    oracle_patch_jump(ORIG_PET_EMOTION, (void *)o_pet_emotion);
    oracle_patch_jump(ORIG_PET_TALK, (void *)o_pet_talk);
    oracle_patch_jump(ORIG_NOTIFY_PLATES, (void *)o_notify_plates);
    oracle_patch_jump(ORIG_MESSAGE_BOX, (void *)o_message_box);
    oracle_patch_jump(ORIG_POST_QUIT, (void *)o_post_quit);
    oracle_patch_jump(ORIG_MAKE_WINDOW, (void *)o_make_window);
    oracle_patch_jump(ORIG_CLEAR_PURCHASE, (void *)o_clear_purchase);
    oracle_patch_jump(ORIG_ACTOR_SET_JOB, (void *)o_actor_set_job);
    oracle_patch_jump(ORIG_SESSION_SET_JOB, (void *)o_session_set_job);
    oracle_patch_jump(ORIG_WEAPON_VIEW, (void *)o_weapon_view);
    oracle_patch_jump(ORIG_SET_COMBO_DELAY, (void *)o_set_combo_delay);
    oracle_patch_jump(ORIG_FORCE_ANIM_END, (void *)o_force_anim_end);
    oracle_patch_jump(ORIG_SESSION_ACTOR, (void *)o_session_actor);
    oracle_patch_jump(ORIG_SET_ARGB, (void *)o_set_argb);
    oracle_patch_jump(ORIG_CONN_INSTANCE, (void *)o_conn_instance);
    oracle_patch_jump(ORIG_CLEAR_HELD, (void *)o_clear_held);
    oracle_patch_jump(ORIG_TITLE_STRING, (void *)o_title_string);
    oracle_patch_jump(ORIG_EMITTER_SPEED, (void *)o_emitter_speed);
    oracle_patch_jump(ORIG_EMITTER_COUNT, (void *)o_emitter_count);
    oracle_patch_jump(ORIG_STOP_AURA, (void *)o_stop_aura);
    oracle_patch_jump(ORIG_LEVEL_AURA, (void *)o_level_aura);
    oracle_patch_jump(ORIG_REMOVE_TYPE, (void *)o_remove_type);
    oracle_patch_jump(ORIG_REROLL, (void *)o_reroll);
    oracle_patch_jump(ORIG_REFRESH_BASIC, (void *)o_refresh_basic);
    oracle_patch_jump(ORIG_IS_PC_JOB_ACTOR, (void *)o_is_pc_job_actor);
    oracle_patch_jump(ORIG_SPRINTF, (void *)o_sprintf);
    oracle_patch_jump(ORIG_SET_COUNTER, (void *)o_set_counter_packets);
    oracle_patch_jump(ORIG_GET_JOB, (void *)o_job_packets);
    oracle_patch_jump(ORIG_OPERATOR_NEW, (void *)o_operator_new);
    oracle_patch_jump(ORIG_PC_CONSTRUCT, (void *)o_pc_construct);
    oracle_patch_jump(ORIG_NEW_ACTOR_FOR_JOB, (void *)o_new_actor_for_job);
    oracle_patch_jump(ORIG_LIST_BUY_NODE, (void *)o_buy_node);
    oracle_patch_jump(ORIG_ACTOR_LIST_BUY_NODE, (void *)o_buy_node);
    oracle_patch_jump(ORIG_OBJECT_LIST_PUSH, (void *)o_list_push);
    oracle_patch_jump(ORIG_ACTOR_LIST_PUSH, (void *)o_list_push);
    oracle_patch_jump(ORIG_MARKS_FIND, (void *)o_marks_find);
    oracle_patch_jump(ORIG_MAKE_EMBLEM_WND, (void *)o_make_emblem_wnd);
    oracle_patch_jump(ORIG_IS_SIEGE_MODE, (void *)o_is_siege_mode);
    oracle_patch_jump(ORIG_IS_PLAYER_JOB, (void *)o_is_player_job);
    oracle_patch_jump(ORIG_IS_HOMUNCULUS_AID, (void *)o_is_homunculus_aid);
    oracle_patch_jump(ORIG_IS_MERCENARY_AID, (void *)o_is_mercenary_aid);
    oracle_patch_jump(ORIG_PET_ACCESSORY_ACT, (void *)o_pet_accessory_act);
    oracle_patch_jump(ORIG_LOAD_ACT_RES, (void *)o_load_act_res);
    oracle_patch_jump(ORIG_CHAMPION_EFFECTS, (void *)o_champion_effects);
    oracle_patch_jump(ORIG_PLAYER_NAMES_SET, (void *)o_player_names_set);
    oracle_patch_jump(ORIG_PLACE_NOWHERE, (void *)o_place_nowhere);
    oracle_patch_jump(ORIG_PLACE_AT_CELL, (void *)o_place_at_cell);
    oracle_patch_jump(ORIG_NAMES_SET_NAME, (void *)o_names_set_name);
    oracle_patch_jump(ORIG_SYNC_SERVER_TIME, (void *)o_sync_server_time);
    oracle_patch_jump(ORIG_CHARACTER_NAME, (void *)o_character_name);
    oracle_patch_jump(ORIG_IS_ADMIN_OPTION, (void *)o_is_admin_option);
    oracle_patch_jump(ORIG_IS_ADMIN_AID, (void *)o_is_admin_aid);
    oracle_patch_jump(ORIG_INVALIDATE_WINDOWS, (void *)o_invalidate_windows);
    oracle_patch_jump(ORIG_IS_SUPER_NOVICE, (void *)o_is_super_novice);
    oracle_patch_jump(ORIG_DRAWN_JOB, (void *)o_drawn_job);
    oracle_patch_jump(ORIG_BASIC_INFO_HEIGHT, (void *)o_basic_info_height);
    oracle_patch_jump(ORIG_BAR_SET_VALUES, (void *)o_bar_set_values);
    oracle_patch_jump(ORIG_REFRESH_ACTOR_EFFECTS, (void *)o_refresh_actor_effects);
    oracle_patch_jump(ORIG_GET_WINDOW, (void *)o_get_window);
    oracle_patch_jump(ORIG_REFRESH_SKILL_WNDS, (void *)o_refresh_skill_wnds);
    oracle_patch_jump(ORIG_STATUS_RESTORE, (void *)o_status_restore);
    oracle_patch_jump(ORIG_END_EXCHANGE, (void *)o_end_exchange);
    oracle_patch_jump(ORIG_FIND_ABR, (void *)o_find_abr);
    oracle_patch_jump(ORIG_ABR_GET, (void *)o_abr_get);
    oracle_patch_jump(ORIG_SET_ADVANCED_STATUS, (void *)o_set_advanced_status);
    oracle_patch_jump(ORIG_PET_TALK_DUE, (void *)o_pet_talk_due);
    oracle_patch_jump(ORIG_ADD_CART_ITEM, (void *)o_add_cart_item);
    oracle_patch_jump(ORIG_ADD_STORAGE_ITEM, (void *)o_add_storage_item);
    oracle_patch_jump(ORIG_INVENTORY_UPDATE, (void *)o_inventory_update);
    oracle_patch_jump(ORIG_ADD_INVENTORY_ITEM, (void *)o_add_inventory_item);
    oracle_patch_jump(ORIG_SET_WORN_ITEM, (void *)o_set_worn_item);
    oracle_patch_jump(ORIG_SET_WORN_COSTUME, (void *)o_set_worn_costume);
    oracle_patch_jump(ORIG_REMOVE_FILTER_TYPES, (void *)o_remove_filter_types);
    oracle_patch_jump(ORIG_REMOVE_OTHER_TYPES, (void *)o_remove_other_types);
    oracle_patch_jump(ORIG_CLEAR_WORN_ITEMS, (void *)o_clear_worn_items);
    oracle_patch_jump(ORIG_CLEAR_WORN_COSTUMES, (void *)o_clear_worn_costumes);
    oracle_patch_jump(ORIG_CLEAR_INVENTORY, (void *)o_clear_inventory);
    oracle_patch_jump(ORIG_CLEAR_CART, (void *)o_clear_cart);
    oracle_patch_jump(ORIG_REFRESH_PARTY_UI, (void *)o_refresh_party_ui);
    oracle_patch_jump(ORIG_REFRESH_FRIEND_UI, (void *)o_refresh_friend_ui);
    oracle_patch_jump(ORIG_REFRESH_ITEM_WNDS, (void *)o_refresh_item_wnds);
    oracle_patch_jump(ORIG_IS_PET_EGG_ID, (void *)o_is_pet_egg_id);
    oracle_patch_jump(ORIG_IS_COSTUME_LOCATION, (void *)o_is_costume_location);
    oracle_patch_jump(ORIG_GET_INVENTORY_ITEM, (void *)o_get_inventory_item);
    oracle_patch_jump(ORIG_PET_AUTO_FEEDING, (void *)o_pet_auto_feeding);
    oracle_patch_jump(ORIG_SEND_PET_AUTO_FEED, (void *)o_send_pet_auto_feed);
    oracle_patch_jump(ORIG_SET_TEXT_ELLIPSIS, (void *)o_set_text_ellipsis);
    oracle_patch_jump(ORIG_STORAGE_SET_NAME, (void *)o_storage_set_name);
    for (i = 0; i < 64; i++)
        orig_window_vt[i] = traps[i];
    orig_window_vt[ORIG_WINDOW_VT_SEND] = (void *)o_window_send;
    if (!oracle_bind_import("api-ms-win-crt-utility-l1-1-0.dll", "rand", (void *)o_rand))
        fprintf(stderr, "the original's rand import was not found\n");

    orig_actor_vt[ORIG_ACTOR_VT_SET_HEAD] = (void *)o_set_head;
    orig_actor_vt[ORIG_ACTOR_VT_SET_WEAPON] = (void *)o_set_weapon;
    orig_actor_vt[ORIG_ACTOR_VT_SET_WEAPON_LOOK] = (void *)o_set_weapon_look;
    orig_actor_vt[ORIG_ACTOR_VT_SET_ACCESSORY] = (void *)o_set_accessory;
    orig_actor_vt[ORIG_ACTOR_VT_SET_ACCESSORY2] = (void *)o_set_accessory2;
    orig_actor_vt[ORIG_ACTOR_VT_SET_ACCESSORY3] = (void *)o_set_accessory3;
    orig_actor_vt[ORIG_ACTOR_VT_SET_SHIELD] = (void *)o_set_shield;
    orig_actor_vt[ORIG_ACTOR_VT_SET_APPEARANCE] = (void *)o_set_appearance;
    orig_actor_vt[ORIG_ACTOR_VT_SET_IMF_NAME] = (void *)o_set_imf_name;
    orig_actor_vt[ORIG_ACTOR_VT_SET_HEAD_PAL] = (void *)o_set_head_pal_name;
    orig_actor_vt[ORIG_ACTOR_VT_SET_BODY_PAL] = (void *)o_set_body_pal_name;
    orig_actor_vt[ORIG_ACTOR_VT_SET_BODY_PALETTE] = (void *)o_set_body_palette;
    orig_actor_vt[ORIG_ACTOR_VT_SET_HEAD_PALETTE] = (void *)o_set_head_palette;
    orig_actor_vt[ORIG_ACTOR_VT_SET_HONOR] = (void *)o_set_honor;
    orig_actor_vt[ORIG_ACTOR_VT_SET_ROBE] = (void *)o_set_robe;
    orig_actor_vt[ORIG_ACTOR_VT_SET_BODY_STYLE] = (void *)o_set_body_style;
    orig_actor_vt[ORIG_ACTOR_VT_REFRESH_ACC3] = (void *)o_refresh_accessory3;
    orig_actor_vt[ORIG_ACTOR_VT_SET_GUILD] = (void *)o_set_guild;
    orig_actor_vt[ORIG_ACTOR_VT_GET_GUILD_ID] = (void *)o_get_guild_id;

    lift_actor_ops.base.get_effect_state = lift_get_effect_state;
    lift_actor_ops.set_head = lift_set_head;
    lift_actor_ops.set_weapon = lift_set_weapon;
    lift_actor_ops.set_weapon_look = lift_set_weapon_look;
    lift_actor_ops.set_accessory = lift_set_accessory;
    lift_actor_ops.set_accessory2 = lift_set_accessory2;
    lift_actor_ops.set_accessory3 = lift_set_accessory3;
    lift_actor_ops.set_shield = lift_set_shield;
    lift_actor_ops.set_appearance_value = lift_set_appearance;
    lift_actor_ops.set_imf_name = lift_set_imf_name;
    lift_actor_ops.set_head_pal_name = lift_set_head_pal_name;
    lift_actor_ops.set_body_pal_name = lift_set_body_pal_name;
    lift_actor_ops.set_body_palette = lift_set_body_palette;
    lift_actor_ops.set_head_palette = lift_set_head_palette;
    lift_actor_ops.set_honor = lift_set_honor;
    lift_actor_ops.set_robe = lift_set_robe;
    lift_actor_ops.set_body_style = lift_set_body_style;
    lift_actor_ops.refresh_accessory3 = lift_refresh_accessory3;
    msg_text_extra = msg_text_packets;
    job_hook = lift_job;
    mem_alloc_hook = lift_alloc;
}

int main(int argc, char **argv)
{
    const char *exe = argc > 1 ? argv[1] : getenv("ORACLE_EXE");
    const char *only = getenv("ORACLE_ONLY");
    const int verbose = getenv("ORACLE_VERBOSE") != NULL;
    const int trace = getenv("ORACLE_TRACE") != NULL;
    const unsigned max_reported = getenv("ORACLE_REPORTS") ? (unsigned)atoi(getenv("ORACLE_REPORTS")) : MAX_REPORTED;
    static const unsigned aids[] = { AID_CASTER, AID_TARGET, AID_OWN, AID_UNKNOWN };
    unsigned char pkt[PACKET_MAX];
    unsigned hi, ai, vi, fill, option;
    unsigned compared = 0, mismatches = 0, faults = 0, reported = 0, recorded = 0;

    if (!exe) {
        fprintf(stderr, "usage: test_packet_handlers <original exe>\n");
        return 2;
    }
    if (!oracle_load(exe))
        return 2;
    packet_oracle_setup(verbose);

    for (hi = 0; hi < sizeof handlers / sizeof *handlers; hi++) {
        const struct Handler *h = &handlers[hi];
        unsigned values = handler_values(h), handler_compared = 0, handler_events = 0, handler_bad = 0;
        if (only && strcmp(only, h->name) != 0)
            continue;
        for (option = 0; option < 2; option++)
        for (ai = 0; ai < sizeof aids / sizeof *aids; ai++)
        for (vi = 0; vi < values; vi++)
        for (fill = 0; fill < FILL_COUNT; fill++) {
            if (h->no_unknown && aids[ai] == AID_UNKNOWN)
                continue;
            unsigned value = h->kind == PK_ENTRY ? vi : h->kind == PK_PAR ? h->extra[vi] : vi < SWEEP_VALUES ? vi : h->extra[vi - SWEEP_VALUES];
            char orig_text[EVENTS * EVENT_LEN], lift_text[EVENTS * EVENT_LEN];

            compared++;
            handler_compared++;
            move_mask = h->kind == PK_ENTRY ? h->move_mask : 7;
            if (verbose) { fprintf(stderr, "%s option %u aid %u value %u fill %u\n", h->name, option, aids[ai], value, fill); fflush(stderr); }
            seed_packets(option);
            build_packet(pkt, h, aids[ai], value, fill);
            event_count = 0;
            overflow = 0;
            srand(1);
            snapshot_regions(0);
            if (!call_original(h, pkt)) {
                faults++;
                handler_bad++;
                if (reported++ < max_reported)
                    printf("  %s option %u aid %u value %u fill %u: the original faulted (code %08x at 0x%x)\n", h->name, option, aids[ai],
                           value, fill, fault_code, fault_va);
                continue;
            }
            after_call(0);
            recorded += event_count;
            handler_events += event_count;
            joined(orig_text, sizeof orig_text);
            if (trace) printf("%s aid %u value %u fill %u original: %s\n", h->name, aids[ai], value, fill, orig_text);

            seed_packets(option);
            build_packet(pkt, h, aids[ai], value, fill);
            event_count = 0;
            overflow = 0;
            srand(1);
            snapshot_regions(1);
            h->lift(pkt);
            after_call(1);
            joined(lift_text, sizeof lift_text);
            if (trace) printf("%s aid %u value %u fill %u lift:     %s\n", h->name, aids[ai], value, fill, lift_text);

            if (strcmp(orig_text, lift_text) != 0) {
                mismatches++;
                handler_bad++;
                if (reported++ < max_reported)
                    printf("  %s option %u aid %u value %u fill %u:\n    original: %s\n    lift:     %s\n", h->name, option, aids[ai], value, fill,
                           orig_text, lift_text);
            }
        }
        printf("  %-22s %5u calls %6u events %s\n", h->name, handler_compared, handler_events, handler_bad ? "MISMATCH" : "ok");
    }
    printf("Packet handlers: %u calls compared against the original (%u events recorded), %u mismatches, %u faults%s\n", compared, recorded,
           mismatches, faults, reported > max_reported ? " (not all shown)" : "");
    return mismatches || faults ? 1 : 0;
}
