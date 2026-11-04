#include <assert.h>
#include <ctype.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <windows.h>
#include "iteminfo/item_api.h"
#include "iteminfo/item_table.h"
#include "iteminfo/item_link.h"

static unsigned allocations;
void *mem_alloc(size_t size) { void *p = malloc(size); assert(p); ++allocations; return p; }
void mem_free(void *p)
{  if (p) { assert(allocations); --allocations; free(p); } }

__declspec(noreturn) void fatal_error(const char *message) { (void)message; abort(); }

static struct ItemTableEntry table = { .name = "한국 아이템" };
const struct ItemTableEntry *item_table_get(int id) { (void)id; return &table; }

#ifdef ITEM_LINK_REFERENCE
static void int_vector_reserve_exactly(struct Vec *v, size_t count)
{
    size_t size = v->first ? (int *)v->last - (int *)v->first : 0;
    int *values = mem_alloc(count * sizeof *values);
    if (size) memcpy(values, v->first, size * sizeof *values);
    mem_free(v->first);
    v->first = values; v->last = values + size; v->end = values + count;
}
static void std_vector_int_resize(struct Vec *v, size_t count, const void *unused)
{
    (void)unused; int_vector_reserve_exactly(v, count);
    memset(v->first, 0, count * sizeof(int)); v->last = (int *)v->first + count;
}
static void std_vector_dword_push_back(struct Vec *v, int value)
{
    size_t size = v->first ? (int *)v->last - (int *)v->first : 0;
    if (v->last == v->end) int_vector_reserve_exactly(v, size + 8);
    *(int *)v->last = value; v->last = (int *)v->last + 1;
}
static void int_vector_destroy(struct Vec *v)
{ mem_free(v->first); memset(v, 0, sizeof *v); }
void string_append_char(struct String *s, char c) { string_append(s, &c, 1); }
static void text_to_lower(struct String *s)
{
    size_t i;
    for (i = 0; i < s->size; ++i) {
        signed char c = (signed char)string_cstr(s)[i];
        if (c <= 0) ++i;
        else if (isupper(c)) string_cstr(s)[i] += 0x20;
    }
}
#include "item_link_chat_reference.c"
#include "item_link_edit_reference.c"

