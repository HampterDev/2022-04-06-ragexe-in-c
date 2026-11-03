#include "platform/error_message.h"
#include "platform/win_utf8.h"

void (*g_errorMsgPrepareDisplay)(void);

void error_msg(const char *text, int flags)
{
    if (g_errorMsgPrepareDisplay)
        g_errorMsgPrepareDisplay();
    win_utf8_message_box(NULL, text, "Error", (UINT)flags);
}
