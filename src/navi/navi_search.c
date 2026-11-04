#include "navi/navi_search.h"
#include "game/cells.h"
#include "render/texture_cache.h"
#include "std/string.h"
#include "contents/client_lua_calls.h"
#include "world/pathfinder_state.h"
#include "resources/attr.h"
#include "session/state.h"
#include "ui/window_manager_state.h"
#include "world/mode_manager.h"
#include "game/context.h"
#include "ui/window_access.h"
#include "contents/connection_api.h"
#include "fileio/resource_api.h"
#include <string.h>
#include "navi/navi_manager.h"
#include "fileio/res_mgr.h"
#include "ui/navigation_view.h"

#include "platform/win_types.h"
#include "platform/code_page.h"
#include <stdlib.h>
#include "core/msg_str.h"
#include "ui/ui_std.h"

static void push_object(struct Vec *results, struct SearchResult *tmp, struct NaviObject *obj)
{
    struct NaviPoint scratch, *pos;

    pos = navi_object_position(obj, &scratch);
    tmp->x = pos->x;
    tmp->y = pos->y;
    string_assign_cstr(&tmp->name, navi_object_name(obj));
    tmp->obj = obj;
    search_results_push_copy(results, tmp);
}

static void widen_words(const struct Vec *words, struct Vec *wwords, uint16_t *wide)
{
    const struct String *w;

    for (w = (const struct String *)words->first; w != (const struct String *)words->last; w++) {
        struct WString ws;

        platform_code_page_to_utf16(CODE_PAGE_UTF8, string_cstr(w), -1, wide, 100);
        wstring_init(&ws);
        wstring_assign(&ws, wide, utf16_length(wide));
        vec_push(wwords, &ws, sizeof ws);
    }
}

static void wide_words_free(struct Vec *wwords)
{
    struct WString *ws;

    for (ws = (struct WString *)wwords->first; ws != (struct WString *)wwords->last; ws++)
        wstring_free(ws);
    vec_free(wwords);
}

static unsigned char all_words_in(const struct Vec *wwords, const struct WString *text)
{
    const struct WString *ww;
    unsigned char all = 1;

    for (ww = (const struct WString *)wwords->first; ww != (const struct WString *)wwords->last; ww++)
        all &= platform_utf16_find_ignore_case(wstring_cstr(text), wstring_cstr(ww)) != NULL ? 0xff : 0;
    return all;
}

static void map_key(struct NaviMap *map, struct String *key)
{
    string_copy(key, &map->name);
    string_append_char(key, ' ');
    string_append(key, string_cstr(&map->displayName), map->displayName.size);
    string_append_char(key, ' ');
}

static void widen(const char *text, uint16_t *wide, struct WString *out)
{
    platform_code_page_to_utf16(CODE_PAGE_UTF8, text, -1, wide, 100);
    wstring_assign(out, wide, utf16_length(wide));
}

static void split_words(struct NaviManager *self, const struct String *query, struct Vec *words_out)
{
    struct Vec words, chars, spaces;
    char nul;
    int i;
    unsigned n;
    int *at, *last_space;

    words.first = words.last = words.end = NULL;
    vec_init(&chars);
    vec_assign(&chars, string_cstr(query), string_cstr(query) + query->size, 1);
    nul = 0;
    vec_push(&chars, &nul, 1);

    spaces.first = spaces.last = spaces.end = NULL;
    i = -1;
    vec_push(&spaces, &i, sizeof i);
    n = (unsigned)((char *)chars.last - (char *)chars.first);
    for (i = 0; (unsigned)i < n; i++) {
        if (((char *)chars.first)[i] != ' ')
            continue;
        vec_push(&spaces, &i, sizeof i);
    }
    i = (int)n - 1;
    vec_push(&spaces, &i, sizeof i);

    last_space = (int *)spaces.last - 1;
    for (at = (int *)spaces.first; at != last_space; ) {
        int start = *at++ + 1;
        int stop = *at;

        if (start < stop)
            string_vec_push_text(&words, (const char *)chars.first + start, (size_t)(stop - start));
    }

    if (spaces.first)
        mem_free(spaces.first);
    if (chars.first)
        mem_free(chars.first);
    *words_out = words;
}

static void search_words(struct NaviManager *self, struct Vec *words, int filter, struct Vec *results)
{
    struct NaviMap **map, **maps_end;
    struct SearchResult *r, *results_end;
    int kind;

    kind = navi_manager_classify_query(self, words);

    results_end = (struct SearchResult *)results->last;
    for (r = (struct SearchResult *)results->first; r != results_end; r++)
        search_result_destroy(r);
    results->last = results->first;

    maps_end = (struct NaviMap **)self->base.maps.last;
    for (map = (struct NaviMap **)self->base.maps.first; map != maps_end; map++) {
        struct String *w = (struct String *)words->first;

        if (kind == NAVI_QUERY_ALL) {
            navi_map_list_all(*map, filter, results);
        } else if (kind == NAVI_QUERY_LEVEL) {
            int hi = atoi(string_cstr(&w[1]));
            int lo = atoi(string_cstr(&w[0]));
            navi_map_mobs_in_level_range(*map, lo, hi, results);
        } else if (kind == NAVI_QUERY_ALIAS) {
            if (VEC_COUNT(words, struct String) == 3) {
                struct String alias;
                const char *text = string_cstr(&w[1]);

                string_init(&alias);
                string_assign_cstr(&alias, text);
                navi_map_mobs_with_alias(*map, alias, results);
            }
        } else {
            navi_map_search_words(*map, filter, words, results);
        }
    }
}

