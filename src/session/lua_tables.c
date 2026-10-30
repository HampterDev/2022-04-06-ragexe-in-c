#include "contents/client_lua_calls.h"
#include "session/state.h"
#include "session/csession.h"
#include "session/session_jobs.h"
#include "session/session_lua.h"

#include <string.h>

BYTE session_is_pet_accessory(struct Session *self, int accessory)
{
    BYTE result = 0;

    if (!client_lua_call(self->luaState, "IsPetAccessory", "d>b", accessory, &result))
        return 0;
    return result;
}

BYTE session_is_top_layer_robe(struct Session *self, int robe)
{
    BYTE result = 0;

    if (robe == -1)
        robe = self->robe;
    client_lua_call(g_session1.luaState, "IsTopLayer", "d>b", robe, &result);
    return result;
}

int session_get_inherited_job(struct Session *self, int job)
{
    int inherited = client_lua_table_int(self->luaState, "JobInheritTable", job);
    int listed = client_lua_table_int(self->luaState, "JOB_INHERIT_LIST2", inherited);

    if (listed != -1)
        inherited = listed;
    return inherited;
}

struct String *session_get_title_string(struct Session *self, struct String *out,
                                                   int id)
{
    const char *title = NULL;
    BYTE ok = client_lua_call(g_session1.luaState, "GetTitleString", "d>s", id, &title);

    string_init(out);
    if (ok) {
        const char *text = title ? title : "";
        string_assign_cstr(out, text);
    } else {
        string_clear(out);
    }
    return out;
}

void session_init_skill_tree_view(struct Session *self)
{
    int count = 0;
    int i, job, kind;

    if (!client_lua_call(self->luaState, "GetInheritJob", "d>d", self->job, &count))
        return;
    if (count == -1)
        return;
    player_skill_bundle_clear_trees(&self->skills);
    player_skill_bundle_clear_extra_skills(&self->skills);
    player_skill_bundle_clear_level_changes(&self->skills);
    for (i = count; i > 0; i--) {
        job = session_get_inherited_job(self, i);
        kind = 1;
        if (job == 0 || session_get_inherited_job(self, i + 1) == 0 || job == 0x107a || job == 0x107c)
            kind = 0;
        else if (session_is_third_job(self, job) == 1 || job == 0x108f || job == 0x1090 || job == 0x1091 ||
                 job == 0x1092)
            kind = 2;
        else if (session_is_fourth_job(self, job) == 1)
            kind = 3;
        switch (job) {
        case 0x10ce: case 0x10cf: kind = 3; break;
        case 0x10d0: case 0x10d1: case 0x10d2: case 0x10d3: kind = 2; break;
        case 0x10d4: kind = 1; break;
        }
        client_lua_call(self->luaState, "InitSkillTreeView", "dd", job, kind);
    }
}
