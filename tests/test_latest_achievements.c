/* session_get_latest_achievements (session_achievements.c) finds the two achievements completed
   last. Its search for the second one started from the address of its own output parameter cut to
   int; a completed achievement is compared with that start only while no latest one is known yet,
   that is for a completion time of -1 or less. A 32-bit stack address is positive there, which
   compares like -1; a 64-bit address cut to int can be negative. The extracted function runs on the
   real tree with `second` placed at an address whose low 32 bits are negative where the platform
   gives one, and must not depend on it. */
#ifndef _WIN32
#define _DEFAULT_SOURCE /* MAP_ANONYMOUS */
#endif
#include "session/state.h"
#include "session/session_achievements.h"
#include "std/tree.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifdef _WIN32
#include <windows.h>
#else
#include <sys/mman.h>
#endif

void *mem_alloc(size_t bytes)
{
    void *p = malloc(bytes);
    assert(p);
    return p;
}
void mem_free(void *p) { free(p); }

#include "latest_achievements.inc"

static struct Session session;

static void add(int id, int completed, int completed_at)
{
    struct AchievementPair *pair = tree_get(&session.achievements, &id, sizeof id, sizeof *pair, tree_less_int);

    pair->id = id;
    pair->value.completed = (char)completed;
    pair->value.completed_at = completed_at;
}

/* An int at an address whose low 32 bits are negative as an int, where one can be had. */
static int *high_int(void)
{
    void *at = NULL;

    if (sizeof(void *) < 8)
        return NULL;
#ifdef _WIN32
    at = VirtualAlloc((void *)(uintptr_t)UINT64_C(0x00000012f0000000), 4096, MEM_RESERVE | MEM_COMMIT,
                      PAGE_READWRITE);
#else
    at = mmap((void *)(uintptr_t)UINT64_C(0x00000012f0000000), 4096, PROT_READ | PROT_WRITE,
              MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    if (at == MAP_FAILED)
        at = NULL;
#endif
    if (at && (int)(uint32_t)(uintptr_t)at >= 0)
        at = NULL;
    return at;
}

int main(void)
{
    int *placed = high_int(), local_second;
    int *second = placed ? placed : &local_second, first;

    tree_init(&session.achievements);

    /* The usual case: the two completed last, whatever the order of the tree. */
    add(10, 1, 100);
    add(11, 1, 300);
    add(12, 1, 200);
    add(13, 0, 999);
    *second = first = 0;
    session_get_latest_achievements(&session, second, &first);
    assert(first == 11 && *second == 12);

    /* Only one completion, at a time of -1 or less: nothing is latest, and the search must not
       compare it with an address. */
    tree_clear(&session.achievements, NULL);
    add(20, 1, -5);
    *second = first = 0;
    session_get_latest_achievements(&session, second, &first);
    if (*second != 0 || first != 0) {
        printf("latest achievements: second = %d, first = %d for a completion at -5 (second at %p)\n", *second,
               first, (void *)second);
        return 1;
    }
    tree_free(&session.achievements, NULL);
    printf("Latest achievements: the two completed last; the search does not start from an address%s.\n",
           placed ? " (second placed at a negative low word)" : "");
    return 0;
}
