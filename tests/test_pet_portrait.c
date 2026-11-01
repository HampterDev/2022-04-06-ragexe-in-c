#include "session/session_talk.h"
#include "contents/client_lua_calls.h"

#include <assert.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>

/* Extract the production helper; substitute only its Lua lookup boundary. */
struct Session { struct ClientLua *luaState; };
static struct Session session;
static const char *lua_name;
static int lua_success, lua_calls, cases;
static const char fallback[] = "UserInterface\\illust\\PET_NOIMAGE.bmp";

int client_lua_call(struct ClientLua *lua, const char *function, const char *format, ...)
{
    va_list args;
    const char **result;
    assert(lua == session.luaState);
    assert(!strcmp(function, "GetPetIllustName_V3"));
    assert(!strcmp(format, "db>s"));
    va_start(args, format);
    assert(va_arg(args, int) == 1002);
    assert(va_arg(args, int) == 1);
    result = va_arg(args, const char **);
    *result = lua_name;
    va_end(args);
    ++lua_calls;
    return lua_success;
}

#include "pet_portrait.inc"

static void check_name(const char *name, int success, size_t capacity, const char *expected)
{
    unsigned char memory[512];
    size_t i, written = capacity ? strlen(expected) + 1 : 0;
    int previous_calls = lua_calls;
    assert(capacity <= sizeof memory - 16);
    memset(memory, 0xa5, sizeof memory);
    lua_name = name;
    lua_success = success;
    session_get_pet_illust_name(&session, 1002, (char *)memory + 8, capacity);
    assert(lua_calls == previous_calls + (capacity != 0));
    assert(written <= capacity);
    if (capacity)
        assert(!memcmp(memory + 8, expected, written));
    for (i = 0; i < 8; ++i)
        assert(memory[i] == 0xa5);
    for (i = 8 + written; i < sizeof memory; ++i)
        assert(memory[i] == 0xa5);
    ++cases;
}

int main(void)
{
    char long_name[1024];
    size_t capacities[] = {260, 264};
    size_t i;
    int previous_calls;

    check_name("UserInterface\\illust\\PET_PORING.bmp", 1, 260,
               "UserInterface\\illust\\PET_PORING.bmp");
    check_name("UserInterface\\illust\\100%%_%s_%n.bmp", 1, 260,
               "UserInterface\\illust\\100%%_%s_%n.bmp");
    check_name("UserInterface\\illust\\\xb8\xf3.bmp", 1, 264,
               "UserInterface\\illust\\\xb8\xf3.bmp");
    check_name("", 1, 260, "");
    check_name(NULL, 1, 260, fallback);
    check_name("ignored.bmp", 0, 260, fallback);
    check_name(NULL, 0, sizeof fallback, fallback);
    check_name(NULL, 0, sizeof fallback - 1, "");
    check_name("x", 1, 2, "x");
    check_name("x", 1, 1, "");
    check_name(NULL, 0, 0, "");
    previous_calls = lua_calls;
    session_get_pet_illust_name(&session, 1002, NULL, 0);
    assert(lua_calls == previous_calls);
    ++cases;
    for (i = 0; i < sizeof capacities / sizeof *capacities; ++i) {
        size_t capacity = capacities[i];
        memset(long_name, 'x', sizeof long_name);
        long_name[capacity - 1] = '\0';
        check_name(long_name, 1, capacity, long_name);
        long_name[capacity - 1] = 'x';
        long_name[capacity] = '\0';
        check_name(long_name, 1, capacity, fallback);
        long_name[capacity] = 'x';
        long_name[sizeof long_name - 1] = '\0';
        check_name(long_name, 1, capacity, fallback);
    }
    printf("Pet portraits: %d cases passed (literal bytes, Lua failures, exact fit, overflow fallback and buffer guards).\n", cases);
    return 0;
}
