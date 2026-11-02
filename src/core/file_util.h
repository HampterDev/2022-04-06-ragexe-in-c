#ifndef CORE_FILE_UTIL_H
#define CORE_FILE_UTIL_H

#include "platform/win_types.h"
#include "base.h"
#include "std/std.h"

void dir_list_names_list(struct List *names, const char *folder, const char *pattern);

#endif
