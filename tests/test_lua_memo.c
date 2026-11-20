#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "contents/lua_memo.h"

/* A reference memo: the kept answers in a list with the time each was last used; a key's set is
   its hash's low bits, and a full set gives up the answer used longest ago. */
struct Known {
    unsigned char key[LUA_MEMO_KEY_MAX + 4];
    size_t key_size;
    unsigned char value[LUA_MEMO_VALUE_MAX + 4];
    size_t value_size;
    unsigned long used;
};
static struct Known known[4096];
static size_t known_count;
static unsigned long known_clock;

static struct Known *known_find(const unsigned char *key, size_t size)
{
    size_t i;
    for (i = 0; i < known_count; ++i)
        if (known[i].key_size == size && !memcmp(known[i].key, key, size)) return &known[i];
    return NULL;
}

static size_t set_index(const void *key, size_t size, size_t sets)
{
    return lua_memo_hash(key, size) & (sets - 1);
}

/* How many answers the key's set holds in the reference, and the one of them used longest ago. */
static size_t known_in_set(const unsigned char *key, size_t key_size, size_t sets, size_t *oldest)
{
    size_t i, in_set = 0, set = set_index(key, key_size, sets);
    *oldest = known_count;
    for (i = 0; i < known_count; ++i) {
        if (set_index(known[i].key, known[i].key_size, sets) != set) continue;
        ++in_set;
        if (*oldest == known_count || known[i].used < known[*oldest].used) *oldest = i;
    }
    return in_set;
}

static void known_store(const unsigned char *key, size_t key_size, const unsigned char *value,
                        size_t value_size, size_t sets)
{
    struct Known *entry = known_find(key, key_size);
    if (!entry) {
        size_t oldest;
        if (known_in_set(key, key_size, sets, &oldest) == LUA_MEMO_WAYS) {
            entry = &known[oldest];
        } else {
            assert(known_count < sizeof known / sizeof *known);
            entry = &known[known_count++];
        }
        memcpy(entry->key, key, key_size);
        entry->key_size = key_size;
    }
    memcpy(entry->value, value, value_size);
    entry->value_size = value_size;
    entry->used = ++known_clock;
}

static unsigned random_state = 0x2545f491u;
static unsigned next_random(void)
{
    random_state ^= random_state << 13;
    random_state ^= random_state >> 17;
    random_state ^= random_state << 5;
    return random_state;
}

static int holds(struct LuaMemo *memo, const char *key, char value)
{
    const struct LuaMemoEntry *entry = lua_memo_find(memo, key, strlen(key));
    return entry && entry->value_size == 1 && entry->value[0] == value;
}

static int store(struct LuaMemo *memo, const char *key, char value)
{
    return lua_memo_store(memo, key, strlen(key), &value, 1);
}

/* The first count keys "k<n>" that fall in one set of a memo of the given sets. */
static void keys_of_one_set(char keys[][8], int count, size_t sets)
{
    int found = 0, i;
    size_t set = 0;
    for (i = 0; found < count; ++i) {
        char key[8];
        sprintf(key, "k%d", i);
        if (found && set_index(key, strlen(key), sets) != set) continue;
        set = set_index(key, strlen(key), sets);
        strcpy(keys[found++], key);
    }
}

enum { WAYS = LUA_MEMO_WAYS };

