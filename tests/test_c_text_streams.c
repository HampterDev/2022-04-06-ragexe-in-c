#include "contents/guild_emblem_name.h"
#include "core/fatal.h"
#include "core/text_util.h"
#include "std/std.h"
#include <assert.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static unsigned allocations;

void *mem_alloc(size_t size)
{
    void *p = malloc(size ? size : 1);
    assert(p);
    ++allocations;
    return p;
}

void mem_free(void *p)
{
    if (p) {
        assert(allocations);
        --allocations;
        free(p);
    }
}

void fatal_error(const char *message)
{
    (void)message;
    abort();
}

static void check_names(void)
{
    struct String world, name;
    char long_world[5000];
    const int versions[] = {INT_MIN, -1, 0, 1, INT_MAX};
    const char *suffixes[] = {"4294967295", "2147483646", "2147483647", "2147483648", "4294967294"};
    size_t i;

    string_init(&world);
    string_assign_cstr(&world, "한글");
    for (i = 0; i < sizeof versions / sizeof versions[0]; ++i) {
        guild_emblem_name(&name, &world, UINT_MAX, versions[i]);
        assert(memcmp(string_cstr(&name), "한글_4294967295_", strlen("한글_4294967295_")) == 0);
        assert(memcmp(string_cstr(&name) + strlen("한글_4294967295_"), suffixes[i], strlen(suffixes[i])) == 0);
        assert(strcmp(string_cstr(&name) + name.size - 4, ".ebm") == 0);
        string_free(&name);
    }
    string_clear(&world);
    guild_emblem_name(&name, &world, 0, 0);
    assert(strcmp(string_cstr(&name), "_0_2147483647.ebm") == 0);
    string_free(&name);
    memset(long_world, 'w', sizeof long_world);
    string_assign(&world, long_world, sizeof long_world);
    guild_emblem_pattern(&name, &world, UINT_MAX);
    assert(name.size == sizeof long_world + strlen("_4294967295_*.ebm"));
    assert(memcmp(string_cstr(&name), long_world, sizeof long_world) == 0);
    assert(strcmp(string_cstr(&name) + sizeof long_world, "_4294967295_*.ebm") == 0);
    string_free(&name);
    string_assign(&world, "a\0b", 3);
    guild_emblem_pattern(&name, &world, 1);
    assert(name.size == 11 && memcmp(string_cstr(&name), "a\0b_1_*.ebm", 11) == 0);
    string_free(&name);
    string_free(&world);
}

static void check_container_lifetimes(void)
{
    static const size_t lengths[] = {0, 1, 15, 16, 31, 4094, 4095, 4096};
    char text[4097];
    size_t i;
    struct String *strings;
    struct Vec integers = {0};
    void (*copy)(struct String *, const struct String *) = string_copy;
    void (*destroy)(struct String *) = string_free;
    void (*destroy_integers)(struct Vec *) = vec_free;
    void *(*allocate)(size_t) = std_allocate_bytes;
    void (*deallocate)(void *) = mem_free;

    memset(text, 'a', sizeof text);
    assert(!allocate(0));
    deallocate(NULL);
    for (i = 0; i < sizeof lengths / sizeof lengths[0]; ++i) {
        struct String source, copied;
        size_t length = lengths[i];
        string_init(&source);
        string_assign(&source, text, length);
        copy(&copied, &source);
        assert(copied.size == length && string_cstr(&copied)[length] == 0);
        assert(!memcmp(string_cstr(&copied), text, length));
        if (length) {
            source.data[0] = 'b';
            assert(string_cstr(&copied)[0] == 'a');
        }
        destroy(&source);
        destroy(&copied);
        assert(!source.size && !source.capacity && !source.data);
        assert(!copied.size && !copied.capacity && !copied.data);
        destroy(&copied);
        assert(!allocations);
    }
    strings = allocate(3 * sizeof *strings);
    for (i = 0; i < 3; i++)
        string_init(&strings[i]);
    string_assign(&strings[1], text, 4096);
    for (i = 0; i < 3; i++)
        string_free(&strings[i]);
    deallocate(strings);
    integers.first = allocate(4 * sizeof(int));
    integers.last = (int *)integers.first + 2;
    integers.end = (int *)integers.first + 4;
    destroy_integers(&integers);
    assert(!integers.first && !integers.last && !integers.end);
    destroy_integers(&integers);
    assert(!allocations);
}

int main(void)
{
    check_container_lifetimes();
    check_names();
    assert(!allocations);
    puts("PASS: C emblem names");
    puts("PASS: ordinary C container callbacks, deep copies, inline/heap/aligned boundaries and repeated destruction");
    return 0;
}
