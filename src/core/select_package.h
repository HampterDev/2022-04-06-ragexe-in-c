#ifndef CORE_SELECT_PACKAGE_H
#define CORE_SELECT_PACKAGE_H

#include "base.h"

struct ItemInfo;
struct SelectPackageMgr;

struct PackageEntry {
    struct String name;
    unsigned id;
    struct Vec items;
};

struct Package {
    unsigned id;
    struct Vec entries;
};
SIZE_ASSERT(Package, 0x10);

extern struct SelectPackageMgr *g_selectPackageMgr;

struct PackageEntry *package_entry_at(struct Package *self, unsigned index);
struct ItemInfo *package_entry_item(struct PackageEntry *self, unsigned index);
struct Package *select_package_find(struct SelectPackageMgr *self, unsigned id);
void select_package_load(struct SelectPackageMgr *self);

#endif
