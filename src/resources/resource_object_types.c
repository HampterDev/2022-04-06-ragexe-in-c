#include "core/object_type_registry.h"
#include "core/object_type_tables.h"

/* The object types of src/resources/: the resource classes. core/object_type.h describes the
   table. In the order of the class names. */
static const struct ObjectTypeRow k_rows[] = {
    {&action_resource_ops, OBJECT_TYPE(ACTION_RESOURCE), "CActRes"},
    {&bitmap_resource_ops, OBJECT_TYPE(BITMAP_RESOURCE), "CBitmapRes"},
    {&emblem_resource_ops, OBJECT_TYPE(EMBLEM_RESOURCE), "CEmblemRes"},
    {&gif_resource_ops, OBJECT_TYPE(BITMAP_RESOURCE) | OBJECT_TYPE(GIF_RESOURCE), "CGifRes"},
    {&palette_resource_ops, OBJECT_TYPE(PALETTE_RESOURCE), "CPaletteRes"},
    {&sprite_resource_ops, OBJECT_TYPE(SPRITE_RESOURCE), "CSprRes"},
};

const struct ObjectTypeRows g_resourceObjectTypes = OBJECT_TYPE_ROWS(k_rows);
