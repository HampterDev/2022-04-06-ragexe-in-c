/* rand() for POSIX scene runs: the Microsoft C runtime's generator, the one the Windows client
   gets from its runtime, so effects that draw random numbers land in the same places on Windows,
   Linux and macOS and the captures can be compared across them. (Not built on Windows.) */
#include <stdlib.h>

static unsigned long g_state = 1;

void srand(unsigned seed)
{
    g_state = seed;
}

int rand(void)
{
    g_state = (g_state * 214013ul + 2531011ul) & 0xfffffffful;
    return (int)((g_state >> 16) & 0x7fff);
}
