#include "world/mode_types.h"
#include "world/mode_dispatch.h"
#include <assert.h>
#include <stdio.h>

static struct Mode *expected;
static int requests, queries, updates;

static void update_mode(struct Mode *mode)
{
    assert(mode == expected);
    ++updates;
}

static intptr_t receive_message(struct Mode *mode, int message, intptr_t a, intptr_t b, intptr_t c, intptr_t d)
{
    assert(mode == expected);
    assert(message == 17 && a == -1 && b == 2 && c == 3 && d == 4);
    ++requests;
    return 91;
}

static struct Mode *current_mode(void)
{
    ++queries;
    return expected;
}

int main(void)
{
    struct Mode first = {0}, second = {0};
    struct ModeOps methods = {0};
    mode_update(NULL);
    mode_update(&first);
    assert(!mode_send_message(NULL, 17, -1, 2, 3, 4));
    assert(!mode_send_message(&first, 17, -1, 2, 3, 4));
    first.ops = &methods;
    assert(!mode_send_message(&first, 17, -1, 2, 3, 4));
    mode_update(&first);
    assert(updates == 0);
    methods.send_message = receive_message;
    methods.on_update = update_mode;
    expected = &first;
    assert(mode_send_message(current_mode(), 17, -1, 2, 3, 4) == 91);
    assert(queries == 1 && requests == 1);
    mode_update(expected);
    assert(updates == 1);
    second.ops = &methods;
    expected = &second;
    assert(mode_send_message(current_mode(), 17, -1, 2, 3, 4) == 91);
    assert(queries == 2 && requests == 2);
    mode_update(expected);
    assert(updates == 2);
    expected = NULL;
    assert(!mode_send_message(current_mode(), 17, -1, 2, 3, 4));
    assert(queries == 3 && requests == 2);
    puts("mode dispatch: absent mode/handler, argument forwarding, one query and mode replacement passed");
    return 0;
}
