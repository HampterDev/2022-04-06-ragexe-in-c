#ifndef BODY_VECTORS_H
#define BODY_VECTORS_H

/* What body_scenario() gives for a seed, a start tick and a number of inputs. The native test
 * (sim/body/body_tests.c) and the wasm test (consumers/body/core.test.mjs, which reads the
 * rows out of this file) both hold the core to these, so the two builds agree with each
 * other and with what the core did when the rows were written.
 *
 * A change to the core's rules changes the digests: raise BODY_RULES_VERSION, run
 * `body_tests --digests` and put its rows here. The start ticks are an ordinary one, one whose
 * run crosses 2^32, and one whose run crosses 2^63: the core's ticks are 64 bits and nothing
 * happens there. */

#include <stdint.h>

struct BodyVector {
    uint32_t seed;
    uint64_t start_tick;
    uint32_t inputs;
    uint64_t digest;
};

static const struct BodyVector body_vectors[] = {
{0x5EED0001u, 0x0000000000000002ull, 50000u, 0xFAD6D967A5F07EDAull},
    {0x5EED0002u, 0x00000000000003E8ull, 50000u, 0x7320D7D384144A9Dull},
    {0x5EED0003u, 0x00000000FFFFFF00ull, 50000u, 0xBF593C677149EE8Cull},
    {0x5EED0004u, 0x7FFFFFFFFFFFFF00ull, 50000u, 0x3FD555D0985861D4ull},
};

#endif
