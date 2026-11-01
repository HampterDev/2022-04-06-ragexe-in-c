#include "contents/character_settings_internal.h"
#include "session/state.h"
#include "std/vec.h"
#include <stdlib.h>
#include <string.h>

enum { PLAYER_LEVELS = 0x157c, HOMUN_LEVELS = 0x3d, HOMUN_FIRST = 8000, MAX_LEVEL = 65535 };

static int ready(void)
{
    return g_session1.skills.chosen_levels.first && g_session1.homunSkillUseLevels.first &&
        vec_count(&g_session1.skills.chosen_levels, sizeof(int)) == PLAYER_LEVELS &&
        vec_count(&g_session1.homunSkillUseLevels, sizeof(int)) == HOMUN_LEVELS;
}

int character_settings_clear(void)
{
    if (!ready()) return 0;
    memset(g_session1.skills.chosen_levels.first, 0, PLAYER_LEVELS * sizeof(int));
    memset(g_session1.homunSkillUseLevels.first, 0, HOMUN_LEVELS * sizeof(int));
    return 1;
}

static void put_int(struct JsonTree *out, const char *name, int value)
{
    struct JsonTree number;
    json_value_from_int(&number, value);
    json_value_assign(json_value_member(out, name), &number);
}

static int write_levels(struct JsonTree *out, const char *name, const int *levels,
                        int first, int count, int offset)
{
    struct JsonTree list, entry;
    int i, ok = 1;
    json_tree_init(&list);
    list.type = JSON_TREE_ARRAY;
    for (i = first; i < count; ++i) {
        if (levels[i] < 0 || levels[i] > MAX_LEVEL) { ok = 0; break; }
        if (!levels[i]) continue;
        json_tree_init(&entry);
        put_int(&entry, "id", i + offset);
        put_int(&entry, "level", levels[i]);
        json_value_append(&list, &entry);
        json_tree_free(&entry);
    }
    if (ok) json_value_assign(json_value_member(out, name), &list);
    json_tree_free(&list);
    return ok;
}

int character_settings_capture(struct JsonTree *out)
{
    struct JsonTree next;
    int ok;
    if (!ready()) return 0;
    json_tree_init(&next);
    put_int(&next, "version", 1);
    ok = write_levels(&next, "player", g_session1.skills.chosen_levels.first, 1, PLAYER_LEVELS, 0) &&
         write_levels(&next, "homunculus", g_session1.homunSkillUseLevels.first, 0, HOMUN_LEVELS, HOMUN_FIRST);
    if (ok) { json_tree_free(out); *out = next; }
    else json_tree_free(&next);
    return ok;
}

static const struct JsonTree *field(const struct JsonTree *data, const char *name)
{
    return json_tree_find(data, name, strlen(name));
}

static int integer(const struct JsonTree *data, const char *name, int max, int *out)
{
    const struct JsonTree *v = field(data, name);
    if (!v || (v->type != JSON_TREE_INT64 && v->type != JSON_TREE_UINT64) ||
        v->u.i64 < 0 || v->u.u64 > (unsigned)max)
        return 0;
    *out = (int)v->u.i64;
    return 1;
}

static int read_levels(const struct JsonTree *list, int *levels, int first, int count, int offset)
{
    size_t i;
    int id, level;
    if (!list || list->type != JSON_TREE_ARRAY || list->count > (size_t)(count - first)) return 0;
    for (i = 0; i < list->count; ++i) {
        const struct JsonTree *entry = &list->u.items[i];
        if (entry->type != JSON_TREE_OBJECT || !integer(entry, "id", offset + count - 1, &id) ||
            id < offset + first || !integer(entry, "level", MAX_LEVEL, &level) || !level)
            return 0;
        id -= offset;
        if (levels[id]) return 0;
        levels[id] = level;
    }
    return 1;
}

int character_settings_apply(const struct JsonTree *data)
{
    int *levels, version, ok;
    if (!ready() || !data || data->type != JSON_TREE_OBJECT ||
        !integer(data, "version", 1, &version) || version != 1)
        return 0;
    levels = calloc(PLAYER_LEVELS + HOMUN_LEVELS, sizeof *levels);
    if (!levels) return 0;
    ok = read_levels(field(data, "player"), levels, 1, PLAYER_LEVELS, 0) &&
         read_levels(field(data, "homunculus"), levels + PLAYER_LEVELS, 0, HOMUN_LEVELS, HOMUN_FIRST);
    if (ok) {
        memcpy(g_session1.skills.chosen_levels.first, levels, PLAYER_LEVELS * sizeof(int));
        memcpy(g_session1.homunSkillUseLevels.first, levels + PLAYER_LEVELS, HOMUN_LEVELS * sizeof(int));
    }
    free(levels);
    return ok;
}
