#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static unsigned allocations, releases, live;
static int fail_allocation;
static void *node_alloc(size_t bytes)
{
    void *p;
    ++allocations;
    if (fail_allocation)
        return NULL;
    p = malloc(bytes);
    assert(p);
    ++live;
    return p;
}
static void node_free(void *p)
{
    assert(p && live);
    --live;
    ++releases;
    free(p);
}
#define malloc node_alloc
#define free node_free
#include "../src/ui/items/item_tag_list.c"
#undef malloc
#undef free

static void expect(const struct ItemTagList *list, struct ItemTagNode **nodes, size_t count)
{
    struct ItemTagNode *node = list->head, *previous = NULL;
    size_t i;
    assert(list->size == count);
    for (i = 0; i < count; ++i) {
        assert(node == nodes[i] && node->prev == previous);
        previous = node;
        node = node->next;
    }
    assert(!node);
    for (i = count; i; --i) {
        assert(previous == nodes[i - 1]);
        previous = previous->prev;
    }
    assert(!previous);
}

int main(void)
{
    struct ItemTagList list = {0}, ordered = {0};
    struct ItemTagPayload payload = {0};
    struct ItemTagNode *nodes[4], *expected[4];
    unsigned before_alloc, before_free;
    size_t i;

    expect(&list, NULL, 0);
    strcpy(payload.code, "12");
    strcpy(payload.name, "한국 item");
    strcpy(payload.link, "0000112&00'00");
    for (i = 0; i < 4; ++i) {
        payload.order = (int)i;
        nodes[i] = item_tag_list_append(&list, &payload);
        assert(nodes[i] && memcmp(&nodes[i]->tag, &payload, sizeof payload) == 0);
    }
    expect(&list, nodes, 4);
    payload.name[0] = '!';
    assert(nodes[0]->tag.name[0] != '!');
    fail_allocation = 1;
    assert(!item_tag_list_append(&list, &nodes[0]->tag));
    expect(&list, nodes, 4);
    before_alloc = allocations;
    before_free = releases;
    expected[0] = nodes[1]; expected[1] = nodes[3];
    expected[2] = nodes[0]; expected[3] = nodes[2];
    for (i = 0; i < 4; ++i) {
        item_tag_list_move(&ordered, &list, expected[i]);
        expect(&ordered, expected, i + 1);
        assert(list.size == 3 - i);
    }
    expect(&list, NULL, 0);
    assert(allocations == before_alloc && releases == before_free && live == 4);
    assert(item_tag_list_erase(&ordered, nodes[0]) == nodes[2]);
    assert(item_tag_list_erase(&ordered, nodes[1]) == nodes[3]);
    assert(item_tag_list_erase(&ordered, nodes[2]) == NULL);
    expected[0] = nodes[3];
    expect(&ordered, expected, 1);
    assert(item_tag_list_erase(&ordered, nodes[3]) == NULL);
    expect(&ordered, NULL, 0);
    assert(!live && releases == 4);
    fail_allocation = 0;
    nodes[0] = item_tag_list_append(&list, &payload);
    nodes[1] = item_tag_list_append(&list, &nodes[0]->tag);
    assert(nodes[1] && memcmp(&nodes[0]->tag, &nodes[1]->tag, sizeof payload) == 0);
    assert(item_tag_list_erase(&list, nodes[0]) == nodes[1]);
    assert(item_tag_list_erase(&list, nodes[1]) == NULL);
    expect(&list, NULL, 0);
    assert(!live);
    puts("Item tag list: ownership, payload copies, allocation failure and allocation-free reordering passed.");
    return 0;
}
