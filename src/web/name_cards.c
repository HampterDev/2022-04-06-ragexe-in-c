#include <limits.h>
#include <stdint.h>
#include "contents/connection_api.h"
#include <stdlib.h>
#include <string.h>
#include "web/name_cards.h"
#include "std/tree.h"
#include "std/vec.h"
#include "platform/clock.h"

struct ActorNameCard g_emptyNameCard;
struct Once g_emptyNameCardGuard;

#define REQ_NAME_PACKET                                                                            \
    0x368
#define REQUEST_TIMEOUT 10000

#pragma pack(push, 1)
struct ReqNameByAid {
    unsigned short packet_id;
    unsigned aid;
};
#pragma pack(pop)
SIZE_ASSERT(ReqNameByAid, 6);

void name_card_init(struct ActorNameCard *card)
{
    string_init(&card->name);
    string_init(&card->party);
    string_init(&card->guild);
    string_init(&card->position);
    string_init(&card->achievement_title);
    string_init(&card->npc_title);
    card->group_id = 0;
    card->complete = 0;
}

void name_card_destroy(struct ActorNameCard *card)
{
    string_free(&card->npc_title);
    string_free(&card->achievement_title);
    string_free(&card->position);
    string_free(&card->guild);
    string_free(&card->party);
    string_free(&card->name);
}

void name_card_copy_init(struct ActorNameCard *self, const struct ActorNameCard *from)
{
    string_copy(&self->name, &from->name);
    string_copy(&self->party, &from->party);
    string_copy(&self->guild, &from->guild);
    string_copy(&self->position, &from->position);
    string_copy(&self->achievement_title, &from->achievement_title);
    string_copy(&self->npc_title, &from->npc_title);
    self->group_id = from->group_id;
    self->complete = from->complete;
}

static void destroy_entry(void *value)
{
    name_card_destroy(&((struct NameCardEntry *)value)->card);
}

static struct ActorNameCard *name_card_at(struct Tree *map, unsigned aid)
{
    struct NameCardEntry *entry = tree_get(map, &aid, sizeof aid, sizeof *entry, tree_less_uint);

    return &entry->card;
}

static void want(struct ActorNameCardHolder *self, unsigned aid)
{
    unsigned *p;

    for (p = self->wanted.first; p != self->wanted.last; p++)
        if (*p == aid)
            return;
    vec_push(&self->wanted, &aid, sizeof aid);
}

static void expire_requests(struct ActorNameCardHolder *self)
{
    struct NameCardRequest *first = self->asked.first, *expired = first;

    while (expired != self->asked.last && platform_ticks_ms() >= expired->sent + REQUEST_TIMEOUT)
        expired++;
    vec_erase(&self->asked, first, expired);
}

static int asked_contains(const struct ActorNameCardHolder *self, unsigned aid)
{
    const struct NameCardRequest *r;

    for (r = self->asked.first; r != self->asked.last; r++)
        if (r->aid == aid)
            return 1;
    return 0;
}

static unsigned take_wanted(struct ActorNameCardHolder *self, char *found)
{
    *found = 0;
    while (self->wanted.first != self->wanted.last) {
        unsigned aid = *(unsigned *)self->wanted.first;
        struct NameCardEntry *entry = tree_find(&self->cards, &aid, tree_less_uint);

        vec_erase(&self->wanted, self->wanted.first, (unsigned *)self->wanted.first + 1);
        if ((!entry || entry->card.complete != 1) && !asked_contains(self, aid)) {
            *found = 1;
            return aid;
        }
    }
    return 0;
}

void name_card_holder_init(struct ActorNameCardHolder *self)
{
    tree_init(&self->cards);
    vec_init(&self->wanted);
    vec_init(&self->asked);
}

void name_card_holder_destroy(struct ActorNameCardHolder *self)
{
    vec_free(&self->asked);
    vec_free(&self->wanted);
    tree_free(&self->cards, destroy_entry);
}

void name_card_holder_erase(struct ActorNameCardHolder *self, unsigned aid)
{
    tree_erase_key(&self->cards, &aid, tree_less_uint, destroy_entry);
}

