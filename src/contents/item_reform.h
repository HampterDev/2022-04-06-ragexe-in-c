#ifndef CONTENTS_ITEM_REFORM_H
#define CONTENTS_ITEM_REFORM_H

#include "base.h"
#include "contents/lua_script.h"
#include "std/std.h"
#include "std/tree.h"

struct ReformMaterial {
    int item;
    long long count;
};

struct ReformInfo {
    int base_item;
    int result_item;
    struct Tree materials;
    long long need_refine_min;
    long long need_refine_max;
    long long need_option_num_min;
    char is_empty_socket;
    long long change_refine_value;
    struct String random_option_code;
    char preserve_socket_item;
    char preserve_grade;
    struct String information;
};
OFFSET_ASSERT(ReformInfo, materials, 0x08);
OFFSET_ASSERT(ReformInfo, is_empty_socket, 0x28);
OFFSET_ASSERT(ReformInfo, change_refine_value, 0x30);
OFFSET_ASSERT(ReformInfo, random_option_code, 0x38);

struct ReformInfoEntry {
    unsigned long long id;
    struct ReformInfo info;
};

struct ReformItem {
    int item;
    struct Vec ids;
};

struct ItemReformMgr {
    struct Tree reforms;
    struct Tree reform_items;
    struct LuaScript lua;
    unsigned max_materials;
};

struct ItemReformMgr *item_reform_mgr(void);
void item_reform_mgr_destroy(void);

struct ItemReformMgr *item_reform_mgr_construct(struct ItemReformMgr *self);
void item_reform_mgr_destruct(struct ItemReformMgr *self);

char item_reform_mgr_load(struct ItemReformMgr *self);

struct Vec *item_reform_base_items(struct ItemReformMgr *self, struct Vec *out,
                                                    int reform_item);
struct ReformInfo *item_reform_find(struct ItemReformMgr *self, int reform_item,
                                               int base_item);
char item_reform_is_reform_item(struct ItemReformMgr *self, int item);

void reform_info_init(struct ReformInfo *info);
void reform_info_destroy(struct ReformInfo *info);

#endif
