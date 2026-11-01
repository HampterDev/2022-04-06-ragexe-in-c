#include "ui/quests/quest_journal_layout.h"
#include "ui/draw.h"
#include "ui/skin.h"
#include <stdio.h>
#include <string.h>

#define QUEST_UI_DIR UI_SKIN "renew_questui\\"

void quest_journal_draw_background(struct UIWindow *wnd, int tab)
{
    static const int skins[QUEST_TAB_COUNT] = {1, 3, 4};
    struct DcBitmap *canvas = wnd->canvas;
    char path[MAX_PATH];
    int y;

    sprintf(path, QUEST_UI_DIR "bg_quest%d.bmp", skins[tab]);
    ui_window_draw_body(wnd, 0, 0, path);
    if (!canvas || !canvas->bits || canvas->width < QUEST_TAB_WIDTH
        || canvas->height < QUEST_TAB_Y + 4 * QUEST_TAB_HEIGHT)
        return;
    for (y = QUEST_TAB_Y + QUEST_TAB_HEIGHT; y < QUEST_TAB_Y + QUEST_TAB_COUNT * QUEST_TAB_HEIGHT; ++y)
        memmove(canvas->bits + y * canvas->width, canvas->bits + (y + QUEST_TAB_HEIGHT) * canvas->width,
                QUEST_TAB_WIDTH * sizeof *canvas->bits);
    ui_window_fill_rect(wnd, 0, QUEST_TAB_Y + QUEST_TAB_COUNT * QUEST_TAB_HEIGHT,
                        QUEST_TAB_WIDTH, QUEST_TAB_HEIGHT, UI_TRANSPARENT);
}
