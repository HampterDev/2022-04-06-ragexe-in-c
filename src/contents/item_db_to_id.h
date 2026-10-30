#ifndef CONTENTS_ITEM_DB_TO_ID_H
#define CONTENTS_ITEM_DB_TO_ID_H

#include "base.h"
#include "contents/lua_script.h"

struct ItemDbToIdMgr {
    struct LuaScript script;
};

struct ItemDbToIdMgr *item_db_to_id_construct(struct ItemDbToIdMgr *self);
void item_db_to_id_destruct(struct ItemDbToIdMgr *self);

char item_db_to_id_load(struct ItemDbToIdMgr *self);
int item_db_to_id_lookup(struct ItemDbToIdMgr *self, const char *name);

struct ItemDbToIdMgr *item_db_to_id_instance(void);
void item_db_to_id_destroy(void);

#endif
