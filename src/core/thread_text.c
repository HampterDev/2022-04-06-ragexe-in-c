#include "core/thread_text.h"
#include <stdarg.h>
#include <stdio.h>
#include <string.h>

static _Thread_local char text_buffer[THREAD_TEXT_CAPACITY];

const char *thread_text_format(const char *format, ...)
{
    char formatted[THREAD_TEXT_CAPACITY];
    va_list args;
    int length;
    size_t bytes;

    va_start(args, format);
    length = vsnprintf(formatted, sizeof formatted, format, args);
    va_end(args);
    if (length < 0) {
        text_buffer[0] = 0;
        return text_buffer;
    }
    bytes = (size_t)length < sizeof text_buffer ? (size_t)length : sizeof text_buffer - 1;
    memcpy(text_buffer, formatted, bytes);
    text_buffer[bytes] = 0;
    return text_buffer;
}
