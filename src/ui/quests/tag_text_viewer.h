#ifndef UI_QUESTS_TAG_TEXT_VIEWER_H
#define UI_QUESTS_TAG_TEXT_VIEWER_H

#include "ui/controls/line_info.h"

struct UITextButton;
struct TagTextPosition;

struct UITagTextViewer {
    struct UIWindow base;
    int scroll_pos;
    struct UIWindow *vbar;
    struct TagTextPosition *items;
    size_t item_count, item_capacity;
    DWORD bg_argb;
    int wrap_width, line_spacing, margin_x, margin_y;
    struct LineStyle style;
    BYTE outlined;
    COLORREF outline_colour;
    BYTE align_h, align_v;
    struct String text;
    int has_text;
};

struct UITagTextViewer *ui_tag_text_viewer_create(int width, int height);
void ui_tag_text_viewer_append_text(struct UITagTextViewer *self, const char *text);
void ui_tag_text_viewer_remove_all(struct UITagTextViewer *self);
void ui_tag_text_viewer_layout(struct UITagTextViewer *self);
void ui_tag_text_viewer_update_scroll(struct UITagTextViewer *self);
int ui_tag_text_viewer_content_height(struct UITagTextViewer *self);
void ui_tag_text_viewer_set_align(struct UITagTextViewer *self, unsigned char align);
void ui_tag_text_viewer_on_navi_link(struct UITagTextViewer *self, struct UITextButton *link);
intptr_t ui_tag_text_viewer_send_msg(struct UIWindow *window, struct UIWindow *sender, int msg, intptr_t val1,
                                       intptr_t val2, intptr_t val3, intptr_t val4);

#define UI_MSG_TAG_ADD_LINE 0x1d

#endif
