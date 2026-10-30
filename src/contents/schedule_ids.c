#include "contents/schedule_ids.h"

static int last_schedule_id;

int schedule_next_id(void)
{
    return ++last_schedule_id;
}
