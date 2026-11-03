/* luadump - load Lua 5.1 data chunks the way the client does and print every
 * value they produce in a canonical, byte-exact form.
 *
 *   luadump [--main] <file> [<file>...]
 *   luadump --compile <source> <output>     (luac: write the chunk's bytecode,
 *                                            used by the tests to make fixtures)
 *
 * Each file is loaded with luaL_loadbuffer (source or 32-bit Lua 5.1
 * bytecode, like client_lua.c) into one shared environment and run.  With
 * --main, a global function main() is called afterwards, as lua_data_run and
 * item_info_mgr do.  Globals the data expects from other files or from the
 * client (jobtbl, AddItem, ...) are stand-ins: indexing one yields another
 * named stand-in, calling one records the call and its arguments.
 *
 * Output (stdout), one value per line, strings hex-encoded:
 *   V <path> <value>     a reachable value (path = space-separated keys)
 *   C <seq> <callee> <argument index> <value>
 * Keys and values: n:<%.17g>  s:<hex>  b:<0|1>  p:<stand-in name hex>
 *                  t (table, its fields follow)  r:<path of first visit>
 *                  f (function)  u (userdata/thread)
 * Table fields are printed in a sorted key order, so two runs over data with
 * the same keys print the same lines in the same order.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <io.h>
#include <fcntl.h>
#include "lua.h"
#include "lauxlib.h"
#include "lualib.h"

static const char prelude[] =
"local names = setmetatable({}, {__mode = 'k'})\n"
"local proxies = {}\n"
"local calls = {}\n"
"local PM = {}\n"
"local function proxy(name)\n"
"  local p = proxies[name]\n"
"  if p then return p end\n"
"  p = setmetatable({}, PM)\n"
"  names[p] = name\n"
"  proxies[name] = p\n"
"  return p\n"
"end\n"
"local function nm(x) if names[x] then return names[x] end return tostring(x) end\n"
"PM.__index = function(t, k) return proxy(names[t] .. '.' .. tostring(k)) end\n"
"PM.__call = function(t, ...)\n"
"  calls[#calls + 1] = {names[t], {...}, select('#', ...)}\n"
"  return proxy(names[t] .. '()')\n"
"end\n"
"for _, op in ipairs({'add', 'sub', 'mul', 'div', 'mod', 'pow', 'concat'}) do\n"
"  PM['__' .. op] = function(a, b) return proxy('(' .. nm(a) .. ' ' .. op .. ' ' .. nm(b) .. ')') end\n"
"end\n"
"PM.__unm = function(a) return proxy('(-' .. nm(a) .. ')') end\n"
"PM.__len = function() return 0 end\n"
"PM.__lt = function() return false end\n"
"PM.__le = function() return false end\n"
"local env = setmetatable({}, {__index = function(t, k)\n"
"  local v = _G[k]\n"
"  if v ~= nil then return v end\n"
"  return proxy(k)\n"
"end})\n"
"local function hex(s) return (string.gsub(s, '.', function(c) return string.format('%02x', string.byte(c)) end)) end\n"
"local function num(x) return string.format('%.17g', x) end\n"
"local function key(k)\n"
"  local t = type(k)\n"
"  if t == 'number' then return 'n:' .. num(k)\n"
"  elseif t == 'string' then return 's:' .. hex(k)\n"
"  elseif t == 'boolean' then return 'b:' .. (k and '1' or '0')\n"
"  elseif names[k] then return 'p:' .. hex(names[k])\n"
"  elseif t == 'table' then return 'tk'\n"
"  else return t end\n"
"end\n"
"local order = {number = 1, string = 2, boolean = 3, table = 4}\n"
"local function less(a, b)\n"
"  local ta, tb = type(a), type(b)\n"
"  if names[a] then ta = 'proxy' end\n"
"  if names[b] then tb = 'proxy' end\n"
"  if ta ~= tb then return (order[ta] or 9) .. ta < (order[tb] or 9) .. tb end\n"
"  if ta == 'number' or ta == 'string' then return a < b end\n"
"  if ta == 'boolean' then return (a and 1 or 0) < (b and 1 or 0) end\n"
"  if ta == 'proxy' then return names[a] < names[b] end\n"
"  return tostring(a) < tostring(b)\n"
"end\n"
"local out = {}\n"
"local function emit(s) out[#out + 1] = s if #out >= 4096 then io.write(table.concat(out, '\\n'), '\\n') out = {} end end\n"
"local seen = {}\n"
"local function value(v, path)\n"
"  local t = type(v)\n"
"  if t == 'number' then return 'n:' .. num(v)\n"
"  elseif t == 'string' then return 's:' .. hex(v)\n"
"  elseif t == 'boolean' then return 'b:' .. (v and '1' or '0')\n"
"  elseif t == 'function' then return 'f'\n"
"  elseif names[v] then return 'p:' .. hex(names[v])\n"
"  elseif t == 'table' then\n"
"    if seen[v] then return 'r:' .. seen[v] end\n"
"    return 't'\n"
"  else return 'u' end\n"
"end\n"
"local function walk(tbl, path)\n"
"  seen[tbl] = path\n"
"  local keys = {}\n"
"  for k in pairs(tbl) do keys[#keys + 1] = k end\n"
"  table.sort(keys, less)\n"
"  for _, k in ipairs(keys) do\n"
"    local v = rawget(tbl, k)\n"
"    local p = path == '' and key(k) or (path .. ' ' .. key(k))\n"
"    local r = value(v, p)\n"
"    emit('V ' .. p .. ' ' .. r)\n"
"    if r == 't' then walk(v, p) end\n"
"  end\n"
"end\n"
"function __luadump_env() return env end\n"
"function __luadump_finish()\n"
"  walk(env, '')\n"
"  for name, p in pairs(proxies) do\n"
"    if next(p) ~= nil then\n"
"      local keys = {}\n"
"      for k in pairs(p) do keys[#keys + 1] = k end\n"
"      if #keys > 0 then walk(p, 'p:' .. hex(name)) end\n"
"    end\n"
"  end\n"
"  for i, c in ipairs(calls) do\n"
"    for a = 1, c[3] do\n"
"      local v = c[2][a]\n"
"      local p = 'C' .. i .. ' ' .. a\n"
"      local r = value(v, p)\n"
"      emit('C ' .. i .. ' ' .. hex(c[1]) .. ' ' .. a .. ' ' .. r)\n"
"      if r == 't' then walk(v, p) end\n"
"    end\n"
"  end\n"
"  if #out > 0 then io.write(table.concat(out, '\\n'), '\\n') end\n"
"end\n";

static int fail(lua_State *L, const char *what)
{
    fprintf(stderr, "luadump: %s: %s\n", what, lua_tostring(L, -1));
    return 1;
}

static char *read_file(const char *path, size_t *size)
{
    FILE *f = fopen(path, "rb");
    char *data;
    long n;
    if (!f) return NULL;
    fseek(f, 0, SEEK_END);
    n = ftell(f);
    fseek(f, 0, SEEK_SET);
    data = malloc(n > 0 ? (size_t)n : 1);
    if (data && n > 0 && fread(data, 1, (size_t)n, f) != (size_t)n) {
        free(data);
        data = NULL;
    }
    fclose(f);
    *size = n > 0 ? (size_t)n : 0;
    return data;
}

static int write_chunk(lua_State *L, const void *p, size_t size, void *ud)
{
    (void)L;
    return fwrite(p, 1, size, (FILE *)ud) != size;
}

static int compile(lua_State *L, const char *source, const char *output)
{
    size_t size;
    char *data = read_file(source, &size);
    FILE *out;
    int bad;
    if (!data) {
        fprintf(stderr, "luadump: cannot read %s\n", source);
        return 1;
    }
    if (luaL_loadbuffer(L, data, size, "=chunk")) {
        free(data);
        return fail(L, source);
    }
    free(data);
    out = fopen(output, "wb");
    if (!out) {
        fprintf(stderr, "luadump: cannot write %s\n", output);
        return 1;
    }
    bad = lua_dump(L, write_chunk, out);
    return fclose(out) || bad;
}

int main(int argc, char **argv)
{
    lua_State *L = luaL_newstate();
    int i, call_main = 0, first = 1;

    if (!L) return 1;
    _setmode(_fileno(stdout), _O_BINARY);
    luaL_openlibs(L);
    if (luaL_loadbuffer(L, prelude, sizeof prelude - 1, "=prelude") || lua_pcall(L, 0, 0, 0))
        return fail(L, "prelude");
    if (argc == 4 && strcmp(argv[1], "--compile") == 0)
        return compile(L, argv[2], argv[3]);
    if (argc > 1 && strcmp(argv[1], "--main") == 0) {
        call_main = 1;
        first = 2;
    }
    if (first >= argc) {
        fputs("usage: luadump [--main] <file> [<file>...]\n", stderr);
        return 2;
    }
    for (i = first; i < argc; i++) {
        size_t size;
        char *data = read_file(argv[i], &size);
        if (!data) {
            fprintf(stderr, "luadump: cannot read %s\n", argv[i]);
            return 1;
        }
        if (luaL_loadbuffer(L, data, size, "buf")) {
            free(data);
            return fail(L, argv[i]);
        }
        free(data);
        lua_getglobal(L, "__luadump_env");
        lua_call(L, 0, 1);
        lua_setfenv(L, -2);
        if (lua_pcall(L, 0, 0, 0))
            return fail(L, argv[i]);
    }
    if (call_main) {
        lua_getglobal(L, "__luadump_env");
        lua_call(L, 0, 1);
        lua_getfield(L, -1, "main");
        lua_remove(L, -2);
        if (lua_type(L, -1) == LUA_TFUNCTION) {
            if (lua_pcall(L, 0, LUA_MULTRET, 0))
                return fail(L, "main");
        } else {
            lua_pop(L, 1);
        }
    }
    lua_getglobal(L, "__luadump_finish");
    if (lua_pcall(L, 0, 0, 0))
        return fail(L, "dump");
    lua_close(L);
    return 0;
}
