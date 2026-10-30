#include "effects/rag/rag_effect.h"
#include "actor/object_ops.h"
#include "actor/actor.h"
#include "actor/game_actor.h"
#include "std/std.h"

static struct RagEffect *ae_launch(struct AbleToMakeEffect *self, int effect)
{
    Vec3 zero = { 0.0f, 0.0f, 0.0f };
    return game_actor_launch_effect((struct GameActor *)self, effect, zero, 0.0f);
}

static void ae_release(struct AbleToMakeEffect *self, struct RagEffect *e)
{
    rag_effect_send_msg(e, (struct RenderObject *)self, 0x18, 0, 0, 0);
}

static void ae_drop_held(struct AbleToMakeEffect *self, int *slot)
{
    struct ListNode *node;

    if (*slot == 0)
        return;
    for (node = self->effects.head->next; node != self->effects.head; node = node->next)
        if (*LIST_VALUE(node, int) == *slot)
            break;
    if (node != self->effects.head) {
        ae_release(self, *LIST_VALUE(node, struct RagEffect *));
        node = list_erase(&self->effects, node, NULL);
    }
    *slot = 0;
}

static void ae_hold(struct AbleToMakeEffect *self, int *slot, long long par1, long long par2, int relaunch)
{
    struct RagEffect *e = (struct RagEffect *)*slot;

    if (e == NULL) {
        e = ae_launch(self, (int)par1);
        *slot = (int)e;
        if (e == NULL)
            return;
    }
    if (relaunch && par1 > 0) {
        render_object_send(self, NULL, 0x6d, par1, 0, 0, 0);
        e = ae_launch(self, (int)par1);
        *slot = (int)e;
    }
    if (e != NULL)
        rag_effect_send_msg(e, NULL, 0x50, par2, 0, 0);
}

static void ae_remove_matching(struct AbleToMakeEffect *self, long long key2, int use_key2, long long key)
{
    struct ListNode *node = self->effects.head->next;

    while (node != self->effects.head) {
        struct RagEffect *e = *LIST_VALUE(node, struct RagEffect *);
        if (e == NULL) {
            node = list_erase(&self->effects, node, NULL);
            continue;
        }
        if ((!use_key2 || e->show_id == key2) && e->effect_id == key) {
            ae_release(self, e);
            node = list_erase(&self->effects, node, NULL);
            return;
        }
        node = node->next;
    }
}

void able_to_make_effect_send_msg(struct GameObject *object, struct GameObject *source,
    RenderMsgArg msg, RenderMsgArg par1, RenderMsgArg par2, RenderMsgArg par3, RenderMsgArg par4)
{
    struct AbleToMakeEffect *self = (struct AbleToMakeEffect *)object;
    struct GameActor *sender = (struct GameActor *)source;

    struct ListNode *node;

    (void)par3;
    (void)par4;
    switch (msg) {
    case 0x17:
        if (sender == NULL)
            break;
        for (node = self->effects.head->next; node != self->effects.head; node = node->next)
            if (*LIST_VALUE(node, void *) == sender)
                break;
        if (node == self->effects.head)
            break;
        node = list_erase(&self->effects, node, NULL);
        if (self->held_effect == (int)sender)
            self->held_effect = 0;
        else if (self->held_effect2 == (int)sender)
            self->held_effect2 = 0;
        break;
    case 0x18:
        for (node = self->effects.head->next; node != self->effects.head; node = node->next)
            ae_release(self, *LIST_VALUE(node, struct RagEffect *));
        list_clear(&self->effects, NULL);
        break;
    case 0x54:
        ae_drop_held(self, &self->held_effect);
        break;
    case 0x55:
        ae_hold(self, &self->held_effect, par1, par2, 1);
        break;
    case 0x56:
        ae_hold(self, &self->held_effect2, par1, par2, 0);
        break;
    case 0x57:
        ae_drop_held(self, &self->held_effect2);
        break;
    case 0x6d:
        if (effect_mgr_is_registered(g_effectMgr, (int)par1) && sender != NULL)
            effect_mgr_remove_type(g_effectMgr, sender->aid, (int)par1);
        else
            ae_remove_matching(self, 0, 0, par1);
        break;
    case 0x78: {
        int *type = MSG_PTR(par1);
        if (effect_mgr_is_registered(g_effectMgr, (int)type) == 1 && sender != NULL) {
            if (effect_mgr_has_effect(g_effectMgr, sender->aid, (int)type) == 1)
                *type = -1;
            break;
        }
        node = self->effects.head->next;
        while (node != self->effects.head) {
            struct RagEffect *e = *LIST_VALUE(node, struct RagEffect *);
            if (e == NULL) {
                node = list_erase(&self->effects, node, NULL);
                continue;
            }
            if (*type == e->effect_id) {
                *type = -1;
                break;
            }
            node = node->next;
        }
        break;
    }
    case 0x8f:
        node = self->effects.head->next;
        while (node != self->effects.head) {
            struct RagEffect *e = *LIST_VALUE(node, struct RagEffect *);
            if (e == NULL) {
                node = list_erase(&self->effects, node, NULL);
                continue;
            }
            if (e->effect_id == par1)
                rag_effect_send_msg(e, (struct RenderObject *)self, 0x8f, par1, par2, 0);
            node = node->next;
        }
        break;
    case 0x9f:
        if (effect_mgr_is_registered(g_effectMgr, (int)par2) && sender != NULL)
            effect_mgr_remove_keyed(g_effectMgr, sender->aid, (int)par1, (int)par2);
        else
            ae_remove_matching(self, par1, 1, par2);
        break;
    }
}
