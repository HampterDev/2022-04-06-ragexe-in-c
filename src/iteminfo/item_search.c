#include "iteminfo/iteminfo.h"
#include "std/list.h"

#include <string.h>

static unsigned char name_matches(const struct String *name, const struct String *query, char fuzzy)
{
    const char *n = string_cstr(name), *q = string_cstr(query);

    if (fuzzy == 1 && query->size <= name->size) {
        const char *end, *p;

        if (query->size == 0)
            return 1;
        end = n + (name->size - query->size + 1);
        for (p = memchr(n, q[0], end - n); p; p = memchr(p + 1, q[0], end - (p + 1)))
            if (memcmp(p, q, query->size) == 0)
                return 1;
    }
    return name->size == query->size && memcmp(n, q, name->size) == 0;
}

unsigned char item_search_match(struct String name, struct String query, char fuzzy,
                                       struct List *out, int itemId)
{
    unsigned char matched = name_matches(&name, &query, fuzzy);

    if (matched)
        list_push_back(out, &itemId, sizeof itemId);
    string_free(&name);
    string_free(&query);
    return matched;
}

static void match_entry(int key, const struct ItemTableEntry *entry, const struct String *query, char cardsOnly, char fuzzy,
                        struct List *out)
{
    struct String q;

    if (cardsOnly == 1 && !card_affix_has(key))
        return;
    if (!entry->name || !*entry->name)
        return;
    string_copy(&q, query);
    if (item_search_match(string_from_cstr(entry->name), q, fuzzy, out, key))
        return;
    if (!card_affix_has(key))
        return;
    if (card_affix_get(key)->prefix) {
        string_copy(&q, query);
        if (item_search_match(string_from_cstr(card_affix_get(key)->prefix), q, fuzzy, out, key))
            return;
    }
    if (card_affix_get(key)->postfix) {
        string_copy(&q, query);
        item_search_match(string_from_cstr(card_affix_get(key)->postfix), q, fuzzy, out, key);
    }
}

struct List *item_search_by_name(struct List *out, struct String query, char cardsOnly, char fuzzy)
{
    size_t i;

    list_init(out);

    for (i = 0; i < item_table_count(); ++i) {
        int key;
        const struct ItemTableEntry *entry = item_table_entry_at(i, &key);
        match_entry(key, entry, &query, cardsOnly, fuzzy, out);
    }
    string_free(&query);
    return out;
}
