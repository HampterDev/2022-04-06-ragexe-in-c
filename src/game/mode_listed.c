#include "game/gamemode.h"
#include "game/mode_util.h"
#include "ui/window_types.h"
#include "std/list.h"

/* The game mode's list of listed windows: an actor leaving drops its window, a changed guild
   emblem tells them all. Apart from game/mode_util.c, whose other helpers reach the renderer,
   the effects and the confirm boxes, so that the actors and the guild code do not. */

void game_mode_remove_listed(struct GameMode *self, void *obj)
{
    struct ListNode *head = self->listed.head;
    struct ListNode *n;

    for (n = head->next; n != head; n = n->next)
        if (*LIST_VALUE(n, void *) == obj)
            break;
    if (n == head)
        return;
    list_erase(&self->listed, n, NULL);
}

void game_mode_notify_listed(struct GameMode *self, int arg)
{
    struct ListNode *n;
    for (n = self->listed.head->next; n != self->listed.head; n = n->next) {
        struct UIWindow *window = *LIST_VALUE(n, struct UIWindow *);
        ui_window_send_msg(window, NULL, 0x3c, arg, 0, 0, 0);
    }
}
