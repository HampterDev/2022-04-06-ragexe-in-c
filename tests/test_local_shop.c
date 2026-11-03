#include "contents/mcstore_backup.h"
#include "contents/settings_file.h"
#include "session/state.h"
#include "session/session_items.h"
#include "contents/game_session.h"
#include "json/service_json.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

struct Session g_session1;
char g_clientDir[0x88];
const char *g_serverAddress = "one.example", *g_serverPort = "6900";
UINT g_codePage = 949;
static int item_count = 1, option_count = 2, fail_write, fail_flush, fail_move;
static BOOL WINAPI write_file(HANDLE h, LPCVOID p, DWORD n, LPDWORD out, LPOVERLAPPED o)
{ return fail_write ? FALSE : WriteFile(h, p, n, out, o); }
static BOOL WINAPI flush_file(HANDLE h)
{ return fail_flush ? FALSE : FlushFileBuffers(h); }
static BOOL WINAPI move_file(LPCSTR a, LPCSTR b, DWORD flags)
{ return fail_move ? FALSE : MoveFileExA(a, b, flags); }
#define WriteFile write_file
#define FlushFileBuffers flush_file
#define MoveFileExA move_file
#include "../src/contents/settings_file.c"
#include "../src/platform/filesystem_win32.c"
#undef WriteFile
#undef FlushFileBuffers
#undef MoveFileExA
#include "../src/contents/mcstore_local.c"

void *mem_alloc(size_t n) { void *p = malloc(n ? n : 1); assert(p); return p; }
void mem_free(void *p) { free(p); }
void fatal_error(const char *m) { (void)m; abort(); }

int session_shop_item_count(struct Session *s) { assert(s == &g_session1); return item_count; }
struct ItemInfo *session_shop_item_at(struct Session *s, struct ItemInfo *out, int index)
{
    int i;
    assert(s == &g_session1 && index >= 0 && index < item_count);
    item_info_construct(out); item_info_set_item_id(out, 501 + index);
    out->count = 10 + index; out->price = 1000 + index; out->identified = 1; out->refine = index;
    out->slot[0] = -1; out->slot[1] = 4001;
    out->option_count = option_count;
    for (i = 0; i < option_count && i < 5; ++i) {
        out->options[i].id = (unsigned short)(65522 + i);
        out->options[i].value = (short)(-5 + i); out->options[i].param = 255;
    }
    return out;
}
struct InventoryMatch *session_find_inventory_item(struct Session *s, struct InventoryMatch *out, const struct ItemInfo *item)
{ (void)s; (void)out; (void)item; abort(); }
int session_get_store_setup_count(struct Session *s, int index)
{ (void)s; (void)index; abort(); }
unsigned char session_sell_list_add(struct Session *s, const struct ItemInfo *i, unsigned char m, unsigned char n)
{ (void)s; (void)i; (void)m; (void)n; abort(); }
unsigned char session_mirror_remove(struct Session *s, const struct ItemInfo *i, int m, unsigned char n)
{ (void)s; (void)i; (void)m; (void)n; abort(); }

