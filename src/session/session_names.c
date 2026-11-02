#include "session/map_names.h"
#include "ui/resource/resource_paths.h"
#include "session/csession.h"
#include "session/session_items.h"
#include "session/session_jobs.h"
#include "session/session_lua.h"
#include "session/session_names.h"
#include "session/session_talk.h"

#include <stdlib.h>
#include <string.h>
#include "std/std.h"
#include "ui/ui_std.h"

struct String s_rswName;
struct Once s_rswNameGuard;

const char *session_get_rsw_name(const char *mapName)
{
    struct String *name = &s_rswName;

    if (once_begin(&s_rswNameGuard)) {
        string_init(name);
        atexit(session_rsw_name_atexit);
        once_complete(&s_rswNameGuard);
    }
    string_assign_cstr(name, mapName);
    if (name->size >= 5) {
        size_t length = name->size - 3;
        string_resize(name, length, 0);
        string_append_cstr(name, "rsw");
    }
    return string_cstr(name);
}

const char *session_get_attr_wave_name(int attr)
{
    int r;

    if (attr == 3) {
        r = rand() % 2;
        if (r == 0)
            return "_enemy_hit_fire1.wav";
        if (r == 1)
            return "_enemy_hit_fire2.wav";
    } else if (attr == 4) {
        r = rand() % 2;
        if (r == 0)
            return "_enemy_hit_wind1.wav";
        if (r == 1)
            return "_enemy_hit_wind2.wav";
    }
    switch (rand() % 4) {
    case 0:  return "_enemy_hit_normal1.wav";
    case 1:  return "_enemy_hit_normal2.wav";
    case 2:  return "_enemy_hit_normal3.wav";
    default: return "_enemy_hit_normal4.wav";
    }
}

struct String *session_get_res_name0(struct String *out, int a, int b, int c, int d)
{
    resource_path_accessory(out, 0, a, b, c, d);
    return out;
}

struct String *session_get_res_name1(struct String *out, int a, int b, int c, int d)
{
    resource_path_accessory(out, 1, a, b, c, d);
    return out;
}

struct String *session_get_body_pal_name(struct String *out, int a, int b, int c, int d,
                                                     int e)
{
    resource_path_body_palette(out, a, b, c, d, e);
    return out;
}

struct String *session_get_body_spr_name(struct String *out, int a, int b, int c, int d, int e)
{
    resource_path_body_sprite(out, a, b, c, d, e);
    return out;
}

struct String *session_get_head_spr_name0(struct String *out, int job, int sex, int *head)
{
    resource_path_head_sprite(out, 0, job, sex, head);
    return out;
}

struct String *session_get_head_spr_name1(struct String *out, int job, int sex, int *head)
{
    resource_path_head_sprite(out, 1, job, sex, head);
    return out;
}

static struct String *own_head_spr(struct Session *self, struct String *out, int variant)
{
    int sex = session_get_sex(self);
    int job = session_get_drawn_job(self);
    resource_path_head_sprite(out, variant, job, sex, &self->hairStyle);
    return out;
}

struct String *session_get_own_head_spr_name0(struct Session *self,
                                                         struct String *out)
{
    return own_head_spr(self, out, 0);
}

struct String *session_get_own_head_spr_name1(struct Session *self,
                                                         struct String *out)
{
    return own_head_spr(self, out, 1);
}

struct String *session_get_head_pal_name(struct String *out, int a, int b, int c, int d)
{
    resource_path_head_palette(out, a, b, c, d);
    return out;
}

struct String *session_get_own_head_pal_name(struct Session *self,
                                                        struct String *out)
{
    resource_path_head_palette(out, self->hairStyle, self->job, self->sexCopy, self->hairColor);
    return out;
}

struct String *session_get_imf_name(struct String *out, int a, int b)
{
    resource_path_imf(out, a, b);
    return out;
}

struct String *session_get_job_imf_name(struct Session *self,
                                                   struct String *out, int i)
{
    const char *text = ((const char **)self->jobImfNames.first)[i];
    string_init(out);
    string_assign_cstr(out, text);
    return out;
}

static const char s_itemBmpFormat[] = "유저인터페이스\\item\\%s.bmp";
static const char s_itemFolder[] = "아이템\\";

int session_get_item_bitmap_file_name(const char *itemIdText, char *buf, int identified)
{
    const struct ItemTableEntry *entry = item_table_get(atoi(itemIdText));
    return sprintf(buf, s_itemBmpFormat, item_table_resource_name(entry, identified));
}

struct String s_itemDisplayName;
struct Once s_itemDisplayNameGuard;
struct String s_itemActName;
struct Once s_itemActNameGuard;

const char *session_get_item_display_name(const char *itemIdText, int identified)
{
    struct String *name = &s_itemDisplayName;
    const char *res;

    if (once_begin(&s_itemDisplayNameGuard)) {
        string_init(name);
        atexit(session_item_display_name_atexit);
        once_complete(&s_itemDisplayNameGuard);
    }
    string_assign(name, s_itemFolder, 7);
    res = item_table_resource_name(item_table_get(atoi(itemIdText)), identified);
    if (res == NULL) {
        struct String apple;
        string_init(&apple);
        string_assign(&apple, "사과", 4);
        string_append(name, string_cstr(&apple), apple.size);
        string_free(&apple);
    } else {
        string_append_cstr(name, res);
    }
    return string_cstr(name);
}

