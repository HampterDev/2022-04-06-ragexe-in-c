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

static const struct NameRow s_doramHairMale[] = {
    NAME(0x1, "1"),
    NAME(0x2, "2"),
    NAME(0x3, "3"),
    NAME(0x4, "4"),
    NAME(0x5, "5"),
    NAME(0x6, "6"),
    NAME(0x7, "7"),
    NAME(0x8, "8"),
    NAME(0x9, "9"),
    NAME(0xa, "10"),
};

static const struct NameRow s_doramHairFemale[] = {
    NAME(0x1, "1"),
    NAME(0x2, "2"),
    NAME(0x3, "3"),
    NAME(0x4, "4"),
    NAME(0x5, "5"),
    NAME(0x6, "6"),
    NAME(0x7, "7"),
    NAME(0x8, "8"),
    NAME(0x9, "9"),
    NAME(0xa, "10"),
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

static const struct NameRow s_jobIdNames[] = {
    NAME(0x0, "NOVICE"),
    NAME(0x17, "SUPERNOVICE"),
    NAME(0x101c, "PORING_NOVICE"),
    NAME(0x1020, "PORING_SNOVICE"),
    NAME(0x18, "GUNSLINGER"),
    NAME(0x1077, "REBELLION"),
    NAME(0x19, "NINJA"),
    NAME(0x1073, "KAGEROU"),
    NAME(0x1074, "OBORO"),
    NAME(0xfce, "TAEKWON"),
    NAME(0xfcf, "STAR"),
    NAME(0x108f, "STAR_EMPEROR"),
    NAME(0xfd1, "LINKER"),
    NAME(0x1090, "SOUL_REAPER"),
    NAME(0xfd0, "STAR2"),
    NAME(0x1093, "STAR2_EMPEROR"),
    NAME(0x1013, "PECO_GUNNER"),
    NAME(0x1078, "PECO_REBELLION"),
    NAME(0x1012, "FROG_NINJA"),
    NAME(0x1075, "FROG_KAGEROU"),
    NAME(0x1076, "FROG_OBORO"),
    NAME(0x103b, "PORING_TAEKWON"),
    NAME(0x101b, "PORING_STAR"),
    NAME(0x1095, "HAETAE_STAR_EMPEROR"),
    NAME(0x1015, "FROG_LINKER"),
    NAME(0x1096, "HAETAE_SOUL_REAPER"),
    NAME(0x1, "SWORDMAN"),
    NAME(0x7, "KNIGHT"),
    NAME(0xfa8, "KNIGHT_H"),
    NAME(0xfd6, "RUNE_KNIGHT"),
    NAME(0xe, "CRUSADER"),
    NAME(0xfaf, "CRUSADER_H"),
    NAME(0xfe2, "ROYAL_GUARD"),
    NAME(0xd, "CHICKEN"),
    NAME(0xfae, "CHICKEN_H"),
    NAME(0xff0, "RUNE_CHICKEN"),
    NAME(0x15, "CHICKEN2"),
    NAME(0xfb6, "CHICKEN2_H"),
    NAME(0xff2, "ROYAL_CHICKEN"),
    NAME(0x1014, "PECO_SWORD"),
    NAME(0x1067, "LION_KNIGHT"),
    NAME(0x1068, "LION_KNIGHT_H"),
    NAME(0x106a, "LION_RUNE_KNIGHT"),
    NAME(0x106b, "LION_CRUSADER"),
    NAME(0x106c, "LION_CRUSADER_H"),
    NAME(0x1069, "LION_ROYAL_GUARD"),
    NAME(0xff8, "RUNE_CHICKEN2"),
    NAME(0xffa, "RUNE_CHICKEN3"),
    NAME(0xffc, "RUNE_CHICKEN4"),
    NAME(0xffe, "RUNE_CHICKEN5"),
    NAME(0x2, "MAGICIAN"),
    NAME(0x9, "WIZARD"),
    NAME(0xfaa, "WIZARD_H"),
    NAME(0xfd7, "WARLOCK"),
    NAME(0x10, "SAGE"),
    NAME(0xfb1, "SAGE_H"),
    NAME(0xfe3, "SORCERER"),
    NAME(0x1022, "FOX_MAGICIAN"),
    NAME(0x1026, "FOX_WIZ"),
    NAME(0x1028, "FOX_HWIZ"),
    NAME(0x1025, "FOX_WARLOCK"),
    NAME(0x1023, "FOX_SAGE"),
    NAME(0x1027, "FOX_PROF"),
    NAME(0x1024, "FOX_SORCERER"),
    NAME(0x4, "ACOLYTE"),
    NAME(0x8, "PRIEST"),
    NAME(0xfa9, "PRIEST_H"),
    NAME(0xfd9, "ARCHBISHOP"),
    NAME(0xf, "MONK"),
    NAME(0xfb0, "MONK_H"),
    NAME(0xfe6, "SURA"),
    NAME(0x101e, "SHEEP_ACO"),
    NAME(0x103c, "SHEEP_PRIEST"),
    NAME(0x103d, "SHEEP_HPRIEST"),
    NAME(0x1021, "SHEEP_ARCB"),
    NAME(0x101d, "SHEEP_MONK"),
    NAME(0x102b, "SHEEP_CHAMP"),
    NAME(0x101f, "SHEEP_SURA"),
    NAME(0x5, "MERCHANT"),
    NAME(0xa, "BLACKSMITH"),
    NAME(0xfab, "BLACKSMITH_H"),
    NAME(0xfda, "MECHANIC"),
    NAME(0x12, "ALCHEMIST"),
    NAME(0xfb3, "ALCHEMIST_H"),
    NAME(0xfe7, "GENETIC"),
    NAME(0xff6, "MADOGEAR"),
    NAME(0x1017, "PIG_MERCHANT"),
    NAME(0x102a, "PIG_BLACKSMITH"),
    NAME(0x1016, "PIG_WHITESMITH"),
    NAME(0x1065, "PIG_MECHANIC"),
    NAME(0x1029, "PIG_ALCHE"),
    NAME(0x1019, "PIG_CREATOR"),
    NAME(0x1018, "PIG_GENETIC"),
    NAME(0x6, "THIEF"),
    NAME(0xc, "ASSASSIN"),
    NAME(0xfad, "ASSASSIN_H"),
    NAME(0xfdb, "GUILLOTINE_CROSS"),
    NAME(0x11, "ROGUE"),
    NAME(0xfb2, "ROGUE_H"),
    NAME(0xfe8, "SHADOW_CHASER"),
    NAME(0x102d, "DOG_THIEF"),
    NAME(0x1031, "DOG_ASSASSIN"),
    NAME(0x1032, "DOG_ASSA_X"),
    NAME(0x102c, "DOG_G_CROSS"),
    NAME(0x102e, "DOG_ROGUE"),
    NAME(0x1030, "DOG_STALKER"),
    NAME(0x102f, "DOG_CHASER"),
    NAME(0x3, "ARCHER"),
    NAME(0xb, "HUNTER"),
    NAME(0xfac, "HUNTER_H"),
    NAME(0xfd8, "RANGER"),
    NAME(0x13, "BARD"),
    NAME(0xfb4, "BARD_H"),
    NAME(0xfe4, "MINSTREL"),
    NAME(0x14, "DANCER"),
    NAME(0xfb5, "DANCER_H"),
    NAME(0xfe5, "WANDERER"),
    NAME(0xff4, "WOLF_RANGER"),
    NAME(0x101a, "OSTRICH_ARCHER"),
    NAME(0x103a, "OSTRICH_HUNTER"),
    NAME(0x1036, "OSTRICH_SNIPER"),
    NAME(0x1066, "OSTRICH_RANGER"),
    NAME(0x1035, "OSTRICH_BARD"),
    NAME(0x1039, "OSTRICH_CROWN"),
    NAME(0x1034, "OSTRICH_MINSTREL"),
    NAME(0x1033, "OSTRICH_DANCER"),
    NAME(0x1038, "OSTRICH_ZIPSI"),
    NAME(0x1037, "OSTRICH_WANDER"),
    NAME(0x107a, "DO_SUMMONER"),
    NAME(0x107b, "DO_CART_SUMMONER"),
    NAME(0x16, "MARRIED"),
    NAME(0x1a, "SANTA"),
    NAME(0x1b, "SUMMER"),
    NAME(0x1c, "HANBOK"),
    NAME(0x1d, "OKTOBERFEST"),
    NAME(0x1e, "SUMMER2"),
    NAME(0xfcc, "CHICKEN2_B"),
};

#define BEGINNER    "초보자"

static void init_job_id_names(struct Session *self)
{
    names_resize(&self->jobIdNames, 0x10eb, "NOVICE");
    APPLY(&self->jobIdNames, s_jobIdNames);
}

void ui_window_mgr_render_menu(struct Session *self)
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
    names_resize(&self->doramHairNamesMale, 0xb, "1");
    APPLY(&self->doramHairNamesMale, s_doramHairMale);
    names_resize(&self->doramHairNamesFemale, 0xb, "1");
    APPLY(&self->doramHairNamesFemale, s_doramHairFemale);

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
    if (g_serviceType == 0)
        APPLY(&self->jobBodyNames, s_jobBodyNamesKorea);
    else
        APPLY(&self->jobBodyNames, s_jobBodyNamesOther);
    APPLY(&self->jobBodyNames, s_jobBodyNames);

    names_resize(&self->shieldSpriteNames, 5, "");
    APPLY(&self->shieldSpriteNames, s_shieldNames);

    init_job_id_names(self);
}
