#ifndef UI_QUESTS_NPC_INPUT_H
#define UI_QUESTS_NPC_INPUT_H

struct UIWindow;
struct UIEditCtrl;
struct ItemInfo;

struct UIEditCtrl *ui_npc_text_dialog_edit(struct UIWindow *dialog);
void megaphone_wnd_add_item_link(struct UIWindow *wnd, const struct ItemInfo *info);

struct UINpcEditDialogWnd *ui_npc_edit_dialog_wnd_construct(struct UINpcEditDialogWnd *self);
struct UIWindow *ui_npc_edit_dialog_wnd_new(void);
struct UINpcTextEditDialogWnd *ui_npc_text_edit_dialog_wnd_construct(struct UINpcTextEditDialogWnd *self);
struct UIWindow *ui_npc_text_edit_dialog_wnd_new(void);

#endif
