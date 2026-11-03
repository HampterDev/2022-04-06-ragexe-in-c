#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "actor/game_actor.h"
#include "actor/job.h"
#include "game/context.h"
#include "session/session_jobs.h"
#include "session/state.h"
#include "world/view_types.h"

/* Run the production mercenary-job predicate and game_actor_dir_of. The mercenary jobs
   are 6017-6046 (MER_ARCHER01 Mina to MER_SWORDMAN10 Wayne in Hercules' mercenary_db);
   6048-6052 are the Homunculus S, which this predicate also covers on main. Mina,
   standing, must face the way every other mercenary does. */

struct Session g_session1;
static struct View view;
static int actor_job, cases;

struct GameMode *game_current_mode(void) { return (struct GameMode *)&view; }
struct View *game_current_view(void) { return &view; }
int game_actor_get_job(struct GameActor *actor) { (void)actor; return actor_job; }
int session_is_pc_job(struct Session *self, int job) { (void)self; return job < 0x1f; }

#include "mercenary_job.inc"

static void check_range(void)
{
    int id;

    for (id = 5990; id <= 6070; ++id) {
        int mercenary = id >= 6017 && id <= 6046, homun_s = id >= 6048 && id <= 6052;
        int got = MERCENARY_JOB(&g_session1, id) != 0;

        if (mercenary)
            assert(got);
        else if (homun_s || id == 6047)
            assert(got == MERCENARY_JOB_COVERS_HOMUN_S);
        else
            assert(!got);
        ++cases;
    }
}

/* The 8-way direction a standing actor of this job shows, facing deg degrees from the
   camera. */
static int facing(int job, float deg)
{
    struct GameActor actor;

    memset(&actor, 0, sizeof actor);
    actor.base_action = 0;
    actor_job = job;
    return game_actor_dir_of((struct GameObject *)&actor, -deg);
}

static void check_facing(void)
{
    static const float angles[] = { 0.0f, 20.0f, 30.0f, 40.0f, 100.0f, 200.0f, 340.0f };
    size_t i;
    int job;

    view.longitude = 0.0f;
    for (i = 0; i < sizeof angles / sizeof *angles; ++i)
        for (job = 6017; job <= 6046; ++job) {
            assert(facing(job, angles[i]) == facing(6018, angles[i]));
            ++cases;
        }
    /* Rounded, not cut down: 40 degrees is direction 1 and 340 direction 0. */
    assert(facing(6017, 40.0f) == 1 && facing(6017, 340.0f) == 0);
}

int main(void)
{
    check_range();
    check_facing();
    printf("Mercenary jobs: %d checks; 6017-6046 are mercenaries and face like them.\n", cases);
    return 0;
}