const char *session_get_item_act_name(const char *itemName)
{
    struct String *name = &s_itemActName;
    struct String act;
    BYTE exists;

    if (once_begin(&s_itemActNameGuard)) {
        string_init(name);
        atexit(session_item_act_name_atexit);
        once_complete(&s_itemActNameGuard);
    }
    string_assign(name, s_itemFolder, 7);
    string_append_cstr(name, itemName);
    string_copy(&act, name);
    string_append_cstr(&act, ".act");
    exists = res_mgr_is_available(g_resMgr(), string_cstr(&act));
    string_free(&act);
    if (!exists)
        string_assign(name, "아이템\\am_berserkpitcher", 0x18);
    return string_cstr(name);
}

static struct NamedStrings *named_lower_bound(struct NamedStrings *first, struct NamedStrings *last, const char *key)
{
    int count = (int)(last - first);

    while (count > 0) {
        int half = count >> 1;
        struct NamedStrings *mid = first + half;
        if (strcmp(string_cstr(&mid->name), key) < 0) {
            first = mid + 1;
            count -= half + 1;
        } else {
            count = half;
        }
    }
    return first;
}

struct Vec s_fogNone;
struct Once s_fogNoneGuard;

struct Vec *session_find_fog_parameters(struct Session *self,
                                                    const char *name, int *found)
{
    struct Vec *result;
    struct String key;
    const char *keyText;
    struct NamedStrings *entry;
    size_t pos;

    *found = 0;
    if (once_begin(&s_fogNoneGuard)) {
        vec_init(&s_fogNone);
        string_vec_resize(&s_fogNone, 4);
        atexit(session_fog_none_atexit);
        once_complete(&s_fogNoneGuard);
    }
    string_init(&key);
    string_assign_cstr(&key, name);
    pos = string_rfind(&key, "@");
    if (pos != (size_t)-1 && pos >= 4)
        string_erase(&key, pos - 4, 3);
    keyText = string_cstr(&key);
    entry = named_lower_bound(self->fogParameters.first, self->fogParameters.last, keyText);
    result = &s_fogNone;
    if (entry != self->fogParameters.last
        && strcmp(string_cstr(&entry->name), string_cstr(&key)) == 0) {
        *found = 1;
        result = &entry->values;
    }
    string_free(&key);
    return result;
}

struct Vec s_viewNone;
struct Once s_viewNoneGuard;

struct Vec *session_get_map_view_preset(struct Session *self,
                                                    const char *name, int *found)
{
    struct Vec *result;
    struct String key;
    const char *keyText;
    struct NamedStrings *entry;
    size_t pos;

    *found = 0;
    if (once_begin(&s_viewNoneGuard)) {
        vec_init(&s_viewNone);
        string_vec_resize(&s_viewNone, 9);
        atexit(session_view_none_atexit);
        once_complete(&s_viewNoneGuard);
    }
    string_init(&key);
    string_assign_cstr(&key, name);
    pos = string_rfind(&key, "@");
    if (pos != (size_t)-1 && pos >= 4)
        string_erase(&key, pos - 4, 3);
    keyText = string_cstr(&key);
    entry = named_lower_bound(self->viewPoints.first, self->viewPoints.last, keyText);
    result = &s_viewNone;
    if (entry != self->viewPoints.last
        && strcmp(string_cstr(&entry->name), string_cstr(&key)) == 0) {
        *found = 1;
        result = &entry->values;
    }
    string_free(&key);
    return result;
}

struct String *session_get_body_spr_name_variant0(struct String *out, int a, int b, int c, int d)
{
    resource_path_job_body_sprite(out, 0, a, b, c, d);
    return out;
}

struct String *session_get_body_spr_name_variant1(struct String *out, int a, int b, int c, int d)
{
    resource_path_job_body_sprite(out, 1, a, b, c, d);
    return out;
}

const char *session_get_job_name(struct Session *self, unsigned job, int sex)
{
    if (job < 0x1f || job - 0xfa1 <= 0x149) {
        int s = sex == -1 ? session_get_sex(self) : sex;
        const struct JobName *name;

        if (s == 0) {
            name = tree_find(&self->jobNamesSex0, &job, tree_less_uint);
            if (name)
                return name->name;
        } else if (sex == 99) {
            name = tree_find(&self->jobNamesSex99, &job, tree_less_uint);
            if (name)
                return name->name;
        }
    }
    if (job >= VEC_COUNT(&self->reqJobNames, const char *))
        return "";
    return ((const char **)self->reqJobNames.first)[job];
}

const char *session_map_display_name(const char *name)
{
    return map_info_display_name(g_mapInfo, name);
}

