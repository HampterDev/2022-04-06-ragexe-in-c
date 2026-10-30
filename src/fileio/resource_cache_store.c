#include "fileio/resource_cache_store.h"
#include "fileio/resource_types.h"
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

static void *grow(void *items, size_t *capacity, size_t count, size_t item_size)
{
    size_t next;
    void *result;
    if (count <= *capacity)
        return items;
    if (count > SIZE_MAX / item_size)
        abort();
    next = *capacity ? *capacity : 8;
    while (next < count) {
        if (next > SIZE_MAX / 2 / item_size) {
            next = count;
            break;
        }
        next *= 2;
    }
    result = realloc(items, next * item_size);
    if (!result)
        abort();
    *capacity = next;
    return result;
}

static int compare_key(const struct Hash *a, const struct Hash *b)
{
    if (a->hash != b->hash)
        return a->hash < b->hash ? -1 : 1;
    return strcmp(a->name, b->name);
}

static size_t lower_bound(const struct ResourceList *cache, const struct Hash *key)
{
    size_t first = 0, last = cache->count;
    while (first < last) {
        size_t middle = first + (last - first) / 2;
        if (compare_key(&cache->items[middle]->name, key) < 0)
            first = middle + 1;
        else
            last = middle;
    }
    return first;
}

struct Res *resource_cache_find(const struct ResourceList *cache, const struct Hash *key)
{
    size_t index = lower_bound(cache, key);
    return index < cache->count && compare_key(&cache->items[index]->name, key) == 0
        ? cache->items[index] : NULL;
}

int resource_cache_insert(struct ResourceList *cache, struct Res *resource)
{
    size_t index = lower_bound(cache, &resource->name);
    if (index < cache->count && compare_key(&cache->items[index]->name, &resource->name) == 0)
        return 0;
    if (cache->count == SIZE_MAX)
        abort();
    cache->items = grow(cache->items, &cache->capacity, cache->count + 1, sizeof *cache->items);
    memmove(cache->items + index + 1, cache->items + index,
            (cache->count - index) * sizeof *cache->items);
    cache->items[index] = resource;
    ++cache->count;
    return 1;
}

size_t resource_cache_after(const struct ResourceList *cache, const struct Hash *key)
{
    size_t index = lower_bound(cache, key);
    if (index < cache->count && compare_key(&cache->items[index]->name, key) == 0)
        ++index;
    return index;
}

void resource_list_append(struct ResourceList *list, struct Res *resource)
{
    if (list->count == SIZE_MAX)
        abort();
    list->items = grow(list->items, &list->capacity, list->count + 1, sizeof *list->items);
    list->items[list->count++] = resource;
}

struct Res *resource_list_take(struct ResourceList *list, size_t index)
{
    struct Res *resource;
    if (index >= list->count)
        return NULL;
    resource = list->items[index];
    --list->count;
    memmove(list->items + index, list->items + index + 1,
            (list->count - index) * sizeof *list->items);
    return resource;
}

void resource_list_remove(struct ResourceList *list, const struct Res *resource)
{
    size_t i;
    for (i = 0; i < list->count; ++i)
        if (list->items[i] == resource) {
            resource_list_take(list, i);
            return;
        }
}

void resource_list_release(struct ResourceList *list)
{
    free(list->items);
    memset(list, 0, sizeof *list);
}

static size_t alias_bound(const struct ResourceAliases *aliases, const char *name)
{
    size_t first = 0, last = aliases->count;
    while (first < last) {
        size_t middle = first + (last - first) / 2;
        if (strcmp(aliases->items[middle].name, name) < 0)
            first = middle + 1;
        else
            last = middle;
    }
    return first;
}

static char *copy_text(const char *text, size_t length)
{
    char *copy;
    if (length == SIZE_MAX)
        abort();
    copy = malloc(length + 1);
    if (!copy)
        abort();
    if (length)
        memcpy(copy, text, length);
    copy[length] = 0;
    return copy;
}

int resource_alias_add(struct ResourceAliases *aliases, const char *name, const char *value)
{
    size_t index = alias_bound(aliases, name);
    struct ResourceAlias item;
    if (index < aliases->count && strcmp(aliases->items[index].name, name) == 0)
        return 0;
    if (aliases->count == SIZE_MAX)
        abort();
    item.name = copy_text(name, strlen(name));
    item.value = copy_text(value, strlen(value));
    aliases->items = grow(aliases->items, &aliases->capacity, aliases->count + 1, sizeof *aliases->items);
    memmove(aliases->items + index + 1, aliases->items + index,
            (aliases->count - index) * sizeof *aliases->items);
    aliases->items[index] = item;
    ++aliases->count;
    return 1;
}

const char *resource_alias_find(const struct ResourceAliases *aliases, const char *name)
{
    size_t index = alias_bound(aliases, name);
    return index < aliases->count && strcmp(aliases->items[index].name, name) == 0
        ? aliases->items[index].value : NULL;
}

const char *resource_alias_resolve(const struct ResourceAliases *aliases, const char *name)
{
    const char *value, *at;
    char *variant;
    size_t length, offset;
    if (!name)
        return NULL;
    value = resource_alias_find(aliases, name);
    if (value)
        return value;
    at = strrchr(name, '@');
    if (!at || at - name < 4)
        return name;
    length = strlen(name);
    offset = (size_t)(at - name) - 4;
    variant = copy_text(name, length);
    memmove(variant + offset, variant + offset + 3, length - offset - 2);
    value = resource_alias_find(aliases, variant);
    free(variant);
    return value ? value : name;
}

void resource_alias_parse(struct ResourceAliases *aliases, const char *text, size_t size)
{
    size_t start = 0, i;
    char *key = NULL;
    resource_alias_release(aliases);
    for (i = 0; i < size; ++i) {
        if (text[i] == '#') {
            size_t length = i - start;
            const char *end = memchr(text + start, 0, length);
            char *field = copy_text(text + start, end ? (size_t)(end - (text + start)) : length);
            if (!key || !*key) {
                free(key);
                key = field;
            } else {
                resource_alias_add(aliases, key, field);
                free(field);
                free(key);
                key = NULL;
            }
        } else if (text[i] != '\r' && text[i] != '\n') {
            continue;
        }
        start = i + 1;
    }
    free(key);
}

void resource_alias_release(struct ResourceAliases *aliases)
{
    size_t i;
    for (i = 0; i < aliases->count; ++i) {
        free(aliases->items[i].name);
        free(aliases->items[i].value);
    }
    free(aliases->items);
    memset(aliases, 0, sizeof *aliases);
}
