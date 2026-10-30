#include "contents/client_lua_calls.h"
#include "session/csession.h"
#include "session/session_options.h"

#include <string.h>

int session_get_option_int(struct Session *self, const char *name, int fallback)
{
    int value = -1;

    client_lua_call(self->luaState, "l_GetOptionValue", "s>d", name, &value);
    return value == -1 ? fallback : value;
}

float session_get_option_float(struct Session *self, const char *name, float fallback)
{
    double value = -1.0;

    client_lua_call(self->luaState, "l_GetOptionValue", "s>l", name, &value);
    if (value == -1.0)
        return fallback;
    return (float)value;
}

struct String *session_get_option_string(struct Session *self, struct String *out,
                                                    const char *name, const char *fallback)
{
    const char *value = "-1";

    client_lua_call(self->luaState, "l_GetOptionValue", "s>s", name, &value);
    string_init(out);
    if (strcmp(value, "-1") == 0)
        string_assign_cstr(out, fallback);
    else
        string_assign_cstr(out, value);
    return out;
}

void session_write_option_int(struct Session *self, struct File *file, const char *name,
                                       int value)
{
    char line[0xc8];

    sprintf(line, "OptionInfoList[\"%s\"] = %d\r\n", name, value);
    file_write(file, line, (int)strlen(line));
}

void session_write_option_float(struct Session *self, struct File *file,
                                         const char *name, float value)
{
    char line[0xc8];

    sprintf(line, "OptionInfoList[\"%s\"] = %f\r\n", name, (double)value);
    file_write(file, line, (int)strlen(line));
}

void session_write_option_string(struct Session *self, struct File *file,
                                          const char *name, const char *value)
{
    char line[0xc8];

    sprintf(line, "OptionInfoList[\"%s\"] = \"%s\"\r\n", name, value);
    file_write(file, line, (int)strlen(line));
}