static const struct NamedText *lower_bound_by_name(const struct Vec *v, const char *key)
{
    const struct NamedText *first = v->first;
    int count = (int)VEC_COUNT(v, const struct NamedText);

    while (count > 0) {
        int half = count >> 1;
        const struct NamedText *mid = first + half;
        if (strcmp(mid->name, key) < 0) {
            first = mid + 1;
            count -= half + 1;
        } else {
            count = half;
        }
    }
    return first;
}

static const struct NamedInt *lower_bound_int_by_name(const struct Vec *v, const char *key)
{
    const struct NamedInt *first = v->first;
    int count = (int)VEC_COUNT(v, const struct NamedInt);

    while (count > 0) {
        int half = count >> 1;
        const struct NamedInt *mid = first + half;
        if (strcmp(mid->name, key) < 0) {
            first = mid + 1;
            count -= half + 1;
        } else {
            count = half;
        }
    }
    return first;
}

const char *session_get_mp3_name(struct Session *self, const char *mapName)
{
    struct String key;
    const struct NamedText *at;
    const char *result;
    size_t pos;

    string_init(&key);
    string_assign_cstr(&key, mapName);
    pos = string_rfind(&key, "@");
    if (pos != (size_t)-1 && pos >= 4)
        string_erase(&key, pos - 4, 3);
    at = lower_bound_by_name(&self->mp3Names, string_cstr(&key));
    if (at != (const struct NamedText *)self->mp3Names.last && strcmp(at->name, string_cstr(&key)) == 0)
        result = at->text;
    else
        result = NULL;
    string_free(&key);
    return result;
}

int session_lookup_talk_type(struct Session *self, const char *name)
{
    const struct NamedInt *at = lower_bound_int_by_name(&self->talkTypes, name);
    if (at != (const struct NamedInt *)self->talkTypes.last && strcmp(at->name, name) == 0)
        return at->value;
    return 3;
}

BYTE session_is_fixed_view_map(struct Session *self, const char *mapName)
{
    struct String key;
    const char *const *first = (const char *const *)self->indoorRsws.first;
    const char *keyText;
    int count;
    size_t pos;
    BYTE found;

    if (self->indoorRsws.first == self->indoorRsws.last)
        return 0;
    string_init(&key);
    string_assign_cstr(&key, mapName);
    pos = string_rfind(&key, "@");
    if (pos != (size_t)-1 && pos >= 4)
        string_erase(&key, pos - 4, 3);
    keyText = string_cstr(&key);
    count = (int)VEC_COUNT(&self->indoorRsws, const char *);
    while (count > 0) {
        int half = count >> 1;
        if (strcmp(first[half], keyText) < 0) {
            first += half + 1;
            count -= half + 1;
        } else {
            count = half;
        }
    }
    found = first != (const char *const *)self->indoorRsws.last
            && strcmp(*first, string_cstr(&key)) == 0;
    string_free(&key);
    return found;
}

static int map_value(struct Tree *map, int key)
{
    struct IntIntPair *pair = tree_find(map, &key, tree_less_int);

    if (!pair)
        return -1;
    return pair->value;
}

int session_get_skill_effect_id(struct Session *self, int key)
{
    return map_value(&self->skillEffectIds, key);
}

int session_get_skill_hit_effect_id(struct Session *self, int key)
{
    return map_value(&self->skillHitEffectIds, key);
}

struct String *session_get_shield_name0(struct String *out, int a, int b, int c)
{
    resource_path_shield(out, 0, a, b, c);
    return out;
}

struct String *session_get_shield_name1(struct String *out, int a, int b, int c)
{
    resource_path_shield(out, 1, a, b, c);
    return out;
}

const char *session_get_shorten_item_name(struct Session *self, int i)
{
    return string_cstr(&self->shortenSkillNames[i]);
}

void session_set_shorten_item_name(struct Session *self, const char *name, int slot)
{
    int i;

    for (i = self->shortenSkillPage * 9; i < (self->shortenSkillPage + 1) * 9; i++) {
        struct String *s = &self->shortenSkillNames[i];
        if (string_equal_cstr(s, name))
            string_assign_cstr(s, "nothing");
    }
    if (strlen(name) == 0)
        string_assign_cstr(&self->shortenSkillNames[slot], "nothing");
    else
        string_assign_cstr(&self->shortenSkillNames[slot], name);
}

int session_format_item_bitmap_path(const char *name, char *buf)
{
    return sprintf(buf, s_itemBmpFormat, name);
}

char *session_remove_underbar(const char *src, char *dst)
{
    char *p;

    if (dst != src) {
        const char *s = src;
        char *d = dst;
        while ((*d++ = *s++) != 0)
            ;
    }
    for (p = dst; *p; p = platform_code_page_next(g_codePage, p))
        if (*p == '_')
            *p = ' ';
    return dst;
}

void session_rsw_name_atexit(void)
{
    string_free(&s_rswName);
}

void session_item_display_name_atexit(void)
{
    string_free(&s_itemDisplayName);
}

void session_item_act_name_atexit(void)
{
    string_free(&s_itemActName);
}

void session_fog_none_atexit(void)
{
    string_vec_free(&s_fogNone);
}

void session_view_none_atexit(void)
{
    string_vec_free(&s_viewNone);
}
