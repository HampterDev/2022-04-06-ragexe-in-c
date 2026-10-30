#ifndef UI_WINDOW_PAINT_H
#define UI_WINDOW_PAINT_H

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>

struct UIWindow;
struct BitmapRes;
struct ItemInfo;
struct String;
struct Vec;

void ui_window_clear_canvas(struct UIWindow *self, DWORD colour);

void ui_window_fill_rect(struct UIWindow *self, int x, int y, int width,
                                  int height, COLORREF colour);

void ui_window_fill_round_rect(struct UIWindow *self, int x, int y, int width,
                                       int height, COLORREF colour);

void ui_window_draw_round_frame(struct UIWindow *self, int x, int y, int width,
                                        int height, COLORREF colour);

void ui_window_draw_bitmap(struct UIWindow *self, int x, int y, struct BitmapRes *bitmap,
                                    int keep_alpha);

void ui_window_draw_bitmap_tinted(struct UIWindow *self, int tint, int x,
                                          int y, struct BitmapRes *bitmap, int keep_alpha,
                                          BYTE remake);

int ui_window_draw_title_bar(struct UIWindow *self, int sys_icon, const char *title,
                                     int width);

void ui_window_text_out_centred(struct UIWindow *self, int x, int y, const char *text,
                                        int width, int font_face, int font_height, COLORREF colour, int font_arg);

void ui_window_text_out(struct UIWindow *self, int x, int y, const char *text, int len,
                                 int font_face, int font_height, COLORREF colour, int font_arg3, int font_arg4);

void ui_window_text_out_right(struct UIWindow *self, int x, int y, const char *text,
                                      int len, int font_face, int font_height, COLORREF colour, int font_arg);

void ui_window_text_out_centred_outlined(struct UIWindow *self, int x, int y,
                                                const char *text, int width, int len, COLORREF colour,
                                                COLORREF outline, int font_face, int font_height, int font_arg);

void ui_window_draw_shadow_text(struct UIWindow *self, int x, int y, const char *text,
                                        int len, COLORREF colour, COLORREF shadow, int font_face, int font_height,
                                        int font_arg);

int ui_window_text_height(struct UIWindow *self, const char *text, int len,
                                   int font_face, int font_height, int font_arg3, int font_arg4);

int ui_window_text_width(struct UIWindow *self, const char *text, int len, int font_face,
                                  int font_height, int font_arg3, int font_arg4);

struct String *ui_window_fit_text(struct UIWindow *self, struct String *out,
                                              int width, const char *text, int font_face, int font_height,
                                              int font_arg3);

#endif
