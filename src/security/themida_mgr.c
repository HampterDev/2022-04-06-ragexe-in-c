#include "security/themida.h"

static int themida_started;

int themida_startup(void)
{
    if (themida_started) return 0;
    themida_started = 1;
    return 1;
}

void themida_shutdown(void)
{
    themida_started = 0;
}
