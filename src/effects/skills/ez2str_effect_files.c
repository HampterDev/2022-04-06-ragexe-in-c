#include "contents/client_lua_calls.h"
#include <stdio.h>
#include "session/state.h"
#include <stdlib.h>
#include <string.h>
#include "effects/effect_data.h"
#include "effects/skills/ez2str_effect.h"

static int rand_below(int n)
{
    int r = rand() % n;
    return r < 0 ? 0 : r;
}

static void load(struct Ez2StrParticle *p, const char *path)
{
    ez2_str_particle_load(p, path);
}

/* A row of the Ez2StrFile table of data/effects/effects.json. */
static void open_str_file(struct Ez2StrParticle *p, const struct Ez2StrFile *f)
{
    char s[256];
    const char *path = f->file;

    if (f->variants) {
        snprintf(s, sizeof s, f->file, rand_below(f->variants) + 1);
        path = s;
    }
    switch (f->open) {
    case EZ2STR_LOAD:
        load(p, f->min_file && g_session1.map_rule_flags[0] == 1 ? f->min_file : path);
        break;
    case EZ2STR_OPEN:
        ez2_str_particle_open(p, path);
        break;
    case EZ2STR_FULL_ONLY:
        if (g_session1.map_rule_flags[0] == 0)
            load(p, path);
        break;
    }
}

void ez2_str_effect_open_for_type(struct Ez2StrEffect *self,
                                           struct Ez2StrParticle *p, int type)
{
    const struct Ez2StrFile *f;

    if (type == 1412)
        ez2_str_particle_load(p, string_cstr(&self->str_name));
    else if ((f = effect_data_ez2str(type)) != NULL)
        open_str_file(p, f);

    if (type >= 2384) {
        const char *name = 0;

        client_lua_call(g_session1.luaState, "GetHatEfResName", "d>s", type - 2384, &name);
        if (name)
            load(p, name);
    }
}
