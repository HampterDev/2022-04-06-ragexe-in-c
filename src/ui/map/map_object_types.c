#include "core/object_type_registry.h"
#include "core/object_type_tables.h"

/* The object types of src/ui/map/: the mini-map window. core/object_type.h describes the
   table. In the order of the class names. */
static const struct ObjectTypeRow k_rows[] = {
    {&ui_mini_map_wnd_ops, OBJECT_TYPE(FRAME_WINDOW), "UIMiniMapWnd"},
};

const struct ObjectTypeRows g_mapObjectTypes = OBJECT_TYPE_ROWS(k_rows);
