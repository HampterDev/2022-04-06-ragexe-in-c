#include "actor/object_ops.h"
#include "actor/actor.h"
#include "actor/status_icons.h"
#include "game/context.h"
#include <assert.h>
#include <stdio.h>

static struct Player *current, *after_message;
static int messages, launches;
static struct RagEffect *launched;

struct Player *game_current_player(void) { return current; }

static void receive_message(struct GameObject *player, struct GameObject *sender,
                            RenderMsgArg message, RenderMsgArg effect, RenderMsgArg b,
                            RenderMsgArg c, RenderMsgArg d)
{
    assert((void *)player == current && !sender && message == 0x6d);
    assert(effect == 0xa4 && !b && !c && !d);
    ++messages;
    current = after_message;
}

struct RagEffect *game_actor_launch_effect(struct GameActor *actor, int id, Vec3 position, float rotation)
{
    assert((void *)actor == current && id == 0xa4);
    assert(position.x == 0 && position.y == 0 && position.z == 0 && rotation == 0);
    ++launches;
    return launched;
}

int main(void)
{
    static struct Player player, replacement;
    const struct GameObjectOps ops = {.send_msg = receive_message};
    int effect;

    assert(!game_hide_player_status_icons() && messages == 0);
    game_show_player_status_icons();
    assert(launches == 0);
    player.ops = &ops;
    replacement.ops = &ops;
    current = after_message = &player;
    player.status_icons = (struct RagEffect *)&effect;
    assert(game_hide_player_status_icons() && messages == 1 && !player.status_icons);
    launched = (struct RagEffect *)&effect;
    game_show_player_status_icons();
    assert(launches == 1 && player.status_icons == launched);
    launched = NULL;
    game_show_player_status_icons();
    assert(launches == 2 && !player.status_icons);

    after_message = NULL;
    assert(!game_hide_player_status_icons() && !current && messages == 2);
    current = &player;
    after_message = &replacement;
    replacement.status_icons = (struct RagEffect *)&effect;
    assert(game_hide_player_status_icons() && current == &replacement && messages == 3);
    assert(!replacement.status_icons);
    puts("status icons: absent player, hide/show, failed launch and player replacement passed");
    return 0;
}
