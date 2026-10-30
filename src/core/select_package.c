#include "iteminfo/item_api.h"
#include "contents/client_lua.h"
#include "iteminfo/item_types.h"
#include "core/memory.h"
#include "core/fatal.h"
#include <string.h>
#include <windows.h>
#include "std/std.h"
#include "core/app.h"
#include "std/vec.h"
#include "core/msg_str.h"
#include "ui/ui_items.h"
#include "core/select_package.h"

struct SelectPackageMgr {
    unsigned char loaded;
    struct Tree packages;
};
SIZE_ASSERT(SelectPackageMgr, 0xc);

struct PackageSlot {
    unsigned id;
    struct Package *package;
};

#define PACKAGE_MAP_LIMIT 10000

struct SelectPackageMgr *g_selectPackageMgr;

void package_entry_add_item(struct PackageEntry *self, int item_id, int count,
                                       int expire_time, int refine, int title)
{
    struct ItemInfo item;

    item_info_construct(&item);
    item_info_clear(&item);
    item_info_set_item_id(&item, item_id);
    item.count = count;
    item.refine = refine;
    item.identified = 1;
    if (expire_time) {
        item.expire_in_hours = 1;
        item.expire_time = expire_time;
    }
    if (title) {
        item.type = 0x15;
        item.options[0].id = (unsigned short)ITEM_OPTION_TITLE;
        item.option_count = 3;
    }
    vec_push(&self->items, &item, sizeof item);
}

struct ItemInfo *package_entry_item(struct PackageEntry *self, unsigned index)
{
    struct ItemInfo *first = (struct ItemInfo *)self->items.first;
    struct ItemInfo *last = (struct ItemInfo *)self->items.last;

    if (first == last || (unsigned)(last - first) <= index)
        return 0;
    return first + index;
}

static void package_free_entries(struct Package *package)
{
    struct PackageEntry **it;

    for (it = package->entries.first; it != (struct PackageEntry **)package->entries.last; it++) {
        struct PackageEntry *entry = *it;

        if (entry) {
            item_info_vec_free(&entry->items);
            string_free(&entry->name);
            mem_free(entry);
        }
    }
    vec_free(&package->entries);
}

static struct PackageEntry *package_find(struct Package *self, unsigned id)
{
    struct PackageEntry **it;

    for (it = self->entries.first; it != (struct PackageEntry **)self->entries.last; it++)
        if ((*it)->id == id)
            return *it;
    return 0;
}

struct PackageEntry *package_entry_at(struct Package *self, unsigned index)
{
    struct PackageEntry **first = self->entries.first, **last = self->entries.last;

    if (first == last || (unsigned)(last - first) <= index)
        return 0;
    return first[index];
}

static struct PackageEntry *package_entry_new(const struct String *name, unsigned id)
{
    struct PackageEntry *entry = mem_alloc(sizeof *entry);
    struct String copy;

    if (!entry)
        return 0;
    string_copy(&copy, name);
    string_init(&entry->name);
    vec_init(&entry->items);
    string_assign(&entry->name, string_cstr(&copy), copy.size);
    entry->id = id;
    string_free(&copy);
    return entry;
}

static void mgr_free_packages(struct SelectPackageMgr *self)
{
    struct TreeNode *node;

    for (node = tree_first(&self->packages); node != self->packages.head; node = tree_next(node)) {
        struct Package *package = TREE_VALUE(node, struct PackageSlot)->package;

        if (package) {
            package_free_entries(package);
            mem_free(package);
        }
    }
}

void select_package_destruct(struct SelectPackageMgr *self)
{
    mgr_free_packages(self);
    tree_clear(&self->packages, NULL);
    self->loaded = 0;
    tree_free(&self->packages, NULL);
}

static void select_package_delete(struct SelectPackageMgr *self)
{
    select_package_destruct(self);
    mem_free(self);
}

