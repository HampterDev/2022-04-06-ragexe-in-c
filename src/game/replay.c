#include "replay/replay.h"
#include <windows.h>
#include <mmsystem.h>
#include "game/replay.h"
#include "actor/object_ops.h"

void game_mode_apply_replay_speed(struct GameMode *self)
{
    struct ListNode *head, *node;

    actor_set_motion_speed((struct RenderObject *)self->world->player, replay_recorder_duration_scale(replay_recorder_instance()));
    head = self->world->actors.head;
    for (node = head->next; node != head; node = node->next)
        actor_set_motion_speed(*LIST_VALUE(node, struct RenderObject *),
                               replay_recorder_duration_scale(replay_recorder_instance()));
}

char game_mode_on_replay_pause(struct GameMode *self, char pausing)
{
    if (pausing == 1 && self->replay_paused_at != 0) {
        if (replay_recorder_instance()->state != REPLAY_STATE_PLAYING) {
            DWORD paused_for = timeGetTime() - self->replay_paused_at;
            replay_recorder_instance()->wallBase += paused_for;
        }
    } else if (replay_recorder_instance()->state == REPLAY_STATE_PLAYING) {
        self->replay_paused_at = timeGetTime();
    }
    if (self->replay_paused_at == 0)
        replay_recorder_resume(replay_recorder_instance());
    return pausing;
}

void game_mode_end_replay(struct GameMode *self)
{
    (void)self;
    if (replay_recorder_instance()->state != REPLAY_STATE_RECORDING)
        return;
    replay_recorder_set_mode(replay_recorder_instance(), 1);
    replay_recorder_stamp_total_time(replay_recorder_instance());
    replay_recorder_close(replay_recorder_instance());
}
