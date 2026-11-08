#ifndef ORACLE_IMAGE_H
#define ORACLE_IMAGE_H

/* The original 2022-04-06 executable mapped into this process as an oracle: its sections are
 * laid out as the loader would lay them out, its imports are bound to the real DLLs (the game's
 * own DLLs are looked up beside the executable), and nothing of it is run: no entry point, no
 * static initialisers, no TLS. Routines that only read their arguments and the image's constant
 * tables can then be called through a function pointer and compared with the lifted C. Globals
 * the original fills at start-up hold the values of the file's .data section; a test that needs
 * them seeds them through oracle_addr, or patches the routine that reads them.
 *
 * The harness must not occupy the original's preferred base 0x400000: link it with
 * /BASE:0x10000000 /FIXED /DYNAMICBASE:NO. If the base is taken anyway, the image is placed
 * elsewhere and relocated, so absolute addresses must always go through oracle_addr. */

/* Maps the executable; prints why and returns 0 on failure. */
int oracle_load(const char *exe_path);

/* The address in this process of an original virtual address (0x400000-based). */
void *oracle_addr(unsigned original_va);

/* Overwrites bytes of the mapped image. */
void oracle_patch(unsigned original_va, const void *bytes, unsigned size);

/* Makes the routine at the address return value at once: mov eax, value; ret stack_bytes
 * (0 for cdecl, the arguments' size for stdcall / thiscall). */
void oracle_patch_return(unsigned original_va, int value, unsigned stack_bytes);

/* Makes the routine at the address jump to fn: a C stand-in with the original's calling
 * convention (__fastcall with this and a dummy edx for a thiscall method). */
void oracle_patch_jump(unsigned original_va, void *fn);

/* The original address a call instruction at the address targets. */
unsigned oracle_call_target(unsigned call_va);

#endif
