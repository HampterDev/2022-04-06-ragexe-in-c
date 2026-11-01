#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "contents/party_booking_data.h"
static int live, allocations, payload_attempts, fail_payload_allocation;
void *mem_alloc(size_t size)
{
    unsigned char *block;
    assert(size <= SIZE_MAX - 64);
    block = malloc(size + 64);
    assert(block);
    memset(block, 0xa7, size + 64);
    *(size_t *)block = size;
    ++live; ++allocations;
    return block + 32;
}

void mem_free(void *pointer)
{
    size_t size;
    unsigned char *block;
    size_t i;
    if (!pointer) return;
    block = (unsigned char *)pointer - 32;
    assert(live > 0);
    size = *(size_t *)block;
    for (i = sizeof(size_t); i < 32; ++i) assert(block[i] == 0xa7);
    for (i = 0; i < 32; ++i) assert(block[32 + size + i] == 0xa7);
    --live;
    free(block);
}

static void *payload_malloc(size_t size)
{
    ++payload_attempts;
    if (payload_attempts == fail_payload_allocation) return NULL;
    return mem_alloc(size);
}
static void payload_free(void *pointer)
{
    if (pointer)
        mem_free(pointer);
}
#define malloc payload_malloc
#define free payload_free
#include "../src/core/owned_text.c"
#undef malloc
#undef free

static void test_text_and_booking(void)
{
    struct OwnedText text = {0};
    struct AdvenPartyInfo original, copy;
    const char bytes[] = {'a', 0, 'b', 'c'};
    char *saved;
    int fail;
    assert(owned_text_assign(&text, text_view(bytes, sizeof bytes)));
    assert(text.size == 4 && !memcmp(text.data, bytes, 4) && !text.data[4]);
    assert(owned_text_assign(&text, text_view(text.data + 1, 3)));
    assert(text.size == 3 && !memcmp(text.data, bytes + 1, 3));
    saved = text.data;
    assert(!owned_text_assign(&text, text_view("", SIZE_MAX)) && saved == text.data);
    payload_attempts = 0; fail_payload_allocation = 1;
    assert(!owned_text_assign(&text, text_view_cstr("new")) && saved == text.data);
    fail_payload_allocation = 0;
    assert(owned_text_assign(&text, text_view("", 0)) && !text.data && !text.size);
    assert(owned_text_concat(&text, text_view(bytes, sizeof bytes), text_view_cstr("tail")));
    assert(text.size == 8 && !memcmp(text.data, "a\0bctail", 8) && !text.data[8]);
    assert(owned_text_concat(&text, text_view(text.data + 1, 3), owned_text_view(&text)));
    assert(text.size == 11 && !memcmp(text.data, "\0bca\0bctail", 11));
    saved = text.data;
    assert(!owned_text_concat(&text, text_view("", SIZE_MAX), text_view("", 0)));
    assert(!owned_text_concat(&text, text_view("", 0), text_view("", SIZE_MAX)));
    assert(!owned_text_concat(&text, text_view("", SIZE_MAX - 1), text_view("", 1)));
    assert(text.data == saved);
    payload_attempts = 0; fail_payload_allocation = 1;
    assert(!owned_text_concat(&text, owned_text_view(&text), text_view_cstr("x")));
    assert(text.data == saved);
    fail_payload_allocation = 0;
    assert(owned_text_concat(&text, text_view("", 0), text_view("", 0)) && !text.data && !text.size);
    owned_text_clear(&text); owned_text_clear(&text);
    party_booking_init(&original); party_booking_init(&copy);
    assert(owned_text_assign(&original.memo, text_view(bytes, sizeof bytes)));
    assert(owned_text_assign(&original.name, text_view_cstr("name")));
    original.aid = 42;
    assert(owned_text_assign(&copy.name, text_view_cstr("old")));
    copy.aid = 7;
    for (fail = 1; fail <= 2; ++fail) {
        payload_attempts = 0; fail_payload_allocation = fail;
        assert(!party_booking_assign(&copy, &original));
        assert(copy.aid == 7 && !strcmp(owned_text_cstr(&copy.name), "old") && !copy.memo.size);
    }
    fail_payload_allocation = 0;
    assert(party_booking_assign(&copy, &original));
    assert(party_booking_assign(&copy, &copy));
    party_booking_clear(&original);
    assert(copy.aid == 42 && !memcmp(copy.memo.data, bytes, sizeof bytes));
    party_booking_clear(&copy); party_booking_clear(&copy);
    assert(!live);
}

int main(void) {
    test_text_and_booking();
    puts("Owned text/booking: counted and aliased strings, allocation failure, transactional copies and guarded cleanup passed.");
    return 0;
}
