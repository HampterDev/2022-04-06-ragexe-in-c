#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include "ui/quests/quest_journal_layout.h"
#include "ui/draw.h"

static int background;
static DWORD pixels[384 * 464];

void ui_window_draw_body(struct UIWindow *wnd, int top, int width, const char *path)
{
    int x, y;
    const char *number = strstr(path, "bg_quest");
    assert(number && !top && !width);
    background = atoi(number + 8);
    for (y = 0; y < wnd->canvas->height; ++y)
        for (x = 0; x < wnd->canvas->width; ++x)
            pixels[y * wnd->canvas->width + x] = x % 7 ? (DWORD)(y + 1) : UI_TRANSPARENT;
}

void ui_window_fill_rect(struct UIWindow *wnd, int x, int y, int width, int height, COLORREF color)
{
    int row, col;
    assert(x >= 0 && y >= 0 && x + width <= wnd->canvas->width && y + height <= wnd->canvas->height);
    for (row = y; row < y + height; ++row)
        for (col = x; col < x + width; ++col)
            pixels[row * wnd->canvas->width + col] = color;
}

int main(void)
{
    static const int expected_skin[] = {1, 3, 4};
    struct DcBitmap canvas = {.width = 384, .height = 464, .bits = pixels};
    struct UIWindow wnd = {.canvas = &canvas};
    int tab, x, y;

    assert(QUEST_TAB_COUNT == 3);
    for (tab = 0; tab < QUEST_TAB_COUNT; ++tab) {
        quest_journal_draw_background(&wnd, tab);
        assert(background == expected_skin[tab]);
        for (y = 0; y < canvas.height; ++y) {
            for (x = 0; x < canvas.width; ++x) {
                DWORD expected = x % 7 ? (DWORD)(y + 1) : UI_TRANSPARENT;
                if (x < 29 && y >= 118 && y < 320)
                    expected = x % 7 ? (DWORD)(y + 102) : UI_TRANSPARENT;
                if (x < 29 && y >= 320 && y < 421)
                    expected = UI_TRANSPARENT;
                assert(pixels[y * canvas.width + x] == expected);
            }
        }
    }
    puts("Quest journal: three retained skins and exact tab-strip pixels including transparency passed");
    return 0;
}
