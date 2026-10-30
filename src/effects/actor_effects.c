#include "game/world_objects.h"
#include "effects/render_object.h"
#include "effects/effect_math.h"
#include "game/context.h"
#include "game/mode_callbacks.h"

#define FOOT_LEFT_B 326
#define FOOT_RIGHT_B 327
#define FOOT_LEFT_2 447
#define FOOT_RIGHT_2 448
#define FOOTPRINT_FRAMES 333

#define ACTOR_FOOTPRINT_FRAME(a) ((a)->footprint_effect_frame)

void actor_footprints(struct RenderObject *actor, int kind)
{
    struct Pc *pc = (struct Pc *)actor;
    int stride = kind == 1 ? 12 : 7;
    struct GameMode *game_mode;
    float dx, dz, len;
    unsigned short heading;
    short deg;
    int step;

    if (g_isSkipFrame <= 0)
        return;
    step = ++pc->footprint_step;
    if (step > stride * 2) {
        pc->footprint_step = 1;
        step = 1;
    }
    if (step < 0) {
        step = 0;
        pc->footprint_step = 0;
    }
    if (step == 0) {
        pc->footprint_last = actor->pos;
        step = pc->footprint_step;
    }
    if (step != stride && step != stride * 2)
        return;

    game_mode = game_current_mode();
    dx = actor->pos.x - pc->footprint_last.x;
    dz = actor->pos.z - pc->footprint_last.z;
    pc->footprint_last = actor->pos;
    if (!game_mode || (dx == 0.0f && dz == 0.0f))
        return;
    len = float_sqrt(dz * dz + dx * dx);
    dx = dx / len;
    dz = dz / len;

    heading = 180;
    for (deg = 0; deg <= 180; deg++) {
        if (dx >= g_cosTable[deg]) {
            heading = dz >= 0.0f ? (unsigned short)deg : (unsigned short)(360 - deg);
            break;
        }
    }
    heading = (unsigned short)(heading + 135);
    if (!((short)heading < 360))
        heading = (unsigned short)(heading - 360);

    if (kind == 1) {
        game_mode_launch_dummy_effect(game_mode, &pc->footprint_last, pc->footprint_step != stride ? FOOT_RIGHT_B : FOOT_LEFT_B, FOOTPRINT_FRAMES,
                   (short)heading);
        return;
    }
    if (kind == 2 && ACTOR_FOOTPRINT_FRAME(actor) > stride)
        game_mode_launch_dummy_effect(game_mode, &pc->footprint_last, pc->footprint_step == stride ? FOOT_LEFT_2 : FOOT_RIGHT_2, FOOTPRINT_FRAMES,
                   (short)heading);
}
