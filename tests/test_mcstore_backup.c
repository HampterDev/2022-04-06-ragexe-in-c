#include "contents/mcstore_backup.h"
#include "contents/game_session.h"
#include "ui/window_manager_state.h"
#include "json/service_json.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static struct { void *pointer; size_t size; } blocks[64];
static int attempts, fail_at, live;

static void *checked_malloc(size_t size)
{
    int i;
    unsigned char *p;
    if (++attempts == fail_at) return NULL;
    for (i = 0; i < 64 && blocks[i].pointer; ++i) {}
    assert(i < 64 && size <= SIZE_MAX - 16);
    p = malloc(size + 16); assert(p);
    memset(p + size, 0xa7, 16);
    blocks[i].pointer = p; blocks[i].size = size; ++live;
    return p;
}

static void checked_free(void *p)
{
    int i, n;
    if (!p) return;
    for (i = 0; i < 64 && blocks[i].pointer != p; ++i) {}
    assert(i < 64);
    for (n = 0; n < 16; ++n) assert(((unsigned char *)p)[blocks[i].size + n] == 0xa7);
    blocks[i].pointer = NULL; --live; free(p);
}

#define malloc checked_malloc
#define free checked_free
#include "../src/core/owned_text.c"
#include "../src/contents/mcstore_backup.c"
#undef malloc
#undef free

struct Session g_session1;
struct UIWindowMgr g_windowMgr;
static int notices, messages, last_message, last_text, address_enabled = 1;
static struct UIMerchantShopMakeWnd *notified;
static int queries, sold, mirrored, expected_mode;
static int seen[16], sold_count[16], sold_index[16];

void ui_merchant_shop_make_wnd_apply_backup(struct UIMerchantShopMakeWnd *window)
{
    ++notices; notified = window;
}

int session_service_urls(struct Session *session, struct OwnedText *save, const char *save_path,
    struct OwnedText *load, const char *load_path)
{
    assert(session == &g_session1);
    assert(!strcmp(save_path, "/MerchantStore/save") && !strcmp(load_path, "/MerchantStore/load"));
    return address_enabled && owned_text_assign(save, text_view_cstr("save-url"))
        && owned_text_assign(load, text_view_cstr("load-url"));
}

char *msg_str(int id) { last_text = id; return "saved"; }
int ui_window_mgr_send_msg_(struct UIWindowMgr *self, int msg, int a, int b, int c, int d)
{
    (void)a; (void)b; (void)c; (void)d;
    assert(self == &g_windowMgr); ++messages; last_message = msg; return 0;
}

struct InventoryMatch *session_find_inventory_item(struct Session *session,
    struct InventoryMatch *out, const struct ItemInfo *item)
{
    int id = atoi(string_cstr(&item->id_text));
    assert(session == &g_session1);
    seen[queries++] = id;
    assert(item->refine == id + 1 && item->identified == 1);
    assert(item->slot[3] == 400 + id && item->option_count == 1);
    assert(item->options[0].id == 0xfff2 && item->options[0].value == -5);
    assert(item->options[4].param == 255);
    item_info_construct(&out->item);
    out->found = id != 1;
    out->item.index = id == 3 ? 0 : id;
    out->item.count = id == 4 ? 2 : 20;
    return out;
}

unsigned char session_sell_list_add(struct Session *session, const struct ItemInfo *item,
    unsigned char mode, unsigned char notify)
{
    assert(session == &g_session1 && mode == expected_mode && !notify);
    sold_count[sold] = item->count; sold_index[sold++] = item->index; return 1;
}

unsigned char session_mirror_remove(struct Session *session, const struct ItemInfo *item,
    int mode, unsigned char notify)
{
    assert(session == &g_session1 && mode == expected_mode && !notify);
    assert(item->count == sold_count[mirrored] && item->index == sold_index[mirrored]);
    ++mirrored; return 1;
}

