#ifndef UI_STRING_TEST_H
#define UI_STRING_TEST_H

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "std/std.h"

void test_ui_string_owner(void);
void test_ui_string_record(const struct String *text);
void test_ui_string_number(int value);
void test_ui_string_no_allocations(void);

#endif
