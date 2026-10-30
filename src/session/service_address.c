#include "session/service_address.h"
#include "session/state.h"
#include "contents/client_lua.h"
#include "platform/client_config.h"
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

int session_assist_addr(struct Session *self, struct OwnedText *out)
{
    char *global, *key = NULL;
    const char *lookup, *value = NULL;

    int ok = 0;

    if (!self) return 0;
    if (!client_lua_copy_global_string_checked(self->luaState, "AssistAddr", &global))
        return 0;
    if (global && *global) {
        ok = owned_text_assign(out, text_view_cstr(global));
        free(global);
        return ok;
    }
    free(global);
    if (g_serverDomain && *g_serverDomain) {
        lookup = g_serverDomain;
    } else {
        const char *server = g_serverAddress ? g_serverAddress : "";
        const char *port = g_serverPort ? g_serverPort : "";
        size_t a = strlen(server), b = strlen(port);
        if (b > SIZE_MAX - 2 || a > SIZE_MAX - b - 2) return ok;
        key = malloc(a + b + 2);
        if (!key) return ok;
        memcpy(key, server, a);
        key[a] = ':';
        memcpy(key + a + 1, port, b + 1);
        lookup = key;
    }
    if (client_lua_call(self->luaState, "GetTableStringValueForC", "ss>s",
                        "AssistAddrTbl", lookup, &value) && value && *value)
        ok = owned_text_assign(out, text_view_cstr(value));
    free(key);
    return ok;
}

int session_service_url(struct Session *self, struct OwnedText *out, const char *path)
{
    struct OwnedText base = {0};
    int ok = session_assist_addr(self, &base)
        && owned_text_concat(out, owned_text_view(&base), text_view_cstr(path));
    owned_text_clear(&base);
    return ok;
}

int session_service_urls(struct Session *self,
    struct OwnedText *first, const char *first_path,
    struct OwnedText *second, const char *second_path)
{
    struct OwnedText base = {0}, a = {0}, b = {0};
    int ok = session_assist_addr(self, &base)
        && owned_text_concat(&a, owned_text_view(&base), text_view_cstr(first_path))
        && owned_text_concat(&b, owned_text_view(&base), text_view_cstr(second_path));
    owned_text_clear(&base);
    if (!ok) {
        owned_text_clear(&a);
        owned_text_clear(&b);
        return 0;
    }
    owned_text_clear(first);
    owned_text_clear(second);
    *first = a;
    *second = b;
    return 1;
}
