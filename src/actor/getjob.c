#include "actor/actor_ops.h"
#include "actor/render_ops.h"
#include "actor/job.h"
#define WIN32_LEAN_AND_MEAN
#include "platform/win_types.h"

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
    case 0xfe2:  if (BIT(5)) return 0xff2; break;
    case 0xfe9:  if (BIT(5)) return 0xff3; break;
    case 0x1006: if (BIT(5)) return 0x100e; break;
    case 0xfc5:  if (BIT(5)) return 0xfcc; break;
    case 0xfcf:  if (BIT(15)) return 0xfd0; break;
    case 0xfd6:
        if (BIT(19)) return 0xff0;
        if (BIT(23)) return 0xff8;
        if (BIT(24)) return 0xffa;
        if (BIT(25)) return 0xffc;
        if (BIT(26)) return 0xffe;
        break;
    case 0xfdc:
        if (BIT(19)) return 0xff1;
        if (BIT(23)) return 0xff9;
        if (BIT(24)) return 0xffb;
        if (BIT(25)) return 0xffd;
        if (BIT(26)) return 0xfff;
        break;
    case 0x1000: if (BIT(19)) return 0x100d; break;
    case 0xfd8:  if (BIT(21)) return 0xff4; break;
    case 0xfde:  if (BIT(21)) return 0xff5; break;
    case 0x1002: if (BIT(21)) return 0x100f; break;
    case 0x109c: if (BIT(19)) return 0x10b8; break;
    case 0x10a2: if (BIT(5)) return 0x10b9; break;
    case 0xfda:  if (actor_get_sprite_variant(self)) return 0xff6; break;
    case 0xfe0:  if (actor_get_sprite_variant(self)) return 0xff7; break;
    case 0x1004: if (actor_get_sprite_variant(self)) return 0x1010; break;
    case 0x109d: if (actor_get_sprite_variant(self)) return 0x10b7; break;
    case 0x1082: if (BIT(15)) return 0x108e; break;
    case 0x108f: if (BIT(15)) return 0x1093; break;
    case 0x1091: if (BIT(15)) return 0x1094; break;
    case 0x10a1: if (BIT(21)) return 0x10b6; break;
    case 0x10ce: if (BIT(15)) return 0x10dc; break;
    default:
        break;
    }
    return job;
}
