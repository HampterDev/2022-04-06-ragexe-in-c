#ifndef ACTOR_MAP_TYPE_TABLE_H
#define ACTOR_MAP_TYPE_TABLE_H

struct MapPropertyMgr;
extern struct MapPropertyMgr *g_mapFlags;

void map_property_mgr_set(struct MapPropertyMgr *self, int map_id, int property, int value);
int map_property_mgr_get(struct MapPropertyMgr *self, int map_id, int property);
void map_property_mgr_clear(struct MapPropertyMgr *self);
unsigned char map_property_mgr_create(void);
void map_property_mgr_release(void);

#endif
