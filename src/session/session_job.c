#include "session/character_name.h"
#include "session/state.h"
#include "ui/window_manager_state.h"
#include "session/csession.h"
#include "session/session_jobs.h"
#include "session/session_talk.h"
#include "session/session_windows.h"
#include "game/context.h"

#include <string.h>
#include "ui/character/gauge.h"

void session_set_job(struct Session *self, int job)
{
    self->job = job;
    session_invalidate_windows(self);
    session_refresh_skill_tree(self);
}