static char paths[24][MAX_PATH];
static int path_count;
static const char *current_path(int type)
{
    char path[MAX_PATH];
    int i;
    assert(file_path(path, g_session1.ownAid, g_session1.ownGid, string_cstr(&g_session1.worldName), type));
    for (i = 0; i < path_count; ++i) if (!strcmp(paths[i], path)) return paths[i];
    assert(path_count < 24); strcpy(paths[path_count], path); return paths[path_count++];
}
static void title(const char *text)
{ assert(owned_text_assign(&g_session1.mcStoreBackup.title, text_view_cstr(text))); }
static int load(int type)
{
    return mcstore_request_load(&g_session1.mcStoreBackup, g_session1.ownAid, g_session1.ownGid,
        string_cstr(&g_session1.worldName), type);
}
static void expect(const char *name, size_t count)
{
    struct McStoreBackup *shop = &g_session1.mcStoreBackup;
    assert(!strcmp(owned_text_cstr(&shop->title), name) && shop->item_count == count);
    if (count) {
        assert(shop->items[0].id == 501 && shop->items[0].count == 10 && shop->items[0].price == 1000);
        assert(shop->items[0].slot[0] == -1 && shop->items[0].slot[1] == 4001);
        assert(shop->items[0].options[0].id == 65522 && shop->items[0].options[0].value == -5);
    }
}
static void cleanup(void)
{
    char path[MAX_PATH], *p;
    size_t length = strlen(g_clientDir);
    int i;
    for (i = 0; i < path_count; ++i) {
        assert(!strncmp(paths[i], g_clientDir, length) && paths[i][length] == '\\');
        DeleteFileA(paths[i]);
    }
    for (i = 0; i < path_count; ++i) {
        strcpy(path, paths[i]);
        while ((p = strrchr(path, '\\')) != NULL && (size_t)(p - path) > length) {
            *p = 0; RemoveDirectoryA(path);
        }
    }
    assert(RemoveDirectoryA(g_clientDir));
}
int main(void)
{
    struct McStoreBackup *shop = &g_session1.mcStoreBackup;
    struct OwnedText before = {0}, after = {0};
    struct JsonTree root, invalid;
    struct JsonTree *entry;
    char parent[MAX_PATH], temporary[MAX_PATH], first[MAX_PATH], buying[MAX_PATH];
    int i;
    assert(GetTempPathA(sizeof parent, parent));
    assert(GetTempFileNameA(parent, "shp", 0, temporary));
    assert(DeleteFileA(temporary) && CreateDirectoryA(temporary, NULL));
    assert(strlen(temporary) < sizeof g_clientDir); strcpy(g_clientDir, temporary);
    string_init(&g_session1.worldName); string_assign_cstr(&g_session1.worldName, "World/..\\이름");
    g_session1.ownAid = 101; g_session1.ownGid = 201;
    title("unchanged"); assert(load(0) == 2); expect("unchanged", 0);
    title("상점\\\"line\n"); strcpy(first, current_path(0));
    assert(mcstore_request_save(shop, 0) && settings_file_read(first, &before) == 1);
    assert(strstr(before.data, "\xec\x83\x81\xec\xa0\x90") && !strstr(before.data, "AuthToken"));
    title("buying"); item_count = 2; strcpy(buying, current_path(1));
    assert(strcmp(first, buying) && mcstore_request_save(shop, 1));
    assert(load(0) == 1); expect("상점\\\"line\n", 1);
    assert(load(1) == 1); expect("buying", 2);
    mcstore_destroy(shop); assert(load(0) == 1); expect("상점\\\"line\n", 1);
    title("new title"); item_count = 1;
    for (i = 0; i < 3; ++i) {
        fail_write = i == 0; fail_flush = i == 1; fail_move = i == 2;
        assert(!mcstore_request_save(shop, 0));
        assert(settings_file_read(first, &after) == 1 && before.size == after.size && !memcmp(before.data, after.data, before.size));
    }
    fail_write = fail_flush = fail_move = 0;
    g_session1.ownGid = 202; assert(load(0) == 2); current_path(0); assert(mcstore_request_save(shop, 0));
    g_session1.ownAid = 102; assert(load(0) == 2); current_path(0); assert(mcstore_request_save(shop, 0));
    g_session1.ownAid = 101; g_session1.ownGid = 201;
    g_serverAddress = "two.example"; assert(load(0) == 2); current_path(0); assert(mcstore_request_save(shop, 0));
    g_serverAddress = "one.example"; g_serverPort = "6901"; assert(load(0) == 2); current_path(0); assert(mcstore_request_save(shop, 0));
    g_serverPort = "6900"; string_assign_cstr(&g_session1.worldName, "another world");
    assert(load(0) == 2); current_path(0); assert(mcstore_request_save(shop, 0));
    string_assign_cstr(&g_session1.worldName, "World/..\\이름"); assert(load(0) == 1); expect("상점\\\"line\n", 1);
    json_tree_init(&root); assert(json_parse_text(&root, before.data, before.size));
    for (i = 0; i < 5; ++i) {
        json_value_copy(&invalid, &root);
        entry = json_value_at(json_value_member(json_value_member(&invalid, "data"), "Item"), 0);
        if (i == 0) json_value_member(&invalid, "version")->u.u64 = 99;
        if (i == 1) json_value_member(entry, "Count")->u.i64 = -1;
        if (i == 2) json_value_member(entry, "RandomOptionCount")->u.i64 = 6;
        if (i == 3) json_value_member(json_value_member(&invalid, "data"), "Title")->u.text[0] = (char)0xff;
        if (i == 4) json_value_member(json_value_member(&invalid, "data"), "Title")->u.text[0] = 0;
        assert(json_write_request(&invalid, &after));
        assert(settings_file_write(first, owned_text_view(&after)));
        assert(load(0) == 0); expect("상점\\\"line\n", 1);
        assert(!mcstore_request_save(shop, 0));
        json_tree_free(&invalid);
    }
    assert(settings_file_write(first, text_view_cstr("{truncated")));
    assert(!mcstore_request_save(shop, 0) && !load(0));
    assert(settings_file_read(first, &after) == 1 && !strcmp(after.data, "{truncated"));
    assert(settings_file_write(first, owned_text_view(&before)));
    item_count = 14; assert(!mcstore_request_save(shop, 0));
    item_count = 6; assert(!mcstore_request_save(shop, 1));
    item_count = 1; option_count = 6; assert(!mcstore_request_save(shop, 0));
    option_count = 5; assert(mcstore_request_save(shop, 0) && load(0) == 1);
    assert(shop->items[0].option_count == 5 && shop->items[0].options[4].id == 65526);
    item_count = 0; title(""); assert(mcstore_request_save(shop, 0) && load(0) == 1); expect("", 0);
    assert(load(1) == 1); expect("buying", 2);
    assert(!mcstore_request_save(shop, 2));
    g_session1.ownGid = 0; assert(!mcstore_request_save(shop, 0) && !load(0));
    g_session1.ownGid = 201; g_session1.ownAid = 0; assert(!mcstore_request_save(shop, 0) && !load(0));
    mcstore_destroy(shop); string_free(&g_session1.worldName);
    owned_text_clear(&before); owned_text_clear(&after); json_tree_free(&root); cleanup();
    puts("Local shops: UTF-8 round trips, vending/buying and identity isolation, empty snapshots, strict validation, corrupt-file preservation and write/flush/replace failures passed; no HTTP linked");
    return 0;
}
