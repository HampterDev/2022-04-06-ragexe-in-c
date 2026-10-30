#include "ui/window_mgr.h"
#include "ui/game.h"
#include "ui/ui_items.h"
#include "ui/controls/controls.h"
#include "ui/ui_std.h"
#include <mmsystem.h>
#include <stdlib.h>
#include <string.h>
#include "iteminfo/iteminfo.h"

#define NAME_FONT 0xc
#define MARK_SHADOW 0xe0e0e0
#define FLAGGED_SHADOW 0x5050fa
#define NAME_SHADOW 0xffffff

static COLORREF name_colour(const struct ItemInfo *info)
{
    COLORREF rgb = item_info_get_name_color(info);

    if (rgb == 0xadff2f)
        rgb = 0x669900;
    return (rgb >> 16 & 0xff) | (rgb & 0xff) << 16 | (rgb & 0xff00);
}

int ui_window_draw_item_name(struct UIWindow *self, int x, int y,
                                     const struct ItemInfo *info, COLORREF colour,
                                     int arg7)
{
    struct ItemNameRuns runs = {0};
    const char *run;
    size_t i, start = 0;
    int width = 0;

    item_info_name_runs(self, info, &runs, 0, arg7);
    run = string_cstr(&runs.text);
    for (i = 0; i < runs.count; ++i) {
        int len = (int)(runs.ends[i] - start);

        if (!len)
            continue;
        if (start == runs.marked)
            ui_window_draw_shadow_text(self, x + width, y, run, len, runs.mark_colour, MARK_SHADOW, 0, NAME_FONT, 0);
        else if (info->damaged)
            ui_window_draw_shadow_text(self, x + width, y, run, len, colour, FLAGGED_SHADOW, 0, NAME_FONT, 0);
        else
            ui_window_draw_shadow_text(self, x + width, y, run, len, name_colour(info), NAME_SHADOW, 0, NAME_FONT,
                                    0);
        width += ui_window_text_width(self, run, len, 0, NAME_FONT, 0, 0);
        start = runs.ends[i];
        run = string_cstr(&runs.text) + start;
    }
    string_free(&runs.text);
    return width;
}

int ui_window_draw_item_name_short(struct UIWindow *self, int x, int y, int max_width,
                                          const struct ItemInfo *info, int line_height)
{
    struct ItemNameRuns runs = {0};
    int result;

    item_info_name_runs(self, info, &runs, 1, 0);
    result = ui_window_draw_runs_shortened(self, x, y, max_width, &runs, name_colour(info), line_height);
    string_free(&runs.text);
    return result;
}
