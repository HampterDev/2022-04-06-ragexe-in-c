#ifndef UI_CHAT_BALLOON_H
#define UI_CHAT_BALLOON_H
#include "ui/window_types.h"
#include "core/text_array.h"

#define BALLOON_TEXT_SIZE 0xa0
struct UIBalloonText {
    struct UIWindow base;
    BYTE bold;
    int font_height;
    struct TextArray lines;
    COLORREF colour;
    COLORREF outline;
    BYTE back_transparent;
    WORD font_code;
    BYTE colour_codes;
};
OFFSET_ASSERT(UIBalloonText, lines, 0x84);
OFFSET_ASSERT(UIBalloonText, font_code, 0x9a);
SIZE_ASSERT(UIBalloonText, BALLOON_TEXT_SIZE);

#define NAME_BALLOON_TEXT_SIZE 0xc0
struct UINameBalloonText {
    struct UIBalloonText balloon;
    struct String emblem;
    int text_length;
    BYTE mark;
    BYTE emblem_frame;
};
OFFSET_ASSERT(UINameBalloonText, emblem, 0xa0);

struct UIVerticalNameBalloonText {
    struct UINameBalloonText name;
    int text_length;
};

struct ActorNameCard;
struct UINameBalloonTextOps {
    struct UIWindowOps base;
    void (*set_text)(struct UINameBalloonText *self, const char *text);
};

struct UINameBalloonText *ui_name_balloon_text_construct(struct UINameBalloonText *self);
struct UIVerticalNameBalloonText *vertical_name_balloon_construct(struct UIVerticalNameBalloonText *self);
void ui_name_balloon_text_destruct(struct UINameBalloonText *self);
void vertical_name_balloon_deleting_destruct(struct UIWindow *window);
void ui_name_balloon_text_set_text(struct UINameBalloonText *self, const char *text);
void ui_vertical_name_balloon_text_set_text(struct UINameBalloonText *name, const char *text);
void ui_name_balloon_text_set_name_text(struct UINameBalloonText *self,
    const struct ActorNameCard *card, BYTE strip_hash, BYTE mark);
void ui_name_balloon_text_set_emblem(struct UINameBalloonText *self, const char *path, BYTE frame);

struct BalloonLineColours {
    COLORREF *items;
    size_t size, capacity;
};

#define TRANS_BALLOON_TEXT_SIZE 0xcc
struct UITransBalloonText {
    struct UIBalloonText balloon;
    struct UIBackdropBox backdrop;
    struct BalloonLineColours line_colours;
    COLORREF frame_colour;
};
OFFSET_ASSERT(UITransBalloonText, backdrop, 0xa0);
OFFSET_ASSERT(UITransBalloonText, line_colours, 0xbc);
SIZE_ASSERT(UITransBalloonText, TRANS_BALLOON_TEXT_SIZE);

struct UIWindow *ui_trans_balloon_text_construct(struct UIWindow *self);
void ui_balloon_text_set_text(struct UIBalloonText *self, const char *text, int line_chars);

BYTE ui_trans_balloon_text_get_backdrop_box(struct UIWindow *window, struct UIBackdropBox *out);

struct TextView ui_trans_balloon_text_get_line(const struct UITransBalloonText *self, size_t index);
void ui_name_balloon_text_on_process(struct UIWindow *window);
void ui_vertical_name_balloon_text_on_draw(struct UIWindow *window);
void ui_trans_balloon_text_resize(struct UIWindow *window, int width, int height);
void ui_trans_balloon_text_move(struct UIWindow *window, int x, int y);
void ui_trans_balloon_text_on_create(struct UIWindow *window, int width_unused, int height_unused);
void ui_color_trans_balloon_text_on_draw(struct UIWindow *window);

struct UIAchBalloonText *ui_ach_balloon_text_new(void);
void ui_balloon_text_set_font_height(struct UIBalloonText *self, int font_height);
void ui_balloon_text_set_lines(struct UIBalloonText *self, const char *text, int line_chars);
void ui_trans_balloon_text_add_colored_line(struct UITransBalloonText *self, const char *text, COLORREF colour);
void ui_trans_balloon_text_add_line(struct UITransBalloonText *self, const char *text);
void ui_trans_balloon_text_destruct(struct UITransBalloonText *self);
void ui_trans_balloon_text_set_first_line(struct UITransBalloonText *self, const char *text, COLORREF colour);
void ui_trans_balloon_text_set_line(struct UITransBalloonText *self, const char *text, int index);

#endif
