#include "session/hotkey_bindings.h"
#include "contents/client_lua_calls.h"
#include "session/state.h"
#include "session/csession.h"
#include "session/session_names.h"
#include "session/session_options.h"

#include <string.h>
#include "ui/ui_std.h"

static int hotkey_hidden(int id)
{
    return id == 0x1e || id == 0x21 || id == 0x22 || id == 0x24 || id == 0x35 || id == 0x38 || id == 0x3a
        || id == 0x2b || id == 0x2d;
}

int session_get_hot_key_list_size(struct Session *self, int list)
{
    int size = 0;

    if (!client_lua_call(g_session1.luaState, "GetOriginalHotKeyListSize", "d>d", list + 1, &size))
        return size;
    return size - session_count_hidden_hotkeys_below(self, list, size);
}

int session_get_original_hot_key_index(struct Session *self, int list, int index)
{
    int id = session_count_hidden_hotkeys_below(self, list, index) + index;
    int size;

    if (list == 1 && hotkey_hidden(id)) {
        do
            id++;
        while (hotkey_hidden(id));
    }
    size = 0;
    client_lua_call(g_session1.luaState, "GetOriginalHotKeyListSize", "d>d", list + 1, &size);
    return id >= size ? -1 : id;
}

int session_hot_key_page_slot(struct Session *self, int slot, int bar)
{
    int page, at;

    if (bar == 0)
        page = self->hotkeyPage0;
    else if (bar == 1)
        page = self->hotkeyPage1;
    else
        return slot;
    if (!page)
        return slot;
    at = slot + page * 9;
    return at >= 0x24 ? at - 0x24 : at;
}

void session_reset_the_hot_keys(struct Session *self)
{
    client_lua_call(g_session1.luaState, "ResetTheHotKey", "dddd", 1, 0x84, 0xc0, 0x12);
    client_lua_call(g_session1.luaState, "ResetTheHotKey", "dddd", 1, 0x87, 0x24, 0x12);
}

int session_get_shorten_skill_id(struct Session *self, int index)
{
    return self->shortenSkillLevels[index];
}

void session_set_shorten_skill_id(struct Session *self, int level, int index)
{
    self->shortenSkillLevels[index] = level;
}

void session_set_shorten_skill_name(struct Session *self, const char *name,
                                              int level, int slot)
{
    int i;

    for (i = self->shortenSkillPage * 9; i < (self->shortenSkillPage + 1) * 9; i++) {
        struct String *s = &self->shortenSkillNames[i];

        if (string_equal_cstr(s, name) &&
            self->shortenSkillLevels[i] == level) {
            string_assign_cstr(s, "nothing");
            self->shortenSkillLevels[i] = 0;
        }
    }
    if (strlen(name) == 0) {
        string_assign_cstr(&self->shortenSkillNames[slot], "nothing");
        self->shortenSkillLevels[slot] = 0;
    } else {
        string_assign_cstr(&self->shortenSkillNames[slot], name);
        self->shortenSkillLevels[slot] = level;
    }
}
