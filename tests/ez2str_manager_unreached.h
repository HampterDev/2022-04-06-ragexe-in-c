#ifdef EZ2STR_MANAGER_REFERENCE
void *g_enchantMgr, *g_item_db, *g_itemReformMgr, *g_mapSkyMgrInstance;
void *enchant_mgr_construct(void *self) { abort(); }
void *item_db_to_id_construct(void *self) { abort(); }
void *item_reform_mgr_construct(void *self) { abort(); }
void *map_sky_mgr_construct(void *self) { abort(); }
#endif
struct ItemInfo;
struct Vec;
struct ItemInfo *item_info_copy(struct ItemInfo *self, const struct ItemInfo *src) { abort(); }
void store_search_item_copy(void *self, const void *src) { abort(); }
void store_search_items_destroy(void *first, void *last) { abort(); }
char *std_copy_memmove(const char *first, const char *last,
                                  char *dest) { abort(); }
void std_vector_free(struct Vec *v, size_t elem_size) { abort(); }
