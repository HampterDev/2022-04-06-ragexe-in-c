#ifndef UI_ITEMS_REFINE_CONTROLS_H
#define UI_ITEMS_REFINE_CONTROLS_H

#include "platform/win_types.h"

struct UIWindow;
struct UIButton;

void ui_refine_button_set_show(struct UIButton *button, unsigned char show);

void ui_grade_enchant_btn_start_destroy(struct UIWindow *window);
void ui_grade_enchant_btn_enhanced_up_destroy(struct UIWindow *window);
void ui_grade_enchant_btn_enhanced_down_destroy(struct UIWindow *window);
void ui_refining_btn_refining_destroy(struct UIWindow *window);
void ui_refining_btn_again_destroy(struct UIWindow *window);
void ui_refining_btn_return_destroy(struct UIWindow *window);
void ui_refining_notice_destroy(struct UIWindow *window);

void ui_grade_enchant_btn_enhanced_down_on_draw(struct UIWindow *window);
void ui_grade_enchant_btn_enhanced_up_on_draw(struct UIWindow *window);
void ui_grade_enchant_btn_start_on_draw(struct UIWindow *window);
void ui_refining_btn_again_on_draw(struct UIWindow *window);
void ui_refining_btn_refining_on_draw(struct UIWindow *window);
void ui_refining_btn_return_on_draw(struct UIWindow *window);
void ui_refining_notice_on_process(struct UIWindow *window);
void ui_refining_notice_on_draw(struct UIWindow *window);

struct GradeBlessing {
    int item_id;
    int step;
    int max_steps;
    int bonus;
};

struct UIGradeEnchantWnd;
void grade_enchant_wnd_set_result(struct UIGradeEnchantWnd *self, int state,
                                            int result);
struct UIGradeEnchantWnd *ui_grade_enchant_wnd_construct(struct UIGradeEnchantWnd *self);
struct UIWindow *ui_grade_enchant_wnd_new(void);
void ui_grade_enchant_wnd_set_materials(struct UIGradeEnchantWnd *self,
                                               unsigned short index, int success_chance, int count,
                                               struct GradeBlessing blessing, int protect_item_id,
                                               int protect_amount, const BYTE *materials);

#endif
