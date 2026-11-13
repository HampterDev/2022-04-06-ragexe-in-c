#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <windows.h>

#define CHECK(condition)                                                                               \
    do {                                                                                               \
        if (!(condition)) {                                                                            \
            fprintf(stderr, "Sprite name table check failed at line %d: %s\n", __LINE__, #condition); \
            exit(2);                                                                                   \
        }                                                                                              \
    } while (0)

#include "../src/session/sprite_names.c"
#include "../src/ui/resource/resource_paths.c"
#include "../src/session/session_lookups.c"
#include "../src/session/talk_tables.c"
#include "../src/session/session_names.c"
#include "sprite_name_tables_unreached.h"

struct Session g_session1;
struct UIWindowMgr g_windowMgr;
struct MapInfoMgr *g_mapInfo;
unsigned int g_codePage = 949;
static int resource_manager;

void *mem_alloc(size_t bytes)
{
    void *block = malloc(bytes);

    CHECK(block);
    return block;
}

void mem_free(void *memory)
{
    free(memory);
}

__declspec(noreturn) void fatal_error(const char *message)
{
    fprintf(stderr, "%s\n", message);
    exit(3);
}
static struct ItemTableEntry shield_item;
static int shield_class = 9;

struct ResMgr *g_resMgr(void)
{
    return (struct ResMgr *)&resource_manager;
}

unsigned char res_mgr_is_available(struct ResMgr *self, const char *name)
{
    CHECK(self == (struct ResMgr *)&resource_manager && name);
    return strstr(name, "_방패.") == NULL;
}

int client_lua_call(struct ClientLua *lua, const char *function, const char *format, ...)
{
    va_list args;

    (void)lua;
    CHECK(!strcmp(function, "ReqRobSprName") && !strcmp(format, "d>s"));
    va_start(args, format);
    (void)va_arg(args, int);
    *va_arg(args, const char **) = "ROBE";
    va_end(args);
    return 1;
}

int session_get_real_weapon_id(int weapon)
{
    return weapon;
}

const struct ItemTableEntry *item_table_get(int item_id)
{
    (void)item_id;
    shield_item.class_num = shield_class;
    return &shield_item;
}

void session_class_from_code(struct Session *self, int *out, int code)
{
    (void)self;
    (void)out;
    (void)code;
}

BYTE session_is_doram_job(struct Session *self, int job)
{
    (void)self;
    (void)job;
    return 0;
}

BYTE session_is_fourth_job(struct Session *self, int job)
{
    (void)self;
    (void)job;
    return 0;
}

BYTE session_job_allows_level(struct Session *self, int job, int level)
{
    (void)self;
    (void)job;
    (void)level;
    return 1;
}

static void guard(struct Vec *names, size_t count, int at_page_start, const char *first, const char *rest)
{
    size_t bytes = count * sizeof(const char *);
    size_t span = (bytes + 0xfff) & ~(size_t)0xfff;
    unsigned char *pages = VirtualAlloc(NULL, span + 0x2000, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
    const char **table;
    DWORD old;
    size_t i;

    CHECK(pages);
    CHECK(VirtualProtect(pages, 0x1000, PAGE_NOACCESS, &old));
    CHECK(VirtualProtect(pages + 0x1000 + span, 0x1000, PAGE_NOACCESS, &old));
    table = (const char **)(at_page_start ? pages + 0x1000 : pages + 0x1000 + span - bytes);
    table[0] = first;
    for (i = 1; i < count; i++)
        table[i] = rest;
    names->first = table;
    names->last = table + count;
    names->end = table + count;
}

static void expect(struct String *path, const char *text)
{
    CHECK(!strcmp(string_cstr(path), text));
    string_free(path);
}

static void tables(int at_page_start)
{
    guard(&g_session1.jobSpriteNames, 0x6d93, at_page_start, "JOB", "unlisted");
    guard(&g_session1.jobImfNames, 0x6d93, at_page_start, "IMF", "unlisted");
    guard(&g_session1.jobSpritePaths, 0x6d93, at_page_start, "PATH", "unlisted");
    guard(&g_session1.pcNames, 0x6d93, at_page_start, "PC", "unlisted");
    guard(&g_session1.sexSpriteNames, 2, at_page_start, "F", "M");
    guard(&g_session1.weaponSpriteNames, 0x69, at_page_start, "", "_W");
    guard(&g_session1.shieldSpriteNames, 5, at_page_start, "", "_S");
    guard(&g_session1.maleHairNames, 0x2b, at_page_start, "2", "9");
    guard(&g_session1.femaleHairNames, 0x2b, at_page_start, "2", "9");
    guard(&g_session1.accessoryNames, 0xbb8, at_page_start, "", "_ACC");
    guard(&g_session1.reqJobNames, 0x7d01, at_page_start, "PORING", "unlisted");
}

static void outside(int job, int sex, int value)
{
    struct String path;

    resource_path_robe(&path, 0, job, sex, 1, 0);
    expect(&path, "sprite\\로브\\ROBE\\F\\JOB_F.act");
    resource_path_robe(&path, 1, job, sex, 1, 1);
    expect(&path, "sprite\\로브\\ROBE\\ROBE.spr");
    session_get_job_sprite_name(&g_session1, &path, job);
    expect(&path, "JOB");
    session_get_sex_sprite_name(&g_session1, &path, sex);
    expect(&path, "F");
    resource_path_job_body_sprite(&path, 0, job, sex, 0, 0);
    expect(&path, "인간족\\몸통\\F\\JOB_F.act");
    resource_path_imf(&path, job, sex);
    expect(&path, "IMF_F.imf");
    resource_path_weapon(&path, 0, job, sex, value, -1);
    expect(&path, "인간족\\PC_F.act");
    resource_path_shield(&path, 1, job, sex, value);
    expect(&path, "방패\\PATH_F.spr");
    resource_path_accessory(&path, 0, job, sex, value, 1);
    expect(&path, "악세사리\\F\\F_ACC.act");
    session_get_monster_act_name(&g_session1, &path, job);
    expect(&path, "몬스터\\PORING.act");
    session_get_monster_sprite_name(&g_session1, &path, job);
    expect(&path, "몬스터\\PORING.spr");
}

static void hair_outside(int hair)
{
    struct String path;

    resource_path_accessory(&path, 0, 0, 1, hair, 1);
    expect(&path, "악세사리\\M\\M_ACC.act");
    resource_path_head_palette(&path, hair, 0, 1, 3);
    expect(&path, "머리\\머리2_M_3.pal");
    resource_path_head_palette(&path, hair, 0, 0, 3);
    expect(&path, "머리\\머리2_F_3.pal");
}

static void last_entry(struct Vec *names, const char *name)
{
    ((const char **)names->last)[-1] = name;
}

static void inside_last(void)
{
    struct String path;

    last_entry(&g_session1.jobSpriteNames, "JOBLAST");
    last_entry(&g_session1.jobImfNames, "IMFLAST");
    last_entry(&g_session1.jobSpritePaths, "PATHLAST");
    last_entry(&g_session1.pcNames, "PCLAST");
    last_entry(&g_session1.weaponSpriteNames, "_WLAST");
    last_entry(&g_session1.shieldSpriteNames, "_SLAST");
    last_entry(&g_session1.reqJobNames, "MONLAST");
    resource_path_robe(&path, 0, 0x6d92 + 0xf6e, 1, 1, 0);
    expect(&path, "sprite\\로브\\ROBE\\M\\JOBLAST_M.act");
    resource_path_weapon(&path, 0, 0x6d92, 1, 0x68, -1);
    expect(&path, "인간족\\PCLAST_M_WLAST.act");
    resource_path_weapon(&path, 0, 0x6d93, 1, 0x68, -1);
    expect(&path, "인간족\\PC_M_WLAST.act");
    shield_class = 4;
    resource_path_shield(&path, 1, 0x6d92 + 0xf6e, 1, 3);
    expect(&path, "방패\\PATHLAST_M_SLAST.spr");
    shield_class = 9;
    resource_path_imf(&path, 0x6d92 + 0xf6e, 1);
    expect(&path, "IMFLAST_M.imf");
    resource_path_imf(&path, 0x6d93 + 0xf6e, 1);
    expect(&path, "IMF_M.imf");
    session_get_monster_act_name(&g_session1, &path, 0x7d00);
    expect(&path, "몬스터\\MONLAST.act");
    session_get_monster_sprite_name(&g_session1, &path, 0x7d00);
    expect(&path, "몬스터\\MONLAST.spr");
    session_get_monster_sprite_name(&g_session1, &path, 0x7d01);
    expect(&path, "몬스터\\PORING.spr");
}

int main(void)
{
    static const int jobs[] = { 0x6d93 + 0xf6e, 0x7fff, 0xffff, 0x7fffffff, -1, -0x8000, (int)0x80000000u };
    static const int values[] = { 0x69, 0xbb8, 0x7fffffff, -1, (int)0x80000000u };
    static const int sexes[] = { 2, 0xff, 0x7fffffff, -1, (int)0x80000000u };
    struct String path;
    size_t i, j;
    int at_page_start;

    for (at_page_start = 0; at_page_start < 2; at_page_start++) {
        tables(at_page_start);
        for (i = 0; i < sizeof jobs / sizeof *jobs; i++)
            for (j = 0; j < sizeof sexes / sizeof *sexes; j++)
                outside(jobs[i], sexes[j], values[j]);
        for (j = 0; j < sizeof values / sizeof *values; j++)
            hair_outside(values[j]);
        resource_path_robe(&path, 0, 0x6d92 + 0xf6e, 1, 1, 0);
        expect(&path, "sprite\\로브\\ROBE\\M\\unlisted_M.act");
        resource_path_robe(&path, 0, 0xf6e, 0, 1, 0);
        expect(&path, "sprite\\로브\\ROBE\\F\\unlisted_F.act");
        resource_path_head_palette(&path, 0x2a, 0, 1, 3);
        expect(&path, "머리\\머리9_M_3.pal");
        inside_last();
    }

    puts("Sprite name tables: jobs, sexes, hair styles, weapons, shields and accessories outside their "
         "name tables read each table's first entry on the robe, body, weapon, shield, accessory, hair "
         "palette, imf and monster paths, passed.");
    return 0;
}
