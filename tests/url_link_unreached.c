/* Helpers of the text viewer's other link kinds (navigation, item, quest)
   that the URL link test never reaches. Each stops the test if called; the
   navigation and quest links go through g_uiHooks, which the test leaves empty. */
#include <stdlib.h>

#define UNREACHED(name) void name(void) { abort(); }

UNREACHED(item_info_construct)
UNREACHED(string_append)
UNREACHED(string_assign)
UNREACHED(string_assign_cstr)
UNREACHED(string_free)
UNREACHED(ui_window_open)
