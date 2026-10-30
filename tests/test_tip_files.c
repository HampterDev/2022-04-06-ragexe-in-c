#include "contents/client_res.h"
#include "contents/tips.h"
#include "core/fatal.h"
#include "std/std.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int g_serviceType = 1;
struct Language *g_language;
struct UIWindowMgr g_windowMgr;
static const char *file_text;
static int file_exists, file_opens = 1;
static unsigned probes, opens, reads, closes, destructs, live;
static struct { unsigned char *p; size_t size; } blocks[256];

void *mem_alloc(size_t size)
{
    unsigned i;
    unsigned char *p = malloc(size + 16);
    assert(p);
    memset(p, 0xa5, size + 16);
    for (i = 0; i < 256 && blocks[i].p; ++i) {}
    assert(i < 256);
    blocks[i].p = p;
    blocks[i].size = size;
    ++live;
    return p;
}

void mem_free(void *memory)
{
    size_t size;
    unsigned i, j;
    if (!memory) return;
    for (i = 0; i < 256 && blocks[i].p != memory; ++i) {}
    assert(i < 256);
    size = blocks[i].size;
    for (j = 0; j < 16; ++j) assert(blocks[i].p[size + j] == 0xa5);
    free(memory);
    blocks[i].p = NULL;
    --live;
}

__declspec(noreturn) void fatal_error(const char *message)
{ (void)message; abort(); }

const char *language_language_file(struct Language *language, const char *name)
{ assert(language == g_language); return name; }
unsigned char file_make_file_name(const char *name, int flags)
{ assert(name && flags == 0); ++probes; return (unsigned char)file_exists; }
struct File *file_construct(struct File *self)
{ memset(self, 0, sizeof *self); return self; }
unsigned char file_open(struct File *self, const char *name, int mode,
                        unsigned char resolve, unsigned char disk_only)
{
    assert(name && mode == 0 && resolve == 1 && disk_only == 0);
    ++opens;
    self->size = (int)strlen(file_text);
    return (unsigned char)file_opens;
}
unsigned char file_read(struct File *self, void *out, int size)
{ assert(size == self->size); memcpy(out, file_text, size); ++reads; return 1; }
void file_close(struct File *self) { (void)self; ++closes; }
void file_destruct(struct File *self) { (void)self; ++destructs; }

static const char *line_at(const struct Vec *pages, unsigned page, unsigned line)
{
    const struct Vec *lines = pages->first;
    assert(page < VEC_COUNT(pages, struct Vec));
    assert(line < VEC_COUNT(&lines[page], struct String));
    return string_cstr((const struct String *)lines[page].first + line);
}

int main(void)
{
    struct Vec copy, *tip;
    unsigned before;
    assert(!g_guildTips.loaded && !g_guildTips.tips.first);
    assert(!g_tipOfTheDay.loaded && !g_tipOfTheDay.tips.first);

    file_exists = 0;
    guild_tips_copy(&g_guildTips, &copy);
    assert(g_guildTips.loaded && !copy.first && probes == 1 && opens == 0);
    guild_tips_copy(&g_guildTips, &copy);
    assert(probes == 1 && !copy.first && !live);

    g_guildTips.loaded = 0;
    file_exists = 1;
    file_opens = 0;
    file_text = "unused";
    assert(!guild_tips_read(&g_guildTips));
    assert(g_guildTips.loaded && opens == 1 && destructs == 1 && !closes && !live);
    before = probes;
    guild_tips_copy(&g_guildTips, &copy);
    assert(probes == before && !copy.first);

    g_guildTips.loaded = 0;
    file_opens = 1;
    file_text = "short\r\nsecond line longer than the inline string capacity#next#third#unfinished";
    guild_tips_copy(&g_guildTips, &copy);
    assert(g_guildTips.loaded && reads == 1 && closes == 1 && destructs == 2);
    assert(VEC_COUNT(&copy, struct Vec) == 3);
    assert(strcmp(line_at(&copy, 0, 0), "short\r") == 0);
    assert(strcmp(line_at(&copy, 0, 1), "second line longer than the inline string capacity") == 0);
    assert(strcmp(line_at(&copy, 1, 0), "next") == 0);
    assert(strcmp(line_at(&copy, 2, 0), "third") == 0);
    assert(copy.first != g_guildTips.tips.first);
    assert(line_at(&copy, 0, 1) != line_at(&g_guildTips.tips, 0, 1));
    tip_list_free(&g_guildTips.tips);
    assert(!g_guildTips.tips.first && !g_guildTips.tips.last && !g_guildTips.tips.end);
    assert(strcmp(line_at(&copy, 0, 1), "second line longer than the inline string capacity") == 0);
    tip_list_free(&copy);
    tip_list_free(&copy);
    assert(!copy.first && !live);

    file_text = "day one#day two#";
    tip = tip_of_the_day_get(&g_tipOfTheDay, 1);
    assert(tip == (struct Vec *)g_tipOfTheDay.tips.first + 1);
    assert(strcmp(line_at(&g_tipOfTheDay.tips, 1, 0), "day two") == 0);
    before = probes;
    assert(tip_of_the_day_get(&g_tipOfTheDay, -1) == g_tipOfTheDay.tips.first);
    assert(tip_of_the_day_get(&g_tipOfTheDay, 2) == g_tipOfTheDay.tips.first);
    assert(probes == before);
    tip_list_free(&g_tipOfTheDay.tips);
    assert(!live && reads == 2 && closes == 2 && destructs == 3);
    puts("tip files: lazy loading, missing files, deep copies and nested cleanup passed");
    return 0;
}