int session_shop_item_count(struct Session *session)
{
    (void)session; assert(0); return 0;
}
struct ItemInfo *session_shop_item_at(struct Session *session, struct ItemInfo *out, int index)
{
    (void)session; (void)out; (void)index; assert(0); return NULL;
}

static void parse(struct JsonTree *tree, const char *text)
{
    json_tree_init(tree); assert(json_parse_text(tree, text, strlen(text)));
}

int session_get_store_setup_count(struct Session *session, int index)
{
    int i;
    assert(session == &g_session1);
    for (i = 0; i < sold; ++i) if (sold_index[i] == index) return sold_count[i];
    return 0;
}

static void load_tests(void)
{
    struct McStoreBackup shop;
    struct JsonTree data, old;
    struct McStoreItem *saved_items;
    char *saved_title;
    int failure, old_live;
    mcstore_init(&shop);
    parse(&old, "{\"Title\":\"old\",\"Item\":[{\"ID\":99}]}");
    parse(&data, "{\"Title\":\"상점\\u0000tail\",\"Item\":{\"z\":{\"ID\":2},\"a\":{\"ID\":1},\"b\":null}}");
    g_windowMgr.merchant_item_wnd = (struct UIWindow *)1;
    g_windowMgr.hot_key_wnd = (struct UIWindow *)2;
    assert(mcstore_read_items(&shop, &old));
    assert(notices == 1 && notified == (struct UIMerchantShopMakeWnd *)1);
    saved_items = shop.items; saved_title = shop.title.data; old_live = live;
    shop.loaded = 1; shop.saves_pending = 7; shop.save_type = 9;
    for (failure = 1; failure <= 2; ++failure) {
        attempts = 0; fail_at = failure;
        assert(!mcstore_read_items(&shop, &data));
        assert(shop.items == saved_items && shop.title.data == saved_title && shop.items[0].id == 99);
        assert(shop.loaded == 1 && shop.saves_pending == 7 && shop.save_type == 9);
        assert(live == old_live && notices == 1);
    }
    attempts = fail_at = 0;
    assert(mcstore_read_items(&shop, &data) && attempts == 2);
    assert(shop.item_count == 2 && shop.items[0].id == 1 && shop.items[1].id == 2);
    assert(shop.title.size == sizeof "상점\0tail" - 1);
    assert(!memcmp(shop.title.data, "상점\0tail", shop.title.size));
    json_tree_free(&data);
    assert(shop.items[1].id == 2 && shop.title.size > strlen(shop.title.data));
    parse(&data, "{\"Item\":[null,4,{},{}],\"Title\":true}");
    g_windowMgr.merchant_item_wnd = NULL;
    assert(mcstore_read_items(&shop, &data));
    assert(notified == (struct UIMerchantShopMakeWnd *)2 && !strcmp(shop.title.data, "true"));
    assert(shop.item_count == 2 && !shop.items[0].id);
    json_tree_free(&data);
    parse(&data, "{}");
    failure = notices;
    assert(mcstore_read_items(&shop, &data));
    assert(!shop.item_count && !shop.items && !shop.title.size && notices == failure);
    json_tree_free(&data); json_tree_free(&old);
    mcstore_destroy(&shop); mcstore_destroy(&shop); assert(!live);
}

