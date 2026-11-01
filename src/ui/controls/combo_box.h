#include "ui/window_manager_state.h"
#ifndef UI_CONTROLS_COMBO_BOX_H
#define UI_CONTROLS_COMBO_BOX_H

#include "ui/window.h"
#include "ui/draw.h"
#include "ui/skin.h"
#include "ui/controls/buttons.h"
#include "ui/controls/edit.h"
#include "ui/controls/listbox.h"

struct Tree;
struct UIBitmapButton;
struct UIFrameWnd;
struct UIListBox;
struct UIListBoxEx;
struct UIListBoxID;
struct UINewComboBoxCtrl;
struct UIWindow;

#define COMBO_MSG_TOGGLED 0x17
#define COMBO_MSG_ADD_LINE 0x1f
#define COMBO_MSG_CHOSEN 0x27
#define COMBO_MSG_SELECT 0x28
#define COMBO_MSG_SET_LABEL 0x39
#define COMBO_MSG_OPENER_ID 0x53
#define UI_ID_COMBO_BUTTON 0x109
struct UIComboBoxEx {
    struct UIListBoxEx list;
    int opener_id;
};
struct UIComboBoxEx2 {
    struct UIComboBoxEx ex;
    int style;
};
struct UIComboBoxEx2 *ui_combo_box_ex2_construct(struct UIComboBoxEx2 *self);
struct UIComboBoxDefWnd {
    struct UIWindow base;
    struct UIBitmapButton *button;
    struct UIWindow *list;
    struct UIWindow *label;
    BYTE no_border;
    BYTE open;
    int back_r, back_g, back_b;
    int border_r, border_g, border_b;
    int list_height;
};
struct UIComboBoxDef2Wnd {
    struct UIComboBoxDefWnd def;
    int login_skin;
    int style;
    int label_y;
    int label_x;
};
SIZE_ASSERT(UIComboBoxDef2Wnd, 0xb8);
struct UIComboBoxBmpWnd {
    struct UIComboBoxDefWnd def;
    struct String bitmap;
};
struct UIWindow *ui_combo_box_def_wnd_construct(struct UIWindow *self);
struct UIWindow *ui_combo_box_def2_wnd_construct(struct UIWindow *self);
struct UIComboBoxBmpWnd *ui_combo_box_bmp_wnd_construct(struct UIComboBoxBmpWnd *self);
struct UIComboBoxV2Wnd {
    struct UIWindow base;
    struct String bitmap;
    int selected;
    struct UIWindow *head;
    struct UIListBoxEx *list;
};
void ui_combo_box_def2_wnd_add_item(struct UIWindow *self, struct String text,
                                          int value);
void ui_combo_box_def_wnd_clear_list(struct UIWindow *self);
void ui_combo_box_def_wnd_select(struct UIWindow *self, int index);
static __inline void ui_combo_add(struct UIWindow *combo, const char *text)
{
    struct String item;

    string_init(&item);
    string_assign_cstr(&item, text);
    ui_combo_box_def2_wnd_add_item(combo, item, 0);
}

#define UI_WND_COMBO_BOX 0x1a
#define COMBO_NO_OPENER 0x166
struct UIComboBox {
    struct UIListBox list;
    int opener_id;
};
SIZE_ASSERT(UIComboBox, 0xd4);
struct UIComboBox *ui_combo_box_construct(struct UIComboBox *self);
struct UIComboBoxID {
    struct UIListBoxID list;
    int opener_id;
    struct Tree values;
};
#define UI_COMBO_BOX_ID_SIZE sizeof(struct UIComboBoxID)
struct UIComboBoxID *ui_combo_box_id_construct(struct UIComboBoxID *self);
struct UIComboBoxWnd {
    struct UIFrameWnd frame;
    struct UIWindow *list;
    struct UIWindow *chosen_to;
};
SIZE_ASSERT(UIComboBoxWnd, 0x9c);
struct UIComboBoxWnd *ui_combo_box_wnd_construct(struct UIComboBoxWnd *self);
struct UIWindow *ui_combo_box_wnd_new(void);
struct UIComboBoxWnd *ui_combo_box_id_wnd_construct(struct UIComboBoxWnd *self);
struct UIWindow *ui_combo_box_id_wnd_new(void);

struct UIComboBoxEx *ui_combo_box_ex_construct(struct UIComboBoxEx *self);
void ui_combo_box_ex_destroy(struct UIWindow *window);
void ui_combo_box_ex_resize(struct UIWindow *self, int width, int height);
void ui_combo_box_ex_on_draw(struct UIWindow *self);
void ui_combo_box_ex_on_mouse_move(struct UIWindow *self, int x, int y);
void ui_combo_box_ex_on_l_btn_up(struct UIWindow *self, int x, int y);
int ui_combo_box_ex_send_msg(struct UIWindow *self, struct UIWindow *sender, int msg,
                                    int val1, int val2, int val3, int val4);
