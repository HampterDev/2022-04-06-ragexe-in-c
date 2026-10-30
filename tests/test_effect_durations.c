#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include "effects/durations.h"

int main(void)
{
    static const struct { int id, frames; } cases[] = {
        {912, 1000}, {1058, 9999}, {1059, 350}, {1060, 9999},
        {1121, 1121}, {1122, 99999999}, {1130, 1130}, {1131, 99},
        {1132, 99999999}, {2025, 2025}, {2026, 60}, {2027, 60},
        {2028, 2028}, {2382, 2382}, {2383, 9999}, {2384, 0}
    };
    uint32_t hash = UINT32_C(2166136261);
    size_t i;
    unsigned shift;

    assert(EFFECT_DURATION_COUNT == 2385);
    for (i = 0; i < sizeof cases / sizeof cases[0]; ++i)
        assert(g_effectDurations[cases[i].id] == cases[i].frames);

    for (i = 0; i < EFFECT_DURATION_COUNT; ++i)
        for (shift = 0; shift < 32; shift += 8) {
            hash ^= ((uint32_t)g_effectDurations[i] >> shift) & 255u;
            hash *= UINT32_C(16777619);
        }
    assert(hash == UINT32_C(0x0333a814));
    puts("effect durations: original table and indices around former scalar aliases passed");
    return 0;
}
