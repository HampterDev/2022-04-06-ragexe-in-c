#ifndef TESTS_PACKET_DISPATCH_TRACE_H
#define TESTS_PACKET_DISPATCH_TRACE_H

#include <stdint.h>

void packet_trace_call(const char *name);
void packet_trace_value(uint64_t value);
void packet_trace_pointer(const void *pointer);

#endif
