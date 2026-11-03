/* Helpers of the text viewer's other link kinds (navigation, item, quest)
   that the URL link test never reaches. Each stops the test if called. */
#include <stdlib.h>

#define UNREACHED(name) void name(void) { abort(); }

int g_navi;

UNREACHED(game_current_mode)
UNREACHED(game_mode_ui_open_v2)
UNREACHED(item_info_construct)
UNREACHED(navi_navigate)
UNREACHED(navi_open_search)
UNREACHED(string_append)
UNREACHED(string_assign)
UNREACHED(string_assign_cstr)
UNREACHED(string_free)
UNREACHED(ui_window_open)
