#include "base.h"
#include "platform/language.h"
#include "platform/code_page.h"
#include "ui/window_manager_state.h"
#ifndef UI_CONTROLS_EDIT_H
#define UI_CONTROLS_EDIT_H

#include "ui/controls/edit_types.h"

#include "ui/window.h"
#include "ui/draw.h"
#include "ui/skin.h"
#include "ui/controls/buttons.h"

void ui_edit_ctrl3_set_tag_colour(struct UIEditCtrl3 *self, DWORD colour);

struct StdBitVector;
struct List;
struct Vec;
struct UICandidateWnd;
struct UIScrollBar;
struct UIWindow;
struct UIWindowMgr;
struct UIWindowOps;

struct UIEditCtrlOps {
    struct UIWindowOps base;
    void (*set_text)(struct UIEditCtrl *self, const char *text);
    void (*draw_text)(struct UIEditCtrl *self);
    void (*truncate)(struct UIEditCtrl *self);
};
static inline void ui_edit_ctrl_set_text(struct UIEditCtrl *edit, const char *text)
{
    const struct UIEditCtrlOps *ops = (const struct UIEditCtrlOps *)edit->base.ops;
    ops->set_text(edit, text);
}

static inline void ui_edit_ctrl_draw_text(struct UIEditCtrl *edit)
{
    const struct UIEditCtrlOps *ops = (const struct UIEditCtrlOps *)edit->base.ops;
    ops->draw_text(edit);
}

static inline void ui_edit_ctrl_truncate(struct UIEditCtrl *edit)
{
    const struct UIEditCtrlOps *ops = (const struct UIEditCtrlOps *)edit->base.ops;
    ops->truncate(edit);
}

#define UI_MSG_KEY 0x100
#define UI_MSG_PARSE_LINKS 0x99
#define UI_EDIT_MASK_CHAR(edit) (((struct UIEditCtrl *)(edit))->mask_char)
#define UI_EDIT_MAX_CHARS(edit) (((struct UIEditCtrl *)(edit))->max_chars)
#define UI_EDIT_NO_BORDER(edit) (((struct UIEditCtrl *)(edit))->no_border)
#define UI_EDIT_TEXT_TOP(edit) (((struct UIEditCtrl *)(edit))->text_top)

struct UIWindow *ui_edit_ctrl_construct(struct UIWindow *self);
struct UIWindow *ui_edit_ctrl2_construct(struct UIWindow *self);
struct UIWindow *ui_edit_ctrl3_construct(struct UIWindow *self);
void ui_edit_ctrl3_destruct(struct UIWindow *self);
void ui_edit_ctrl3_on_draw(struct UIWindow *self);
struct UIEditCtrl4 *ui_edit_ctrl4_construct(struct UIEditCtrl4 *self);
struct UIEditCtrl4 *ui_edit_ctrl5_construct(struct UIEditCtrl4 *self);
void ui_edit_ctrl4_set_empty_text(struct UIWindow *self, const char *text);
int ui_edit_ctrl_send_msg(struct UIWindow *self, struct UIWindow *sender, int msg, int val1, int val2,
                                  int val3, int val4);
void ui_edit_ctrl_on_focus(struct UIWindow *self);
void ui_edit_ctrl_on_draw(struct UIWindow *self);
void ui_edit_ctrl_on_focus_lost(struct UIWindow *self);
void ui_edit_ctrl_set_text_default(struct UIEditCtrl *self, const char *text);
void ui_edit_ctrl_truncate_text(struct UIEditCtrl *self, int to_language);
void ui_edit_ctrl_sync_from_ime_with(struct UIEditCtrl *self, int font_flag, int right_margin);
int ui_edit_ctrl2_send_msg(struct UIWindow *self, struct UIWindow *sender, int msg, int val1, int val2,
                                   int val3, int val4);
enum UIEditAlign {
    UI_EDIT_ALIGN_LEFT = 0,
    UI_EDIT_ALIGN_RIGHT = 1,
    UI_EDIT_ALIGN_CENTRE = 2,
};
struct UIWindow *ui_edit_ctrl_custom_construct(struct UIWindow *self);
struct UIEditCtrlCurrency *ui_edit_ctrl_currency_construct(struct UIEditCtrlCurrency *self);
const char *ui_edit_ctrl_get_text(struct UIWindow *self);
int ui_edit_ctrl_get_text_size(struct UIWindow *self);
void ui_edit_ctrl_select_all(struct UIWindow *self);
void ui_edit_ctrl_set_caret(struct UIWindow *self, int pos);
void ui_edit_ctrl_set_back_colour(struct UIWindow *self, int red,
                                         int green, int blue);
void ui_edit_ctrl_set_border_colour(struct UIWindow *self, int red,
                                           int green, int blue);
void ui_edit_ctrl_set_text_colour(struct UIWindow *self, int red, int green, int blue,
                                         int double_colour);
void ui_edit_ctrl_destruct(struct UIEditCtrl *self);
void ui_edit_ctrl_sync_from_ime(struct UIEditCtrl *self);
void ui_edit_ctrl_draw_highlight(struct UIEditCtrl *self, int x, int width);
void ui_edit_ctrl_place_candidate_wnd(struct UIEditCtrl *self, int x);
void ui_edit_ctrl_place_key_stroke_wnd(struct UIEditCtrl *self, int x);
void ui_edit_ctrl_layout_navi_links(struct UIEditCtrl *self);
BYTE ui_edit_ctrl_caret_shown(void);
unsigned long long ui_edit_ctrl_focus_key(void);

void ui_window_text_out_doubled(struct UIWindow *self, int x, int y, const char *text,
                                        COLORREF colour, COLORREF colour2, int len, int font_face,
                                        int font_height);