static void equal_item(const struct ItemInfo *a, const struct ItemInfo *b)
{
    assert(a->type == b->type && a->equip_location == b->equip_location);
    assert(!strcmp(string_cstr(&a->name), string_cstr(&b->name)));
    assert(!strcmp(string_cstr(&a->id_text), string_cstr(&b->id_text)));
    assert(a->refine == b->refine && a->view == b->view && a->grade == b->grade);
    assert(a->identified == b->identified && a->option_count == b->option_count);
    assert(!memcmp(a->slot, b->slot, sizeof a->slot));
    assert(!memcmp(a->options, b->options, sizeof a->options));
}
static void compare(const char *text, int edit_safe)
{
    struct ItemInfo item, old;
    struct Vec fields = {0};
    struct String link;
    unsigned before;
    int result;
    string_init(&link); string_assign_cstr(&link, text);
    item_info_construct(&item); item_info_construct(&old);
    before = allocations;
    result = item_link_read(&item, text_view_cstr(text));
    assert(allocations <= before + 2);
    chat_parse_link(&link, &fields);
    assert(result == chat_fields_to_item(&fields, &old));
    if (result) equal_item(&item, &old);
    int_vector_destroy(&fields); item_info_destruct(&old);
    if (edit_safe) {
        item_info_construct(&old);
        edit_parse_link(&link, &fields);
        assert(result == edit_fields_to_item(&fields, &old));
        if (result) equal_item(&item, &old);
        int_vector_destroy(&fields); item_info_destruct(&old);
    }
    item_info_destruct(&item); string_free(&link); assert(!allocations);
}
static char *append_number(char *out, unsigned value, int width)
{
    static const char digits[] = "0123456789abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ";
    char reversed[16]; int n = 0;
    do { reversed[n++] = digits[value % 62]; value /= 62; } while (value);
    while (n < width) reversed[n++] = '0';
    while (n) *out++ = reversed[--n];
    *out = 0; return out;
}
static unsigned random_value(unsigned *state) { *state = *state * 1664525u + 1013904223u; return *state; }
static void generated(void)
{
    unsigned seed = 7654321, i;
    for (i = 0; i < 6000; ++i) {
        int cards = i % 5, opts = i / 5 % 6, j;
        char text[1024], *p = text;
        p = append_number(p, random_value(&seed) % 900000000u, 5);
        *p++ = i % 2 ? '1' : '0';
        p = append_number(p, random_value(&seed) & INT32_MAX, 2);
        if (i % 3) { *p++ = '%'; p = append_number(p, random_value(&seed) % 100, 2); }
        *p++ = '&'; p = append_number(p, random_value(&seed) & INT32_MAX, 2);
        *p++ = '\''; p = append_number(p, random_value(&seed) % 10, 2);
        for (j = 0; j < cards; ++j) { *p++ = ')'; p = append_number(p, random_value(&seed) & INT32_MAX, 2); }
        for (j = 0; j < opts; ++j) {
            *p++ = '+'; p = append_number(p, random_value(&seed) % 65536, 2);
            *p++ = ','; p = append_number(p, random_value(&seed) % 256, 2);
            *p++ = '-'; p = append_number(p, random_value(&seed) % 65536, 2);
        }
        compare(text, cards + 9 + opts * 3 <= 23);
    }
}
#endif
static void rejection(struct TextView text)
{
    struct ItemInfo item, before;
    item_info_construct(&item); item.count = 999;
    item_info_set_item_id(&item, 10); item_info_set_name(&item, "keep this name and owner");
    before = item;
    assert(!item_link_read(&item, text));
    assert(!memcmp(&item, &before, sizeof item));
    item_info_destruct(&item); assert(!allocations);
}
int main(void)
{
    SYSTEM_INFO info; char *memory, *end; DWORD previous;
    struct ItemInfo item;
    const char *valid = "00001110%02&03'04)05)00)00)00+06,07-08";
    size_t i, size = strlen(valid);
#ifdef ITEM_LINK_REFERENCE
    compare(valid, 1);
    compare("00001110", 1);
    compare("00001110+06,07-08", 1);
    generated();
    table.name = "An UnKnOwN ItEm description"; compare(valid, 1); rejection(text_view_cstr(valid));
    table.name = "\x81" "Unknown Item"; compare(valid, 1);
    table.name = "\x81" "unknown Item"; compare(valid, 1);
#endif
    table.name = "한국 아이템";
    item_info_construct(&item);
    assert(item_link_read(&item, text_view_cstr(valid)));
    assert(item.equip_location == 1 && item.type == 4 && item.identified == 1);
    assert(!strcmp(string_cstr(&item.id_text), "62"));
    assert(!strcmp(string_cstr(&item.name), "한국 아이템"));
    assert(item.refine == 2 && item.view == 3 && item.grade == 4);
    assert(item.slot[0] == 5 && !item.slot[1] && !item.slot[2] && !item.slot[3]);
    assert(item.option_count == 1 && item.options[0].id == 6 && item.options[0].param == 7 && item.options[0].value == 8);
    item_info_destruct(&item);
    item_info_construct(&item);
    item_info_set_name(&item, valid);
    assert(item_link_read(&item, text_view(string_cstr(&item.name), item.name.size)));
    assert(!strcmp(string_cstr(&item.name), "한국 아이템") && item.view == 3);
    item_info_destruct(&item);
    item_info_construct(&item);
    assert(item_link_read(&item, text_view_cstr("00001110%02&03'04)05)06)07)08+01,02-03+04,05-06+07,08-09+0a,0b-0c+0d,0e-0f")));
    assert(item.option_count == 5);
    for (i = 0; i < 4; ++i) assert(item.slot[i] == (int)i + 5);
    for (i = 0; i < 5; ++i) {
        assert(item.options[i].id == 1 + 3 * i);
        assert(item.options[i].param == 2 + 3 * i);
        assert(item.options[i].value == 3 + 3 * (int)i);
    }
    item_info_destruct(&item);
    item_info_construct(&item);
    assert(item_link_read(&item, text_view_cstr("00001110+06,07-08")));
    assert(!item.option_count);
    item_info_destruct(&item);
    table.name = "An UnKnOwN ItEm description"; rejection(text_view_cstr(valid));
    table.name = "한국 아이템";
    rejection(text_view("00001", 5));
    rejection(text_view("000011\0ab", 9));
    item_info_construct(&item);
    assert(item_link_read(&item, text_view_cstr("00001110)1)2)3)4)5)6+7,8-9")));
    for (i = 0; i < 4; ++i) assert(item.slot[i] == (int)i + 1);
    assert(item.option_count == 1 && item.options[0].id == 7 && item.options[0].param == 8 && item.options[0].value == 9);
    item_info_destruct(&item);
    rejection(text_view_cstr("00001110)1+1,2"));
    rejection(text_view_cstr("00001110)1+1,2-3,4"));
    rejection(text_view_cstr("00001110)1+1,2-3+1,2-3+1,2-3+1,2-3+1,2-3+1,2-3"));
    GetSystemInfo(&info);
    memory = VirtualAlloc(NULL, info.dwPageSize * 2, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE); assert(memory);
    end = memory + info.dwPageSize;
    assert(VirtualProtect(end, info.dwPageSize, PAGE_NOACCESS, &previous));
    for (i = 0; i <= size; ++i) {
        memcpy(end - i, valid, i); item_info_construct(&item);
        item_link_read(&item, text_view(end - i, i)); item_info_destruct(&item);
        assert(!allocations);
    }
    memset(memory, 'z', info.dwPageSize);
    item_info_construct(&item); assert(item_link_read(&item, text_view(memory, info.dwPageSize))); item_info_destruct(&item);
    assert(VirtualFree(memory, 0, MEM_RELEASE)); assert(!allocations);
#ifdef ITEM_LINK_REFERENCE
    puts("Item link reference: 6000 archived chat/edit comparisons passed.");
#endif
    puts("Item links: four cards/five options, extra cards ignored, Korean names, guarded prefixes, bounded rejection and atomic output passed.");
    return 0;
}
