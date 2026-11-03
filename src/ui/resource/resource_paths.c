#include "contents/client_lua_calls.h"
#include "ui/resource/resource_paths.h"
#include "session/session_names.h"
#include "session/session_jobs.h"
#include "session/session_lua.h"
#include "session/session_state.h"
#include "std/std.h"
#include <string.h>

static const char *const resource_extensions[2] = {
    "act",
    "spr",
};

#define JOB_ALT_BASE 0xf6e
#define EXT_SPR 1

#define DIR_BODY "몸통"
#define DIR_HEAD "머리통"
#define DIR_ACCESSORY "악세사리"
#define DIR_HAIR "머리"
#define DIR_BODY_PAL "몸"
#define DIR_SHIELD "방패"
#define DIR_ROBE "로브"
#define RACE_HUMAN_DIR "인간족"
#define MADOGEAR "마도기어_"
#define MADOARMOR "마도아머_"
#define MOUNT_MADOGEAR 2

static struct String *race_folder(struct String *out);

static const char *ext_name(int ext)
{
    return resource_extensions[ext];
}

static const char *name_at(const struct Vec *names, int i)
{
    return ((const char *const *)names->first)[i];
}

static int job_base(int job)
{
    return job > JOB_ALT_BASE ? job - JOB_ALT_BASE : job;
}

static void race_prefixed(struct String *out, struct String *tail)
{
    struct String race;

    race_folder(&race);
    string_append(&race, string_cstr(tail), tail->size);
    string_free(out);
    string_move(out, &race);
    string_free(tail);
}

static const char *hair_name(int male, int style)
{
    const struct Vec *names = male ? &g_session1.maleHairNames : &g_session1.femaleHairNames;
    return name_at(names, style);
}

static void madogear_names(struct String *path)
{
    size_t at = string_rfind(path, MADOGEAR);

    if (at != (size_t)-1)
        string_replace(path, at, 9, MADOARMOR, 9);
}

static struct String *race_folder(struct String *out)
{
    string_init(out);
    string_assign(out, RACE_HUMAN_DIR, 6);
    return out;
}

static struct String *get_robe_name(struct String *out, int robe)
{
    const char *name = "";

    client_lua_call(g_session1.luaState, "ReqRobSprName", "d>s", robe, &name);
    string_init(out);
    string_assign_cstr(out, name);
    return out;
}

struct String *resource_path_accessory(struct String *out, int ext, int job, int sex,
    int hair, int accessory)
{
    struct String path, sex_name, hair_text, acc_name;

    string_init(out);
    if (!accessory)
        return out;
    string_init(&path);
    string_init(&hair_text);
    if (sex == 1)
        string_assign_cstr(&hair_text, hair_name(1, hair));
    else if (sex == 0)
        string_assign_cstr(&hair_text, hair_name(0, hair));
    session_get_sex_sprite_name(&g_session1, &sex_name, sex);
    session_get_accessory_name(&g_session1, &acc_name, accessory);
    if (acc_name.size) {
        string_format(&path, DIR_ACCESSORY "\\%s\\%s%s.%s", string_cstr(&sex_name),
                      string_cstr(&sex_name), string_cstr(&acc_name), ext_name(ext));
        string_free(out);
        string_move(out, &path);
    }
    string_free(&acc_name);
    string_free(&hair_text);
    string_free(&sex_name);
    string_free(&path);
    return out;
}

struct String *resource_path_body_palette(struct String *out, int job, int sex, int palette,
    int costume, int mount)
{
    struct String sex_name;
    const char *body;
    int index;

    costume = costume == 1 ? 1 : 0;
    if (palette < 2 && costume == 0) {
        if ((unsigned)(job - 0xfa1) <= 0x15)
            palette = job <= 0xfa7;
        else
            palette = (unsigned)(session_remap_job(&g_session1, job) - 0xfa1) <= 6;
    }
    index = session_remap_job(&g_session1, job);
    if (index > JOB_ALT_BASE)
        index -= JOB_ALT_BASE;
    body = name_at(&g_session1.jobBodyNames, index);
    session_get_sex_sprite_name(&g_session1, &sex_name, sex);
    string_init(out);
    if (costume > 0)
        string_format(out, DIR_BODY_PAL "\\costume_%d\\%s_%s_%d_%d.pal", costume, body,
                      string_cstr(&sex_name), palette, costume);
    else
        string_format(out, DIR_BODY_PAL "\\%s_%s_%d.pal", body, string_cstr(&sex_name), palette);
    if (mount == MOUNT_MADOGEAR)
        madogear_names(out);
    string_free(&sex_name);
    return out;
}