struct UIEditCtrl3Ops {
    struct UIEditCtrlOps edit;
    void (*update_bar)(struct UIEditCtrl3 *self);
};
#define EDIT3_CARET_LINE(self) ((self)->edit.text_y)
int ui_edit_ctrl3_send_msg(struct UIWindow *self, struct UIWindow *sender, int msg, int val1, int val2,
                                   int val3, int val4);
void ui_edit_ctrl3_draw_text(struct UIEditCtrl *base_edit);
void ui_edit_ctrl3_place_navi_links(struct UIEditCtrl3 *self, struct TextView line, int y, int index, int first,
                                int last);
void ui_edit_ctrl3_rewrap(struct UIEditCtrl3 *self, struct TextArray *lines);
void ui_edit_ctrl3_place_tags(struct UIEditCtrl3 *self, const int *x, const int *y, const int *first,
                                      const int *last, const int *line);
static inline void ui_edit_ctrl3_update_bar(struct UIEditCtrl3 *edit)
{
    const struct UIEditCtrl3Ops *ops = (const struct UIEditCtrl3Ops *)edit->edit.base.ops;
    ops->update_bar(edit);
}

struct UIWindowMgr;
void ui_window_mgr_focus_edit_lost(struct UIWindowMgr *self);

void ui_edit_ctrl_destroy(struct UIWindow *window);
void ui_edit_ctrl_on_l_btn_down(struct UIWindow *self, int x_unused, int y_unused);
void ui_edit_ctrl_on_mouse_hover(struct UIWindow *self, int x, int y);
void ui_edit_ctrl_draw_text_default(struct UIEditCtrl *self);
void ui_edit_ctrl4_draw_text(struct UIEditCtrl *base_edit);
void ui_edit_ctrl_truncate_default(struct UIEditCtrl *self);
void ui_edit_ctrl_make_select(struct UIWindow *wnd);
struct TextView ui_edit_ctrl_link_text(const struct UIEditCtrl *self);

void ui_edit_ctrl3_destroy(struct UIWindow *window);
void ui_edit_ctrl3_resize(struct UIWindow *self, int width, int height);
void ui_edit_ctrl3_on_create(struct UIWindow *self, int width, int height);
void ui_edit_ctrl3_on_wheel(struct UIWindow *self, int delta);
void ui_edit_ctrl3_set_text(struct UIEditCtrl *base_edit, const char *text);
void ui_edit_ctrl_truncate_to_ime(struct UIEditCtrl *base_edit);
void ui_edit_ctrl3_update_bar_default(struct UIEditCtrl3 *self);
struct String *ui_edit_ctrl3_join_lines(struct UIEditCtrl3 *self, struct String *out);

void ui_edit_ctrl_custom_destruct(struct UIEditCtrlCustom *self);
void ui_edit_ctrl_custom_destroy(struct UIWindow *window);
void ui_edit_ctrl_custom_draw_text(struct UIEditCtrl *base_edit);
void ui_edit_ctrl_currency_destroy(struct UIWindow *window);
void ui_edit_ctrl_currency_on_process(struct UIWindow *window);
int ui_edit_ctrl_currency_send_msg(struct UIWindow *self, struct UIWindow *sender,
                                          int msg, int val1, int val2, int val3, int val4);

void ui_edit_ctrl2_destruct(struct UIEditCtrl *self);
void ui_edit_ctrl2_destroy(struct UIWindow *window);
void ui_edit_ctrl4_destruct(struct UIEditCtrl4 *self);
void ui_edit_ctrl5_destruct(struct UIEditCtrl4 *self);
void ui_edit_ctrl4_destroy(struct UIWindow *window);
void ui_edit_ctrl5_destroy(struct UIWindow *window);
void ui_edit_ctrl4_on_focus(struct UIWindow *self);
void ui_edit_ctrl_var_id_destruct(struct UIEditCtrl *self);
void ui_edit_ctrl_var_id_destroy(struct UIWindow *window);
void ui_edit_ctrl_var_id_on_focus_lost(struct UIWindow *self);
void ui_edit_ctrl_wheel_to_parent(struct UIWindow *self, int delta);
int ui_edit_ctrl_var_id_send_msg(struct UIWindow *self, struct UIWindow *sender, int msg,
                                       int val1, int val2, int val3, int val4);
void ui_edit_ctrl_var_id_set_text(struct UIEditCtrl *self, const char *text);
void ui_edit_ctrl_wheel_destruct(struct UIEditCtrl *self);
void ui_edit_ctrl_wheel_destroy(struct UIWindow *window);
void ui_bitmap_edit_ctrl_destroy(struct UIWindow *window);
void ui_bitmap_edit_ctrl_set_bitmap(struct UIBitmapEditCtrl *self, const char *name);
void ui_bitmap_edit_ctrl_on_draw(struct UIWindow *self);

struct CandidatePage {
    int selected;
    int page;
    int last_page;
    int count;
};

struct UICandidateWnd *ui_candidate_wnd_construct(struct UICandidateWnd *self);
struct UIWindow *ui_candidate_wnd_new(void);
void ui_candidate_wnd_destroy(struct UIWindow *window);
void ui_candidate_wnd_on_create(struct UIWindow *self, int width_unused, int height_unused);
void ui_candidate_wnd_on_draw(struct UIWindow *self);
int ui_candidate_wnd_send_msg(struct UIWindow *self, struct UIWindow *sender, int msg,
                                      int val1, int val2, int val3, int val4);

void ui_edit_ctrl_custom_draw_string(struct UIEditCtrlCustom *self, int x, int y, const char *text);

#endif
