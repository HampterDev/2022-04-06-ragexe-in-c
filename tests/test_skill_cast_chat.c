#include <assert.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "game/gamemode.h"
#include "game/skill_use.h"
#include "game/skill_cast.h"
#include "game/actor_names.h"
#include "session/party_queries.h"
#include "std/string.h"

#undef assert
#define assert(condition)                                                                          \
    do {                                                                                           \
        if (!(condition)) {                                                                        \
            fprintf(stderr, "Skill-cast chat check failed at line %d: %s\n", __LINE__,             \
                    #condition);                                                                   \
            exit(2);                                                                               \
        }                                                                                          \
    } while (0)

struct Session g_session1;
struct UIWindowMgr g_windowMgr;
static struct GameMode mode;
static struct ActorNameCard card;
static const char *actor_name;
static int party_member, printed, print_color, allocations;
static char line[512];

void *mem_alloc(size_t size)
{
    void *p = malloc(size);
    assert(p);
    ++allocations;
    return p;
}

void mem_free(void *p)
{
    if (p) {
        --allocations;
        free(p);
    }
}

__declspec(noreturn) void fatal_error(const char *message)
{
    fprintf(stderr, "%s\n", message);
    abort();
}

struct GameMode *game_current_mode(void)
{
    return &mode;
}

struct String *game_mode_get_actor_name(struct GameMode *self, struct String *out, int aid)
{
    assert(self == &mode && (aid == 10 || aid == 20));
    string_init(out);
    string_assign_cstr(out, actor_name); return out;
}

struct ActorNameCard *game_mode_get_name_card(struct GameMode *self, unsigned aid)
{
    assert(self == &mode && aid == 20);
    return &card;
}

int session_is_party_member_aid(struct Session *session, int aid, unsigned char flag)
{
    assert(session == &g_session1 && aid == 10 && flag == 1);
    return party_member;
}

char *msg_str(int id)
{
    switch (id) {
    case 0x644:
        return "someone";
    case 0x64c:
        return "%s casts ";
    case 0x658:
        return "on %s ";
    case 0x659:
        return "%s";
    default:
        abort();
    }
}

const char *skill_name(int id)
{
    assert(id == 42);
    return "Heal";
}

int crt_snprintf_s(char *out, size_t size, size_t count, const char *format, ...)
{
    int result;
    va_list args;
    assert(count == (size_t)-1);
    va_start(args, format);
    result = vsnprintf(out, size, format, args);
    va_end(args);
    return result;
}

int ui_window_mgr_send_msg_(struct UIWindowMgr *self, int message, int text, int a, int color, int b)
{
    assert(self == &g_windowMgr && message == UIM_CHAT_LINE && a == 0xffff && b == 0);
    assert(strlen((const char *)text) < sizeof line);
    strcpy(line, (const char *)text);
    print_color = color;
    ++printed;
    return 0;
}

static void check_line(const char *name, const char *title, const char *expected, int party)
{
    struct GameActor caster = {0}, target = {0};
    caster.aid = 10;
    target.aid = 20;
    g_session1.ownAid = party ? 30 : 10;
    party_member = party;
    actor_name = name;
    printed = 0;
    string_init(&card.npc_title);
    string_assign_cstr(&card.npc_title, title);
    skill_cast_chat_line(&caster, &target, 42, 0);
    assert(printed == 1 && print_color == (party ? 0x10 : 0xf));
    assert(strcmp(line, expected) == 0);
    assert(strcmp(string_cstr(&card.npc_title), title) == 0);
    skill_cast_chat_line(&caster, &target, 42, 1);
    assert(printed == 1);
    g_session1.ownAid = 30;
    party_member = 0;
    skill_cast_chat_line(&caster, &target, 42, 0);
    assert(printed == 1);
    string_free(&card.npc_title);
    assert(allocations == 0);
}

int main(void)
{
    check_line("", "상인", "on 상인 Heal", 0);
    check_line("", "A title long enough to use heap storage",
               "on A title long enough to use heap storage Heal", 0);
    check_line("", "", "on someone Heal", 0);
    check_line("Alice#hidden", "unused title", "on Alice Heal", 0);
    check_line("", "상인", "someone casts on 상인 Heal", 1);
    puts("Skill-cast chat: NPC title, empty fallback, actor name, party and quiet checks passed.");
    return 0;
}
