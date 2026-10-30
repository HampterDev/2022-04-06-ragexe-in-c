#include "contents/ranking_output.h"
#include "ui/window_messages.h"
#include "ui/window_manager_state.h"
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>

int ranking_line(int color, const char *format, ...)
{
    va_list args;
    char *text;
    int size, written;

    va_start(args, format);
    size = vsnprintf(NULL, 0, format, args);
    va_end(args);
    if (size < 0) return 0;
    text = malloc((size_t)size + 1);
    if (!text) return 0;
    va_start(args, format);
    written = vsnprintf(text, (size_t)size + 1, format, args);
    va_end(args);
    if (written == size)
        ui_window_mgr_send_msg_(&g_windowMgr, UIWM_MSG_CHAT_LINE, (int)text, color, 0, 0);
    free(text);
    return written == size;
}
