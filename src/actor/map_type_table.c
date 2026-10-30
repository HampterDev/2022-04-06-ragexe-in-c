#include "core/memory.h"
#include "std/tree.h"
#include "actor/map_type_table.h"

struct MapPropertyMgr *g_mapFlags;

struct MapProperty {
    int id;
    int value;
};

struct MapProperties {
    int map_id;
    struct Tree values;
};

struct MapPropertyMgr {
    struct Tree maps;
};

static void map_properties_destroy(void *value)
{
    struct MapProperties *properties = value;

    tree_free(&properties->values, NULL);
}

static struct MapPropertyMgr *map_property_mgr_construct(struct MapPropertyMgr *self)
{
    tree_init(&self->maps);
    return self;
}

static void map_property_mgr_destruct(struct MapPropertyMgr *self)
{
    tree_free(&self->maps, map_properties_destroy);
}

void map_property_mgr_set(struct MapPropertyMgr *self, int map_id, int property, int value)
{
    struct MapProperties *properties = tree_find(&self->maps, &map_id, tree_less_int);
    struct MapProperty *entry;

    if (!properties) {
        struct MapProperties made;

        made.map_id = map_id;
        tree_init(&made.values);
        properties = tree_add(&self->maps, &made, sizeof made, tree_less_int, NULL);
    }
    entry = tree_get(&properties->values, &property, sizeof property, sizeof *entry, tree_less_int);
    entry->value = value;
}

void map_property_mgr_clear(struct MapPropertyMgr *self)
{
    if (self)
        tree_clear(&self->maps, map_properties_destroy);
}

int map_property_mgr_get(struct MapPropertyMgr *self, int map_id, int property)
{
    struct MapProperties *properties = tree_find(&self->maps, &map_id, tree_less_int);
    struct MapProperty *entry;

    if (!properties)
        return 0;
    entry = tree_find(&properties->values, &property, tree_less_int);
    return entry ? entry->value : 0;
}

unsigned char map_property_mgr_create(void)
{
    struct MapPropertyMgr *mgr;

    if (g_mapFlags)
        return 0;
    mgr = (struct MapPropertyMgr *)mem_try_alloc(sizeof *mgr);
    if (mgr)
        map_property_mgr_construct(mgr);
    g_mapFlags = mgr;
    if (!mgr)
        return 0;
    return 1;
}

void map_property_mgr_release(void)
{
    struct MapPropertyMgr *mgr = g_mapFlags;

    if (mgr) {
        map_property_mgr_destruct(mgr);
        mem_free(mgr);
        g_mapFlags = NULL;
    }
}
