#include <assert.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <windows.h>
#include <mmsystem.h>
#include "web/name_cards.h"

#ifdef NAME_CARD_OWNERS_REFERENCE
#define name_card_init name_card_construct
#define name_card_destroy ActorNameCard_Destruct
#define name_card_copy_init ActorNameCard_Copy
#define name_card_holder_erase ActorNameCardHolder_Erase
#define name_card_holder_find_id ActorNameCardHolder_FindIdByName
#define name_card_holder_clear ActorNameCardHolder_Clear
#define name_card_holder_set_all ActorNames_SetAll
#define name_card_holder_set_name ActorNameCardHolder_SetName
#define name_card_holder_set_position ActorTitles_Set
#define name_card_holder_set_title ActorNameCardHolder_SetTitle
#endif

static struct Block {
    unsigned char *base;
    size_t size;
    unsigned id;
} blocks[256];
static unsigned trace[100000], trace_count, next_block_id;
static void note(unsigned value)
{
    assert(trace_count < sizeof trace / sizeof trace[0]);
    trace[trace_count++] = value;
}
static int allocations, attempts, fail_at;
static int block_index(void *p)
{
    int i;
    for (i = 0; i < 256; ++i)
        if (blocks[i].base && blocks[i].base + 8 == p)
            return i;
    assert(0);
    return -1;
}
static void check_guards(void)
{
    int i, j;
    for (i = 0; i < 256; ++i)
        if (blocks[i].base)
            for (j = 0; j < 8; ++j) {
                assert(blocks[i].base[j] == 0xa7);
                assert(blocks[i].base[8 + blocks[i].size + j] == 0xa7);
            }
}
static void *allocate(size_t size)
{
    int i;
    if (++attempts == fail_at) {
        note(3);
        return NULL;
    }
    for (i = 0; i < 256 && blocks[i].base; ++i) {
    }
    assert(i < 256 && size <= SIZE_MAX - 16);
    blocks[i].base = malloc(size + 16);
    assert(blocks[i].base);
    blocks[i].size = size;
    blocks[i].id = ++next_block_id;
    note(1);
    note(blocks[i].id);
    memset(blocks[i].base, 0xa7, size + 16);
    ++allocations;
    return blocks[i].base + 8;
}
static void release(void *p)
{
    int i;
    if (!p)
        return;
    check_guards();
    i = block_index(p);
    note(2);
    note(blocks[i].id);
    free(blocks[i].base);
    blocks[i].base = NULL;
    --allocations;
}
static void *resize(void *p, size_t size)
{
    void *result = allocate(size);
    if (result && p) {
        size_t old_size = blocks[block_index(p)].size;
        memcpy(result, p, size < old_size ? size : old_size);
        release(p);
    }
    return result;
}

static unsigned test_time = 100;
#define malloc allocate
#define realloc resize
#define free release
#define timeGetTime() test_time
#include <web/name_cards.c>

#define ASKED(h) vec_count(&(h)->asked, sizeof(struct NameCardRequest))
#define WANTED(h) vec_count(&(h)->wanted, sizeof(unsigned))
#undef timeGetTime
#undef malloc
#undef realloc
#undef free

void *mem_alloc(size_t size)
{
    void *p = allocate(size);
    assert(p);
    return p;
}
void mem_free(void *p)
{
    release(p);
}
void fatal_error(const char *s)
{
    (void)s;
    abort();
}
static unsigned requested;
static int requests;
struct RagConnection *rag_connection_instance(void)
{
    return (struct RagConnection *)1;
}
char rag_connection_send_packet(struct RagConnection *connection, int size, const void *data)
{
    const unsigned char *p = data;
    assert(connection && size == 6 && p[0] == 0x68 && p[1] == 3);
    memcpy(&requested, p + 2, 4);
    ++requests;
    note(4);
    note(requested);
    note(test_time);
    return 1;
}
static struct String owned(const char *text)
{
    struct String s;
    string_init(&s);
    string_assign_cstr(&s, text);
    return s;
}
static void seed(struct ActorNameCardHolder *holder)
{
    name_card_holder_set_all(holder, 0x80000002u, owned("beta_actor_with_long_name"), owned(""),
                             owned("Beta guild"), owned("Captain"), owned("Beta achievement title"),
                             owned("Beta NPC title"), 22);
    name_card_holder_set_all(holder, 11, owned("alpha_actor"), owned("Alpha party"),
                             owned("Alpha guild"), owned("Officer"),
                             owned("Alpha achievement title"), owned("Alpha NPC title"), 11);
}
static void values(struct ActorNameCardHolder *holder)
{
    struct {
        struct ActorNameCard card;
        unsigned guard;
    } copy;
    struct ActorNameCard *original = name_card_holder_get(holder, 11);
    assert(original->complete && original->group_id == 11);
    assert(!strcmp(string_cstr(&original->achievement_title), "Alpha achievement title"));
    assert(!strcmp(string_cstr(&original->npc_title), "Alpha NPC title"));
    memset(&copy, 0xa5, sizeof copy);
    name_card_copy_init(&copy.card, original);
    assert(copy.guard == 0xa5a5a5a5u && copy.card.complete && copy.card.group_id == 11);
    assert(copy.card.achievement_title.data != original->achievement_title.data);
    name_card_holder_set_title(holder, 11, owned("replacement achievement"));
    assert(!strcmp(string_cstr(&copy.card.achievement_title), "Alpha achievement title"));
    assert(!strcmp(string_cstr(&original->npc_title), "Alpha NPC title"));
    name_card_holder_set_position(holder, 11, owned("new position"));
    assert(!strcmp(string_cstr(&original->position), "new position"));
    assert(!strcmp(string_cstr(&copy.card.position), "Officer"));
    name_card_destroy(&copy.card);
    name_card_destroy(&copy.card);
    assert(copy.guard == 0xa5a5a5a5u);
    assert(name_card_holder_find_id(holder, owned("alpha_actor")) == 11);
    assert(!name_card_holder_find_id(holder, owned("missing")));
}

