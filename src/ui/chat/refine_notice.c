#include "ui/items/refine_controls.h"
#include <string.h>
#include <windows.h>
#include <mmsystem.h>
#include "ui/chat/chat.h"
#include "core/object_type_tables.h"
#include "ui/items/refining_internal.h"

#define REFINE_NOTICE_LINE_HEIGHT 0xb
#define REFINE_NOTICE_RED 0x2e15ca
#define REFINE_NOTICE_BLUE 0xc4281c

void ui_refining_notice_destroy(struct UIWindow *window)
{
    struct UIRefining_Notice *self = (struct UIRefining_Notice *)window;
    self->base.base.ops = &ui_refining_notice_ops;
    string_free(&self->next_text);
    ui_static_text_destruct(&self->base);
    mem_free(self);
}

static void destroy_string(void *value) { string_free(value); }

void ui_refining_notice_on_draw(struct UIWindow *window)
{
    struct UIRefining_Notice *self = (struct UIRefining_Notice *)window;
    struct UIWindow *wnd = &self->base.base;
    struct List lines, dropped;
    struct ListNode *node;
    struct String text;
    COLORREF colour;
    int offset;

    ui_window_clear_canvas(wnd, 0xffff00ff);
    if (self->base.text.size == 0)
        return;
    ui_window_draw_bitmap(wnd, 0, 3, res_mgr_get(g_resMgr(), UI_SKIN "refining_renewal\\ico_notice.bmp"),
                        0);
    colour = self->red == 1 ? REFINE_NOTICE_RED : REFINE_NOTICE_BLUE;
    list_init(&lines);
    string_copy(&text, &self->base.text);
    ui_window_wrap_text(wnd, &dropped, text, &lines, wnd->width - 0x12, 0, REFINE_NOTICE_LINE_HEIGHT, 0);
    list_free(&dropped, NULL);

    for (node = lines.head->next, offset = 0; node != lines.head; node = node->next,
         offset += REFINE_NOTICE_LINE_HEIGHT) {
        int y = (int)((double)(unsigned)(wnd->height - (int)lines.size * REFINE_NOTICE_LINE_HEIGHT) * 0.5
                      + (double)offset);

        ui_window_text_out(wnd, 0x12, y, string_cstr(LIST_VALUE(node, struct String)), 0, 0,
                         REFINE_NOTICE_LINE_HEIGHT, colour, 0, 0);
    }
    list_free(&lines, destroy_string);
}

void ui_refining_notice_on_process(struct UIWindow *window)
{
    struct UIRefining_Notice *self = (struct UIRefining_Notice *)window;
    if (self->due == 0 || timeGetTime() <= self->due)
        return;
    self->due = 0;
    if (self->hide_when_due == 1) {
        self->hide_when_due = 0;
        self->base.base.visible = 0;
    }
    string_assign(&self->base.text, string_cstr(&self->next_text), self->next_text.size);
    self->red = self->next_red;
    ui_window_invalidate_base(&self->base.base);
    if (self->base.base.parent)
        ui_window_invalidate(self->base.base.parent);
}
