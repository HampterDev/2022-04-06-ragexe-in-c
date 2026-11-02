#ifndef UI_ITEMS_REFINE_CONTROLS_H
#define UI_ITEMS_REFINE_CONTROLS_H

#include <windows.h>

struct UIWindow;
struct UIButton;

void ui_refine_button_set_show(struct UIButton *button, unsigned char show);

void ui_refining_btn_refining_destroy(struct UIWindow *window);
void ui_refining_btn_again_destroy(struct UIWindow *window);
void ui_refining_btn_return_destroy(struct UIWindow *window);
void ui_refining_notice_destroy(struct UIWindow *window);

void ui_refining_btn_again_on_draw(struct UIWindow *window);
void ui_refining_btn_refining_on_draw(struct UIWindow *window);
void ui_refining_btn_return_on_draw(struct UIWindow *window);
void ui_refining_notice_on_process(struct UIWindow *window);
void ui_refining_notice_on_draw(struct UIWindow *window);

#endif
