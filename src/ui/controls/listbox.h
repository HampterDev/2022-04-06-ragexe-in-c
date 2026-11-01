#include "session/state.h"
#ifndef UI_CONTROLS_LISTBOX_H
#define UI_CONTROLS_LISTBOX_H

#include "ui/window.h"
#include "ui/draw.h"
#include "ui/skin.h"
#include "ui/controls/buttons.h"
#include "ui/controls/scroll.h"

struct List;
struct TreeNode;
struct Vec;
struct UIScrollBar;
struct UITVTextButton;
struct UITreeNode;
struct UITreeView;
struct UIWindow;
struct UIWindowOps;

struct UIListBox {
    struct UIWindow base;
    int r, g, b;
    struct Vec items;
    int selected;
    int top;
    int scroll_x;
    int widest;
    int has_vscroll;
    int extra_lines;
    struct UIScrollBar *vbar;
    struct UIScrollBar *hbar;
    int transparent;
    int line_height;
    BYTE outlined;
    COLORREF selection_colour;
    BYTE selectable;
    DWORD text_colour;
    BYTE centre_lines;
};
OFFSET_ASSERT(UIListBox, items, 0x88);
OFFSET_ASSERT(UIListBox, selected, 0x94);
OFFSET_ASSERT(UIListBox, vbar, 0xac);
OFFSET_ASSERT(UIListBox, centre_lines, 0xcc);
SIZE_ASSERT(UIListBox, 0xd0);
struct UIListBoxOps {
    struct UIWindowOps base;
    void (*create)(struct UIListBox *self, int x, int y, int width, int height,
                              BYTE transparent);
    void (*add_item)(struct UIListBox *self, const char *text);
    void (*clear_items)(struct UIListBox *self);
    void (*update_scroll_bar)(struct UIListBox *self);
};
static inline void ui_list_box_create_window(struct UIListBox *list, int x, int y, int width, int height, BYTE transparent)
{
    const struct UIListBoxOps *ops = (const struct UIListBoxOps *)list->base.ops;
    ops->create(list, x, y, width, height, transparent);
}

static inline void ui_list_box_append_text(struct UIListBox *list, const char *text)
{
    const struct UIListBoxOps *ops = (const struct UIListBoxOps *)list->base.ops;
    ops->add_item(list, text);
}

static inline void ui_list_box_clear(struct UIListBox *list)
{
    const struct UIListBoxOps *ops = (const struct UIListBoxOps *)list->base.ops;
    ops->clear_items(list);
}

static inline void ui_list_box_refresh_scroll(struct UIListBox *list)
{
    const struct UIListBoxOps *ops = (const struct UIListBoxOps *)list->base.ops;
    ops->update_scroll_bar(list);
}

struct UIListBox *ui_list_box_construct(struct UIListBox *self);
void ui_list_box_destruct(struct UIListBox *self);
void ui_list_box_resize(struct UIWindow *self, int width, int height);
int ui_list_box_hit_line(struct UIListBox *self, int x, int y);
void ui_list_box_add_item(struct UIListBox *self, const char *text);
void ui_list_box_clear_items(struct UIListBox *self);
int ui_list_box_send_msg(struct UIWindow *self, struct UIWindow *sender, int msg,
                                 int val1, int val2, int val3, int val4);
struct UIListBoxNavi {
    struct UIListBox list;
    COLORREF flag_colour;
    struct Vec flags;
    int row_base;
};
OFFSET_ASSERT(UIListBoxNavi, flags, 0xd4);
SIZE_ASSERT(UIListBoxNavi, 0xe4);
struct UIListBoxNavi *ui_list_box_navi_construct(struct UIListBoxNavi *self);

void ui_list_box_on_l_btn_down(struct UIWindow *self, int x, int y);
int ui_list_box_count(struct UIListBox *self);
#define UI_MSG_LIST_SELECT 0x87

struct UIListBoxExRow {
    struct String text;
    COLORREF colour;
};
struct UIListBoxExRow *list_box_ex_entry_at(struct Tree *rows, const int *key);