struct String *resource_path_body_sprite(struct String *out, int ext, int job, int body,
    int sex, int costume)
{
    struct String sex_name, body_name, tail;

    string_init(out);
    session_get_sex_sprite_name(&g_session1, &sex_name, sex);
    session_get_job_sprite_name(&g_session1, &body_name, body);
    costume = costume == 1 ? 1 : 0;
    string_init(&tail);
    if (costume > 0)
        string_format(&tail, "\\" DIR_BODY "\\%s\\costume_%d\\%s_%s_%d.%s", string_cstr(&sex_name), costume,
                      string_cstr(&body_name), string_cstr(&sex_name), costume, ext_name(ext));
    else
        string_format(&tail, "\\" DIR_BODY "\\%s\\%s_%s.%s", string_cstr(&sex_name),
                      string_cstr(&body_name), string_cstr(&sex_name), ext_name(ext));
    race_prefixed(out, &tail);
    string_free(&body_name);
    string_free(&sex_name);
    return out;
}

struct String *resource_path_job_body_sprite(struct String *out, int ext, int job, int sex,
    int costume, int mount)
{
    struct String sex_name, body_name, tail;
    int base = job_base(job);

    string_init(out);
    costume = costume == 1 ? 1 : 0;
    session_get_sex_sprite_name(&g_session1, &sex_name, sex);
    session_get_job_sprite_name(&g_session1, &body_name, base);
    string_init(&tail);
    if (costume > 0)
        string_format(&tail, "\\" DIR_BODY "\\%s\\costume_%d\\%s_%s_%d.%s", string_cstr(&sex_name), costume,
                      string_cstr(&body_name), string_cstr(&sex_name), costume, ext_name(ext));
    else
        string_format(&tail, "\\" DIR_BODY "\\%s\\%s_%s.%s", string_cstr(&sex_name),
                      string_cstr(&body_name), string_cstr(&sex_name), ext_name(ext));
    race_prefixed(out, &tail);
    if (mount == MOUNT_MADOGEAR)
        madogear_names(out);
    string_free(&body_name);
    string_free(&sex_name);
    return out;
}

struct String *resource_path_head_palette(struct String *out, int hair, int job, int sex,
    int palette)
{
    struct String path, sex_name, hair_text;

    string_init(&path);
    string_init(&sex_name);
    string_init(&hair_text);
    session_class_from_code(&g_session1, &hair, job);
    if (sex == 1)
        string_assign_cstr(&hair_text, hair_name(1, hair));
    else if (sex == 0)
        string_assign_cstr(&hair_text, hair_name(0, hair));
    string_free(&sex_name);
    session_get_sex_sprite_name(&g_session1, &sex_name, sex);
    string_free(&path);
    string_format(&path, DIR_HAIR "\\" DIR_HAIR "%s_%s_%d.pal", string_cstr(&hair_text),
                  string_cstr(&sex_name), palette);
    *out = path;
    string_free(&hair_text);
    string_free(&sex_name);
    return out;
}

struct String *resource_path_head_sprite(struct String *out, int ext, int job, int sex,
    int *hair)
{
    struct String sex_name, hair_text, tail;
    int male, style;

    male = sex == 0 ? 0 : 1;
    string_init(out);
    string_init(&hair_text);
    style = *hair;
    if (style < 0 || style >= 0x2b)
        *hair = style = 0xd;
    string_assign_cstr(&hair_text, hair_name(male, style));
    session_get_sex_sprite_name(&g_session1, &sex_name, male);
    string_init(&tail);
    string_format(&tail, "\\" DIR_HEAD "\\%s\\%s_%s.%s", string_cstr(&sex_name), string_cstr(&hair_text),
                  string_cstr(&sex_name), ext_name(ext));
    race_prefixed(out, &tail);
    string_free(&hair_text);
    string_free(&sex_name);
    return out;
}

struct String *resource_path_imf(struct String *out, int job, int sex)
{
    struct String sex_name, job_name;

    session_get_sex_sprite_name(&g_session1, &sex_name, sex);
    session_get_job_imf_name(&g_session1, &job_name, job_base(job));
    string_init(out);
    string_format(out, "%s_%s.imf", string_cstr(&job_name), string_cstr(&sex_name));
    string_free(&job_name);
    string_free(&sex_name);
    return out;
}

