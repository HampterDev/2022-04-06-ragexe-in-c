#include "ui/draw_key.h"
#include "ui/window_types.h"
#include "iteminfo/item_types.h"
#include "iteminfo/item_name.h"
#include "skills/skill_info.h"
#include "std/list.h"
#include "std/string.h"
#include <string.h>

unsigned long long draw_key_bytes(unsigned long long key, const void *data, size_t size)
{
    const unsigned char *p = data;
    size_t i;

    for (i = 0; i < size; i++) {
        key ^= p[i];
        key *= 0x100000001b3ull;
    }
    return key;
}

unsigned long long draw_key_int(unsigned long long key, int value)
{
    return draw_key_bytes(key, &value, sizeof value);
}

unsigned long long draw_key_text(unsigned long long key, const char *text)
{
    if (!text)
        return draw_key_int(key, -1);
    return draw_key_bytes(key, text, strlen(text) + 1);
}

unsigned long long draw_key_string(unsigned long long key, const struct String *text)
{
    key = draw_key_int(key, (int)text->size);
    return draw_key_bytes(key, string_cstr(text), text->size);
}

unsigned long long draw_key_item(unsigned long long key, const struct ItemInfo *item)
{
    struct String name;

    string_init(&name);
    item_info_get_display_name(item, &name);
    key = draw_key_string(key, &name);
    string_free(&name);
    key = draw_key_int(key, item->type);
    key = draw_key_int(key, item->index);
    key = draw_key_int(key, item->equip_location);
    key = draw_key_int(key, item->worn_location);
    key = draw_key_int(key, item->count);
    key = draw_key_int(key, item->price);
    key = draw_key_int(key, item->charged_price);
    key = draw_key_bytes(key, item->slot, sizeof item->slot);
    key = draw_key_string(key, &item->id_text);
    key = draw_key_string(key, &item->name);
    key = draw_key_int(key, item->identified);
    key = draw_key_int(key, item->damaged);
    key = draw_key_int(key, item->refine);
    key = draw_key_int(key, item->bind_on_equip_type);
    key = draw_key_int(key, item->expire_in_hours);
    key = draw_key_int(key, item->expire_time);
    key = draw_key_int(key, item->weight);
    key = draw_key_int(key, item->view);
    key = draw_key_int(key, item->favorite);
    key = draw_key_int(key, item->is_new);
    key = draw_key_int(key, item->move_pending);
    key = draw_key_int(key, item->by_index);
    key = draw_key_int(key, item->sale_kind);
    key = draw_key_int(key, item->sale_start_time);
    key = draw_key_int(key, item->sale_end_time);
    key = draw_key_int(key, item->sale_remaining);
    key = draw_key_int(key, item->grade);
    key = draw_key_int(key, item->option_count);
    return draw_key_bytes(key, item->options, sizeof item->options);
}

unsigned long long draw_key_skill(unsigned long long key, const struct SkillInfo *skill)
{
    key = draw_key_int(key, skill->found);
    key = draw_key_int(key, skill->id);
    key = draw_key_int(key, skill->type);
    key = draw_key_int(key, skill->level);
    key = draw_key_int(key, skill->sp_cost);
    key = draw_key_int(key, skill->upgradable);
    key = draw_key_int(key, skill->range);
    key = draw_key_text(key, skill->id_name);
    key = draw_key_int(key, skill->cell_index);
    key = draw_key_int(key, skill->max_level);
    key = draw_key_int(key, skill->level_limit);
    key = draw_key_int(key, skill->level2);
    return draw_key_int(key, skill->ap_amount);
}

unsigned long long draw_key_children(unsigned long long key, const struct UIWindow *window)
{
    const struct ListNode *head = window->children.head, *node;

    for (node = head->next; node != head; node = node->next) {
        const struct UIWindow *child = *LIST_VALUE(node, struct UIWindow *);

        key = draw_key_bytes(key, &child, sizeof child);
        key = draw_key_int(key, child->visible);
        if (child->visible != 1)
            continue;
        key = draw_key_int(key, child->x);
        key = draw_key_int(key, child->y);
        key = draw_key_int(key, child->width);
        key = draw_key_int(key, child->height);
        key = draw_key_children(key, child);
    }
    return key;
}
