#include "ui/quests/npc_menu.h"
#include "ui/quests/say_dialog.h"
#include "ui/quests/tag_text_viewer.h"
#include "ui/controls/listbox.h"
#include "ui/record_tags.h"
#include "replay/snapshot.h"

int ui_choose_wnd_save_state(struct UIWindow *window, struct ReplayBuffer *out)
{
    struct UIChooseWnd *self = (struct UIChooseWnd *)window;
    struct SnapshotWriter writer = {0};
    const struct String *line;

    snapshot_write(&writer, NPC_MENU_STATE_BEGIN, NULL, 0);
    for (line = self->list->items.first; line != self->list->items.last; ++line)
        snapshot_write(&writer, NPC_MENU_STATE_LINE, string_cstr(line), line->size);
    snapshot_write(&writer, NPC_MENU_STATE_END, NULL, 0);
    return snapshot_finish(&writer, out);
}

int ui_choose_warp_wnd_save_state(struct UIWindow *window, struct ReplayBuffer *out)
{
    struct UIChooseWarpWnd *self = (struct UIChooseWarpWnd *)window;
    struct SnapshotWriter writer = {0};
    const struct String *dest;

    snapshot_write(&writer, WARP_STATE_BEGIN, NULL, 0);
    for (dest = self->destinations.first; dest != self->destinations.last; ++dest)
        snapshot_write(&writer, WARP_STATE_LINE, string_cstr(dest), dest->size);
    snapshot_write(&writer, WARP_STATE_END, NULL, 0);
    return snapshot_finish(&writer, out);
}

int ui_say_dialog_wnd_save_state(struct UIWindow *window, struct ReplayBuffer *out)
{
    struct UISayDialogWnd *self = (struct UISayDialogWnd *)window;
    struct SnapshotWriter writer = {0};

    snapshot_write(&writer, SAY_STATE_BEGIN, NULL, 0);
    if (self->text->has_text)
        snapshot_write(&writer, SAY_STATE_LINE, string_cstr(&self->text->text), self->text->text.size);
    snapshot_write(&writer, SAY_STATE_CLEAR, &self->clear_on_text, sizeof self->clear_on_text);
    snapshot_write(&writer, SAY_STATE_END, NULL, 0);
    return snapshot_finish(&writer, out);
}
