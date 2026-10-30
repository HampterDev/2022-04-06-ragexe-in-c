#include "contents/schedule_ids.h"
#include <assert.h>
#include <stdio.h>

int main(void)
{
    int i;
    for (i = 1; i <= 10000; ++i)
        assert(schedule_next_id() == i);
    puts("schedule IDs: one shared increasing sequence starts at one");
    return 0;
}
