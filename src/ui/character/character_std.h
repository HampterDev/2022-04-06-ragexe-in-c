#ifndef UI_CHARACTER_CHARACTER_STD_H
#define UI_CHARACTER_CHARACTER_STD_H

#include "ui/ui_std.h"
#include "core/fatal.h"
#include <time.h>

struct String;

BYTE text_within_columns(void *self, struct String text, int limit);

#endif
