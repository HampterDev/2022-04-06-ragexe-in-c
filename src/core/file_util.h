#ifndef CORE_FILE_UTIL_H
#define CORE_FILE_UTIL_H

#include <windows.h>
#include "base.h"
#include "std/std.h"

struct lua_State;

void dir_list_names_list(struct List *names, const char *folder, const char *pattern);
void dir_list_names_vector(struct Vec *names, const char *folder, const char *pattern);
void dir_remove_tree(const char *path);
unsigned char make_directories(struct String path);

#endif
