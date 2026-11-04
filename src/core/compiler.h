#ifndef CORE_COMPILER_H
#define CORE_COMPILER_H

#if defined(_MSC_VER)
#define RAGE_NORETURN __declspec(noreturn)
#define RAGE_NOINLINE __declspec(noinline)
#define RAGE_ALIGN(n) __declspec(align(n))
#else
#define RAGE_NORETURN _Noreturn
#define RAGE_NOINLINE __attribute__((noinline))
#define RAGE_ALIGN(n) __attribute__((aligned(n)))
#endif

#endif
