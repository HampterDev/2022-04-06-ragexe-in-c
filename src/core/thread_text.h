#ifndef CORE_THREAD_TEXT_H
#define CORE_THREAD_TEXT_H

enum { THREAD_TEXT_CAPACITY = 2048 };

const char *thread_text_format(const char *format, ...);

#endif