unsigned char select_package_create(void)
{
    struct SelectPackageMgr *mgr;

    if (g_selectPackageMgr)
        return 0;
    mgr = mem_try_alloc(sizeof *mgr);
    if (mgr) {
        tree_init(&mgr->packages);
        mgr->loaded = 1;
    } else {
        mgr = 0;
    }
    g_selectPackageMgr = mgr;
    if (!mgr)
        return 0;
    if (!mgr->loaded) {
        select_package_delete(mgr);
        g_selectPackageMgr = 0;
        return 0;
    }
    return 1;
}

void select_package_free(void)
{
    if (g_selectPackageMgr) {
        select_package_delete(g_selectPackageMgr);
        g_selectPackageMgr = 0;
    }
}

struct Package *select_package_find(struct SelectPackageMgr *self, unsigned id)
{
    struct PackageSlot *slot = tree_find(&self->packages, &id, tree_less_uint);

    if (!slot)
        return 0;
    return slot->package;
}

void select_package_load(struct SelectPackageMgr *self)
{
    struct ClientLua lua;
    char text[0x104];
    int i;

    client_lua_construct(&lua);
    client_lua_open(&lua);
    if (!client_lua_load_file(&lua, "Lua Files\\selectpackage\\selectpackageitem", 1, 0)) {
        const char *caption = msg_str(0xf85);

        MessageBoxA(0, msg_str(0x69e), caption, 0);
        goto done;
    }

    for (i = 1; i < PACKAGE_MAP_LIMIT; i++) {
        int package_id = 0, entry_id = 0;
        const char *entry_name = 0;
        unsigned key;
        struct PackageSlot *slot;
        struct Package *package;
        struct String name;

        if (!client_lua_call(&lua, "querypackageitemboxname", "d>dds", i, &package_id, &entry_id,
                     &entry_name)) {
            memset(text, 0, sizeof text);
            sprintf(text, "querypackageitemboxname // %d Read fail", i);
            MessageBoxA(g_hMainWnd, text, "Error", 0);
            break;
        }
        if (package_id <= 0)
            break;
        key = package_id;
        slot = tree_find(&self->packages, &key, tree_less_uint);
        if (!slot) {
            package = mem_alloc(sizeof *package);
            if (package) {
                vec_init(&package->entries);
                package->id = key;
            } else {
                package = 0;
            }
            slot = tree_get(&self->packages, &key, sizeof key, sizeof *slot, tree_less_uint);
            slot->package = package;
        }
        package = slot->package;
        if (!package)
            continue;

        string_init(&name);
        string_assign_cstr(&name, entry_name);
        if (!package_find(package, entry_id)) {
            struct PackageEntry *entry = package_entry_new(&name, entry_id);

            vec_push(&package->entries, &entry, sizeof entry);
        }
        string_free(&name);
    }

    for (i = 1; i < PACKAGE_MAP_LIMIT; i++) {
        int package_id = 0, entry_id = 0, item_id = 0, count = 0, expire_time = 0, refine = 0, title = 0;
        unsigned key;
        struct PackageSlot *slot;
        struct Package *package;
        struct PackageEntry *entry;

        if (!client_lua_call(&lua, "querychoicepackageitemlist", "d>ddddddd", i, &package_id, &entry_id,
                     &item_id, &count, &expire_time, &refine, &title)) {
            memset(text, 0, sizeof text);
            sprintf(text, "querychoicepackageitemlist // %d Read fail", i);
            MessageBoxA(g_hMainWnd, text, "Error", 0);
            break;
        }
        if (package_id <= 0)
            break;
        key = package_id;
        slot = tree_find(&self->packages, &key, tree_less_uint);
        if (!slot)
            continue;
        package = slot->package;
        if (!package || package->entries.first == package->entries.last)
            continue;
        entry = package_find(package, entry_id);
        if (entry)
            package_entry_add_item(entry, item_id, count, expire_time, refine, title);
    }

done:
    client_lua_close(&lua);
    client_lua_destruct(&lua);
}