struct UIListBoxExPair {
    int key;
    struct UIListBoxExRow row;
};
struct UIListBoxEx {
    struct UIWindow base;
    int r, g, b;
    struct Tree rows;
    int selected;
    int top;
    int scroll_x;
    int widest;
    int has_vscroll;
    int extra_lines;
    struct UIScrollBar *vbar;
    struct UIScrollBar *hbar;
    int transparent;
    int line_height;
    COLORREF selection_colour;
};
OFFSET_ASSERT(UIListBoxEx, rows, 0x88);
OFFSET_ASSERT(UIListBoxEx, vbar, 0xa8);
struct UIListBoxExOps {
    struct UIWindowOps base;
    void (*clear_items)(struct UIListBoxEx *self);
    void (*update_scroll_bar)(struct UIListBoxEx *self);
};
static inline void ui_list_box_ex_clear(struct UIListBoxEx *list)
{
    const struct UIListBoxExOps *ops = (const struct UIListBoxExOps *)list->base.ops;
    ops->clear_items(list);
}

static inline void ui_list_box_ex_refresh_scroll(struct UIListBoxEx *list)
{
    const struct UIListBoxExOps *ops = (const struct UIListBoxExOps *)list->base.ops;
    ops->update_scroll_bar(list);
}

struct UIListBoxEx *ui_list_box_ex_construct(struct UIListBoxEx *self);
int ui_list_box_ex_send_msg(struct UIWindow *self, struct UIWindow *sender, int msg, int val1, int val2,
                                   int val3, int val4);
void ui_list_box_ex_resize(struct UIWindow *self, int width, int height);

struct UIListBoxIDRow {
    int id;
    struct String text;
};
struct UIListBoxIDPair {
    int key;
    struct UIListBoxIDRow row;
};
struct UIListBoxID {
    struct UIListBoxEx list;
};
struct UIListBoxID *ui_list_box_id_construct(struct UIListBoxID *self);
void ui_list_box_id_destruct(struct UIListBoxID *self);
void ui_list_box_id_add_item(struct UIListBoxID *self, const char *text, int id);
struct UIListBoxIDRow *ui_list_box_id_row_at(struct Tree *rows,
                                                    const int *key);
struct UIListBoxTitle {
    struct UIListBoxID id;
    COLORREF listed_colour;
};

struct TableBox {
    struct UIWindow base;
    DWORD colour;
};
struct UITableBox {
    struct UIWindow base;
    struct Vec items;
};

void ui_list_box_agit_destruct(struct UIListBox *self);
void ui_list_box_agit_destroy(struct UIWindow *window);
void ui_list_box_agit_on_l_btn_down(struct UIWindow *self, int x, int y);
void ui_list_box_agit_on_r_btn_down(struct UIWindow *self, int x, int y);
void ui_list_box_agit_menu_destruct(struct UIListBox *self);
void ui_list_box_agit_menu_destroy(struct UIWindow *window);
void ui_list_box_agit_menu_on_draw(struct UIWindow *self);
void ui_list_box_agit_menu_on_l_btn_down(struct UIWindow *self, int x, int y);
void ui_list_box_agit_menu_on_mouse_hover(struct UIWindow *self, int x, int y);
int ui_list_box_agit_menu_send_msg(struct UIWindow *self, struct UIWindow *sender, int msg,
                                         int val1, int val2, int val3, int val4);

void ui_list_box_destroy(struct UIWindow *window);
void ui_list_box_on_create(struct UIWindow *self, int width, int height);
void ui_list_box_on_draw(struct UIWindow *self);
void ui_list_box_on_l_btn_dbl_clk(struct UIWindow *self, int x_unused, int y_unused);
void ui_list_box_on_wheel(struct UIWindow *self, int delta);
void ui_list_box_create(struct UIListBox *self, int x, int y, int width, int height,
                                 BYTE transparent);
