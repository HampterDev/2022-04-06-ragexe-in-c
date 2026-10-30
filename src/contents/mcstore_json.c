#include "json/service_json.h"
#include "session/state.h"
#include <stdlib.h>
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

static int json_get_int(const struct JsonTree *obj, const char *key)
{
    return json_value_as_int(json_tree_find(obj, key, strlen(key)));
}

void mcstore_write_items(struct McStoreBackup *self, struct JsonTree *out)
{
    struct JsonTree tmp, items;
    int n, i;

    json_value_from_text(&tmp, owned_text_view(&self->title));
    json_value_assign(json_value_member(out, "Title"), &tmp);
    json_tree_free(&tmp);

    json_tree_init(&items);
    n = session_shop_item_count(&g_session1);
    for (i = 0; i < n; i++) {
        struct ItemInfo item;
        struct JsonTree obj, options, v;
        int k;

        session_shop_item_at(&g_session1, &item, i);
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
        for (k = 0; k < item.option_count; k++) {
            const struct ItemOption *opt = &item.options[k];
            struct JsonTree o;

            json_tree_init(&o);
            json_set_int(&o, "OptIndex", (short)opt->id);
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
}

void mcstore_read_item(struct McStoreItem *item, const struct JsonTree *obj)
{
    const struct JsonTree *options;
    size_t it;
    struct McStoreOption *opt;
    int k;

    memset(item, 0, sizeof *item);
    item->id = json_get_int(obj, "ID");
    item->count = json_get_int(obj, "Count");
    item->identified = json_value_as_bool(json_tree_find(obj, "Identified", 10));
    item->refine = json_get_int(obj, "RefiningLevel");
    item->slot[0] = json_get_int(obj, "Slot_1");
    item->slot[1] = json_get_int(obj, "Slot_2");
    item->slot[2] = json_get_int(obj, "Slot_3");
    item->slot[3] = json_get_int(obj, "Slot_4");
    item->option_count = json_get_int(obj, "RandomOptionCount");
    if (item->option_count < 0)
        item->option_count = 0;
    if (item->option_count > MCSTORE_OPTION_COUNT)
        item->option_count = MCSTORE_OPTION_COUNT;

    options = json_tree_find(obj, "RandomOptionList", 16);
    it = 0;
    for (k = 0; it < json_value_size(options) && k < MCSTORE_OPTION_COUNT; k++) {
        const struct JsonTree *o = options->type == JSON_TREE_ARRAY ? &options->u.items[it] : &options->u.members[it].value;

        opt = &item->options[k];
        opt->id = (unsigned short)json_get_int(o, "OptIndex");
        opt->value = (short)json_get_int(o, "Value");
        opt->param = (unsigned char)json_get_int(o, "Parm1");
        ++it;
    }
    item->price = json_get_int(obj, "Price");
}
