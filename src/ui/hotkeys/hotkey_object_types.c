#include "core/object_type_registry.h"
#include "core/object_type_tables.h"

/* The object types of src/ui/hotkeys/: the hotkey windows. core/object_type.h describes the
   table. In the order of the class names. */
static const struct ObjectTypeRow k_rows[] = {
    {&ui_hot_key_wnd_ops, OBJECT_TYPE(FRAME_WINDOW), "UIHotKeyWnd"},
    {&ui_hotkey_guide_wnd_ops, OBJECT_TYPE(FRAME_WINDOW), "UIHotkeyGuideWnd"},
    {&ui_quik_slot_wnd_ops, OBJECT_TYPE(FRAME_WINDOW), "UIQuikSlotWnd"},
    {&ui_short_cut_wnd_ops, OBJECT_TYPE(FRAME_WINDOW), "UIShortCutWnd"},
};

const struct ObjectTypeRows g_hotkeyObjectTypes = OBJECT_TYPE_ROWS(k_rows);
