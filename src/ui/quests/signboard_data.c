#include "ui/quests/signboard_list.h"
#include "contents/client_lua.h"
#include <limits.h>
#include <stdlib.h>
#include <string.h>

struct OwnedSignboard {
    struct SignBoardEntry row;
    char *strings;
};

static struct OwnedSignboard *rows;
static size_t row_count;

static void free_rows(struct OwnedSignboard *entries, size_t count)
{
    size_t i;
    for (i = 0; i < count; ++i)
        free(entries[i].strings);
    free(entries);
}

void signboards_clear(void)
{
    free_rows(rows, row_count);
    rows = NULL;
    row_count = 0;
}

size_t signboards_count(void) { return row_count; }

const struct SignBoardEntry *signboards_entry(size_t index)
{
    return index < row_count ? &rows[index].row : NULL;
}

static int copy_strings(struct OwnedSignboard *entry, const struct SignBoardEntry *value)
{
    const char *values[] = { value->map, value->image, value->text, value->colour };
    const char **outputs[] = { &entry->row.map, &entry->row.image, &entry->row.text, &entry->row.colour };
    size_t lengths[4], total = 0, i;
    char *next;
    entry->row = *value;
    for (i = 0; i < 4; ++i) {
        lengths[i] = values[i] ? strlen(values[i]) + 1 : 0;
        if (lengths[i] > (size_t)-1 - total)
            return 0;
        total += lengths[i];
    }
    if (!total)
        return 1;
    entry->strings = malloc(total);
    if (!entry->strings)
        return 0;
    next = entry->strings;
    for (i = 0; i < 4; ++i) {
        if (values[i]) {
            memcpy(next, values[i], lengths[i]);
            *outputs[i] = next;
            next += lengths[i];
        }
    }
    return 1;
}

static int begin_query(struct ClientLua *lua, const char *name)
{
    if (!lua_checkstack(lua->L, 12))
        return 0;
    lua_getglobal(lua->L, name);
    return 1;
}

static int call_query(struct ClientLua *lua, const char *name, int results)
{
    const char *message;
    if (lua_pcall(lua->L, 1, results, 0) == 0)
        return 1;
    message = lua_tostring(lua->L, -1);
    client_lua_error_box(lua, name, "%s\n", message ? message : "Lua query failed");
    return 0;
}

static int read_integer(lua_State *L, int index, int *value)
{
    double number = lua_tonumber(L, index);
    if (!(number >= INT_MIN && number <= INT_MAX))
        return 0;
    *value = (int)number;
    return 1;
}

static int read_row(struct ClientLua *lua, int index, const struct SignBoardEntry *defaults,
                    struct OwnedSignboard *entry)
{
    lua_State *L = lua->L;
    int base = lua_gettop(L), ok = 0;
    struct SignBoardEntry row = { 0 };
    if (!begin_query(lua, "GetSignBoardListTable"))
        return 0;
    lua_pushinteger(L, index);
    if (!call_query(lua, "GetSignBoardListTable", 8))
        goto done;
    row.map = lua_tostring(L, base + 1);
    if (!read_integer(L, base + 2, &row.x) || !read_integer(L, base + 3, &row.y) ||
        !read_integer(L, base + 4, &row.height) || !read_integer(L, base + 5, &row.type))
        goto done;
    row.image = lua_tostring(L, base + 6);
    row.text = lua_tostring(L, base + 7);
    row.colour = lua_tostring(L, base + 8);
    if (defaults) {
        if (row.type != SIGNBOARD_BOARD && row.type != SIGNBOARD_ICON)
            row.type = SIGNBOARD_PICTURE;
        if (!row.image)
            row.image = defaults->image;
        if (row.type == SIGNBOARD_BOARD) {
            if (!row.text)
                row.text = defaults->text;
            if (!row.colour)
                row.colour = defaults->colour;
            if (!row.text || !row.colour)
                goto done;
        }
        if (!row.map || !row.image || row.height < INT_MIN + 3)
            goto done;
        row.height -= 3;
    }
    ok = copy_strings(entry, &row);
done:
    lua_settop(L, base);
    return ok;
}

static int read_count(struct ClientLua *lua, size_t *count)
{
    lua_State *L = lua->L;
    int base = lua_gettop(L), ok = 0;
    double number;
    if (!begin_query(lua, "GetTableSizeForC"))
        return 0;
    lua_pushliteral(L, "SignBoardList");
    if (!call_query(lua, "GetTableSizeForC", 1))
        goto done;
    number = lua_tonumber(L, -1);
    if (!lua_isnumber(L, -1) || !(number >= 0 && number <= INT_MAX) || number != (int)number)
        goto done;
    *count = number > 1 ? (size_t)number - 1 : 0;
    ok = *count <= (size_t)-1 / sizeof(struct OwnedSignboard);
done:
    lua_settop(L, base);
    return ok;
}

int signboards_load(struct ClientLua *lua)
{
    struct OwnedSignboard defaults = { 0 }, *replacement = NULL;
    size_t count = 0, built = 0;
    if (!lua || !lua->L)
        return 0;
    if (!read_row(lua, 1, NULL, &defaults) || !read_count(lua, &count))
        goto failed;
    if (count) {
        replacement = calloc(count, sizeof *replacement);
        if (!replacement)
            goto failed;
    }
    for (built = 0; built < count; ++built)
        if (!read_row(lua, (int)built + 2, &defaults.row, &replacement[built]))
            goto failed;
    free(defaults.strings);
    signboards_clear();
    rows = replacement;
    row_count = count;
    return 1;
failed:
    free(defaults.strings);
    free_rows(replacement, built);
    return 0;
}
