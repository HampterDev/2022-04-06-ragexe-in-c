#include "session/csession.h"
#include "session/state.h"
#include "session/session_jobs.h"

/* The session's job queries: the job its character is drawn as and a job's sprite height.
   Apart from session/session_job.c, whose job change refreshes the windows, so that the
   predicates and names asking for the drawn job do not reach them. */

#define STATE_BIT(n) (((DWORD)g_session1.effectState >> (n)) & 1)

int session_get_drawn_job(struct Session *self)
{
    int job = self->job;

    switch (job) {
    case 0x7:    return STATE_BIT(5) ? job + 0x6 : job;
    case 0xe:    return STATE_BIT(5) ? job + 0x7 : job;
    case 0xfa8:  return STATE_BIT(5) ? job + 0x6 : job;
    case 0xfbe:  return STATE_BIT(5) ? job + 0x6 : job;
    case 0xfaf:  return STATE_BIT(5) ? job + 0x7 : job;
    case 0xfc5:  return STATE_BIT(5) ? job + 0x7 : job;
    case 0xfcf:  return STATE_BIT(15) ? job + 0x1 : job;
    case 0x1082: return STATE_BIT(15) ? job + 0xc : job;
    default:     return job;
    }
}

float session_get_job_float(struct Session *self,
                                         int job, int b, int c, int mode)
{
    if (mode == 0) {
        if (job == 5)
            return 5.85f;
        if (job == 6)
            return 5.75f;
        return 6.0f;
    }
    if (mode != 1)
        return 6.0f;
    switch (job) {
    case 0xc: case 0xfad: case 0xfc3:
        if (c == 0x10 || (unsigned)(c - 0x19) <= 5)
            return 3.0f;
        return 6.0f;
    case 0: case 0x17: case 0xfcd: case 0x105b: case 0x105c:
        if (b == 1)
            return 5.85f;
        return 6.0f;
    default:
        return 6.0f;
    }
}
