#ifndef TESTS_WINDOW_LISTS_TRACE_H
#define TESTS_WINDOW_LISTS_TRACE_H

/* Recording side of test_window_lists.c, used by its generated stubs (window_lists_stubs.c).
   A stub calls wl_call with its name, one wl_* per argument and wl_end; the line goes into
   the test's digest. Pointers are written as names (W3 for the fourth fake window, mgr.field
   for a window-manager field, D1 for an environment block), never as addresses. */

#include "ui/window_types.h"

void wl_call(const char *name);
void wl_int(long long value);
void wl_float(double value);
void wl_str(const char *text);
void wl_ptr(const void *pointer);
void wl_end(void);

/* A new fake window: a zeroed block whose ops record every call. */
struct UIWindow *wl_fake_window(const char *constructor);
/* Makes memory a caller allocated (a *_construct stub's object) a fake window: its UIWindow
   part is zeroed and gets the recording ops. */
void wl_construct(void *object, const char *constructor);
/* What the environment returns: NULL and 0 in the first pass, a new zeroed block and 1 in the
   second. */
void *wl_env_ptr(void);
long long wl_env_int(void);

#endif
