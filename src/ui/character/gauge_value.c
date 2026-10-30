#include "ui/character/gauge_value.h"

static long long read_int(const void *context)
{
    return *(const int *)context;
}

struct GaugeValue gauge_value_from_int(const int *value)
{
    struct GaugeValue source = {read_int, value};
    return source;
}

long long gauge_value_read(const struct GaugeValue *value)
{
    return value->read(value->context);
}
