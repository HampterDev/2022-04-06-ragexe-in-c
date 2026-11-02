#include "ui/hotkeys/hotkey_bars.h"
#include "iteminfo/item_api.h"
#include "std/vec.h"
#include "game/item_info.h"
#include "iteminfo/iteminfo.h"
#include "iteminfo/item_list.h"
#include <assert.h>
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

static void item_list_ownership(void)
{
    struct List list = {0};
    struct ItemInfo item = {0};
    struct ListNode *head, *node;
    const char *name = "independently owned item text";
    unsigned original = allocations;

    list_free(&list, item_info_destroy);
    list_init(&list);
    head = list.head;
    assert(!list.size && head->next == head && head->prev == head && allocations == original + 1);
    list_clear(&list, item_info_destroy);
    item_info_construct(&item);
    string_assign_cstr(&item.name, name);
    string_assign_cstr(&item.id_text, "00000000000000000501");
    item.index = 1;
    item_info_copy(list_push_back(&list, NULL, sizeof item), &item);
    item.index = 2;
    item_info_copy(list_push_back(&list, NULL, sizeof item), &item);
    item.index = 3;
    item_info_copy(list_push_back(&list, NULL, sizeof item), &item);
    assert(list.size == 3);
    assert(LIST_VALUE(head->next, struct ItemInfo)->name.data != item.name.data);
    assert(LIST_VALUE(head->next, struct ItemInfo)->name.data != LIST_VALUE(head->prev, struct ItemInfo)->name.data);
    assert(LIST_VALUE(head->next, struct ItemInfo)->id_text.data != item.id_text.data);
    item_info_destruct(&item);
    assert(!strcmp(string_cstr(&LIST_VALUE(head->next, struct ItemInfo)->name), name));
    node = list_erase(&list, head->next->next, item_info_destroy);
    assert(list.size == 2 && LIST_VALUE(node, struct ItemInfo)->index == 3);
    assert(LIST_VALUE(node->prev, struct ItemInfo)->index == 1);
    assert(node == head->prev && node->prev->next == node);
    node = list_erase(&list, head->next, item_info_destroy);
    assert(list.size == 1 && node == head->next && node->prev == head);
    assert(list_erase(&list, node, item_info_destroy) == head);
    assert(!list.size && head->next == head && head->prev == head);
    assert(allocations == original + 1);

    item_info_construct(&item);
    string_assign_cstr(&item.name, name);
    item_info_copy(list_push_back(&list, NULL, sizeof item), &item);
    item_info_copy(list_push_back(&list, NULL, sizeof item), &item);
    item_info_destruct(&item);
    list_clear(&list, item_info_destroy);
    assert(list.head == head && !list.size && head->next == head && head->prev == head);
    assert(allocations == original + 1);
    list_free(&list, item_info_destroy);
    list_free(&list, item_info_destroy);
    assert(!list.head && !list.size && allocations == original);
}

int main(void)
{
    struct {
        struct ItemInfo item[2];
        unsigned guard;
    } pair;
    struct ItemInfo copy;
    const char *name = "긴 이름의 검증용 장비 아이템";
    const char *id = "00000000000000000000000000000501";
    unsigned char untouched[sizeof(struct ItemInfo)];

    memset(&pair, 0xa5, sizeof pair);
    memset(untouched, 0xa5, sizeof untouched);
    item_info_construct(&pair.item[0]);
    assert(!memcmp(&pair.item[1], untouched, sizeof untouched));
    assert(pair.guard == 0xa5a5a5a5u);
    assert(pair.item[0].index == 0 && pair.item[0].option_count == 0);
    assert(!strcmp(string_cstr(&pair.item[0].name), ""));
    item_info_construct(&pair.item[1]);
    pair.item[0].type = 5;
    pair.item[0].index = 17;
    pair.item[0].equip_location = 0x200;
    pair.item[0].count = 3;
    pair.item[0].price = 12345;
    pair.item[0].charged_price = 12000;
    pair.item[0].slot[0] = 501;
    pair.item[0].identified = 1;
    pair.item[0].refine = 7;
    pair.item[0].bind_on_equip_type = 258;
    pair.item[0].expire_time = 123456;
    pair.item[0].view = 42;
    pair.item[0].option_count = 1;
    pair.item[0].options[0].id = 0xfff2;
    pair.item[0].options[0].value = -5;
    pair.item[0].options[0].param = 255;
    string_assign_cstr(&pair.item[0].id_text, id);
    string_assign_cstr(&pair.item[0].name, name);

    item_info_assign(&pair.item[1], &pair.item[0]);
    {
        unsigned char before[sizeof pair.item[1]];
        memcpy(before, &pair.item[1], sizeof before);
        item_info_assign(&pair.item[1], &pair.item[1]);
        assert(memcmp(before, &pair.item[1], sizeof before) == 0);
    }
    memset(&copy, 0xcc, sizeof copy);
    item_info_copy(&copy, &pair.item[0]);
    assert(pair.item[1].name.data != pair.item[0].name.data);
    assert(copy.name.data != pair.item[0].name.data);
    assert(copy.id_text.data != pair.item[0].id_text.data);
    assert(copy.options[0].id == 0xfff2 && copy.options[0].value == -5);
    assert(copy.options[0].param == 255);
    assert(copy.guild_log.id == pair.item[0].guild_log.id);
    assert(copy.guild_log.amount == pair.item[0].guild_log.amount);
    assert(copy.guild_log.action == pair.item[0].guild_log.action);
    assert(!memcmp(copy.guild_log.name, pair.item[0].guild_log.name, sizeof copy.guild_log.name));
    assert(!memcmp(copy.guild_log.time, pair.item[0].guild_log.time, sizeof copy.guild_log.time));
    string_assign_cstr(&pair.item[0].name, "changed");
    assert(!strcmp(string_cstr(&copy.name), name));
    assert(!strcmp(string_cstr(&pair.item[1].name), name));

    item_info_destruct(&copy);
    item_info_destruct(&pair.item[1]);
    item_info_destruct(&pair.item[0]);
    assert(pair.guard == 0xa5a5a5a5u && !allocations);
    item_list_ownership();
    puts("item records: owned lists, adjacent storage, deep copies, padding and CP949 names passed");
    return 0;
}
