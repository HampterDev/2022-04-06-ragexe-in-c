#include "contents/client_lua_calls.h"
#include "session/state.h"
#include "session/csession.h"
#include "session/session_items.h"
#include "session/session_jobs.h"

#include <string.h>

struct JobNameRow {
    int index;
    int msgId;
    const char *text;
};
#define NAME(i, t)      { (i), 0, (t) }
#define MSG(i, id)      { (i), (id), NULL }
#define ROWS(rows)      (rows), sizeof(rows) / sizeof((rows)[0])

static const char *row_text(const struct JobNameRow *row)
{
    return row->text != NULL ? row->text : msg_str(row->msgId);
}

static void set_rows(struct Vec *v, const struct JobNameRow *row, size_t n)
{
    for (; n != 0; n--, row++) {
        const char **names = (const char **)v->first;
        names[row->index] = row_text(row);
    }
}

static void insert_name(struct Tree *map, unsigned key, const char *text)
{
    struct JobName name;

    name.job = key;
    name.name = text;
    tree_add(map, &name, sizeof name, tree_less_uint, NULL);
}

static void insert_rows(struct Tree *map, const struct JobNameRow *row, size_t n)
{
    for (; n != 0; n--, row++)
        insert_name(map, (unsigned)row->index, row_text(row));
}

static void lua_job_names(struct Session *self, int first, int last)
{
    int i;

    for (i = first; i <= last; i++)
        client_lua_call(g_session1.luaState, "ReqJobName", "d>s", i, &((const char **)self->reqJobNames.first)[i]);
}

static const struct JobNameRow s_jobNames[] = {
    NAME(0x0, "Novice"),
    NAME(0x1, "Swordman"),
    NAME(0x2, "Magician"),
    NAME(0x3, "Archer"),
    NAME(0x4, "Acolyte"),
    NAME(0x5, "Merchant"),
    NAME(0x6, "Thief"),
    NAME(0x7, "Knight"),
    NAME(0x8, "Priest"),
    NAME(0x9, "Wizard"),
    NAME(0xa, "Blacksmith"),
    NAME(0xb, "Hunter"),
    NAME(0xc, "Assassin"),
    NAME(0xd, "Knight"),
    NAME(0xe, "Crusader"),
    NAME(0xf, "Monk"),
    NAME(0x10, "Sage"),
    NAME(0x11, "Rogue"),
    NAME(0x12, "Alchemist"),
    NAME(0x13, "Bard"),
    NAME(0x14, "Dancer"),
    NAME(0x15, "Crusader"),
    NAME(0x17, "Super Novice"),
    NAME(0x18, "Gunslinger"),
    NAME(0x19, "Ninja"),
    NAME(0xfa1, "Novice High"),
    NAME(0xfa2, "Swordman High"),
    NAME(0xfa3, "Magician High"),
    NAME(0xfa4, "Archer High"),
    NAME(0xfa5, "Acolyte High"),
    NAME(0xfa6, "Merchant High"),
    NAME(0xfa7, "Thief High"),
    NAME(0xfb2, "Stalker"),
    NAME(0xfa8, "Lord Knight"),
    NAME(0xfa9, "High Priest"),
    NAME(0xfaa, "High Wizard"),
    NAME(0xfab, "Whitesmith"),
    NAME(0xfac, "Sniper"),
    NAME(0xfad, "Assassin Cross"),
    NAME(0xfae, "Lord Knight"),
    NAME(0xfaf, "Paladin"),
    NAME(0xfb0, "Champion"),
    NAME(0xfb1, "Professor"),
    NAME(0xfb3, "Creator"),
    NAME(0xfb4, "Clown"),
    NAME(0xfb5, "Gypsy"),
    NAME(0xfb6, "Paladin"),
    NAME(0xfb7, "Baby"),
    NAME(0xfb8, "Baby Swordman"),
    NAME(0xfb9, "Baby Magician"),
    NAME(0xfba, "Baby Archer"),
    NAME(0xfbb, "Baby Acolyte"),
    NAME(0xfbc, "Baby Merchant"),
    NAME(0xfbd, "Baby Thief"),
    NAME(0xfbe, "Baby Knight"),
    NAME(0xfbf, "Baby Priest"),
    NAME(0xfc0, "Baby Wizard"),
    NAME(0xfc1, "Baby Blacksmith"),
    NAME(0xfc2, "Baby Hunter"),
    NAME(0xfc3, "Baby Assassin"),
    NAME(0xfc4, "Baby Knight"),
    NAME(0xfc5, "Baby Crusader"),
    NAME(0xfc6, "Baby Monk"),
    NAME(0xfc7, "Baby Sage"),
    NAME(0xfc8, "Baby Rogue"),
    NAME(0xfc9, "Baby Alchemist"),
    NAME(0xfca, "Baby Bard"),
    NAME(0xfcb, "Baby Dancer"),
    NAME(0xfcc, "Baby Crusader"),
    NAME(0xfcd, "Super Baby"),
    NAME(0xfce, "TaeKwon Boy"),
    NAME(0xfcf, "Star Gladiator"),
    NAME(0xfd0, "Star Gladiator"),
    NAME(0xfd1, "Soul Linker"),
    NAME(0xfd2, "Munak"),
    NAME(0xfd3, "Death Knight"),
    NAME(0xfd4, "Dark Collector"),
    NAME(0x107e, "Baby Ninja"),
    NAME(0x1081, "Baby TaeKwon"),
    NAME(0x1082, "Baby Star Gladiator"),
    NAME(0x108e, "Baby Star Gladiator"),
    NAME(0x1083, "Baby Soul Linker"),
    NAME(0x1084, "Baby Gunslinger"),
};

