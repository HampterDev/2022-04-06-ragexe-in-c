#include "platform/error_message.h"
#include "platform/window.h"

void (*g_errorMsgPrepareDisplay)(void);

void error_msg(const char *text, int flags)
{
    if (g_errorMsgPrepareDisplay)
        g_errorMsgPrepareDisplay();
    platform_message_box(NULL, text, "Error", (unsigned)flags);
}