static void trace_card(const struct ActorNameCard *card)
{
    const struct String *fields[] = {
        &card->name,     &card->party, &card->guild, &card->position, &card->achievement_title,
        &card->npc_title};
    unsigned i, j;
    note(6);
    note(card->group_id);
    note(card->complete);
    for (i = 0; i < sizeof fields / sizeof fields[0]; ++i) {
        note(fields[i]->size);
        for (j = 0; j < fields[i]->size; ++j)
            note((unsigned char)string_cstr(fields[i])[j]);
    }
}

static void request_lifetimes(void)
{
    unsigned round, i;
    for (round = 0; round < 16; ++round) {
        struct {
            unsigned before;
            struct ActorNameCardHolder holder;
            unsigned after;
        } state;
        struct ActorNameCardHolder *holder = &state.holder;
        unsigned begin = 1000 + round * 20000;
        memset(&state, 0xa5, sizeof state);
        requests = 0;
        test_time = begin;
        name_card_holder_init(holder);
        for (i = 1; i <= 24; ++i) {
            assert(!name_card_holder_get(holder, 1000 + i)->complete);
            assert(!name_card_holder_get(holder, 1000 + i)->complete);
        }
        assert(WANTED(holder) == 24);
        name_card_holder_set_all(holder, 1003, owned("한글이름"), owned("party with a long name"),
                                 owned("guild"), owned("Officer"),
                                 owned("achievement title with a long name"), owned("NPC title"),
                                 (int)round);
        trace_card(name_card_holder_get(holder, 1003));
        for (i = 1; i <= 24; ++i) {
            if (i == 3)
                continue;
            name_card_holder_update(holder);
            assert(requested == 1000 + i);
        }
        assert(requests == 23 && ASKED(holder) == 23 && !WANTED(holder));
        name_card_holder_get(holder, 1001);
        name_card_holder_update(holder);
        assert(requests == 23 && ASKED(holder) == 23 && !WANTED(holder));
        test_time = begin + 9999;
        name_card_holder_update(holder);
        assert(ASKED(holder) == 23);
        test_time = begin + 10000;
        name_card_holder_get(holder, 1001);
        name_card_holder_update(holder);
        assert(requests == 24 && requested == 1001 && ASKED(holder) == 1);
        name_card_holder_erase(holder, 1003);
        assert(!name_card_holder_has(holder, 1003));
        name_card_holder_clear(holder);
        name_card_holder_clear(holder);
        assert(!holder->cards.size && !WANTED(holder) && !ASKED(holder));
        name_card_holder_destroy(holder);
        assert(state.before == 0xa5a5a5a5u && state.after == 0xa5a5a5a5u && !allocations);
    }
}

int main(int argc, char **argv)
{
    struct {
        struct ActorNameCardHolder holder;
        unsigned guard;
    } state;
    memset(&state, 0xa5, sizeof state);
    name_card_holder_init(&state.holder);
    assert(state.guard == 0xa5a5a5a5u);
    seed(&state.holder);

    values(&state.holder);
    name_card_holder_clear(&state.holder);
    name_card_holder_clear(&state.holder);
    name_card_holder_destroy(&state.holder);
    assert(state.guard == 0xa5a5a5a5u && !allocations);
    request_lifetimes();
    if (argc > 1) {
        FILE *file = fopen(argv[1], "wb");
        assert(file);
        assert(fwrite(trace, sizeof trace[0], trace_count, file) == trace_count);
        assert(fclose(file) == 0);
    }
    printf("Name-card owners: 16 request/expiry/lifecycle rounds and guarded value tests "
           "passed; %u trace words.\n",
           trace_count);
    puts("actor names: authoritative fields, deep copies, owned arguments and request lifetimes passed");
    return 0;
}
