#include "ui/window_access.h"
#include "ui/chat/balloon.h"
#include "ui/items/book_wnd.h"
#include "core/text_format.h"
#include "platform/code_page.h"
#include "ui/controls/popups.h"
#include "ui/window_mgr.h"
#include <mmsystem.h>

#define UIWM_MSG_SHOW_CHAT 3

void ui_book_wnd_on_process(struct UIWindow *window)
{
    struct UIBookWnd *self = (struct UIBookWnd *)window;
    struct OwnedText spoken = {0};
    int line;

    if (!self->speaking || !self->speak_time || timeGetTime() <= self->speak_time + BOOK_SPEAK_INTERVAL)
        return;
    line = self->speak_line;
    if (line >= self->line_count) {
        self->speak_time = 0;
        if (self->balloon) {
            ui_window_mgr_post_quit2(&g_windowMgr, self->balloon);
            self->balloon = NULL;
        }
        ui_close_window(BOOK_WND_ID);
        return;
    }
    {
        const char *text = book_line(self, line);

        if ((strlen(text) == 0 || strcmp(text, " ") == 0) && line < self->line_count - 1)
            self->speak_line = line + 1;
    }
    if (!text_copy_without_colours(&spoken, text_view_cstr(book_line(self, self->speak_line)),
                                   g_codePage, g_useCharsetTag))
        return;
    ui_window_mgr_send_msg_(&g_windowMgr, UIWM_MSG_SHOW_CHAT, 0, 0, 0, 0);
    ui_chat_line(owned_text_cstr(&spoken), BOOK_SPEAK_COLOUR);
    if (!self->balloon) {
        void *mem = mem_alloc(sizeof(struct UITransBalloonText));

        self->balloon = mem ? ui_trans_balloon_text_construct(mem) : NULL;
        if (!self->balloon) {
            owned_text_clear(&spoken);
            return;
        }
        ui_window_create(self->balloon, 8, 8);
        ui_window_mgr_add_to_window_list(&g_windowMgr, self->balloon);
    }
    ui_balloon_text_set_text((struct UIBalloonText *)self->balloon, owned_text_cstr(&spoken), 100);
    owned_text_clear(&spoken);
    ui_window_move(self->balloon, (g_renderer->width - 640) / 2 - self->balloon->width / 2 + 320, 0x51);
    ((struct UIBalloonText *)self->balloon)->colour = BOOK_SPEAK_COLOUR;
    ((struct UIBalloonText *)self->balloon)->outline = 0;
    self->speak_line++;
    self->speak_time = timeGetTime();
}