static const struct JobNameRow s_sex0Names[] = {
    NAME(0xfd2, "Munak"),
    NAME(0xfce, "TaeKwon Girl"),
};

static const struct JobNameRow s_sex99Names[] = {
    MSG(0x17, 0x692),
    MSG(0xfce, 0xd3c),
    MSG(0xfcf, 0x696),
    MSG(0xfd0, 0x696),
    MSG(0xfd1, 0x697),
};

static const struct JobNameRow s_jobNamesMsg[] = {
    MSG(0x0, 0x35d),
    MSG(0x1, 0x35e),
    MSG(0x2, 0x35f),
    MSG(0x3, 0x360),
    MSG(0x4, 0x361),
    MSG(0x5, 0x362),
    MSG(0x6, 0x363),
    MSG(0x7, 0x364),
    MSG(0x8, 0x365),
    MSG(0x9, 0x366),
    MSG(0xa, 0x367),
    MSG(0xb, 0x368),
    MSG(0xc, 0x369),
    MSG(0xd, 0x364),
    MSG(0xe, 0x3be),
    MSG(0xf, 0x3bf),
    MSG(0x10, 0x3c0),
    MSG(0x11, 0x3c1),
    MSG(0x12, 0x3c2),
    MSG(0x13, 0x3c3),
    MSG(0x15, 0x3be),
    MSG(0x17, 0x45b),
    MSG(0xfa1, 0x3ca),
    MSG(0xfa2, 0x3cb),
    MSG(0xfa3, 0x3cc),
    MSG(0xfa4, 0x3cd),
    MSG(0xfa5, 0x3ce),
    MSG(0xfa6, 0x3cf),
    MSG(0xfa7, 0x3d0),
    MSG(0xfa8, 0x3d8),
    MSG(0xfa9, 0x3d9),
    MSG(0xfaa, 0x3da),
    MSG(0xfab, 0x3db),
    MSG(0xfac, 0x3dc),
    MSG(0xfad, 0x3dd),
    MSG(0xfae, 0x3d8),
    MSG(0xfaf, 0x3e4),
    MSG(0xfb0, 0x3e5),
    MSG(0xfb1, 0x3e6),
    MSG(0xfb2, 0x3e7),
    MSG(0xfb3, 0x3e8),
    MSG(0xfb4, 0x3e9),
    MSG(0xfb6, 0x3e4),
    MSG(0xfce, 0x45d),
    MSG(0xfcf, 0x45f),
    MSG(0xfd0, 0x45f),
    MSG(0xfd1, 0x461),
};

static const struct JobNameRow s_sex0NamesMsg[] = {
    MSG(0x0, 0x36a),
    MSG(0x1, 0x36b),
    MSG(0x2, 0x36c),
    MSG(0x3, 0x36d),
    MSG(0x4, 0x36e),
    MSG(0x5, 0x36f),
    MSG(0x6, 0x370),
    MSG(0x7, 0x371),
    MSG(0x8, 0x372),
    MSG(0x9, 0x373),
    MSG(0xa, 0x374),
    MSG(0xb, 0x375),
    MSG(0xc, 0x376),
    MSG(0xd, 0x371),
    MSG(0xe, 0x3c4),
    MSG(0xf, 0x3c5),
    MSG(0x10, 0x3c6),
    MSG(0x11, 0x3c7),
    MSG(0x12, 0x3c8),
    MSG(0x14, 0x3c9),
    MSG(0x15, 0x3c4),
    MSG(0x17, 0x45c),
    MSG(0xfa1, 0x3d1),
    MSG(0xfa2, 0x3d2),
    MSG(0xfa3, 0x3d3),
    MSG(0xfa4, 0x3d4),
    MSG(0xfa5, 0x3d5),
    MSG(0xfa6, 0x3d6),
    MSG(0xfa7, 0x3d7),
    MSG(0xfa8, 0x3de),
    MSG(0xfa9, 0x3df),
    MSG(0xfaa, 0x3e0),
    MSG(0xfab, 0x3e1),
    MSG(0xfac, 0x3e2),
    MSG(0xfad, 0x3e3),
    MSG(0xfae, 0x3de),
    MSG(0xfaf, 0x3ea),
    MSG(0xfb0, 0x3eb),
    MSG(0xfb1, 0x3ec),
    MSG(0xfb2, 0x3ed),
    MSG(0xfb3, 0x3ee),
    MSG(0xfb5, 0x3ef),
    MSG(0xfb6, 0x3ea),
    MSG(0xfce, 0x45e),
    MSG(0xfcf, 0x460),
    MSG(0xfd0, 0x460),
    MSG(0xfd1, 0x462),
};

