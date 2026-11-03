#include "core/object_type_registry.h"
#include "core/object_type_tables.h"

/* The object types of src/ui/render/: the graphics settings window. core/object_type.h
   describes the table. In the order of the class names. */
static const struct ObjectTypeRow k_rows[] = {
    {&ui_graphic_setting_wnd_ops, OBJECT_TYPE(FRAME_WINDOW) | OBJECT_TYPE(GRAPHICS_SETTINGS_WINDOW), "UIGraphicSettingWnd"},
};

const struct ObjectTypeRows g_renderUiObjectTypes = OBJECT_TYPE_ROWS(k_rows);
