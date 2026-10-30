#include "json/service_json.h"
#include <assert.h>
#include <stdio.h>
#include "contents/mcstore_backup.h"
#include "contents/game_session.h"
#include "std/std.h"

struct Session g_session1;

void item_info_set_item_id(struct ItemInfo *item, int id)
{
    char number[32];
    sprintf(number, "%d", id);
    string_assign_cstr(&item->id_text, number);
}

int session_shop_item_count(struct Session *session)
{
    assert(session == &g_session1);
    return 1;
}

struct ItemInfo *session_shop_item_at(struct Session *session, struct ItemInfo *out, int index)
{
    assert(session == &g_session1 && index == 0);
    memset(out, 0, sizeof *out);
    string_init(&out->id_text);
    string_init(&out->name);
    item_info_set_item_id(out, 501);
    out->count = 10;
    out->price = 100;
    out->identified = 1;
    out->option_count = 2;
    out->options[0].id = 2;
    out->options[0].value = 3;
    out->options[0].param = 4;
    out->options[1].id = 0xfff2;
    out->options[1].value = -5;
    out->options[1].param = 255;
    return out;
}

int main(void)
{
    struct JsonTree root, decoded;
    struct McStoreBackup shop;
    struct { struct McStoreItem value; unsigned char guard[16]; } guarded;
    struct OwnedText text = {0};
    unsigned i;
    memset(&shop, 0, sizeof shop);
    owned_text_init(&shop.title);
    assert(owned_text_assign(&shop.title, text_view("test shop", 9)));
    json_tree_init(&root);
    json_tree_init(&decoded);
    mcstore_write_items(&shop, &root);
    assert(json_write_request(&root, &text));
    assert(json_parse_text(&decoded, owned_text_cstr(&text), text.size));
    assert(json_value_size(json_value_member(&decoded, "Item")) == 1);
    memset(&guarded.value, 0xa5, sizeof guarded.value);
    mcstore_read_item(&guarded.value, json_value_at(json_value_member(&decoded, "Item"), 0));
    assert(guarded.value.id == 501);
    assert(guarded.value.count == 10 && guarded.value.price == 100 && guarded.value.identified == 1);
    assert(guarded.value.option_count == 2 && guarded.value.options[0].id == 2 && guarded.value.options[0].value == 3 && guarded.value.options[0].param == 4);
    assert(guarded.value.options[1].id == 0xfff2 && guarded.value.options[1].value == -5 && guarded.value.options[1].param == 255);
    {
        struct JsonTree *options = json_value_member(json_value_at(json_value_member(&decoded, "Item"), 0), "RandomOptionList");
        struct JsonTree *option = json_value_at(options, 1);
        assert(json_value_as_int(json_value_member(option, "OptIndex")) == -14);
        assert(json_value_as_int(json_value_member(option, "Value")) == -5);
    }
    owned_text_clear(&text);
    json_tree_free(&root);
    json_tree_free(&decoded);
    assert(owned_text_assign(&text, text_view_cstr("{\"RandomOptionCount\":6,\"RandomOptionList\":[{},{},{},{},{},{}]}")));
    assert(json_parse_text(&root, owned_text_cstr(&text), text.size));
    memset(guarded.guard, 0xa5, sizeof guarded.guard);
    mcstore_read_item(&guarded.value, &root);
    for (i = 0; i < sizeof guarded.guard; ++i) assert(guarded.guard[i] == 0xa5);
    assert(guarded.value.option_count == 5);
    for (i = 0; i < MCSTORE_OPTION_COUNT; ++i)
        assert(guarded.value.options[i].id == 0);
    owned_text_clear(&text);
    owned_text_clear(&shop.title);
    json_tree_free(&root);
    puts("merchant JSON: real writer/reader round trip and six-option bounds passed");
    return 0;
}
