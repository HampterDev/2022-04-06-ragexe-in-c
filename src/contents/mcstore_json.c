#include "json/service_json.h"
#include "session/state.h"
#include <stdlib.h>
#include <limits.h>
#include <string.h>
#include "contents/mcstore_backup.h"
#include "contents/game_session.h"
#include "std/string.h"

static void json_set_int(struct JsonTree *obj, const char *key, int value)
{
    struct JsonTree v;

    json_value_from_int(&v, value);
    json_value_assign(json_value_member(obj, key), &v);
    json_tree_free(&v);
}

int mcstore_write_items(struct McStoreBackup *self, struct JsonTree *out)
{
    struct JsonTree tmp, items;
    int n, i;

    json_value_from_text(&tmp, owned_text_view(&self->title));
    json_value_assign(json_value_member(out, "Title"), &tmp);
    json_tree_free(&tmp);

    n = session_shop_item_count(&g_session1);
    if (n < 0 || n > 13) return 0;
    json_tree_init(&items);
    items.type = JSON_TREE_ARRAY;
    for (i = 0; i < n; i++) {
        struct ItemInfo item;
        struct JsonTree obj, options, v;
        int k;

        session_shop_item_at(&g_session1, &item, i);
        if (item.option_count < 0 || item.option_count > MCSTORE_OPTION_COUNT) {
            item_info_destruct(&item);
            json_tree_free(&items);
            return 0;
        }
        json_tree_init(&obj);
        json_set_int(&obj, "ID", atoi(string_cstr(&item.id_text)));
        json_set_int(&obj, "Count", item.count);
        json_value_from_bool(&v, item.identified);
        json_value_assign(json_value_member(&obj, "Identified"), &v);
        json_tree_free(&v);
        json_set_int(&obj, "RefiningLevel", item.refine);
        json_set_int(&obj, "Slot_1", item.slot[0]);
        json_set_int(&obj, "Slot_2", item.slot[1]);
        json_set_int(&obj, "Slot_3", item.slot[2]);
        json_set_int(&obj, "Slot_4", item.slot[3]);
        json_set_int(&obj, "RandomOptionCount", item.option_count);

        json_tree_init(&options);
        options.type = JSON_TREE_ARRAY;
        for (k = 0; k < item.option_count; k++) {
            const struct ItemOption *opt = &item.options[k];
            struct JsonTree o;

            json_tree_init(&o);
            json_set_int(&o, "OptIndex", opt->id);
            json_set_int(&o, "Value", opt->value);
            json_set_int(&o, "Parm1", opt->param);
            json_value_append(&options, &o);
            json_tree_free(&o);
        }
        json_value_assign(json_value_member(&obj, "RandomOptionList"), &options);
        json_set_int(&obj, "Price", item.price);
        json_value_append(&items, &obj);
        json_tree_free(&options);
        json_tree_free(&obj);
        item_info_destruct(&item);
    }
    json_value_assign(json_value_member(out, "Item"), &items);
    json_tree_free(&items);
    return 1;
}

static int read_int(const struct JsonTree *obj, const char *key, int min, int max, int *out)
{
    const struct JsonTree *v = json_tree_find(obj, key, strlen(key));
    if (!v || (v->type != JSON_TREE_INT64 && v->type != JSON_TREE_UINT64) ||
        (v->type == JSON_TREE_UINT64 && v->u.u64 > (unsigned)max) ||
        v->u.i64 < min || v->u.i64 > max) return 0;
    *out = (int)v->u.i64;
    return 1;
}

int mcstore_read_item(struct McStoreItem *item, const struct JsonTree *obj)
{
    struct McStoreItem next = {0};
    const struct JsonTree *options, *identified;
    static const char *const slots[] = {"Slot_1", "Slot_2", "Slot_3", "Slot_4"};
    int k, id, value, param;
    if (!obj || obj->type != JSON_TREE_OBJECT ||
        !read_int(obj, "ID", 1, INT_MAX, &next.id) ||
        !read_int(obj, "Count", 1, INT_MAX, &next.count) ||
        !read_int(obj, "Price", 0, INT_MAX, &next.price) ||
        !read_int(obj, "RefiningLevel", 0, 255, &next.refine) ||
        !read_int(obj, "RandomOptionCount", 0, MCSTORE_OPTION_COUNT, &next.option_count)) return 0;
    identified = json_tree_find(obj, "Identified", 10);
    if (!identified || identified->type != JSON_TREE_BOOL) return 0;
    next.identified = identified->boolean != 0;
    for (k = 0; k < MCSTORE_CARD_COUNT; ++k)
        if (!read_int(obj, slots[k], INT_MIN, INT_MAX, &next.slot[k])) return 0;
    options = json_tree_find(obj, "RandomOptionList", 16);
    if (!options || options->type != JSON_TREE_ARRAY || options->count != (size_t)next.option_count) return 0;
    for (k = 0; k < next.option_count; ++k) {
        const struct JsonTree *o = &options->u.items[k];
        if (o->type != JSON_TREE_OBJECT || !read_int(o, "OptIndex", 0, 65535, &id) ||
            !read_int(o, "Value", -32768, 32767, &value) || !read_int(o, "Parm1", 0, 255, &param)) return 0;
        next.options[k].id = (uint16_t)id;
        next.options[k].value = (int16_t)value;
        next.options[k].param = (uint8_t)param;
    }
    *item = next;
    return 1;
}
