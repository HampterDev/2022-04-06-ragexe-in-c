#include "world/mode_types.h"
#include "world/mode_dispatch.h"

int mode_send_message(struct Mode *mode, int message, int a, int b, int c, int d)
{
    const struct ModeOps *methods;
    if (!mode || !mode->ops)
        return 0;
    methods = mode->ops;
    return methods->send_message ? methods->send_message(mode, message, a, b, c, d) : 0;
}

void mode_update(struct Mode *mode)
{
    const struct ModeOps *methods;
    if (!mode || !mode->ops)
        return;
    methods = mode->ops;
    if (methods->on_update)
        methods->on_update(mode);
}
