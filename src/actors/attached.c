#include "actors/attached.h"
#include "actors/types.h"
#include "std/list.h"

void game_actor_detach(struct GameActor *self, struct ListNode *node, const struct GameObject *sender)
{
    const struct GameObject *value;

    if (!node || *LIST_VALUE(node, struct GameObject *) != sender)
        return;
    value = sender;
    if (self->cart == value) self->cart = 0;
    else if (self->falcon == value) self->falcon = 0;
    else if (self->emotion_mark == value) self->emotion_mark = 0;
    else if (self->frozen_mark == value) self->frozen_mark = 0;
    else if (self->curse_mark == value) self->curse_mark = 0;
    else if (self->sleep_mark == value) self->sleep_mark = 0;
    else if (self->stun_mark == value) self->stun_mark = 0;
    else if (self->msg_mark_1 == value) self->msg_mark_1 = 0;
    else if (self->msg_mark_2 == value) self->msg_mark_2 = 0;
    else if (self->emotion_mark_a == value) self->emotion_mark_a = 0;
    else if (self->emotion_mute == value) self->emotion_mute = 0;
    else if (self->cursor_mark == value) self->cursor_mark = 0;
    list_erase(&self->attached, node, NULL);
}