void ui_combo_box_ex_update_scroll_bar(struct UIListBoxEx *base_list);
void ui_combo_box_id_destroy(struct UIWindow *window);
void ui_combo_box_id_on_draw(struct UIWindow *self);
void ui_combo_box_id_on_l_btn_up(struct UIWindow *self, int x, int y);
int ui_combo_box_id_send_msg(struct UIWindow *self, struct UIWindow *sender, int msg,
                                    int val1, int val2, int val3, int val4);
void ui_combo_box_ex2_destroy(struct UIWindow *window);
void ui_combo_box_ex2_resize(struct UIWindow *self, int width, int height);
void ui_combo_box_ex2_on_create(struct UIWindow *self, int width, int height);
void ui_combo_box_def_wnd_destroy(struct UIWindow *window);
void ui_combo_box_def_wnd_on_create(struct UIWindow *self, int width_unused,
                                          int height_unused);
void ui_combo_box_def_wnd_on_draw(struct UIWindow *self);
void ui_combo_box_def_wnd_on_l_btn_up(struct UIWindow *self, int x_unused, int y_unused);
int ui_combo_box_def_wnd_send_msg(struct UIWindow *self, struct UIWindow *sender,
                                        int msg, int val1, int val2, int val3, int val4);
struct String *ui_combo_box_def_wnd_get_label_text(struct UIWindow *wnd,
                                                           struct String *out);
void ui_combo_box_def2_wnd_destroy(struct UIWindow *window);
void ui_combo_box_def2_wnd_on_create(struct UIWindow *self, int width, int height_unused);
void ui_combo_box_def2_wnd_on_draw(struct UIWindow *self);
void ui_combo_box_bmp_wnd_destroy(struct UIWindow *window);
void ui_combo_box_bmp_wnd_on_draw(struct UIWindow *self);
void ui_combo_box_bmp_wnd_create(struct UIComboBoxBmpWnd *self, int width, int height,
                                        const char *name, const char *bitmap);
void ui_combo_box_v2_wnd_destroy(struct UIWindow *window);
struct String *ui_combo_box_v2_wnd_get_selected_text(struct UIComboBoxV2Wnd *self,
                                                             struct String *out);
void ui_combo_box_v2_wnd_on_draw(struct UIWindow *self);
void ui_combo_box_v2_wnd_on_wheel(struct UIWindow *self, int delta);
int ui_combo_box_v2_wnd_send_msg(struct UIWindow *self, struct UIWindow *sender, int msg,
                                       int val1, int val2, int val3, int val4);

void ui_combo_box_destroy(struct UIWindow *window);
void ui_combo_box_resize(struct UIWindow *self, int width, int height);
void ui_combo_box_on_draw(struct UIWindow *self);
void ui_combo_box_ignore_click(struct UIWindow *self, int x_unused,
                                       int y_unused);
void ui_combo_box_on_mouse_move(struct UIWindow *self, int x, int y);
void ui_combo_box_on_l_btn_up(struct UIWindow *self, int x, int y);
int ui_combo_box_send_msg(struct UIWindow *self, struct UIWindow *sender, int msg, int val1,
                                  int val2, int val3, int val4);
void ui_combo_box_update_scroll_bar(struct UIListBox *base_list);
void ui_combo_box_wnd_destroy(struct UIWindow *window);
void ui_combo_box_id_wnd_destroy(struct UIWindow *window);
void ui_combo_box_wnd_resize(struct UIWindow *self, int width, int height);
void ui_combo_box_wnd_on_create(struct UIWindow *self, int width, int height);
void ui_combo_box_id_wnd_on_create(struct UIWindow *self, int width, int height);
void ui_combo_box_wnd_on_draw(struct UIWindow *self);
int ui_combo_box_wnd_send_msg(struct UIWindow *self, struct UIWindow *sender, int msg,
                                     int val1, int val2, int val3, int val4);

void ui_new_combo_box_ctrl_destruct(struct UINewComboBoxCtrl *self);
void ui_new_combo_box_ctrl_destroy(struct UIWindow *window);
void ui_new_combo_box_ctrl_on_create(struct UIWindow *self, int width, int height);
void ui_new_combo_box_ctrl_on_draw(struct UIWindow *self);
void ui_new_combo_box_ctrl_on_l_btn_dbl_clk(struct UIWindow *self, int x, int y);
int ui_new_combo_box_ctrl_send_msg(struct UIWindow *self, struct UIWindow *sender,
                                         int msg, int val1, int val2, int val3, int val4);

#endif
