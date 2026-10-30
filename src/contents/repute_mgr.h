#ifndef CONTENTS_REPUTE_MGR_H
#define CONTENTS_REPUTE_MGR_H

#include "contents/repute_api.h"
#include "base.h"
#include "std/std.h"
#include "std/tree.h"
#include "json/json_loader.h"
#include "contents/lua_script.h"
#include "contents/repute_groups.h"

#define REPUTE_SHOWN             0
#define REPUTE_SHOWN_WITH_POINTS 2

struct ReputeInfo {
    struct String name;
    long long max_point_positive;
    long long max_point_negative;
    long long visibility;
    long long points;
    unsigned char highlighted;
};

struct ReputeInfoEntry {
    unsigned long long id;
    struct ReputeInfo info;
};

struct ReputeGroupEntry {
    unsigned long long id;
    struct ReputeGroup group;
};

struct ReputeInfoRecord {
    struct String name;
    int max_point_positive;
    int max_point_negative;
    struct String visibility;
};

struct ReputeInfoRecordPair {
    int id;
    struct ReputeInfoRecord record;
};

struct ReputeMgr {
    struct Tree infos;
    struct Tree groups;
    unsigned char no_info;
    struct LuaScript lua;
};

struct ReputeInfoStore {
    void *repute_mgr;
    struct Tree *infos;
};

#define REPUTE_WND_ID 0x15a

void repute_info_store(const struct ReputeInfoStore *self,
                                  const struct ReputeInfoRecordPair *rec);
struct Vec *repute_mgr_group_names(struct ReputeMgr *self, struct Vec *out);
struct Vec *repute_mgr_group_infos(struct ReputeMgr *self, struct Vec *out,
                                                   const struct String *group_name);
struct Vec *repute_mgr_all_infos(struct ReputeMgr *self, struct Vec *out);
struct Vec *repute_mgr_search_infos(struct ReputeMgr *self, struct Vec *out,
                                                    const struct String *text);

char repute_mgr_load_info_bson(struct ReputeMgr *self, struct Tree *infos);
char repute_mgr_load_group_bson(struct ReputeMgr *self, struct Tree *groups);

unsigned char repute_info_read(struct JsonCppLoader *loader, struct Tree *records);

void repute_info_record_pair_destroy(void *value);
void repute_group_record_pair_destroy(void *value);
void repute_info_entry_destroy(void *value);
void repute_group_entry_destroy(void *value);

#endif
