#ifndef UI_CHARACTER_CHAR_PREVIEW_H
#define UI_CHARACTER_CHAR_PREVIEW_H

#include "std/string.h"

struct CharPreview {
    int x, y;
    struct String act[5];
    struct String spr[5];
    struct String imf;
    int head_palette;
    int body_palette;
    struct String head_palette_name;
    struct String body_palette_name;
    int action_base;
    int action;
    int frame;
    int job;
    int robe;
};
void char_preview_init(struct CharPreview *preview);
void char_preview_destroy(struct CharPreview *preview);

struct UIMakeCharWnd *ui_make_char_wnd_construct(struct UIMakeCharWnd *self);
struct UIWindow *ui_make_char_wnd_new(void);

#endif
