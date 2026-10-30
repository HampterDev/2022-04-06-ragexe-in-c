#ifndef FILEIO_RESOURCE_CACHE_STORE_H
#define FILEIO_RESOURCE_CACHE_STORE_H

#include <stddef.h>
struct Res;
struct Hash;

struct ResourceList {
    struct Res **items;
    size_t count, capacity;
};

struct Res *resource_cache_find(const struct ResourceList *cache, const struct Hash *key);
int resource_cache_insert(struct ResourceList *cache, struct Res *resource);
size_t resource_cache_after(const struct ResourceList *cache, const struct Hash *key);
void resource_list_append(struct ResourceList *list, struct Res *resource);
struct Res *resource_list_take(struct ResourceList *list, size_t index);
void resource_list_remove(struct ResourceList *list, const struct Res *resource);
void resource_list_release(struct ResourceList *list);

struct ResourceAlias {
    char *name, *value;
};
struct ResourceAliases {
    struct ResourceAlias *items;
    size_t count, capacity;
};

int resource_alias_add(struct ResourceAliases *aliases, const char *name, const char *value);
const char *resource_alias_find(const struct ResourceAliases *aliases, const char *name);
const char *resource_alias_resolve(const struct ResourceAliases *aliases, const char *name);
void resource_alias_parse(struct ResourceAliases *aliases, const char *text, size_t size);
void resource_alias_release(struct ResourceAliases *aliases);

#endif
