#include "core/thread_text.h"
#include "platform/win_types.h"
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include "game/debug.h"
#include "game/client.h"
#include "core/memory.h"
#include "core/fatal.h"
#include "render/math3d.h"
#include "base.h"
#include "ui/items/styling_shop.h"

void debug_divide_by_zero(void)
{
    volatile int zero = 0;
    volatile int result;

    result = 100 / zero;
    (void)result;
}