struct String *resource_path_robe(struct String *out, int ext, int job, int sex, int robe,
    BYTE map_job)
{
    struct String path, body_name, sex_name, robe_name;

    string_init(out);
    if (robe <= 0)
        return out;
    if (map_job)
        job = session_get_mounted_job(job);
    session_get_job_sprite_name(&g_session1, &body_name, job_base(job));
    session_get_sex_sprite_name(&g_session1, &sex_name, sex);
    get_robe_name(&robe_name, robe);
    if (robe_name.size) {
        if (ext == EXT_SPR) {
            string_init(&path);
            string_format(&path, "sprite\\" DIR_ROBE "\\%s\\%s.%s", string_cstr(&robe_name),
                          string_cstr(&robe_name), ext_name(EXT_SPR));
            if (!res_mgr_is_available(g_resMgr(), string_cstr(&path))) {
                string_free(&path);
                string_format(&path, "sprite\\" DIR_ROBE "\\%s\\%s\\%s_%s.%s", string_cstr(&robe_name),
                              string_cstr(&sex_name), string_cstr(&body_name),
                              string_cstr(&sex_name), ext_name(EXT_SPR));
            }
        } else {
            string_init(&path);
            string_format(&path, "sprite\\" DIR_ROBE "\\%s\\%s\\%s_%s.%s", string_cstr(&robe_name),
                          string_cstr(&sex_name), string_cstr(&body_name), string_cstr(&sex_name),
                          ext_name(ext));
        }
        string_free(out);
        string_move(out, &path);
    }
    string_free(&robe_name);
    string_free(&sex_name);
    string_free(&body_name);
    return out;
}

struct String *resource_path_shield(struct String *out, int ext, int job, int sex, int shield)
{
    struct String path, sex_name, kind_name;
    const char *job_name = name_at(&g_session1.jobSpritePaths, job_base(job));
    int kind = session_get_item_view_id(shield);

    string_init(out);
    string_init(&path);
    session_get_sex_sprite_name(&g_session1, &sex_name, sex);
    if (shield >= 5 && session_job_allows_level(&g_session1, job, shield) == 1) {
        string_free(&path);
        string_format(&path, DIR_SHIELD "\\%s_%s_%d_" DIR_SHIELD ".%s", job_name, string_cstr(&sex_name),
                      shield, ext_name(ext));
        if (res_mgr_is_available(g_resMgr(), string_cstr(&path))) {
            string_copy(out, &path);
            goto done;
        }
    }
    if (session_job_allows_level(&g_session1, job, kind)) {
        session_get_shield_sprite_name(&g_session1, &kind_name, kind);
        string_free(&path);
        string_format(&path, DIR_SHIELD "\\%s_%s%s.%s", job_name, string_cstr(&sex_name),
                      string_cstr(&kind_name), ext_name(ext));
        string_free(&kind_name);
        if (res_mgr_is_available(g_resMgr(), string_cstr(&path)))
            string_copy(out, &path);
    }
done:
    string_free(&sex_name);
    string_free(&path);
    return out;
}

static const char *const weapon_glows[2] = {
    "_검광",
    "_발광",
};

struct String *resource_path_weapon_glow(struct String *out, int ext, int job, int sex,
    int weapon, int view, int glow)
{
    struct String sex_name, weapon_name, tail;
    const char *job_name = name_at(&g_session1.pcNames, job);

    string_init(out);
    session_get_sex_sprite_name(&g_session1, &sex_name, sex);
    session_get_weapon_sprite_name(&g_session1, &weapon_name, weapon);
    string_init(&tail);
    string_format(&tail, "\\%s_%s%s%s.%s", job_name, string_cstr(&sex_name),
                  view == -1 ? string_cstr(&weapon_name) : session_get_weapon_name_by_class(view),
                  weapon_glows[glow], ext_name(ext));
    race_prefixed(out, &tail);
    string_free(&weapon_name);
    string_free(&sex_name);
    return out;
}

struct String *resource_path_weapon(struct String *out, int ext, int job, int sex, int weapon,
    int view)
{
    struct String sex_name, weapon_name, tail;
    const char *job_name = name_at(&g_session1.pcNames, job);

    string_init(out);
    session_get_sex_sprite_name(&g_session1, &sex_name, sex);
    if (view == -1) {
        session_get_weapon_sprite_name(&g_session1, &weapon_name, weapon);
        string_init(&tail);
        string_format(&tail, "\\%s_%s%s.%s", job_name, string_cstr(&sex_name),
                      string_cstr(&weapon_name), ext_name(ext));
        string_free(&weapon_name);
        race_prefixed(out, &tail);
    } else {
        string_init(&tail);
        string_format(&tail, "\\%s_%s%s.%s", job_name, string_cstr(&sex_name),
                      session_get_weapon_name_by_class(view), ext_name(ext));
        race_prefixed(out, &tail);
    }
    string_free(&sex_name);
    return out;
}