static void basics(void)
{
    static struct LuaMemoEntry storage[4 * WAYS], larger[8 * WAYS], same[4 * WAYS];
    struct LuaMemo memo, grown, none, copy;
    const struct LuaMemoEntry *entry;
    unsigned char key[LUA_MEMO_KEY_MAX + 1], value[LUA_MEMO_VALUE_MAX + 1];
    char keys[WAYS + 3][8];
    int i;

    lua_memo_init(&none, NULL, 16);
    assert(!lua_memo_slots(&none) && !lua_memo_find(&none, "a", 1) && !lua_memo_store(&none, "a", 1, "b", 1));
    assert(!lua_memo_would_replace(&none, "a", 1));
    lua_memo_init(&none, storage, WAYS - 1);
    assert(!lua_memo_slots(&none) && !none.entries && !lua_memo_store(&none, "a", 1, "b", 1));
    lua_memo_init(&memo, storage, 4 * WAYS + 3);
    assert(lua_memo_slots(&memo) == 4 * WAYS && memo.sets == 4 && !memo.count);
    lua_memo_init(&memo, storage, 3 * WAYS);
    assert(lua_memo_slots(&memo) == 2 * WAYS && memo.sets == 2);
    assert(!lua_memo_find(&memo, "a", 1));
    assert(store(&memo, "a", '1') && holds(&memo, "a", '1'));
    assert(lua_memo_store(&memo, "a", 1, "22", 2) && memo.count == 1);
    entry = lua_memo_find(&memo, "a", 1);
    assert(entry && entry->value_size == 2 && !memcmp(entry->value, "22", 2));
    assert(!lua_memo_find(&memo, "ab", 2) && !lua_memo_find(&memo, "b", 1));
    assert(lua_memo_store(&memo, "silent", 6, NULL, 0) && lua_memo_find(&memo, "silent", 6)->value_size == 0);
    memset(key, 'k', sizeof key);
    memset(value, 'v', sizeof value);
    assert(!lua_memo_store(&memo, key, LUA_MEMO_KEY_MAX + 1, value, 1));
    assert(!lua_memo_store(&memo, key, 0, value, 1) && !lua_memo_find(&memo, key, 0));
    assert(!lua_memo_store(&memo, key, 3, value, LUA_MEMO_VALUE_MAX + 1));
    assert(lua_memo_store(&memo, key, LUA_MEMO_KEY_MAX, value, LUA_MEMO_VALUE_MAX));
    assert(lua_memo_find(&memo, key, LUA_MEMO_KEY_MAX)->value_size == LUA_MEMO_VALUE_MAX);
    assert(!lua_memo_find(&memo, key, LUA_MEMO_KEY_MAX + 1) && memo.count == 3);
    assert(!lua_memo_would_replace(&memo, key, LUA_MEMO_KEY_MAX + 1) && !lua_memo_would_replace(&memo, key, 0));

    /* A full set gives up the answer used longest ago, found or stored, and nothing else; it says
       so before, and a key it holds never would. */
    lua_memo_init(&memo, storage, 4 * WAYS);
    keys_of_one_set(keys, WAYS + 3, memo.sets);
    for (i = 0; i < WAYS; ++i) {
        assert(!lua_memo_would_replace(&memo, keys[i], strlen(keys[i])));
        assert(store(&memo, keys[i], (char)('0' + i)));
    }
    assert(memo.count == WAYS && holds(&memo, keys[0], '0'));
    assert(lua_memo_would_replace(&memo, keys[WAYS], strlen(keys[WAYS])));
    assert(!lua_memo_would_replace(&memo, keys[1], strlen(keys[1])));
    assert(store(&memo, keys[WAYS], 'n') && memo.count == WAYS);
    assert(!holds(&memo, keys[1], '1'));
    assert(store(&memo, keys[2], 'x'));
    assert(store(&memo, keys[WAYS + 1], 'm') && memo.count == WAYS);
    assert(!holds(&memo, keys[3], '3'));
    for (i = 4; i < WAYS; ++i) assert(holds(&memo, keys[i], (char)('0' + i)));
    assert(holds(&memo, keys[0], '0') && holds(&memo, keys[WAYS], 'n') && holds(&memo, keys[2], 'x')
           && holds(&memo, keys[WAYS + 1], 'm'));

    /* Moved into the same or more slots: every answer, the order of use and the count of answers
       given are kept. */
    for (i = 0; i < 200; ++i) assert(!lua_memo_check_due(&memo));
    lua_memo_init(&grown, larger, 8 * WAYS);
    lua_memo_move(&grown, &memo);
    assert(grown.count == WAYS && grown.unchecked == 200 && grown.clock == memo.clock);
    for (i = 4; i < WAYS; ++i) assert(holds(&grown, keys[i], (char)('0' + i)));
    assert(holds(&grown, keys[0], '0') && holds(&grown, keys[WAYS], 'n') && holds(&grown, keys[2], 'x')
           && holds(&grown, keys[WAYS + 1], 'm'));
    lua_memo_init(&copy, same, 4 * WAYS);
    lua_memo_move(&copy, &memo);
    assert(copy.count == WAYS && store(&copy, keys[WAYS + 2], 'z') && copy.count == WAYS);
    assert(!holds(&copy, keys[WAYS > 4 ? 4 : 0], WAYS > 4 ? '4' : '0') && holds(&copy, keys[WAYS + 2], 'z'));

    /* Every LUA_MEMO_CHECK_EVERY-th answer given is due to be checked. */
    lua_memo_init(&memo, storage, 4 * WAYS);
    for (i = 1; i <= 3 * LUA_MEMO_CHECK_EVERY; ++i)
        assert(lua_memo_check_due(&memo) == (i % LUA_MEMO_CHECK_EVERY == 0));

    /* The order of use survives the clock wrapping. */
    lua_memo_init(&memo, storage, WAYS);
    memo.clock = UINT32_MAX - 2;
    for (i = 0; i < WAYS; ++i) {
        char name[2] = {(char)('a' + i), 0};
        assert(store(&memo, name, (char)('0' + i)));
    }
    assert(memo.clock < WAYS + 2 && holds(&memo, "a", '0'));
    assert(store(&memo, "z", 'z'));
    assert(!holds(&memo, "b", '1') && holds(&memo, "a", '0') && holds(&memo, "c", '2') && holds(&memo, "z", 'z'));
}

