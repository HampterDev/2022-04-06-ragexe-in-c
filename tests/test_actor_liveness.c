#include "actor/liveness.h"
#include "game/context.h"
#include "std/list.h"
#include <assert.h>
#include <stdio.h>

static struct Player *player;
static const struct List *actors;

struct Player *game_current_player(void) { return player; }
const struct List *game_current_actor_list(void) { return actors; }

int main(void)
{
    int own, first, second, released;
    struct ActorNode { struct ListNode links; void *actor; } a, b;
    struct ListNode head;
    struct List list = { NULL, 0 };

    assert(!actor_is_live(NULL));
    assert(!actor_is_live(&released));
    player = (struct Player *)&own;
    assert(actor_is_live(&own));
    actors = &list;
    assert(!actor_is_live(&released));
    list.head = &head;
    head.next = head.prev = &head;
    assert(!actor_is_live(&released));
    a.links.prev = &head;
    a.links.next = &b.links;
    a.actor = &first;
    b.links.prev = &a.links;
    b.links.next = &head;
    b.actor = &second;
    head.next = &a.links;
    head.prev = &b.links;
    list.size = 2;
    assert(actor_is_live(&first) && actor_is_live(&second));
    assert(!actor_is_live(&released));
    head.next = &b.links;
    b.links.prev = &head;
    list.size = 1;
    assert(!actor_is_live(&first) && actor_is_live(&second));
    player = NULL;
    actors = NULL;
    assert(!actor_is_live(&own) && !actor_is_live(&second));
    assert(!actor_is_live(NULL));
    puts("actor liveness: player, list membership, removal and world teardown passed");
    return 0;
}
