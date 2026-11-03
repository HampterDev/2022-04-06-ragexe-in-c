#ifndef SESSION_SESSION_GRANNY_HOOKS_H
#define SESSION_SESSION_GRANNY_HOOKS_H

/* The session's side of g_grannyJobModelName (resources/granny_res.h): the job's
   name, session_get_job_name(&g_session1, job, -1). */
const char *session_granny_job_model(int job);

#endif