unsigned char navi_manager_search(struct NaviManager *self,
        struct String query, int filter, struct Vec *results)
{
    unsigned char ok = 0;

    if (query.size) {
        struct Vec words;

        split_words(self, &query, &words);
        if (words.first != words.last) {
            search_words(self, &words, filter, results);
            ok = 1;
        }
        string_vec_free(&words);
    }
    string_free(&query);
    return ok;
}

int navi_manager_classify_query(struct NaviManager *self, struct Vec *words)
{
    struct String *first = (struct String *)words->first;
    size_t prefix;

    if (strncmp(string_cstr(first), msg_str(0x912), strlen(msg_str(0x912))) != 0)
        return NAVI_QUERY_TEXT;

    prefix = strlen(msg_str(0x913));
    if (prefix < first->size && strncmp(string_cstr(first), msg_str(0x913), prefix) == 0) {
        struct String copy, lo, hi;
        const char *data, *sep, *after;
        int kind, a, b;

        string_copy(&copy, first);
        data = string_cstr(&copy);
        sep = strstr(data, msg_str(0x914));
        string_init(&lo);
        string_init(&hi);
        if (!sep) {
            string_assign(&lo, data + prefix, copy.size - prefix);
            string_assign(&hi, string_cstr(&lo), lo.size);
        } else {
            after = sep + strlen(msg_str(0x914));
            string_assign(&lo, data + prefix, (size_t)(sep - (data + prefix)));
            string_assign(&hi, after, (size_t)(data + copy.size - after));
            if (hi.size == 0)
                string_assign_cstr(&hi, "199");
        }

        a = atoi(string_cstr(&lo));
        if ((unsigned)(a - 1) > 199) {
            kind = 0;
        } else if (b = atoi(string_cstr(&hi)), (unsigned)(b - 1) > 199) {
            kind = 0;
        } else if (a > b) {
            kind = 0;
        } else {
            string_vec_clear(words);
            string_vec_push_copy(words, &lo);
            string_vec_push_copy(words, &hi);
            kind = NAVI_QUERY_LEVEL;
        }
        string_free(&hi);
        string_free(&lo);
        string_free(&copy);
        return kind;
    }

    if (strncmp(string_cstr(first), msg_str(0x915), strlen(msg_str(0x915))) == 0)
        return NAVI_QUERY_ALL;
    return strncmp(string_cstr(first), "$$DB", 4) == 0 ? NAVI_QUERY_ALIAS : NAVI_QUERY_COMMAND;
}

void navi_map_search_words(struct NaviMap *self, int filter,
        const struct Vec *words, struct Vec *results)
{
    navi_map_match_map_words(self, words, results);
    if (filter == 2 || filter == 0)
        navi_map_match_mob_words(self, words, results);
    if (filter == 3 || filter == 0)
        navi_map_match_npc_words(self, words, results);
}

void navi_map_list_all(struct NaviMap *self, int filter, struct Vec *results)
{
    struct SearchResult tmp;
    struct NaviObject **it, **end;

    search_result_init(&tmp, filter);
    if (!strstr(string_cstr(&self->name), "_in") && (filter == 1 || filter == 0)) {
        string_assign_cstr(&tmp.name, string_cstr(&self->name));
        tmp.obj = self;
        tmp.kind = NAVI_RESULT_MAP;
        search_results_push_copy(results, &tmp);
    }
    if (filter == 2 || filter == 0) {
        tmp.kind = NAVI_RESULT_NPC;
        end = (struct NaviObject **)self->npcs.last;
        for (it = (struct NaviObject **)self->npcs.first; it != end; it++)
            push_object(results, &tmp, *it);
    }
    if (filter == 3 || filter == 0) {
        end = (struct NaviObject **)self->mobs.last;
        it = (struct NaviObject **)self->mobs.first;
        tmp.kind = NAVI_RESULT_MOB;
        for (; it != end; it++)
            push_object(results, &tmp, *it);
    }
    search_result_destroy(&tmp);
}

void navi_map_mobs_in_level_range(struct NaviMap *self, int lo, int hi,
        struct Vec *results)
{
    struct SearchResult tmp;
    struct NaviObject **it, **end;

    search_result_init(&tmp, NAVI_RESULT_MOB);
    end = (struct NaviObject **)self->mobs.last;
    for (it = (struct NaviObject **)self->mobs.first; it != end; it++) {
        struct NaviPoint p1, p2;

        if (lo > navi_object_position(*it, &p1)->x)
            continue;
        if (hi < navi_object_position(*it, &p2)->x)
            continue;
        push_object(results, &tmp, *it);
    }
    search_result_destroy(&tmp);
}

