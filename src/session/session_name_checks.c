#include "session/character_name.h"
#include "contents/guild_info.h"
#include "core/markup_tags.h"
#include "session/csession.h"
#include "session/session_names.h"
#include "session/session_party.h"

#include <string.h>
#include "ui/ui_std.h"

static void guild_members_copy(struct List *copy)
{
    struct ListNode *node;

    list_init(copy);
    for (node = g_guildInfo.members.head->next; node != g_guildInfo.members.head; node = node->next) {
        struct GuildMember member;

        guild_member_copy(&member, LIST_VALUE(node, struct GuildMember));
        list_push_back(copy, &member, sizeof member);
    }
}

static void destroy_member(void *value)
{
    guild_member_destruct(value);
}

static void guild_members_free(struct List *copy)
{
    list_free(copy, destroy_member);
}

#define GUILD_MEMBER(node) LIST_VALUE(node, struct GuildMember)

static int same_letter(char a, char b)
{
    if (a == 'I' || a == 'l')
        return b == 'I' || b == 'l';
    return a == b;
}

int session_not_lookalike(struct Session *self, const char *name,
                                     const char *other)
{
    char a[0x18], b[0x18];
    short lenA, lenB, diff, minLen, count, i, j, count2;
    BYTE skipped;

    if (strlen(other) == 0)
        return 1;
    sprintf(a, "%s", name);
    lenA = (short)(WORD)strlen(a);
    sprintf(b, "%s", other);
    lenB = (short)(WORD)strlen(b);
    diff = lenA >= lenB ? (short)(lenA - lenB) : (short)(lenB - lenA);
    minLen = lenA >= lenB ? lenB : lenA;

    if (lenA >= lenB + 2 && lenA <= lenB + 4) {
        count = 0;
        for (i = 0; i < minLen; i++)
            if (same_letter(a[i + 2], b[i]))
                count++;
        if (count == minLen)
            return 0;
    }
    if (lenA >= lenB + 1 && lenA <= lenB + 4) {
        count = 0;
        for (i = 0; i < minLen; i++)
            if (same_letter(a[i + 1], b[i]))
                count++;
        if (count == minLen)
            return 0;
    }
    if (diff > 2)
        return 1;
    count = 0;
    for (i = 0; i < minLen; i++)
        if (same_letter(a[i], b[i]))
            count++;
    if (count >= minLen - 2)
        return 0;

    count2 = 0;
    skipped = 1;
    for (i = 0, j = 0; i < minLen; i++, j++) {
        if (same_letter(a[i], b[j]))
            count2++;
        if (count2 != i + 1 && skipped == 1) {
            skipped = 2;
            j--;
        }
    }
    if (count2 == minLen - 1)
        return 0;
    return 1;
}

int session_looks_like_a_friend(struct Session *self, const char *name)
{
    struct List copy;
    struct ListNode *node;
    int found = 0;

    guild_members_copy(&copy);
    for (node = copy.head->next; node != copy.head; node = node->next)
        if (session_not_lookalike(self, name, string_cstr(&GUILD_MEMBER(node)->name)) == 0) {
            found = 1;
            break;
        }
    guild_members_free(&copy);
    return found;
}

int session_looks_like_a_party_member(struct Session *self, const char *name)
{
    struct ListNode *node;

    for (node = self->friendList.head->next; node != self->friendList.head; node = node->next)
        if (session_not_lookalike(self, name,
                                  string_cstr(&LIST_VALUE(node, struct PartyMember)->name)) == 0)
            return 1;
    return 0;
}

BYTE np_dect(const char *name)
{
    size_t len = strlen(name);

    if (len == 0 || name[0] == ' ' || name[len - 1] == ' ')
        return 1;
    return 0;
}

int session_has_friend_named(struct Session *self, const char *name)
{
    struct ListNode *node;

    for (node = self->friendList.head->next; node != self->friendList.head; node = node->next) {
        const struct PartyMember *m = LIST_VALUE(node, struct PartyMember);
        if (string_equal_cstr(&m->name, name))
            return 1;
    }
    return 0;
}

int session_is_guild_member_by_name(struct Session *self, const char *name)
{
    struct List copy;
    struct ListNode *node;
    int found;

    guild_members_copy(&copy);
    for (node = copy.head->next; node != copy.head; node = node->next) {
        const struct GuildMember *f = GUILD_MEMBER(node);
        if (string_equal_cstr(&f->name, name))
            break;
    }
    found = node != copy.head || strcmp(string_cstr(&g_guildInfo.master_name), name) == 0;
    guild_members_free(&copy);
    return found;
}

int session_is_guild_member_aid(struct Session *self, int id)
{
    struct List copy;
    struct ListNode *node;
    int found = 0;

    guild_members_copy(&copy);
    for (node = copy.head->next; node != copy.head; node = node->next)
        if (GUILD_MEMBER(node)->aid == id) {
            found = 1;
            break;
        }
    guild_members_free(&copy);
    return found;
}

int session_is_guild_member_char_id(struct Session *self, int id)
{
    struct List copy;
    struct ListNode *node;
    int found = 0;

    guild_members_copy(&copy);
    for (node = copy.head->next; node != copy.head; node = node->next)
        if (GUILD_MEMBER(node)->gid == id) {
            found = 1;
            break;
        }
    guild_members_free(&copy);
    return found;
}

BYTE session_named_body_palettes_enabled(struct Session *self)
{
    (void)self;
    return 0;
}

BYTE session_is_account_char_name(struct Session *self, struct String name)
{
    const struct String *s;
    BYTE found = 0;

    for (s = (const struct String *)self->accountCharNames.first;
         s != (const struct String *)self->accountCharNames.last; s++)
        if (string_equal(&name, s)) {
            found = 1;
            break;
        }
    string_free(&name);
    return found;
}

static int is_friend_name(struct Session *self, const char *text)
{
    struct ListNode *node;

    for (node = self->friendList.head->next; node != self->friendList.head; node = node->next) {
        const struct PartyMember *m = LIST_VALUE(node, struct PartyMember);
        if (string_equal_cstr(&m->name, text))
            return 1;
    }
    return 0;
}

BYTE session_is_known_name(struct Session *self, struct String name)
{
    const char *player = session_character_name(self);
    BYTE known;

    known = string_equal_cstr(&name, player) != 0
        || is_friend_name(self, string_cstr(&name))
        || guild_info_member_aid_by_name(&g_guildInfo, string_cstr(&name)) != -1
        || text_view_find(string_view(&name), string_view(&text_tag_MSG_END), 0) != STRING_NPOS;
    string_free(&name);
    return known;
}

BYTE session_is_ascii_name(struct Session *self, const char *name)
{
    int len = (int)strlen(name);
    int i;

    (void)self;
    for (i = 0; i < len; i++)
        if ((signed char)name[i] < 0)
            return 0;
    return 1;
}
