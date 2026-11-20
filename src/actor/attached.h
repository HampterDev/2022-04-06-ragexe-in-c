#ifndef ACTOR_ATTACHED_H
#define ACTOR_ATTACHED_H

struct GameActor;
struct GameObject;
struct ListNode;

/* custom: an actor's list of attached objects (struct GameActor, attached) as they leave it. An
   attached message effect keeps its node on the list (struct MsgEffect, attached_node,
   msg_effect_attach) and gives it with message 0x30 when it goes (msg_effect_leave_owner): the
   actor takes the node off where it holds the sender, and forgets the sender in the fields that
   name it (the cart, the falcon, the marks); a node that is NULL or holds another object changes
   nothing. The original's handler of 0x30 (sub_751dc7, through the jump table at 0x75bc98)
   searched the list for the sender. */
void game_actor_detach(struct GameActor *self, struct ListNode *node, const struct GameObject *sender);

#endif
