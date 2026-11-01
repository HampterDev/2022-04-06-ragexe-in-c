#ifndef CORE_COMPILER_H
#define CORE_COMPILER_H

#if defined(_MSC_VER)
#define RAGE_NORETURN __declspec(noreturn)
#else
#define RAGE_NORETURN _Noreturn
#endif

#endif