unsigned name_card_holder_find_id(struct ActorNameCardHolder *self, struct String name)
{
    struct String wanted_name;
    struct TreeNode *node;
    struct ActorNameCard copy;
    unsigned aid;
    char equal;

    string_copy(&wanted_name, &name);
    for (node = tree_first(&self->cards); node != self->cards.head; node = tree_next(node)) {
        name_card_copy_init(&copy, &TREE_VALUE(node, struct NameCardEntry)->card);
        equal =
            copy.name.size == wanted_name.size &&
            memcmp(string_cstr(&copy.name), string_cstr(&wanted_name), copy.name.size) == 0;
        name_card_destroy(&copy);
        if (equal)
            break;
    }
    string_free(&wanted_name);
    aid = node != self->cards.head ? TREE_VALUE(node, struct NameCardEntry)->aid : 0;
    string_free(&name);
    return aid;
}

void name_card_empty_atexit(void);

struct ActorNameCard *name_card_holder_get(struct ActorNameCardHolder *self, unsigned aid)
{
    struct NameCardEntry *entry = tree_find(&self->cards, &aid, tree_less_uint);

    if (entry && entry->card.complete == 1)
        return &entry->card;
    want(self, aid);
    if (once_begin(&g_emptyNameCardGuard)) {
        name_card_init(&g_emptyNameCard);
        atexit(name_card_empty_atexit);
        once_complete(&g_emptyNameCardGuard);
    }
    return &g_emptyNameCard;
}

void name_card_empty_atexit(void)
{
    name_card_destroy(&g_emptyNameCard);
}

struct String *name_card_holder_name(struct ActorNameCardHolder *self, struct String *out,
                                        unsigned aid)
{
    string_copy(out, &name_card_at(&self->cards, aid)->name);
    return out;
}

void name_card_holder_clear(struct ActorNameCardHolder *self)
{
    tree_clear(&self->cards, destroy_entry);
    vec_free(&self->wanted);
    vec_free(&self->asked);
}

char name_card_holder_has(struct ActorNameCardHolder *self, unsigned aid)
{
    return tree_find(&self->cards, &aid, tree_less_uint) != NULL;
}

void name_card_holder_update(struct ActorNameCardHolder *self)
{
    struct NameCardRequest request;
    struct ReqNameByAid packet;
    unsigned aid;
    char found;

    expire_requests(self);
    aid = take_wanted(self, &found);
    if (!found || aid == 0)
        return;

    packet.packet_id = REQ_NAME_PACKET;
    packet.aid = aid;
    rag_connection_send_packet(rag_connection_instance(), sizeof packet, &packet);

    request.aid = aid;
    request.sent = platform_ticks_ms();
    vec_push(&self->asked, &request, sizeof request);
}

void name_card_holder_set_all(struct ActorNameCardHolder *self, unsigned aid, struct String name,
                              struct String party, struct String guild,
                              struct String position, struct String title,
                              struct String npc_title, int group_id)
{
    struct ActorNameCard *card;

    card = name_card_at(&self->cards, aid);
    string_assign(&card->name, string_cstr(&name), name.size);
    string_assign(&card->party, string_cstr(&party), party.size);
    string_assign(&card->guild, string_cstr(&guild), guild.size);
    string_assign(&card->position, string_cstr(&position), position.size);
    string_assign(&card->achievement_title, string_cstr(&title), title.size);
    {
        struct String copy;

        string_copy(&copy, &npc_title);
        string_assign(&card->npc_title, string_cstr(&copy), copy.size);
        card->group_id = group_id;
        string_free(&copy);
    }
    card->complete = 1;
    string_free(&name);
    string_free(&party);
    string_free(&guild);
    string_free(&position);
    string_free(&title);
    string_free(&npc_title);
}

static void set_one(struct ActorNameCardHolder *self, unsigned aid, size_t offset,
                    struct String *value)
{
    struct String copy;
    struct ActorNameCard *card;
    struct String *field;

    string_copy(&copy, value);
    card = name_card_at(&self->cards, aid);
    field = (struct String *)((char *)card + offset);
    string_assign(field, string_cstr(&copy), copy.size);
    string_free(&copy);
    string_free(value);
}

void name_card_holder_set_name(struct ActorNameCardHolder *self, unsigned aid,
                               struct String name)
{
    set_one(self, aid, offsetof(struct ActorNameCard, name), &name);
}

void name_card_holder_set_position(struct ActorNameCardHolder *self, unsigned aid,
                                   struct String position)
{
    set_one(self, aid, offsetof(struct ActorNameCard, position), &position);
}

void name_card_holder_set_title(struct ActorNameCardHolder *self, unsigned aid,
                                struct String title)
{
    set_one(self, aid, offsetof(struct ActorNameCard, achievement_title), &title);
}
