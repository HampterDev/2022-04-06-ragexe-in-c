#include <assert.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include "base.h"

static uintptr_t freed_address;
static unsigned frees;

void *mem_alloc(size_t bytes)
{
    void *block = malloc(bytes ? bytes : 1);

    assert(block);
    return block;
}

void mem_free(void *block)
{
    freed_address = (uintptr_t)block;
    ++frees;
    free(block);
}

void fatal_error(const char *message)
{
    fprintf(stderr, "%s\n", message);
    abort();
}

static void empty(const struct String *s)
{
    assert(s->size == 0);
    assert(s->capacity == 0);
    assert(s->data == NULL);
}

int main(void)
{
    struct String dst, src;
    char short_text[] = "short text";
    char *old_block, *owned_block;
    uintptr_t old_address, owned_address;

    string_init(&dst);
    string_init(&src);
    src.data = short_text;
    src.size = src.capacity = 10;
    string_free(&dst);
    string_move(&dst, &src);
    assert(strcmp(string_cstr(&dst), "short text") == 0);
    assert(dst.size == 10);
    empty(&src);
    assert(frees == 0);

    old_block = malloc(32);
    owned_block = malloc(64);
    assert(old_block && owned_block);
    old_address = (uintptr_t)old_block;
    owned_address = (uintptr_t)owned_block;
    memcpy(old_block, "old destination contents", sizeof "old destination contents");
    memcpy(owned_block, "a source string too long for the inline buffer",
           sizeof "a source string too long for the inline buffer");
    dst.data = old_block;
    dst.size = (unsigned)strlen(old_block);
    dst.capacity = 31;
    src.data = owned_block;
    src.size = (unsigned)strlen(owned_block);
    src.capacity = 63;

    string_free(&dst);
    string_move(&dst, &src);
    assert(dst.data == owned_block);
    assert(dst.capacity == 63 && dst.size == strlen(owned_block));
    assert(frees == 1 && freed_address == old_address);
    empty(&src);

    string_free(&dst);
    string_move(&dst, &src);
    assert(frees == 2 && freed_address == owned_address);
    empty(&src);
    empty(&dst);
    string_free(&dst);
    assert(frees == 2);
    puts("string ownership: moves, frees and empty strings passed");
    return 0;
}
