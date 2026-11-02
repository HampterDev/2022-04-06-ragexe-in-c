#include <stdio.h>
#include "session/session_misc.c"

struct Session g_session1;

enum {
    SUPER_NOVICE = 0x17,
    RUNE_KNIGHT = 0xfd6,
    DRAGON_KNIGHT = 0x10ce,
    REBELLION = 0x1077,
    SUMMONER = 0x107a,
    HYPER_NOVICE = 0x10d4
};

BYTE session_is_super_novice_job(struct Session *self, int id)
{
    (void)self;
    return id == SUPER_NOVICE;
}

int session_is_third_job(struct Session *self, int job)
{
    (void)self;
    return job == RUNE_KNIGHT;
}

BYTE session_is_fourth_job(struct Session *self, int job)
{
    (void)self;
    return job == DRAGON_KNIGHT;
}

BYTE session_is_expanded_job(struct Session *self, int id)
{
    (void)self;
    return id == REBELLION;
}

BYTE session_is_doram_job(struct Session *self, int job)
{
    (void)self;
    return job == SUMMONER;
}

BYTE session_is_expanded_super_novice_job(struct Session *self, int id)
{
    (void)self;
    return id == HYPER_NOVICE;
}

void *vec_push(struct Vec *v, const void *elem, size_t size)
{
    (void)v;
    (void)elem;
    (void)size;
    return NULL;
}

void vec_free(struct Vec *v)
{
    (void)v;
}

void string_free(struct String *s)
{
    (void)s;
}

void string_assign_cstr(struct String *s, const char *text)
{
    (void)s;
    (void)text;
}

void string_append_cstr(struct String *s, const char *text)
{
    (void)s;
    (void)text;
}

const char *session_map_display_name(const char *name)
{
    return name;
}

static int failures;

static void expect(int job, const char *what, int want)
{
    int got = session_get_max_job_level(&g_session1, job);

    if (got != want) {
        printf("FAIL %s (0x%x): maximum job level %d, expected %d\n", what, job, got, want);
        failures++;
    }
}

int main(void)
{
    g_session1.jobLevelNovice = 10;
    g_session1.jobLevelSuperNovice = 99;
    g_session1.jobLevelBase = 50;
    g_session1.jobLevel2nd = 70;
    g_session1.jobLevel3rd = 71;
    g_session1.jobLevel4th = 52;
    g_session1.jobLevelUpperJob = 73;
    g_session1.jobLevelDoram = 60;

    expect(0x1, "Swordman", 50);
    expect(0x7, "Knight", 50);
    expect(0xe, "Crusader", 50);
    expect(0x12, "Alchemist", 50);
    expect(0xfa8, "Lord Knight", 70);
    expect(0xfaf, "Paladin", 70);
    expect(0xfb2, "Stalker", 70);
    expect(0x18, "Gunslinger", 70);
    expect(0x19, "Ninja", 70);
    expect(0x1084, "Baby Gunslinger", 70);
    expect(0x107e, "Baby Ninja", 70);
    expect(0x0, "Novice", 10);
    expect(0xfa1, "High Novice", 10);
    expect(SUPER_NOVICE, "Super Novice", 99);
    expect(RUNE_KNIGHT, "Rune Knight", 71);
    expect(DRAGON_KNIGHT, "Dragon Knight", 52);
    expect(REBELLION, "Rebellion", 73);
    expect(SUMMONER, "Summoner", 60);
    expect(HYPER_NOVICE, "Hyper Novice", 73);

    if (failures)
        return 1;
    printf("Maximum job levels: first and second classes at JobLevelBase, transcendent, Gunslinger and Ninja at JobLevel2nd passed\n");
    return 0;
}
