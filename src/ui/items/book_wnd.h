#ifndef UI_ITEMS_BOOK_WND_H
#define UI_ITEMS_BOOK_WND_H
#include "ui/window.h"
#include "core/text_array.h"

#define BOOK_WND_ID 0x6a
#define BOOK_SPEAK_INTERVAL 4000
#define BOOK_SPEAK_COLOUR 0xe1e4ff

struct UIBookWnd {
    struct UIFrameWnd frame;
    struct UIWindow *text;
    struct UIWindow *balloon;
    char title[0x64];
    int line_count;
    int page_count;
    int page;
    int speak_line;
    DWORD speak_time;
    BYTE speaking;
    int red, green, blue;
    struct TextArray lines;
    struct UIWindow *buttons[4];
};
OFFSET_ASSERT(UIBookWnd, line_count, 0x100);
OFFSET_ASSERT(UIBookWnd, red, 0x118);
OFFSET_ASSERT(UIBookWnd, lines, 0x124);
SIZE_ASSERT(UIBookWnd, 0x140);

static __inline const char *book_line(const struct UIBookWnd *self, int i)
{
    return text_array_at(&self->lines, (size_t)i).data;
}

void ui_book_wnd_on_process(struct UIWindow *self);

struct UIBookWnd *ui_book_wnd_construct(struct UIBookWnd *self);
struct UIWindow *ui_book_wnd_new(void);

#endif
