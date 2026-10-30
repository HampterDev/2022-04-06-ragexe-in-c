#include "contents/client_lua.h"
#include "session/state.h"
#include <string.h>
#include "effects/effect_mgr.h"
#include "game/context.h"
#include "std/std.h"

static size_t string_find_char(const struct String *s, char c)
{
    const char *text = string_cstr(s);
    const char *at = s->size ? memchr(text, c, s->size) : 0;
    return at ? (size_t)(at - text) : (size_t)-1;
}

static void erase_first_at(struct String *s)
{
    size_t at = string_find_char(s, '@');
    if (at != (size_t)-1)
        string_erase(s, at, 1);
}

static void begin_key(struct String *out, const struct String *map)
{
    string_init(out);
    string_assign_cstr(out, "_");
    string_append(out, string_cstr(map), map->size);
}

struct String *effect_tool_key(struct String *out, struct String map, struct String table)
{
    begin_key(out, &map);
    string_append_cstr(out, "_");
    string_append(out, string_cstr(&table), table.size);
    erase_first_at(out);
    string_free(&map);
    string_free(&table);
    return out;
}

struct String *effect_tool_emitter_key(struct String *out, struct String map)
{
    begin_key(out, &map);
    string_append_cstr(out, "_emitterInfo");
    erase_first_at(out);
    string_free(&map);
    return out;
}

struct String *effect_tool_effect_key(struct String *out, struct String map)
{
    begin_key(out, &map);
    string_append_cstr(out, "_Effect");
    erase_first_at(out);
    string_free(&map);
    return out;
}

struct String *effect_tool_map_name(struct String *out)
{
    const char *file = game_current_map_name();
    struct String name;
    size_t dot;

    string_init(&name);
    string_assign_cstr(&name, file);
    if (name.size >= 1) {
        const char *text = string_cstr(&name);
        const char *p = text + name.size - 1;
        while (*p != '@' && p != text)
            p--;
        if (*p == '@' && (size_t)(p - text) >= 4)
            string_erase(&name, (size_t)(p - text) - 4, 3);
    }
    dot = string_find_char(&name, '.');
    string_init(out);
    string_assign(out, string_cstr(&name), name.size < dot ? name.size : dot);
    string_free(&name);
    return out;
}

static void lua_call_with_key(const char *func, const struct String *key)
{
    client_lua_call(g_session1.luaState, func, "s", string_cstr(key));
}

static void table_key(struct String *key, const struct String *map, const char *table)
{
    struct String map_copy, table_name;
    string_init(&table_name);
    string_assign_cstr(&table_name, table);
    string_copy(&map_copy, map);
    effect_tool_key(key, map_copy, table_name);
}

unsigned char effect_tool_is_loaded(struct String path, struct String map)
{
    struct String version_key, emitter_key;
    unsigned char exists = 0, loaded;

    table_key(&version_key, &map, "effect_version");

    client_lua_call(g_session1.luaState, "IsExistValue", "s>b", string_cstr(&version_key), &exists);
    if (exists) {
        loaded = 1;
    } else {
        table_key(&emitter_key, &map, "emitterInfo");

        client_lua_call(g_session1.luaState, "IsExistValue", "s>b", string_cstr(&emitter_key), &exists);
        loaded = exists != 0;
        string_free(&emitter_key);
    }
    string_free(&version_key);
    string_free(&path);
    string_free(&map);
    return loaded;
}

void effect_tool_read_v1(struct String map)
{
    struct String key, map_copy;

    string_copy(&map_copy, &map);
    effect_tool_emitter_key(&key, map_copy);
    lua_call_with_key("ReadEmitterInfoz_ver_1_0", &key);
    string_free(&key);

    string_copy(&map_copy, &map);
    effect_tool_effect_key(&key, map_copy);
    lua_call_with_key("ReadEffectInfoz", &key);
    string_free(&key);

    string_free(&map);
}

void effect_tool_read_v2(struct String map)
{
    static const char *const tables[][2] = {
        { "emitterInfo", "ReadEmitterInfoz" },
        { "magicfloorInfo", "ReadMagicFloorInfoz" },
        { "evilsPawInfo", "ReadEvilsPawInfoz" },
        { "animatedEmitterInfo", "ReadAnimatedEmitterInfoz" },
        { "ez2strInfo", "ReadEZ2STRInfoz" },
        { "windEffectInfo", "ReadWindEffectInfoz" },
    };
    struct String key;
    int i;

    for (i = 0; i < 6; i++) {
        table_key(&key, &map, tables[i][0]);
        lua_call_with_key(tables[i][1], &key);
        string_free(&key);
    }
    string_free(&map);
}

void effect_mgr_load_effect_tool(struct EffectMgr *self)
{
    struct String dir, map, path, path_arg, map_arg, key;
    int version;

    if (!self->tracked.size)
        return;
    string_init(&dir);
    string_assign_cstr(&dir, "Lua Files\\EffectTool\\");
    effect_tool_map_name(&map);
    string_copy(&path, &dir);
    string_append(&path, string_cstr(&map), map.size);

    string_copy(&map_arg, &map);
    string_init(&path_arg);
    string_assign_cstr(&path_arg, string_cstr(&path));
    if (effect_tool_is_loaded(path_arg, map_arg)
        || client_lua_load_file(g_session1.luaState, string_cstr(&path), 1, 0)) {
        version = 0;
        table_key(&key, &map, "effect_version");

        client_lua_call(g_session1.luaState, "GetValue", "s>d", string_cstr(&key), &version);
        if (version >= 0) {
            if (version <= 1) {
                string_copy(&map_arg, &map);
                effect_tool_read_v1(map_arg);
            } else if (version == 2) {
                string_copy(&map_arg, &map);
                effect_tool_read_v2(map_arg);
            }
        }
        string_free(&key);
    }
    string_free(&path);
    string_free(&map);
    string_free(&dir);
}
