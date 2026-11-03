#include "core/object_type_registry.h"

/* The composition point: the row tables of every directory that defines classes. A class
   has at most one row in all of them, so the order of this list decides nothing. */
static const struct ObjectTypeRows *const k_object_type_tables[] = {
    &g_controlsObjectTypes,
    &g_itemsObjectTypes,
    &g_characterObjectTypes,
    &g_questsObjectTypes,
    &g_socialObjectTypes,
    &g_shopsObjectTypes,
    &g_chatObjectTypes,
    &g_actorObjectTypes,
    &g_resourceObjectTypes,
    &g_mailObjectTypes,
    &g_hotkeyObjectTypes,
    &g_skillObjectTypes,
    &g_renderUiObjectTypes,
    &g_mapObjectTypes,
    &g_worldmapObjectTypes,
};

const struct ObjectTypeTable g_objectTypes = {
    k_object_type_tables, sizeof k_object_type_tables / sizeof k_object_type_tables[0]
};
