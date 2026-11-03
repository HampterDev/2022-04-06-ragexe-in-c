/* Forced into every client file the effect-trace harness compiles (cl /FI). It declares
   the CRT's rand() first, then renames every later rand() call to the harness's seeded,
   counting copy of the MSVC CRT generator in test_effect_trace.c. The client sources are
   compiled unchanged. */
#ifndef TESTS_EFFECT_TRACE_RAND_H
#define TESTS_EFFECT_TRACE_RAND_H

#include <stdlib.h>

int effect_trace_rand(void);
#define rand effect_trace_rand

#endif
