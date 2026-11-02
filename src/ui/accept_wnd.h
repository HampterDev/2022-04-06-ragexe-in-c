#ifndef UI_ACCEPT_WND_H
#define UI_ACCEPT_WND_H

#include "ui/window.h"
#include "core/object_type_tables.h"

struct UIAcceptWnd {
    struct UIFrameWnd frame;
    int requester;
    struct UIWindow *text;
    int info;
};

static __inline struct UIAcceptWnd *accept_construct(struct UIAcceptWnd *self, const struct UIWindowOps *ops)
{
    ui_frame_wnd_construct(&self->frame, 0);
    self->frame.base.ops = &ui_choose_sell_buy_wnd_ops;
    self->requester = 0;
    self->frame.base.ops = &ui_exchange_accept_wnd_ops;
    self->info = 0;
    self->frame.base.ops = ops;
    return self;
}

static __inline void accept_destruct(struct UIAcceptWnd *self, const struct UIWindowOps *ops)
{
    self->frame.base.ops = ops;
    self->frame.base.ops = &ui_exchange_accept_wnd_ops;
    self->frame.base.ops = &ui_choose_sell_buy_wnd_ops;
    ui_frame_wnd_destruct(&self->frame);
}

void ui_exchange_accept_wnd_store_info(struct UIWindow *window);
void ui_exchange_accept_wnd_on_create(struct UIWindow *window, int width, int height);
void ui_accept_wnd_on_draw(struct UIWindow *window);

#endif
