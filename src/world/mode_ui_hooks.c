#include "world/mode_ui_hooks.h"
#include "world/mode_actions.h"
#include "world/mode_manager.h"
#include "world/tooltip.h"

void mode_show_current_tooltip(const char *text, int x, int y, DWORD colour, BYTE flags)
{
    mode_show_tooltip(client_current_mode(), text, x, y, colour, flags);
}

void mode_set_current_cursor_action(int action)
{
    mode_set_cursor_action(client_current_mode(), action);
}