static void map_results(struct NaviMap *self, const struct Vec *wwords, const struct WString *wkey,
        struct SearchResult *tmp, struct Vec *results)
{
    const char *name = string_cstr(&self->name);

    if (strcmp(name, "gl_in01") != 0 && strstr(name, "_in"))
        return;
    if (wwords->first != wwords->last && !all_words_in(wwords, wkey))
        return;

    string_assign(&tmp->name, string_cstr(&self->name), self->name.size);
    tmp->obj = self;
    search_results_push_copy(results, tmp);

    tmp->kind = NAVI_RESULT_MOB;
    {
        struct NaviObject **it;

        for (it = (struct NaviObject **)self->mobs.first; it != (struct NaviObject **)self->mobs.last; it++) {
            tmp->x = NAVI_POS_UNDER_MAP;
            tmp->y = NAVI_POS_UNDER_MAP;
            string_assign_cstr(&tmp->name, navi_object_name(*it));
            tmp->obj = *it;
            search_results_push_copy(results, tmp);
        }
    }
}

void navi_map_match_map_words(struct NaviMap *self,
        const struct Vec *words, struct Vec *results)
{
    struct SearchResult tmp;
    uint16_t wide[100];
    struct Vec wwords;
    struct String key;
    struct WString wkey;

    search_result_init(&tmp, NAVI_RESULT_MAP);
    memset(wide, 0, sizeof(wide));
    wwords.first = NULL;
    wwords.last = NULL;
    wwords.end = NULL;
    widen_words(words, &wwords, wide);

    map_key(self, &key);
    platform_code_page_to_utf16(CODE_PAGE_UTF8, string_cstr(&key), -1, wide, 100);
    wstring_init(&wkey);
    wstring_assign(&wkey, wide, utf16_length(wide));

    map_results(self, &wwords, &wkey, &tmp, results);

    wstring_free(&wkey);
    string_free(&key);
    wide_words_free(&wwords);
    search_result_destroy(&tmp);
}

void navi_map_match_mob_words(struct NaviMap *self,
        const struct Vec *words, struct Vec *results)
{
    struct SearchResult tmp;
    uint16_t wide[100];
    struct Vec wwords;
    struct WString wkey, wname;
    struct String key;
    struct NaviObject **it;

    search_result_init(&tmp, NAVI_RESULT_MOB);
    memset(wide, 0, sizeof(wide));
    wwords.first = NULL;
    wwords.last = NULL;
    wwords.end = NULL;
    widen_words(words, &wwords, wide);

    wstring_init(&wname);
    map_key(self, &key);
    platform_code_page_to_utf16(CODE_PAGE_UTF8, string_cstr(&key), -1, wide, 100);
    wstring_init(&wkey);
    wstring_assign(&wkey, wide, utf16_length(wide));

    for (it = (struct NaviObject **)self->mobs.first; it != (struct NaviObject **)self->mobs.last; it++) {
        widen(navi_object_name(*it), wide, &wname);
        if (wwords.first != wwords.last && !all_words_in(&wwords, &wname))
            continue;
        push_object(results, &tmp, *it);
    }

    wstring_free(&wkey);
    string_free(&key);
    wstring_free(&wname);
    wide_words_free(&wwords);
    search_result_destroy(&tmp);
}

void navi_map_mobs_with_alias(struct NaviMap *self, struct String alias,
        struct Vec *results)
{
    struct SearchResult tmp;
    struct NaviObject **it;

    search_result_init(&tmp, NAVI_RESULT_MOB);
    for (it = (struct NaviObject **)self->mobs.first; it != (struct NaviObject **)self->mobs.last; it++) {
        if (strcmp(navi_object_alias(*it), string_cstr(&alias)) != 0)
            continue;
        push_object(results, &tmp, *it);
    }
    search_result_destroy(&tmp);
    string_free(&alias);
}

void navi_map_match_npc_words(struct NaviMap *self,
        const struct Vec *words, struct Vec *results)
{
    struct SearchResult tmp;
    uint16_t wide[100];
    struct Vec wwords;
    struct WString wname;
    struct NaviObject **it;

    search_result_init(&tmp, NAVI_RESULT_NPC);
    memset(wide, 0, sizeof(wide));
    wwords.first = NULL;
    wwords.last = NULL;
    wwords.end = NULL;
    widen_words(words, &wwords, wide);

    wstring_init(&wname);
    for (it = (struct NaviObject **)self->npcs.first; it != (struct NaviObject **)self->npcs.last; it++) {
        widen(navi_object_name(*it), wide, &wname);
        if (wwords.first != wwords.last && !all_words_in(&wwords, &wname))
            continue;
        push_object(results, &tmp, *it);
    }

    wstring_free(&wname);
    wide_words_free(&wwords);
    search_result_destroy(&tmp);
}

unsigned char navi_result_name_greater(struct SearchResult *self, const struct SearchResult *other)
{
    return strcmp(string_cstr(&self->name), string_cstr(&other->name)) > 0;
}
