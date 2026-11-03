/* The window table's rows with their data columns only, for tests that cannot link the window
   clusters' factories and close functions. Include it before window_rows.inc
   (extract_window_rows.py). Every row keeps its ID, slot and flags; its make column becomes
   WINDOW_ROWS_MAKE (NULL unless the test defines it first) and its close and get columns NULL,
   so post_quit1 treats every ID as WM_CLOSE_WINDOW. */
#ifndef TESTS_WINDOW_ROWS_DATA_H
#define TESTS_WINDOW_ROWS_DATA_H

#include "ui/wndmgr/window_registry.h"

#ifndef WINDOW_ROWS_MAKE
#define WINDOW_ROWS_MAKE NULL
#endif

#undef WM_ROW
#undef WM_ROW_OPEN
#undef WM_ROW_OPEN_INFO
#undef WM_ROW_OPEN_INFO1
#undef WM_ROW_GET
#define WM_ROW(id, slot, flags, make, close) { (id), (slot), (flags), WINDOW_ROWS_MAKE }
#define WM_ROW_OPEN(id, field, flags, ...) { (id), WM_SLOT(field), (flags), WINDOW_ROWS_MAKE }
#define WM_ROW_OPEN_INFO(id, field, flags, ...) { (id), WM_SLOT(field), (flags), WINDOW_ROWS_MAKE }
#define WM_ROW_OPEN_INFO1(id, field, flags, ...) { (id), WM_SLOT(field), (flags), WINDOW_ROWS_MAKE }
#define WM_ROW_GET(id, get) { (id), WM_NO_SLOT, 0 }

#endif
