#include "core/object_type_registry.h"
#include "core/object_type_tables.h"

/* The object types of src/ui/mail/: the mail windows and their controls. core/object_type.h
   describes the table. In the order of the class names. */
static const struct ObjectTypeRow k_rows[] = {
    {&ui_rodex_compose_wnd_ops, OBJECT_TYPE(FRAME_WINDOW), "UIRodexComposeWnd"},
    {&ui_rodex_contents_wnd_ops, OBJECT_TYPE(FRAME_WINDOW), "UIRodexContentsWnd"},
    {&ui_rodex_edit_ctrl3_ops, 0, "UIRodexEditCtrl3"},
    {&ui_rodex_inbox_wnd_ops, OBJECT_TYPE(FRAME_WINDOW), "UIRodexInboxWnd"},
    {&ui_rodex_scroll_bar_ops, 0, "UIRodexScrollBar"},
};

const struct ObjectTypeRows g_mailObjectTypes = OBJECT_TYPE_ROWS(k_rows);
