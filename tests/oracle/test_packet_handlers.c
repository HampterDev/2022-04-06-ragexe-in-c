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

#define AID_UNKNOWN 4242
#define PACKET_MAX 256
#define SWEEP_VALUES 16
#define MAX_REPORTED 40
#define FAKE_CONNECTION 0x5151

enum { FILL_ZERO, FILL_PATTERN, FILL_ONES, FILL_COUNT };
enum { PK_DEFAULT, PK_NAMES };

enum { R_SESSION, R_MODE, R_CASTER, R_TARGET, R_PLAYER, R_EFFECT, R_COUNT };
#define M(r) (1u << (r))
#define M_ACTORS (M(R_CASTER) | M(R_TARGET) | M(R_PLAYER))

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
};

static struct Region regions[R_COUNT];
static struct IntInt64Pair lift_instant_var;
static long long orig_instant_var;
static int instant_var_set, lift_instant_var_got;
static unsigned char lift_connection[0x100];
static unsigned char lift_names_holder;
static struct UIWindow *lift_window;

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
};

static const struct { unsigned mask; unsigned orig_off, lift_off; const char *name; } strings[] = {
    { M(R_EFFECT), 0x130, EF(texture), "texture" },
};

static const char *who_packets(const void *p)
{
    if (p == (const char *)oracle_addr(ORIG_INSTANT_VARS) - ORIG_ACTOR_COUNTERS) return "V";
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
struct UIWindow *ui_window_open(int id) { event("open %x", id); return lift_window; }
unsigned ui_merchant_shop_seller(const struct UIWindow *wnd) { (void)wnd; return 0; }
void session_clear_purchase_lists(struct Session *self) { (void)self; event("clear purchase"); }
void session_set_job(struct Session *self, int job) { (void)self; event("session job %d", job); }
int session_get_weapon_view_id(int id) { return id % 3 == 0 ? -1 : id + 1000; }
void game_actor_set_job(struct GameActor *self, int job) { event("setjob %s %d", who(self), job); }
void *session_get_actor_by_aid(struct Session *self, DWORD aid) { (void)self; return world_get_game_actor_by_aid(NULL, aid); }
void render_object_set_argb(struct RenderObject *self, int a, int r, int g, int b) { event("argb %s %d %d %d %d", who(self), a, r, g, b); }
void *tree_get(struct Tree *tree, const void *key, size_t key_size, size_t size, TreeLessFn less)
{ (void)tree; (void)key_size; (void)size; (void)less; event("instvar get %d", *(const int *)key); lift_instant_var_got = 1; return &lift_instant_var; }
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
{ (void)window; (void)sender; event("wndsend %x %x %x %x %x", message, a, b, c, d); return 0; }
BYTE effect_state_bit6(DWORD state) { return (BYTE)((state >> 6) & 1); }
BYTE is_effect_state_hiding(DWORD state) { return (BYTE)((state >> 1) & 1); }
BYTE is_effect_state_cloaking(DWORD state) { return (BYTE)((state >> 2) & 1); }

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
static void *__fastcall o_make_window(void *self, void *edx, int id) { (void)self; (void)edx; event("open %x", id); return NULL; }
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
static int __cdecl o_is_pc_job_actor(int job) { return game_actor_is_pc_job(job); }
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

static const unsigned effect_ids[] = { 0x3fd, 0x5a, 0x5c, 0x5f, 0x238, 0x14b, 0x14c, 0x291, 0x294, 0x298, 0x299 };
static const unsigned actor_aids[] = { AID_CASTER, AID_TARGET, AID_OWN };
static const unsigned state_bits[] = { 0x40, 0x42, 0x44, 0x46 };

struct Handler {
    const char *name;
    unsigned orig;
    unsigned short id;
    int length;
    void (*lift)(const unsigned char *pkt);
    int kind;
    const unsigned *extra;
    int extra_count;
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
};

static void build_packet(unsigned char *pkt, const struct Handler *h, unsigned aid, unsigned value, int fill)
{
    int i;
    memset(pkt, 0, PACKET_MAX);
    for (i = 2; i < h->length; i++)
        pkt[i] = fill == FILL_ZERO ? 0 : fill == FILL_ONES ? 0xff : (unsigned char)(i * 7 + 3);
    *(unsigned short *)pkt = h->id;
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
        { "S", R_SESSION }, { "M", R_MODE }, { "C", R_CASTER }, { "T", R_TARGET }, { "P", R_PLAYER }, { "E", R_EFFECT },
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
    for (r = 0; r < R_COUNT; r++) {
        regions[r].orig_snap = calloc(1, regions[r].orig_size);
        regions[r].orig_seen = calloc(1, regions[r].orig_size);
        regions[r].lift_snap = calloc(1, regions[r].lift_size);
        regions[r].lift_seen = calloc(1, regions[r].lift_size);
    }
}

static void packet_oracle_setup(int verbose)
{
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
}

int main(int argc, char **argv)
{
    const char *exe = argc > 1 ? argv[1] : getenv("ORACLE_EXE");
    const char *only = getenv("ORACLE_ONLY");
    const int verbose = getenv("ORACLE_VERBOSE") != NULL;
    const int trace = getenv("ORACLE_TRACE") != NULL;
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
        unsigned values = SWEEP_VALUES + h->extra_count, handler_compared = 0, handler_events = 0, handler_bad = 0;
        if (only && strcmp(only, h->name) != 0)
            continue;
        for (option = 0; option < 2; option++)
        for (ai = 0; ai < sizeof aids / sizeof *aids; ai++)
        for (vi = 0; vi < values; vi++)
        for (fill = 0; fill < FILL_COUNT; fill++) {
            unsigned value = vi < SWEEP_VALUES ? vi : h->extra[vi - SWEEP_VALUES];
            char orig_text[EVENTS * EVENT_LEN], lift_text[EVENTS * EVENT_LEN];

            compared++;
            handler_compared++;
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
                if (reported++ < MAX_REPORTED)
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
                if (reported++ < MAX_REPORTED)
                    printf("  %s option %u aid %u value %u fill %u:\n    original: %s\n    lift:     %s\n", h->name, option, aids[ai], value, fill,
                           orig_text, lift_text);
            }
        }
        printf("  %-22s %5u calls %6u events %s\n", h->name, handler_compared, handler_events, handler_bad ? "MISMATCH" : "ok");
    }
    printf("Packet handlers: %u calls compared against the original (%u events recorded), %u mismatches, %u faults%s\n", compared, recorded,
           mismatches, faults, reported > MAX_REPORTED ? " (first 40 shown)" : "");
    return mismatches || faults ? 1 : 0;
}
