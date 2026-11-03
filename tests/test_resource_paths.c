#include "contents/client_lua_calls.h"
#include "ui/resource/resource_paths.h"
#include "session/session_names.h"
#include "session/session_jobs.h"
#include "session/session_state.h"
#include "session/session_lua.h"
#include "world_state_memory.h"
#include <stdarg.h>
#include <stdio.h>

struct Session g_session1;
static int manager, allowed = 1;
static int probes, available_from = 1, mounted_job;
static const char *robe_text = "cape";
static const char *body_text = "novice";
static char last_probe[512];
static const char *hair_names[43];
static const char *body_names[4096];

static struct String *text(struct String *out, const char *value)
{
    string_init(out);
    string_assign_cstr(out, value); return out;
}

struct ResMgr *g_resMgr(void) { return (struct ResMgr *)&manager; }
unsigned char res_mgr_is_available(struct ResMgr *self, const char *path)
{
    assert((void *)self == &manager);
    assert(strlen(path) < sizeof last_probe);
    strcpy(last_probe, path);
    ++probes;
    return *path && available_from && probes >= available_from;
}
unsigned __int64 *crt_printf_options(void)
{ static unsigned __int64 options; return &options; }
int session_remap_job(struct Session *self, int job)
{ assert(self == &g_session1); return job; }
int session_get_mounted_job(int job)
{ ++mounted_job; return job; }
int session_get_item_view_id(int item)
{ (void)item; return 2; }
BYTE session_job_allows_level(struct Session *self, int job, int level)
{ assert(self == &g_session1); (void)job; (void)level; return (BYTE)allowed; }
void session_class_from_code(struct Session *self, int *hair, int job)
{ assert(self == &g_session1); (void)job; *hair = 2; }
const char *session_get_weapon_name_by_class(int view)
{ assert(view == 20); return "_view"; }
struct String *session_get_sex_sprite_name(struct Session *self, struct String *out, int sex)
{ assert(self == &g_session1); return text(out, sex ? "남" : "여"); }
struct String *session_get_accessory_name(struct Session *self, struct String *out, int id)
{ assert(self == &g_session1); return text(out, id == 1 ? "_hat" : ""); }
struct String *session_get_job_sprite_name(struct Session *self, struct String *out, int job)
{ assert(self == &g_session1); (void)job; return text(out, body_text); }
struct String *session_get_weapon_sprite_name(struct Session *self, struct String *out, int weapon)
{ assert(self == &g_session1 && weapon == 3); return text(out, "_sword"); }
struct String *session_get_job_imf_name(struct Session *self, struct String *out, int job)
{ assert(self == &g_session1 && job == 1); return text(out, "motion"); }
struct String *session_get_shield_sprite_name(struct Session *self, struct String *out, int shield)
{ assert(self == &g_session1 && shield == 2); return text(out, "_guard"); }
int client_lua_call(struct ClientLua *lua, const char *name, const char *format, ...)
{
    va_list args;
    const char **out;
    (void)lua;
    assert(!strcmp(name, "ReqRobSprName"));
    assert(!strcmp(format, "d>s"));
    va_start(args, format);
    assert(va_arg(args, int) == 7);
    out = va_arg(args, const char **);
    *out = robe_text;
    va_end(args);
    return 1;
}

static void check(struct String *out, const char *expected)
{
    if (strcmp(string_cstr(out), expected)) {
        fprintf(stderr, "expected [%s], got [%s]\n", expected, string_cstr(out));
        abort();
    }
    assert(out->size == strlen(expected));
    string_free(out);
    assert(live_blocks == 0);
    probes = 0;
}