static const struct JobNameRow s_jobNamesTail[] = {
    NAME(0x1771, "LIF"),
    NAME(0x1772, "AMISTR"),
    NAME(0x1773, "FILIR"),
    NAME(0x1774, "VANILMIRTH"),
    NAME(0x1775, "LIF2"),
    NAME(0x1776, "AMISTR2"),
    NAME(0x1777, "FILIR2"),
    NAME(0x1778, "VANILMIRTH2"),
    NAME(0x1779, "LIF_H"),
    NAME(0x177a, "AMISTR_H"),
    NAME(0x177b, "FILIR_H"),
    NAME(0x177c, "VANILMIRTH_H"),
    NAME(0x177d, "LIF_H2"),
    NAME(0x177e, "AMISTR_H2"),
    NAME(0x177f, "FILIR_H2"),
    NAME(0x1780, "VANILMIRTH_H2"),
    NAME(0x1781, "여\\활용병"),
    NAME(0x1782, "여\\활용병"),
    NAME(0x1783, "여\\활용병"),
    NAME(0x1784, "여\\활용병"),
    NAME(0x1785, "여\\활용병"),
    NAME(0x1786, "여\\활용병"),
    NAME(0x1787, "여\\활용병"),
    NAME(0x1788, "여\\활용병"),
    NAME(0x1789, "여\\활용병"),
    NAME(0x178a, "여\\활용병"),
    NAME(0x178b, "남\\창용병"),
    NAME(0x178c, "남\\창용병"),
    NAME(0x178d, "남\\창용병"),
    NAME(0x178e, "남\\창용병"),
    NAME(0x178f, "남\\창용병"),
    NAME(0x1790, "남\\창용병"),
    NAME(0x1791, "남\\창용병"),
    NAME(0x1792, "남\\창용병"),
    NAME(0x1793, "남\\창용병"),
    NAME(0x1794, "남\\창용병"),
    NAME(0x1795, "남\\검용병"),
    NAME(0x1796, "남\\검용병"),
    NAME(0x1797, "남\\검용병"),
    NAME(0x1798, "남\\검용병"),
    NAME(0x1799, "남\\검용병"),
    NAME(0x179a, "남\\검용병"),
    NAME(0x179b, "남\\검용병"),
    NAME(0x179c, "남\\검용병"),
    NAME(0x179d, "남\\검용병"),
    NAME(0x179e, "남\\검용병"),
};

void session_init_job_name_tables(struct Session *self)
{
    names_resize(&self->reqJobNames, 0x7d01, "Poring");
    set_rows(&self->reqJobNames, ROWS(s_jobNames));
    tree_clear(&self->jobNamesSex0, NULL);
    insert_rows(&self->jobNamesSex0, ROWS(s_sex0Names));
    tree_clear(&self->jobNamesSex99, NULL);
    insert_rows(&self->jobNamesSex99, ROWS(s_sex99Names));
    set_rows(&self->reqJobNames, ROWS(s_jobNamesMsg));
    insert_rows(&self->jobNamesSex0, ROWS(s_sex0NamesMsg));

    lua_job_names(self, 0x2d, 0x7d);
    lua_job_names(self, 0x190, 0x3e7);
    lua_job_names(self, 0x3e9, 0xf9e);
    lua_job_names(self, 0x2710, 0x4e1e);
    lua_job_names(self, 0x4e35, 0x7cff);
    set_rows(&self->reqJobNames, ROWS(s_jobNamesTail));

    names_resize(&self->sexLetters, 2, "F");
    ((const char **)self->sexLetters.first)[0] = "F";
    ((const char **)self->sexLetters.first)[1] = "";

    vec_resize(&self->guildSkillUseLevels, 0x14, sizeof(int));
    vec_resize(&self->skills.chosen_levels, 0x157c, sizeof(int));
    vec_resize(&self->merSkillUseLevels, 0x102, sizeof(int));
    vec_resize(&self->homunSkillUseLevels, 0x3d, sizeof(int));
}
