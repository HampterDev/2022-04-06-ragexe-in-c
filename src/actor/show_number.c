#include "session/state.h"
#include "game/context.h"
#include "actor/actor.h"
#include "std/std.h"
#include "actor/game_actor.h"
#include "effects/skills/msg_effect.h"

void game_actor_show_number(struct GameActor *self, int value, unsigned argb, int style)
{
    struct Vec digits = { NULL, NULL, NULL };
    int n = value >= 0 ? value : 0;
    struct GameMode *mode = game_current_mode();
    const int *d;
    int count, i;

    if (!mode || !game_current_world())
        return;
    if (g_session1.extendDamage != 0) {
        int marker = 0;
        if (n >= 100000000) {
            marker = 11;
            n /= 1000000;
        } else if (n >= 1000000) {
            marker = 10;
            n /= 1000;
        }
        if (marker != 0)
            vec_push(&digits, &marker, sizeof marker);
    } else if (n > 999999) {
        n = 999999;
    }
    do {
        int digit = n % 10;

        vec_push(&digits, &digit, sizeof digit);
        n /= 10;
    } while (n != 0);

    if (style == 0x16 && self->attached.size != 0) {
        struct ListNode *node = self->attached.head->next;
        while (node != self->attached.head) {
            struct MsgEffect *obj = *LIST_VALUE(node, struct MsgEffect *);
            if (obj->style == 0x16) {
                game_actor_forget_handle(self, (struct GameObject *)obj);
                msg_effect_send_msg((struct GameObject *)obj, (struct GameObject *)self, 0x18, 0, 0, 0, 0);
                node = list_erase(&self->attached, node, NULL);
                game_remove_object(obj);
            } else {
                node = node->next;
            }
        }
    }

    count = (int)VEC_COUNT(&digits, int);
    for (d = (const int *)digits.first, i = 0; d != (const int *)digits.last; d++, i++) {
        void *mem = mem_alloc(sizeof(struct MsgEffect));
        struct RenderObject *e = mem != NULL ? (struct RenderObject *)msg_effect_construct(mem) : NULL;
        if (!e)
            break;
        e->argb = argb;
        render_object_send(e, NULL, 0x16, self->aid, style, 0, 0);
        render_object_send(e, NULL, 0x40, (intptr_t)&self->pos, *d, (count - 2 * i) * 4 - 4, 0);
        render_object_send(e, self, 0x32, 0, 0, 0, 0);
        e->cell_lit = 0;
        if (style == 0x16)
            e->zoom = 0.5f;
        game_mode_add_world_object(mode, e);
        list_push_back(&self->attached, &e, sizeof e);
    }
    vec_free(&digits);
}