static void book(void)
{
    struct LuaMemoBook book;
    struct LuaMemoFunction *global, *in_table, *again;
    char long_name[LUA_MEMO_NAME_MAX + 1], name[8];
    int i;

    lua_memo_book_init(&book);
    assert(!lua_memo_book_function(&book, NULL, NULL) && !lua_memo_book_function(&book, "T", NULL));
    global = lua_memo_book_function(&book, NULL, "f");
    in_table = lua_memo_book_function(&book, "T", "f");
    assert(global && in_table && global != in_table);
    assert(lua_memo_book_function(&book, NULL, "f") == global && lua_memo_book_function(&book, "T", "f") == in_table);
    assert(lua_memo_book_function(&book, "", "f") != global && lua_memo_book_function(&book, "Tf", "") != in_table);
    assert(!global->off && !global->answers.entries && !lua_memo_slots(&global->answers));
    memset(long_name, 'n', sizeof long_name);
    long_name[LUA_MEMO_NAME_MAX - 3] = 0;
    assert(lua_memo_book_function(&book, long_name, "f") && !lua_memo_book_function(&book, long_name, "fg"));
    long_name[LUA_MEMO_NAME_MAX - 3] = 'n';
    long_name[LUA_MEMO_NAME_MAX - 1] = 0;
    assert(lua_memo_book_function(&book, NULL, long_name));
    long_name[LUA_MEMO_NAME_MAX - 1] = 'n';
    long_name[LUA_MEMO_NAME_MAX] = 0;
    assert(!lua_memo_book_function(&book, NULL, long_name) && !lua_memo_book_function(&book, long_name, "f"));
    for (i = 0; i < LUA_MEMO_FUNCTIONS - 6; ++i) {
        sprintf(name, "g%d", i);
        assert(lua_memo_book_function(&book, NULL, name));
    }
    assert(!lua_memo_book_function(&book, NULL, "one more") && lua_memo_book_function(&book, NULL, "f") == global);
    global->off = 1;
    in_table->name_size = 0;
    again = lua_memo_book_function(&book, NULL, "one more");
    assert(again == in_table && !again->off && lua_memo_book_function(&book, NULL, "f")->off);
}

