#include "web/name_card_snapshot.h"
#include "replay/snapshot.h"
#include <limits.h>
#include <stdint.h>
#include "contents/connection_api.h"
#include <stdlib.h>
#include <string.h>
#include "web/name_cards.h"
#include "std/tree.h"

struct ActorNameCard g_emptyNameCard;
INIT_ONCE g_emptyNameCardGuard;

#define REQ_NAME_PACKET                                                                            \
    0x368
#define REQUEST_TIMEOUT 10000

#define NAME_CARD_SNAPSHOT_MAX 0x1745d16u

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

static void name_card_clear(struct ActorNameCard *card)
{
    string_clear(&card->name);
    string_clear(&card->party);
    string_clear(&card->guild);
    string_clear(&card->position);
    string_clear(&card->achievement_title);
    string_clear(&card->npc_title);
    card->group_id = 0;
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

const char *name_card_name_text(const struct ActorNameCardHolder *self, unsigned aid,
                                size_t *length)
{
    const struct String *name = NULL;
    if (self && self->cards.head) {
        struct NameCardEntry *entry = tree_find(&self->cards, &aid, tree_less_uint);
        if (entry)
            name = &entry->card.name;
    }
    if (length)
        *length = name ? name->size : 0;
    return name ? string_cstr(name) : "";
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

    while (expired != self->asked.last && timeGetTime() >= expired->sent + REQUEST_TIMEOUT)
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

struct NameSnapshotCursor {
    const unsigned char *data;
    size_t size, offset;
};

static int name_snapshot_word(struct NameSnapshotCursor *cursor, uint32_t *value)
{
    const unsigned char *p;
    if (cursor->offset > cursor->size || cursor->size - cursor->offset < 4)
        return 0;
    p = cursor->data + cursor->offset;
    *value = (uint32_t)p[0] | (uint32_t)p[1] << 8 | (uint32_t)p[2] << 16 | (uint32_t)p[3] << 24;
    cursor->offset += 4;
    return 1;
}

static void name_snapshot_put(unsigned char **position, uint32_t value)
{
    unsigned char *p = *position;
    p[0] = (unsigned char)value;
    p[1] = (unsigned char)(value >> 8);
    p[2] = (unsigned char)(value >> 16);
    p[3] = (unsigned char)(value >> 24);
    *position += 4;
}

int name_cards_snapshot_valid(const void *data, size_t size)
{
    struct NameSnapshotCursor cursor = {data, size, 0};
    uint32_t count, i, aid, length;
    int field;
    if (!size)
        return 1;
    if (!data || size > INT_MAX || !name_snapshot_word(&cursor, &count) ||
        count > NAME_CARD_SNAPSHOT_MAX || count > (size - 4) / 20)
        return 0;
    for (i = 0; i < count; ++i) {
        if (!name_snapshot_word(&cursor, &aid))
            return 0;
        for (field = 0; field < 4; ++field) {
            if (!name_snapshot_word(&cursor, &length) || length > cursor.size - cursor.offset)
                return 0;
            cursor.offset += length;
        }
    }
    return cursor.offset == size;
}

int name_cards_snapshot_save(const struct ActorNameCardHolder *holder, struct ReplayBuffer *out)
{
    struct ReplayBuffer result = {0};
    struct TreeNode *node, *head;
    unsigned char *position;
    size_t size = 4, count = 0;
    if (!holder || !out || !(head = holder->cards.head))
        return 0;
    for (node = head->left; node != head; node = tree_next(node)) {
        const struct ActorNameCard *card = &TREE_VALUE(node, struct NameCardEntry)->card;
        const struct String *fields[] = {&card->name, &card->party, &card->guild,
                                            &card->position};
        size_t i;
        if (size > INT_MAX - 20 || ++count > NAME_CARD_SNAPSHOT_MAX)
            return 0;
        size += 20;
        for (i = 0; i < 4; ++i) {
            if (fields[i]->size > INT_MAX - size)
                return 0;
            size += fields[i]->size;
        }
    }
    if (count != holder->cards.size)
        return 0;
    result.data = malloc(size);
    if (!result.data)
        return 0;
    result.size = size;
    position = result.data;
    name_snapshot_put(&position, (uint32_t)count);
    for (node = head->left; node != head; node = tree_next(node)) {
        const struct NameCardEntry *entry = TREE_VALUE(node, struct NameCardEntry);
        const struct ActorNameCard *card = &entry->card;
        const struct String *fields[] = {&card->name, &card->party, &card->guild,
                                            &card->position};
        size_t i;
        name_snapshot_put(&position, entry->aid);
        for (i = 0; i < 4; ++i) {
            name_snapshot_put(&position, fields[i]->size);
            if (fields[i]->size)
                memcpy(position, string_cstr(fields[i]), fields[i]->size);
            position += fields[i]->size;
        }
    }
    replay_buffer_destroy(out);
    *out = result;
    return 1;
}

int name_cards_snapshot_load(struct ActorNameCardHolder *holder, const void *data, size_t size)
{
    struct NameSnapshotCursor cursor = {data, size, 0};
    uint32_t count, i, aid, length;
    if (!holder || !holder->cards.head || !name_cards_snapshot_valid(data, size))
        return 0;
    if (!size)
        return 1;
    tree_clear(&holder->cards, destroy_entry);
    name_snapshot_word(&cursor, &count);
    for (i = 0; i < count; ++i) {
        struct ActorNameCard *card;
        struct String *fields[4];
        size_t field;
        name_snapshot_word(&cursor, &aid);
        card = name_card_at(&holder->cards, aid);
        name_card_clear(card);
        fields[0] = &card->name;
        fields[1] = &card->party;
        fields[2] = &card->guild;
        fields[3] = &card->position;
        for (field = 0; field < 4; ++field) {
            name_snapshot_word(&cursor, &length);
            string_assign(fields[field], (const char *)cursor.data + cursor.offset, length);
            cursor.offset += length;
        }
    }
    return 1;
}

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
    request.sent = timeGetTime();
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