int main(void)
{
    struct String out;
    int i, hair;
    for (i = 0; i < 43; ++i) hair_names[i] = "hair";
    hair_names[2] = "two"; hair_names[10] = "ten"; hair_names[13] = "thirteen";
    for (i = 0; i < 4096; ++i) body_names[i] = "novice";
    g_session1.maleHairNames.first = g_session1.femaleHairNames.first = hair_names;
    g_session1.jobBodyNames.first = body_names;
    g_session1.jobSpritePaths.first = body_names;
    g_session1.pcNames.first = body_names;

    resource_path_accessory(&out, 0, 1, 1, 2, 0); check(&out, "");
    resource_path_accessory(&out, 0, 1, 1, 2, 2); check(&out, "");
    resource_path_accessory(&out, 0, 1, 1, 2, 1); check(&out, "악세사리\\남\\남_hat.act");
    resource_path_accessory(&out, 1, 1, 0, 2, 1);
    assert(probes == 0); check(&out, "악세사리\\여\\여_hat.spr");

    resource_path_body_sprite(&out, 0, 1, 1, 1, 0); check(&out, "인간족\\몸통\\남\\novice_남.act");
    resource_path_body_sprite(&out, 1, 1, 1, 0, 1); check(&out, "인간족\\몸통\\여\\costume_1\\novice_여_1.spr");
    resource_path_body_palette(&out, 1, 1, 3, 1, 0); check(&out, "몸\\costume_1\\novice_남_3_1.pal");
    resource_path_body_palette(&out, 1, 1, 3, 0, 0); check(&out, "몸\\novice_남_3.pal");
    resource_path_body_palette(&out, 0xfa1, 1, 0, 0, 0); check(&out, "몸\\novice_남_1.pal");
    resource_path_body_palette(&out, 0xfa8, 1, 1, 0, 0); check(&out, "몸\\novice_남_0.pal");

    hair = 99;
    resource_path_head_sprite(&out, 0, 1, 1, &hair);
    assert(hair == 13); check(&out, "인간족\\머리통\\남\\thirteen_남.act");
    hair = -1;
    resource_path_head_sprite(&out, 1, 1, 0, &hair);
    assert(hair == 13); check(&out, "인간족\\머리통\\여\\thirteen_여.spr");
    resource_path_head_palette(&out, 1, 1, 0, 3); check(&out, "머리\\머리two_여_3.pal");
    resource_path_imf(&out, 0xf6f, 1); check(&out, "motion_남.imf");

    resource_path_robe(&out, 1, 1, 1, 0, 0); check(&out, "");
    resource_path_robe(&out, 1, 1, 1, 7, 1);
    assert(mounted_job == 1); check(&out, "sprite\\로브\\cape\\cape.spr");
    available_from = 0;
    resource_path_robe(&out, 1, 1, 0, 7, 0); check(&out, "sprite\\로브\\cape\\여\\novice_여.spr");
    resource_path_robe(&out, 0, 1, 1, 7, 0); check(&out, "sprite\\로브\\cape\\남\\novice_남.act");
    robe_text = "";
    resource_path_robe(&out, 0, 1, 1, 7, 0); check(&out, "");
    robe_text = "cape"; available_from = 1;

    resource_path_shield(&out, 1, 1, 1, 5); check(&out, "방패\\novice_남_5_방패.spr");
    available_from = 2;
    resource_path_shield(&out, 0, 1, 0, 5); check(&out, "방패\\novice_여_guard.act");
    available_from = 0;
    resource_path_shield(&out, 1, 1, 1, 5); check(&out, "");
    available_from = 1; allowed = 0;
    resource_path_shield(&out, 1, 1, 1, 5); assert(!probes); check(&out, "");
    allowed = 1;
    resource_path_weapon(&out, 0, 1, 1, 3, -1); check(&out, "인간족\\novice_남_sword.act");
    resource_path_weapon(&out, 1, 1, 0, 3, 20); check(&out, "인간족\\novice_여_view.spr");
    resource_path_weapon_glow(&out, 1, 1, 1, 3, -1, 0); check(&out, "인간족\\novice_남_sword_검광.spr");
    resource_path_weapon_glow(&out, 0, 1, 0, 3, 20, 1); check(&out, "인간족\\novice_여_view_발광.act");
    puts("Resource paths: CP949 names, race, costume, mount, hair normalization, resource fallbacks and string ownership passed");
    return 0;
}
