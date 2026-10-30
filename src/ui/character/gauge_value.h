#ifndef UI_GAUGE_VALUE_H
#define UI_GAUGE_VALUE_H

typedef long long (*GaugeReadValue)(const void *context);
struct GaugeValue {
    GaugeReadValue read;
    const void *context;
};

struct GaugeValue gauge_value_from_int(const int *value);
long long gauge_value_read(const struct GaugeValue *value);

#endif
