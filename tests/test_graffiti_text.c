#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <windows.h>
#include "game/packet_ids.h"

#define CHECK(condition)                                                                          \
    do {                                                                                          \
        if (!(condition)) {                                                                       \
            fprintf(stderr, "Graffiti text check failed at line %d: %s\n", __LINE__, #condition); \
            exit(2);                                                                              \
        }                                                                                         \
    } while (0)

#include "../src/effects/skills/graffi_skill.c"
#include "../src/game/skill_units.c"
#include "graffiti_text_unreached.h"

struct Session g_session1;
struct TexCache g_textureCache;

static struct GraffiSkill *made;
static size_t pushed;
static char drawn[GRAFFITI_TEXT_SIZE + 1];
static struct World world;
static struct GameMode mode;

void *mem_alloc(size_t bytes)
{
    void *block = calloc(1, bytes);

    CHECK(block);
    made = block;
    return block;
}

struct Skill *skill_construct(struct Skill *self, int unit_aid)
{
    (void)unit_aid;
    return self;
}

void *list_insert(struct List *list, struct ListNode *where, const void *value, size_t size)
{
    CHECK(list == &world.gameObjects || list == &world.skills);
    CHECK(where == NULL);
    CHECK(size == sizeof made && *(struct GraffiSkill *const *)value == made);
    ++pushed;
    return NULL;
}

struct Texture *texture_cache_get(struct TexCache *cache, const char *name, int keyed, int full_size)
{
    CHECK(cache == &g_textureCache && keyed == 0 && full_size == 0 && name);
    return NULL;
}

struct Texture *texture_create(int width, int height, int format)
{
    CHECK(width == 0x180 && height == 0x180 && format == 0);
    CHECK(strlen(made->text) < sizeof drawn);
    strcpy(drawn, made->text);
    return NULL;
}

int texture_blit_rect(struct Texture *texture, struct Surface *source, int sx, int sy, int sw, int sh,
                      int dx, int dy, int dw, int dh)
{
    (void)texture;
    (void)source;
    (void)sx;
    (void)sy;
    (void)sw;
    (void)sh;
    (void)dx;
    (void)dy;
    (void)dw;
    (void)dh;
    return 0;
}

struct Player *game_current_player(void)
{
    return NULL;
}

static unsigned char *at_page_end(const void *bytes, size_t size)
{
    static unsigned char *pages;
    DWORD old;

    if (!pages) {
        pages = VirtualAlloc(NULL, 0x3000, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
        CHECK(pages);
        CHECK(VirtualProtect(pages + 0x2000, 0x1000, PAGE_NOACCESS, &old));
    }
    CHECK(size <= 0x2000);
    memcpy(pages + 0x2000 - size, bytes, size);
    return pages + 0x2000 - size;
}

static void expect(const char *text, size_t length)
{
    CHECK(made);
    CHECK(pushed == 2);
    CHECK(made->base.unit_aid == 0x1234);
    CHECK(strlen(made->text) == length);
    CHECK(memcmp(made->text, text, length) == 0);
    CHECK(strcmp(drawn, made->text) == 0);
    CHECK(made->cell_x == 7 && made->cell_y == 9);
    made = NULL;
    pushed = 0;
    drawn[0] = 0;
}

static unsigned char *graffiti_packet(unsigned short id, size_t header, const char *text, size_t size)
{
    unsigned char bytes[0x2000];
    size_t length = header + size;

    CHECK(length <= sizeof bytes);
    memset(bytes, 0, header);
    bytes[0] = (unsigned char)id;
    bytes[1] = (unsigned char)(id >> 8);
    bytes[2] = (unsigned char)length;
    bytes[3] = (unsigned char)(length >> 8);
    bytes[4] = 0x34;
    bytes[5] = 0x12;
    bytes[12] = 7;
    bytes[14] = 9;
    bytes[16] = UNT_GRAFFITI;
    memcpy(bytes + header, text, size);
    return at_page_end(bytes, length);
}

static void entry3(const char *text, size_t size)
{
    game_mode_zc_skill_entry3(&mode, (const struct PACKET_ZC_SKILL_ENTRY3 *)graffiti_packet(
        HEADER_ZC_SKILL_ENTRY3, offsetof(struct PACKET_ZC_SKILL_ENTRY3, text), text, size));
}

static void entry4(const char *text, size_t size)
{
    game_mode_zc_skill_entry4(&mode, (const struct PACKET_ZC_SKILL_ENTRY4 *)graffiti_packet(
        HEADER_ZC_SKILL_ENTRY4, offsetof(struct PACKET_ZC_SKILL_ENTRY4, text), text, size));
}

static void entry5(const char *text, size_t size)
{
    game_mode_zc_skill_entry5(&mode, (const struct PACKET_ZC_SKILL_ENTRY5 *)graffiti_packet(
        HEADER_ZC_SKILL_ENTRY5, offsetof(struct PACKET_ZC_SKILL_ENTRY5, text), text, size));
}

int main(void)
{
    static void (*const entries[3])(const char *, size_t) = { entry3, entry4, entry5 };
    char long_text[300];
    struct PACKET_ZC_SKILL_ENTRY2 two;
    struct GraffiSkill *direct;
    int i;

    mode.world = &world;
    for (i = 0; i < (int)sizeof long_text; i++)
        long_text[i] = (char)('A' + i % 26);

    for (i = 0; i < 3; i++) {
        entries[i](long_text, sizeof long_text);
        expect(long_text, GRAFFITI_TEXT_SIZE - 1);
        entries[i](long_text, GRAFFITI_TEXT_SIZE - 1);
        expect(long_text, GRAFFITI_TEXT_SIZE - 1);
        entries[i]("HELLO", 5);
        expect("HELLO", 5);
        entries[i]("HI\0XYZ", 6);
        expect("HI", 2);
        entries[i]("", 0);
        CHECK(made == NULL && pushed == 0 && drawn[0] == 0);
        entries[i]("Z", 1);
        expect("Z", 1);
    }

    memset(&two, 'B', sizeof two);
    two.id = HEADER_ZC_SKILL_ENTRY2;
    two.aid = 0x1234;
    two.x = 7;
    two.y = 9;
    two.has_text = 1;
    game_mode_zc_skill_entry2(&mode, (struct PACKET_ZC_SKILL_ENTRY2 *)at_page_end(&two, sizeof two));
    expect(two.text, sizeof two.text);

    direct = graffi_skill_new();
    render_object_send(direct, NULL, OBJ_MSG_SET_TEXT, (long long)(int)at_page_end(long_text, sizeof long_text), 3, 4, 0);
    CHECK(made == direct);
    CHECK(strlen(direct->text) == GRAFFITI_TEXT_SIZE - 1);
    CHECK(memcmp(direct->text, long_text, GRAFFITI_TEXT_SIZE - 1) == 0);
    CHECK(strcmp(drawn, direct->text) == 0);

    puts("Graffiti text: skill entries 2 to 5 read the graffiti text within the packet, and the graffiti "
         "keeps at most 255 bytes of what it is sent, passed.");
    return 0;
}
