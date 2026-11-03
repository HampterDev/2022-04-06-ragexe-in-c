#include "platform/code_page.h"
#include "core/utf8.h"
#include "core/text_format.h"
#include "iteminfo/iteminfo.h"
#include "ui/items/item_name_runs.h"
#include "ui/window_mgr.h"
#include "ui/draw.h"
#include "core/client.h"
#include "core/msg_str.h"
#include <limits.h>
#include <stdlib.h>
#include <string.h>
#include "platform/language.h"
#include "ui/wndmgr/wndmgr.h"

#define MAKER_NAME_COLOUR 0xe04040
#define MAKER_WAIT_COLOUR 0x4040e0
#define MSG_NAME_WAITING 0x245

void item_name_runs_end(struct ItemNameRuns *runs)
{
    if (runs->count == sizeof runs->ends / sizeof runs->ends[0] || runs->text.size > INT_MAX)
        abort();
    runs->ends[runs->count++] = runs->text.size;
}

static void piece_text(struct ItemNameRuns *runs, const char *text)
{
    string_append_cstr(&runs->text, text);
    item_name_runs_end(runs);
}

static void piece(struct ItemNameRuns *runs, const struct ItemInfo *item,
                  void (*append)(const struct ItemInfo *, struct String *))
{
    append(item, &runs->text);
    item_name_runs_end(runs);
}

static void maker_name(struct UIWindow *window, const struct ItemInfo *item,
                       struct ItemNameRuns *runs)
{
    const char *name = item_info_get_maker_name(item);
    if (*name) {
        runs->marked = runs->text.size;
        runs->mark_colour = MAKER_NAME_COLOUR;
    } else {
        ui_window_mgr_add_to_name_waiting_list(&g_windowMgr, window);
        name = msg_str(MSG_NAME_WAITING);
        runs->mark_colour = MAKER_WAIT_COLOUR;
    }
    piece_text(runs, name);
}

void item_info_name_runs(struct UIWindow *self, const struct ItemInfo *info,
                       struct ItemNameRuns *runs, int twice_maker, int with_sockets)
{
    twice_maker = (BYTE)twice_maker;
    with_sockets = (BYTE)with_sockets;
    string_clear(&runs->text);
    runs->count = 0;
    runs->marked = (size_t)-1;
    runs->mark_colour = 0;
    if (!info->identified) {
        piece_text(runs, item_info_get_unid_name(info));
        return;
    }
    if (info->type == 4 && item_info_is_pet_range_id(info) == 1 && info->slot[3] == 1)
        piece(runs, info, item_name_append_pet_prefix);

    if (info->slot[0] == 0xfe && info->slot[1] == 0) {
        if (info->refine)
            piece(runs, info, item_name_append_refine);
        maker_name(self, info, runs);
        piece_text(runs, item_info_get_maker_element_text(info));
        piece_text(runs, item_info_get_name(info));
        return;
    }

    if (!item_info_is_equipment_type(info)) {
        piece_text(runs, item_info_get_name(info));
        if (with_sockets == 1)
            piece(runs, info, item_name_append_sockets);
        return;
    }
    if (info->refine)
        piece(runs, info, item_name_append_refine);
    if (item_info_has_card(info)) {
        piece(runs, info, item_name_append_card_prefixes);
        piece_text(runs, item_info_get_name(info));
        piece(runs, info, item_name_append_card_postfixes);
    } else if (!item_info_has_maker_slot(info)) {
        piece_text(runs, item_info_get_name(info));
    } else {
        piece_text(runs, item_info_get_maker_star_text(info));
        runs->marked = runs->text.size;
        maker_name(self, info, runs);
        if (twice_maker)
            item_name_runs_end(runs);
        piece_text(runs, item_info_get_maker_element_text(info));
        piece_text(runs, item_info_get_name(info));
        return;
    }
    if (with_sockets == 1)
        piece(runs, info, item_name_append_sockets);
    return;
}

int ui_window_draw_runs_shortened(struct UIWindow *self, int x, int y, int max_width,
                               const struct ItemNameRuns *runs, COLORREF colour, int line_height)
{
    size_t i = 0;
    const char *text = string_cstr(&runs->text);
    const char *mark_start = runs->marked == (size_t)-1 ? NULL : text + runs->marked;
    COLORREF mark_colour = runs->mark_colour;
    const char *p = text;
    int drawn = 0;
    int line;

    if (ui_window_text_width(self, text, 0, 0, 0xc, 0, 0) < max_width) {
        for (; i < runs->count; ++i) {
            const char *end = text + runs->ends[i];
            int len = (int)(end - p);

            if (!len)
                continue;
            ui_window_draw_shadow_text(self, x + drawn, y, p, len, p == mark_start ? mark_colour : colour,
                                    0xe0e0e0, 0, 0xc, 0);
            drawn += ui_window_text_width(self, p, len, 0, 0xc, 0, 0);
            p = end;
        }
        return x;
    }

    line = 1;
    y += -(line_height / 2);
    for (; i < runs->count; ++i) {
        const char *run_start = p, *run_end_at = text + runs->ends[i];

        while (p < run_end_at) {
            const char *next = utf8_step(p, run_end_at);
            int len, rest, width;

            len = (int)(next - p);
            rest = ui_window_text_width(self, next, (int)strlen(next), 0, 0xc, 0, 0);
            width = ui_window_text_width(self, p, len, 0, 0xc, 0, 0) + drawn;
            if (line == 2 && width >= max_width - 15 && width + rest > max_width) {
                if (run_start == mark_start)
                    ui_window_draw_shadow_text(self, x + drawn + len, y, "...", 3, mark_colour, 0xe0e0e0, 0,
                                            0xc, 0);
                else
                    ui_window_draw_shadow_text(self, x + drawn + len, y - 1, "...", 3, colour, 0xe0e0e0, 0,
                                            0xc, 0);
                break;
            }
            ui_window_draw_shadow_text(self, x + drawn, y, p, len, run_start == mark_start ? mark_colour : colour,
                                    0xe0e0e0, 0, 0xc, 0);
            drawn += ui_window_text_width(self, p, len, 0, 0xc, 0, 0);
            if (drawn > max_width) {
                y += 1 + line_height;
                drawn = 0;
                line++;
            }
            p = next;
        }
    }
    return x;
}
