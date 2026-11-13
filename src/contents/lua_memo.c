#include "contents/lua_memo.h"
#include <string.h>

uint32_t lua_memo_hash(const void *key, size_t key_size)
{
    const unsigned char *bytes = key;
    uint32_t hash = 2166136261u;
    size_t i;

    for (i = 0; i < key_size; ++i)
        hash = (hash ^ bytes[i]) * 16777619u;
    return hash;
}

void lua_memo_init(struct LuaMemo *memo, struct LuaMemoEntry *entries, size_t slots)
{
    size_t sets = 0;

    if (entries && slots >= LUA_MEMO_WAYS) {
        sets = 1;
        while (sets <= slots / LUA_MEMO_WAYS / 2)
            sets *= 2;
    }
    memo->entries = sets ? entries : NULL;
    memo->sets = sets;
    memo->count = 0;
    memo->clock = 0;
    memo->unchecked = 0;
    if (sets)
        memset(entries, 0, sets * LUA_MEMO_WAYS * sizeof *entries);
}

size_t lua_memo_slots(const struct LuaMemo *memo)
{
    return memo->sets * LUA_MEMO_WAYS;
}

static struct LuaMemoEntry *set_of(const struct LuaMemo *memo, uint32_t hash)
{
    return &memo->entries[(hash & (memo->sets - 1)) * LUA_MEMO_WAYS];
}

static int sizes_fit(size_t key_size, size_t value_size)
{
    return key_size && key_size <= LUA_MEMO_KEY_MAX && value_size <= LUA_MEMO_VALUE_MAX;
}

static struct LuaMemoEntry *kept(struct LuaMemoEntry *set, uint32_t hash, const void *key, size_t key_size)
{
    int way;

    for (way = 0; way < LUA_MEMO_WAYS; ++way)
        if (set[way].key_size == key_size && set[way].hash == hash && !memcmp(set[way].key, key, key_size))
            return &set[way];
    return NULL;
}

const struct LuaMemoEntry *lua_memo_find(struct LuaMemo *memo, const void *key, size_t key_size)
{
    struct LuaMemoEntry *entry;
    uint32_t hash;

    if (!memo->sets || !sizes_fit(key_size, 0))
        return NULL;
    hash = lua_memo_hash(key, key_size);
    entry = kept(set_of(memo, hash), hash, key, key_size);
    if (entry)
        entry->used = ++memo->clock;
    return entry;
}

/* The slot a new answer takes in its set: a free one, else the one used longest ago (by age, so
   that the clock may wrap). */
static struct LuaMemoEntry *place_in(struct LuaMemoEntry *set, uint32_t clock)
{
    struct LuaMemoEntry *oldest = &set[0];
    int way;

    for (way = 0; way < LUA_MEMO_WAYS; ++way) {
        if (!set[way].key_size)
            return &set[way];
        if ((uint32_t)(clock - set[way].used) > (uint32_t)(clock - oldest->used))
            oldest = &set[way];
    }
    return oldest;
}

int lua_memo_store(struct LuaMemo *memo, const void *key, size_t key_size, const void *value,
                   size_t value_size)
{
    struct LuaMemoEntry *set, *entry;
    uint32_t hash;

    if (!memo->sets || !sizes_fit(key_size, value_size))
        return 0;
    hash = lua_memo_hash(key, key_size);
    set = set_of(memo, hash);
    entry = kept(set, hash, key, key_size);
    if (!entry) {
        entry = place_in(set, memo->clock);
        if (!entry->key_size)
            ++memo->count;
        entry->hash = hash;
        entry->key_size = (unsigned char)key_size;
        memcpy(entry->key, key, key_size);
    }
    entry->used = ++memo->clock;
    entry->value_size = (unsigned char)value_size;
    if (value_size)
        memcpy(entry->value, value, value_size);
    return 1;
}

int lua_memo_would_replace(const struct LuaMemo *memo, const void *key, size_t key_size)
{
    struct LuaMemoEntry *set;
    uint32_t hash;
    int way;

    if (!memo->sets || !sizes_fit(key_size, 0))
        return 0;
    hash = lua_memo_hash(key, key_size);
    set = set_of(memo, hash);
    if (kept(set, hash, key, key_size))
        return 0;
    for (way = 0; way < LUA_MEMO_WAYS; ++way)
        if (!set[way].key_size)
            return 0;
    return 1;
}

void lua_memo_move(struct LuaMemo *to, const struct LuaMemo *from)
{
    size_t i;

    for (i = 0; i < lua_memo_slots(from); ++i) {
        const struct LuaMemoEntry *entry = &from->entries[i];

        if (entry->key_size) {
            struct LuaMemoEntry *set = set_of(to, entry->hash);
            int way = 0;

            while (way < LUA_MEMO_WAYS - 1 && set[way].key_size)
                ++way;
            set[way] = *entry;
            ++to->count;
        }
    }
    to->clock = from->clock;
    to->unchecked = from->unchecked;
}

int lua_memo_check_due(struct LuaMemo *memo)
{
    if (++memo->unchecked < LUA_MEMO_CHECK_EVERY)
        return 0;
    memo->unchecked = 0;
    return 1;
}

/* A function's name as the book keeps it: the table's name with its terminator, if any, then the
   function's with its terminator (a name holds no terminator, so the two kinds never meet). 0 when
   it does not fit. */
static size_t book_name(unsigned char *name, const char *table, const char *function)
{
    size_t table_size = table ? strlen(table) + 1 : 0, function_size = strlen(function) + 1;

    if (table_size > LUA_MEMO_NAME_MAX || function_size > LUA_MEMO_NAME_MAX
        || table_size + function_size > LUA_MEMO_NAME_MAX)
        return 0;
    if (table)
        memcpy(name, table, table_size);
    memcpy(name + table_size, function, function_size);
    return table_size + function_size;
}

void lua_memo_book_init(struct LuaMemoBook *book)
{
    int i;

    for (i = 0; i < LUA_MEMO_FUNCTIONS; ++i) {
        struct LuaMemoFunction *place = &book->functions[i];

        place->name_size = 0;
        place->off = 0;
        lua_memo_init(&place->answers, NULL, 0);
    }
}

struct LuaMemoFunction *lua_memo_book_function(struct LuaMemoBook *book, const char *table,
                                               const char *function)
{
    unsigned char name[LUA_MEMO_NAME_MAX];
    size_t size = function ? book_name(name, table, function) : 0;
    struct LuaMemoFunction *place = NULL;
    int i;

    if (!size)
        return NULL;
    for (i = 0; i < LUA_MEMO_FUNCTIONS; ++i) {
        struct LuaMemoFunction *at = &book->functions[i];

        if (at->name_size == size && !memcmp(at->name, name, size))
            return at;
        if (!at->name_size && !place)
            place = at;
    }
    if (place) {
        memcpy(place->name, name, size);
        place->name_size = (unsigned char)size;
        place->off = 0;
        lua_memo_init(&place->answers, NULL, 0);
    }
    return place;
}
