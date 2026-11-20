#include "platform/error_message.h"
#include "platform/window.h"

void error_msg(const char *text, int flags)
{
    platform_message_box(NULL, text, "Error", (unsigned)flags);
}