/* Stores and finds against the reference, with growth by moving and forgetting by starting over,
   on keys from a small alphabet so that full sets, collisions and repeated keys are common. */
static void fuzz(unsigned seed, size_t first_slots, size_t most_slots, unsigned restart_per_mille)
{
    static struct LuaMemoEntry storage_a[1024], storage_b[1024];
    struct LuaMemoEntry *storage = storage_a, *spare = storage_b;
    struct LuaMemo memo;
    unsigned step, replaced = 0;

    random_state = seed;
    known_count = 0;
    known_clock = 0;
    lua_memo_init(&memo, storage, first_slots);
    for (step = 0; step < 100000; ++step) {
        unsigned char key[LUA_MEMO_KEY_MAX + 2], value[LUA_MEMO_VALUE_MAX + 2];
        size_t key_size = 1 + next_random() % (next_random() % 8 ? 4 : LUA_MEMO_KEY_MAX + 1);
        size_t value_size = next_random() % (LUA_MEMO_VALUE_MAX + 2), i, oldest;
        unsigned op = next_random() % 1000;
        const struct LuaMemoEntry *entry;
        struct Known *reference;

        for (i = 0; i < key_size; ++i) key[i] = (unsigned char)('a' + next_random() % 4);
        for (i = 0; i < value_size; ++i) value[i] = (unsigned char)next_random();
        if (op < restart_per_mille) {
            lua_memo_init(&memo, storage, first_slots);
            known_count = 0;
        } else if (op < 20 && lua_memo_slots(&memo) < most_slots) {
            struct LuaMemo grown;
            struct LuaMemoEntry *swap = storage;
            lua_memo_init(&grown, spare, lua_memo_slots(&memo) * 2);
            lua_memo_move(&grown, &memo);
            assert(grown.count == memo.count && grown.clock == memo.clock);
            memo = grown;
            storage = spare;
            spare = swap;
        } else if (op < 500) {
            int fits = key_size <= LUA_MEMO_KEY_MAX && value_size <= LUA_MEMO_VALUE_MAX;
            int would = key_size <= LUA_MEMO_KEY_MAX && !known_find(key, key_size)
                        && known_in_set(key, key_size, memo.sets, &oldest) == LUA_MEMO_WAYS;
            assert(lua_memo_would_replace(&memo, key, key_size) == would);
            assert(lua_memo_store(&memo, key, key_size, value, value_size) == fits);
            if (fits) known_store(key, key_size, value, value_size, memo.sets);
            replaced += would && fits;
        }
        reference = known_find(key, key_size);
        entry = lua_memo_find(&memo, key, key_size);
        assert(!entry == !reference);
        if (entry) {
            assert(entry->value_size == reference->value_size
                   && !memcmp(entry->value, reference->value, reference->value_size));
            reference->used = ++known_clock;
        }
        assert(memo.count == known_count && memo.count <= lua_memo_slots(&memo));
        if (step % 4096 == 0) {
            for (i = 0; i < known_count; ++i) {
                entry = lua_memo_find(&memo, known[i].key, known[i].key_size);
                assert(entry && entry->value_size == known[i].value_size
                       && !memcmp(entry->value, known[i].value, known[i].value_size));
                known[i].used = ++known_clock;
            }
        }
    }
    assert(replaced > 100);
}

int main(void)
{
    basics();
    book();
    fuzz(0x2545f491u, 2 * WAYS, 1024, 2);
    fuzz(0x9e3779b9u, WAYS, WAYS, 0);
    fuzz(0x12345678u, 8 * WAYS, 32 * WAYS, 1);
    puts("Lua memo: keys and answers kept and bounded, a full set giving up its answer used longest ago "
         "(across the clock wrapping) and saying so before, growth by moving, the checks due, the book's "
         "functions apart, and 300000 fuzzed steps against a list passed.");
    return 0;
}
