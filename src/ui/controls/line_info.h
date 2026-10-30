#ifndef UI_CONTROLS_LINE_INFO_H
#define UI_CONTROLS_LINE_INFO_H

#include "ui/window_types.h"
#include "std/string.h"

struct LineStyle {
    COLORREF colour;
    int font_height;
    BYTE bold, italic;
};

struct CreateLineInfo {
    struct UIWindow **windows;
    size_t count;
    int height;
};

struct CreateLineInfo *create_line_info_construct(struct CreateLineInfo *self,
    struct UIWindow *wnd, const char *text, int width, int line_spacing,
    struct LineStyle style, BYTE outlined, COLORREF outline_colour, BYTE align);
void create_line_info_destruct(struct CreateLineInfo *self);

struct LineCutterText {
    struct UIWindow base;
    COLORREF colour;
    int font_height;
    BYTE bold, italic, outlined;
    COLORREF outline_colour;
    struct String text;
};

struct LineCutterText *line_cutter_text_construct(struct LineCutterText *self);
void line_cutter_text_destruct(struct LineCutterText *self);
void line_cutter_text_destroy(struct UIWindow *window);
void line_cutter_text_on_draw(struct UIWindow *self);
void line_cutter_text_on_l_btn_down(struct UIWindow *self, int x, int y);
void line_cutter_text_on_l_btn_dbl_clk(struct UIWindow *self, int x,
                                            int y);
void line_cutter_text_on_mouse_move(struct UIWindow *self, int x, int y);
void line_cutter_text_on_mouse_shape(struct UIWindow *self, int x,
                                            int y);
void line_cutter_text_on_l_btn_up(struct UIWindow *self, int x, int y);
void line_cutter_text_on_r_btn_down(struct UIWindow *self, int x, int y);
void line_cutter_text_on_r_btn_up(struct UIWindow *self, int x, int y);
void line_cutter_text_on_wheel(struct UIWindow *self, int delta);
void line_cutter_text_set_text(struct LineCutterText *self, struct TextView text,
                                       struct LineStyle style, BYTE outlined, COLORREF outline_colour);

#endif
