#include "session/session_granny_hooks.h"
#include "session/session_names.h"
#include "session/state.h"

const char *session_granny_job_model(int job)
{
    return session_get_job_name(&g_session1, job, -1);
}
