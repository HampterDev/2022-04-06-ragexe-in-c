#ifndef CONTENTS_LUA_MEMO_H
#define CONTENTS_LUA_MEMO_H

#include <stddef.h>
#include <stdint.h>

/* The answers of Lua functions that answer from their arguments alone, remembered by the call they
   answered. Pure: no allocation, no clock, no Lua.

   A LuaMemo holds one function's answers, a key of bytes (the call's format and arguments, packed
   by the caller) to the answer's bytes, in storage the caller gives. A key's hash picks a set of
   LUA_MEMO_WAYS slots; a new key whose set is full takes the place of the answer of that set used
   longest ago, so a memo never refuses an answer and never forgets more than one to keep another.
   The caller grows a memo, up to a bound of its choosing, when a new answer would take another's
   place (lua_memo_would_replace), by moving it into larger storage (lua_memo_move).

   Every LUA_MEMO_CHECK_EVERY-th answer a memo gives is due to be checked against Lua
   (lua_memo_check_due), so that a function that does not answer from its arguments alone is
   found out.

   A LuaMemoBook keeps the memos of up to LUA_MEMO_FUNCTIONS functions apart, by the function's
   table and name, so that one function's answers never take the place of another's; a function
   found to answer the same call differently is turned off and remembers nothing more. */

#define LUA_MEMO_KEY_MAX 40
#define LUA_MEMO_VALUE_MAX 16
#define LUA_MEMO_WAYS 8
#define LUA_MEMO_CHECK_EVERY 256
#define LUA_MEMO_FUNCTIONS 8
#define LUA_MEMO_NAME_MAX 64

struct LuaMemoEntry {
    uint32_t hash;
    uint32_t used; /* the memo's clock when the answer was last stored or found */
    unsigned char key_size; /* 0: a free slot */
    unsigned char value_size;
    unsigned char key[LUA_MEMO_KEY_MAX];
    unsigned char value[LUA_MEMO_VALUE_MAX];
};

struct LuaMemo {
    struct LuaMemoEntry *entries;
    size_t sets; /* 0, or a power of two; LUA_MEMO_WAYS slots each */
    size_t count;
    uint32_t clock;
    unsigned unchecked; /* answers given since the last one due to be checked */
};

uint32_t lua_memo_hash(const void *key, size_t key_size);
/* slots is rounded down to a power of two of at least LUA_MEMO_WAYS, or to 0; entries may be NULL
   with slots 0. */
void lua_memo_init(struct LuaMemo *memo, struct LuaMemoEntry *entries, size_t slots);
size_t lua_memo_slots(const struct LuaMemo *memo);
/* The answer kept for the key, marked as used now; NULL when there is none. */
const struct LuaMemoEntry *lua_memo_find(struct LuaMemo *memo, const void *key, size_t key_size);
/* 1 when the answer is kept (a key already kept takes the new answer); 0 when the memo has no
   storage, the key is empty or longer than LUA_MEMO_KEY_MAX, or the value is longer than
   LUA_MEMO_VALUE_MAX. */
int lua_memo_store(struct LuaMemo *memo, const void *key, size_t key_size, const void *value,
                   size_t value_size);
/* 1 when storing an answer for the key would take the place of another's: the memo has storage, the
   key fits and is not kept, and its set is full. */
int lua_memo_would_replace(const struct LuaMemo *memo, const void *key, size_t key_size);
/* Every answer of from into to, which must be empty and have at least from's slots (both are
   powers of two, so each of to's sets takes answers from one of from's sets only and none
   overflows); from is left as it was. */
void lua_memo_move(struct LuaMemo *to, const struct LuaMemo *from);
/* Counts an answer given from the memo: 1 for every LUA_MEMO_CHECK_EVERY-th. */
int lua_memo_check_due(struct LuaMemo *memo);

struct LuaMemoFunction {
    unsigned char name[LUA_MEMO_NAME_MAX]; /* the table's name and the function's, each ended */
    unsigned char name_size; /* 0: a free place */
    unsigned char off; /* found to answer the same call differently: remembers nothing */
    struct LuaMemo answers;
};

struct LuaMemoBook {
    struct LuaMemoFunction functions[LUA_MEMO_FUNCTIONS];
};

/* Every place free, every memo without storage. A place the caller frees again (name_size 0)
   must have no storage left. */
void lua_memo_book_init(struct LuaMemoBook *book);
/* The place of the function (table NULL: a global one), taken when it has none yet, with no
   storage and not off; NULL when the names are too long or every place is taken. */
struct LuaMemoFunction *lua_memo_book_function(struct LuaMemoBook *book, const char *table,
                                               const char *function);

#endif