static void answer_reset_tests(void)
{
    struct McStoreBackup shop;
    const char *response = "{\"Type\":1,\"data\":{\"Title\":\"shop\",\"Item\":[{}]}}";
    char *save, *load;
    int failure;
    mcstore_init(&shop);
    g_windowMgr.merchant_item_wnd = g_windowMgr.hot_key_wnd = NULL;
    for (failure = 1; failure <= 2; ++failure) {
        attempts = 0; fail_at = failure;
        mcstore_on_loaded(&shop, 1, text_view_cstr(response));
        assert(!shop.loaded && !shop.item_count && !shop.title.size && !live);
    }
    fail_at = 0;
    mcstore_on_loaded(&shop, 0, text_view_cstr(response));
    assert(!shop.loaded);
    mcstore_on_loaded(&shop, 1, text_view_cstr(response));
    assert(shop.loaded && shop.item_count == 1);
    mcstore_on_loaded(&shop, 1, text_view_cstr("{\"Type\":4}"));
    assert(shop.loaded && shop.item_count == 1 && !strcmp(shop.title.data, "shop"));
    shop.saves_pending = 4; shop.save_type = 1;
    mcstore_on_saved(&shop, 0, text_view_cstr("{}"));
    mcstore_on_saved(&shop, 1, text_view_cstr("invalid"));
    assert(shop.saves_pending == 2 && !messages);
    mcstore_on_saved(&shop, 1, text_view_cstr("{\"Type\":1}"));
    assert(shop.saves_pending == 1 && messages == 2 && last_message == 1 && last_text == 0xe66);
    shop.save_type = 0;
    mcstore_on_saved(&shop, 1, text_view_cstr("{\"Type\":1}"));
    assert(!shop.saves_pending && messages == 4 && last_text == 0xe65);
    shop.saves_pending = 7; shop.save_type = 9;
    mcstore_reset(&shop);
    assert(!shop.loaded && !shop.items && !shop.title.size);
    assert(shop.saves_pending == 7 && shop.save_type == 9);
    assert(!strcmp(shop.save_url.data, "save-url") && !strcmp(shop.load_url.data, "load-url"));
    save = shop.save_url.data; load = shop.load_url.data;
    for (failure = 1; failure <= 2; ++failure) {
        attempts = 0; fail_at = failure; mcstore_reset(&shop);
        assert(shop.save_url.data == save && shop.load_url.data == load && live == 2);
    }
    fail_at = 0; address_enabled = 0; mcstore_reset(&shop);
    assert(shop.save_url.data == save && shop.load_url.data == load);
    mcstore_destroy(&shop); assert(!live);
}

static void restore_tests(void)
{
    struct McStoreBackup shop;
    int mode, i;
    for (mode = 0; mode <= 1; ++mode) {
        mcstore_init(&shop);
        shop.items = checked_malloc(4 * sizeof *shop.items); shop.item_count = 4;
        memset(shop.items, 0, 4 * sizeof *shop.items);
        for (i = 0; i < 4; ++i) {
            shop.items[i].id = i + 1; shop.items[i].count = 5; shop.items[i].price = (i + 1) * 100;
            shop.items[i].refine = i + 2; shop.items[i].identified = 1;
            shop.items[i].slot[3] = 401 + i; shop.items[i].option_count = 1;
            shop.items[i].options[0].id = 0xfff2; shop.items[i].options[0].value = -5;
            shop.items[i].options[4].param = 255;
        }
        queries = sold = mirrored = 0; expected_mode = mode;
        mcstore_restore_shop(&shop, (char)mode, 0); mcstore_restore_shop(&shop, (char)mode, -1);
        assert(!queries);
        mcstore_restore_shop(&shop, (char)mode, 2);
        assert(queries == 4 && seen[0] == 1 && seen[3] == 4);
        assert(shop.item_count == 2 && shop.items[0].id == 2 && shop.items[1].id == 4);
        assert(shop.items[0].price == 200 && shop.items[1].price == 400);
        assert(shop.items[1].count == (mode ? 5 : 2));
        assert(sold == 2 && mirrored == 2 && sold_index[0] == 2 && sold_index[1] == 4 && sold_count[0] == (mode ? 20 : 5));
        mcstore_clear_items(&shop); mcstore_restore_shop(&shop, (char)mode, 9);
        assert(queries == 4); mcstore_destroy(&shop); assert(!live);
    }
}

int main(void)
{
    load_tests(); answer_reset_tests(); restore_tests();
    puts("Merchant backup: owned snapshots, allocation rollback, counted Korean title, restore ordering and request counters passed");
    return 0;
}
