#include "actor/actor_ops.h"
#include "actor/render_ops.h"
#include "actor/job.h"
#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#include "actor/types.h"

#define BIT(n)          ((st >> (n)) & 1)

int game_actor_get_job(struct GameActor *self)
{
    DWORD st = ((unsigned long)game_actor_get_effect_state((struct RenderObject *)self));
    int job;

    if (BIT(12))
        return 0x16;
    if (BIT(16) && !BIT(15))
        return 0x1a;
    if (BIT(18) && !BIT(15))
        return 0x1b;
    if (BIT(27) && !BIT(15))
        return 0x1c;
    if (BIT(28) && !BIT(15))
        return 0x1d;
    if (self->mount_look == 1 && !BIT(15))
        return 0x1e;
    job = self->job;
    switch (job) {
    case 0x7:    if (BIT(5)) return 0xd; break;
    case 0xe:    if (BIT(5)) return 0x15; break;
    case 0xfa8:  if (BIT(5)) return 0xfae; break;
    case 0xfbe:  if (BIT(5)) return 0xfc4; break;
    case 0xfaf:  if (BIT(5)) return 0xfb6; break;
    case 0xfc5:  if (BIT(5)) return 0xfcc; break;
    case 0xfcf:  if (BIT(15)) return 0xfd0; break;
    case 0x1082: if (BIT(15)) return 0x108e; break;
    default:
        break;
    }
    return job;
}