void ui_list_box_update_scroll_bar(struct UIListBox *self);
void ui_list_box_navi_destruct(struct UIListBoxNavi *self);
void ui_list_box_navi_destroy(struct UIWindow *window);
void ui_list_box_navi_on_draw(struct UIWindow *self);
void ui_list_box_navi_on_l_btn_down(struct UIWindow *self, int x, int y);
void ui_list_box_navi_add_item(struct UIListBox *base_list, const char *text);
void ui_list_box_navi_clear_items(struct UIListBox *base_list);
void ui_list_box_navi_set_flag(struct UIListBoxNavi *self, int index, int flag);
void ui_list_box_navi_clear_flags(struct UIListBoxNavi *self);
int ui_list_box_navi_row_base(struct UIListBoxNavi *self);
void ui_list_box_navi_set_row_base(struct UIListBoxNavi *self, int base);
void ui_list_box_navi_set_flag_colour(struct UIListBoxNavi *self, COLORREF colour);

void ui_list_box_ex_destruct(struct UIListBoxEx *self);
void ui_list_box_ex_destroy(struct UIWindow *window);
void ui_list_box_ex_on_create(struct UIWindow *self, int width, int height);
void ui_list_box_ex_on_draw(struct UIWindow *self);
void ui_list_box_ex_on_l_btn_down(struct UIWindow *self, int x, int y);
void ui_list_box_ex_on_l_btn_dbl_clk(struct UIWindow *self, int x_unused, int y_unused);
void ui_list_box_ex_on_wheel(struct UIWindow *self, int delta);
void ui_list_box_ex_create(struct UIListBoxEx *self, int x, int y, int width, int height,
                                   BYTE transparent);
void ui_list_box_ex_add_item(struct UIListBoxEx *self, const char *text, COLORREF colour);
void ui_list_box_ex_clear_items(struct UIListBoxEx *self);
void ui_list_box_ex_update_scroll_bar(struct UIListBoxEx *self);

void ui_list_box_id_destroy(struct UIWindow *window);
void ui_list_box_id_on_draw(struct UIWindow *self);
void ui_list_box_id_on_l_btn_dbl_clk(struct UIWindow *self, int x_unused, int y_unused);
void ui_list_box_id_clear_items(struct UIListBoxEx *base_list);
struct UIListBoxTitle *ui_list_box_title_construct(struct UIListBoxTitle *self);
void ui_list_box_title_destroy(struct UIWindow *window);
void ui_list_box_title_on_draw(struct UIWindow *self);

void table_box_destruct(struct TableBox *self);
void table_box_destroy(struct UIWindow *window);
void table_box_on_create(struct UIWindow *self, int width, int height);
void table_box_on_draw(struct UIWindow *self);
void ui_table_box_destruct(struct UITableBox *self);
void ui_table_box_destroy(struct UIWindow *window);
void ui_table_box_on_draw(struct UIWindow *self);

void ui_tree_view_hide_rows(struct UITreeView *self, struct List *list);
BYTE ui_tree_view_clear_children(struct UITreeView *self, struct UITreeNode *node);
BYTE ui_tree_view_remove(struct UITreeView *self, struct UITreeNode *parent, int id,
                                  BYTE keep_children);
int ui_tree_view_layout(struct UITreeView *self, struct List *list, int x, int y);
void ui_tree_view_destroy(struct UIWindow *window);
void ui_tree_view_resize(struct UIWindow *self, int width, int height);
void ui_tree_view_on_create(struct UIWindow *self, int width, int height);
void ui_tree_view_on_draw(struct UIWindow *self);
void ui_tree_view_on_l_btn_down(struct UIWindow *self, int x, int y);
void ui_tree_view_on_mouse_move(struct UIWindow *self, int x, int y);
void ui_tree_view_on_l_btn_up(struct UIWindow *self, int x, int y);
void ui_tree_view_on_wheel(struct UIWindow *self, int delta);
void ui_tree_view_update_scroll_bar(struct UITreeView *self);
int ui_tree_view_send_msg(struct UIWindow *self, struct UIWindow *sender, int msg, int val1,
                                  int val2, int val3, int val4);

struct UIComboBoxWnd;
void ui_combo_box_wnd_set_chosen_to(struct UIComboBoxWnd *self, struct UIWindow *chosen_to);

struct UIMessageBox *ui_message_box_new(void);
void ui_message_box_create(struct UIMessageBox *self, const char *text, int type, int default_yes,
                                    DWORD duration, const char *title, int width, int height, int modal,
                                    const char *picture);

#endif
