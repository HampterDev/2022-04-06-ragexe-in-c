#include "contents/client_lua_calls.h"
#include "session/csession.h"
#include "session/session_items.h"
#include "session/session_jobs.h"
#include "platform/language.h"

struct NameRow {
    short index;
    short copyOf;
    const char *text;
};
#define NAME(i, t)      { (short)(i), 0, (t) }
#define COPY(i, from)   { (short)(i), (short)(from), NULL }

static void apply_rows(struct Vec *v, const struct NameRow *row, size_t n)
{
    for (; n != 0; n--, row++) {
        const char **names = (const char **)v->first;
        names[row->index] = row->text != NULL ? row->text : names[row->copyOf];
    }
}
#define APPLY(v, rows)  apply_rows((v), (rows), sizeof(rows) / sizeof((rows)[0]))

#include "session/session_job_sprite_names.def"

#include "session/session_job_imf_names.def"

static const struct NameRow s_sexNames[] = {
    NAME(0x0, "여"),
    NAME(0x1, "남"),
};

static const struct NameRow s_maleHairNames[] = {
    NAME(0x0, "2"),
    NAME(0x1, "2"),
    NAME(0x2, "1"),
    NAME(0x3, "7"),
    NAME(0x4, "5"),
    NAME(0x5, "4"),
    NAME(0x6, "3"),
    NAME(0x7, "6"),
    NAME(0x8, "8"),
    NAME(0x9, "9"),
    NAME(0xa, "10"),
    NAME(0xb, "12"),
    NAME(0xc, "11"),
    NAME(0xd, "13"),
    NAME(0xe, "14"),
    NAME(0xf, "15"),
    NAME(0x10, "16"),
    NAME(0x11, "17"),
    NAME(0x12, "18"),
    NAME(0x13, "19"),
    NAME(0x14, "20"),
    NAME(0x15, "21"),
    NAME(0x16, "22"),
    NAME(0x17, "23"),
    NAME(0x18, "24"),
    NAME(0x19, "25"),
    NAME(0x1a, "26"),
    NAME(0x1b, "27"),
    NAME(0x1c, "28"),
    NAME(0x1d, "29"),
    NAME(0x1e, "30"),
    NAME(0x1f, "31"),
    NAME(0x20, "32"),
    NAME(0x21, "33"),
    NAME(0x22, "34"),
    NAME(0x23, "35"),
    NAME(0x24, "36"),
    NAME(0x25, "37"),
    NAME(0x26, "38"),
    NAME(0x27, "39"),
    NAME(0x28, "40"),
    NAME(0x29, "41"),
    NAME(0x2a, "42"),
};

static const struct NameRow s_femaleHairNames[] = {
    NAME(0x0, "2"),
    NAME(0x1, "2"),
    NAME(0x2, "4"),
    NAME(0x3, "7"),
    NAME(0x4, "1"),
    NAME(0x5, "5"),
    NAME(0x6, "3"),
    NAME(0x7, "6"),
    NAME(0x8, "12"),
    NAME(0x9, "10"),
    NAME(0xa, "9"),
    NAME(0xb, "11"),
    NAME(0xc, "8"),
    NAME(0xd, "13"),
    NAME(0xe, "14"),
    NAME(0xf, "15"),
    NAME(0x10, "16"),
    NAME(0x11, "17"),
    NAME(0x12, "18"),
    NAME(0x13, "19"),
    NAME(0x14, "20"),
    NAME(0x15, "21"),
    NAME(0x16, "22"),
    NAME(0x17, "23"),
    NAME(0x18, "24"),
    NAME(0x19, "25"),
    NAME(0x1a, "26"),
    NAME(0x1b, "27"),
    NAME(0x1c, "28"),
    NAME(0x1d, "29"),
    NAME(0x1e, "30"),
    NAME(0x1f, "31"),
    NAME(0x20, "32"),
    NAME(0x21, "33"),
    NAME(0x22, "34"),
    NAME(0x23, "35"),
    NAME(0x24, "36"),
    NAME(0x25, "37"),
    NAME(0x26, "38"),
    NAME(0x27, "39"),
    NAME(0x28, "40"),
    NAME(0x29, "41"),
    NAME(0x2a, "42"),
};

#include "session/session_job_sprite_paths.def"

#include "session/session_job_body_names.def"

static const struct NameRow s_shieldNames[] = {
    NAME(0x0, ""),
    NAME(0x1, "_가드"),
    NAME(0x3, "_쉴드"),
    NAME(0x4, "_미러쉴드"),
    NAME(0x2, "_버클러"),
};

#define BEGINNER    "초보자"

void session_init_sprite_name_tables(struct Session *self)
{
    int i;

    names_resize(&self->jobSpriteNames, 0x6d93, BEGINNER);
    APPLY(&self->jobSpriteNames, s_jobSpriteNames);
    names_resize(&self->jobImfNames, 0x6d93, BEGINNER);
    APPLY(&self->jobImfNames, s_jobImfNames);
    names_resize(&self->sexSpriteNames, 2, "여");
    APPLY(&self->sexSpriteNames, s_sexNames);
    names_resize(&self->maleHairNames, 0x2b, "2");
    APPLY(&self->maleHairNames, s_maleHairNames);
    names_resize(&self->femaleHairNames, 0x2b, "2");
    APPLY(&self->femaleHairNames, s_femaleHairNames);

    names_resize(&self->jobSpritePaths, 0x6d93, BEGINNER "\\" BEGINNER);
    APPLY(&self->jobSpritePaths, s_jobSpritePaths);

    names_resize(&self->sexSuffixes, 2, "f");
    ((const char **)self->sexSuffixes.first)[0] = "f";
    ((const char **)self->sexSuffixes.first)[1] = "";

    names_resize(&self->weaponSpriteNames, 0x69, "");
    for (i = 0; i < 0x69; i++) {
        const char *name = NULL;
        if (client_lua_call(self->luaState, "ReqWeaponName", "d>s", i, &name))
            ((const char **)self->weaponSpriteNames.first)[i] = name;
    }

    names_resize(&self->jobBodyNames, 0x6d93, BEGINNER);
    APPLY(&self->jobBodyNames, s_jobBodyNamesFirst);
    APPLY(&self->jobBodyNames, s_jobBodyNamesOther);
    APPLY(&self->jobBodyNames, s_jobBodyNames);

    names_resize(&self->shieldSpriteNames, 5, "");
    APPLY(&self->shieldSpriteNames, s_shieldNames);
}
