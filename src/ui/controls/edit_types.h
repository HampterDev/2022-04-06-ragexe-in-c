#ifndef UI_CONTROLS_EDIT_TYPES_H
#define UI_CONTROLS_EDIT_TYPES_H

#include "ui/window_types.h"
#include "std/string.h"
#include "core/text_array.h"
#include "core/byte_array.h"
#include "ui/items/item_tag_buttons.h"
#include "std/container_types.h"

struct UIScrollBar;
struct UINaviLinkButton;

struct UIEditCtrl {
    struct UIWindow base;
    int sel_start, sel_end;
    int mask_char;
    int max_chars;
    int no_border;
    int back_r, back_g, back_b;
    int border_r, border_g, border_b;
    COLORREF text_colour;
    int double_colour;
    int scroll;
    int text_y;
    int multiline;
    struct String tooltip;
    BYTE disabled;
    struct String text;
    int text_top;
    BYTE keep_ime_on_enter;
    struct UINaviLinkButton *navigation_link;
    struct String raw_text;
};

struct UIEditCtrl4 {
    struct UIEditCtrl edit;
    COLORREF alt_colour;
    struct String empty_text;
    BYTE use_alt;
};

struct UIBitmapEditCtrl {
    struct UIEditCtrl edit;
    struct String bitmap;
};

struct UIEditCtrlCustom {
    struct UIEditCtrl edit;
    BYTE font_flag;
    int offset_x;
    int offset_y;
    int align;
};

struct UIEditCtrlCurrency {
    struct UIEditCtrlCustom custom;
    int formatted_size;
};

struct UIEditCtrl3 {
    struct UIEditCtrl edit;
    struct ItemTagButtons tag_buttons;
    struct TextArray lines;
    struct UIScrollBar *bar;
    int scrolls;
    int line_height;
    int line_count;
    int caret_line_len;
    int caret_col;
    int caret_rest;
    int caret_x;
    int step;
    int step_caret;
    int first_line;
    BYTE scrolled;
    BYTE stepped_up, stepped_down;
    struct String line_above;
    struct String line_below;
    struct TextArray shown_lines;
    struct ByteArray line_flags;
    BYTE caret_eol;
    BYTE extra_line;
};

#endif
