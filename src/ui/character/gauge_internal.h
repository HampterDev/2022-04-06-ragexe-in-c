#ifndef UI_GAUGE_INTERNAL_H
#define UI_GAUGE_INTERNAL_H

#include "ui/window.h"
#include "ui/character/gauge.h"

struct UIValueGauge {
    struct UIWindow base;
    struct GaugeValue current;
    struct GaugeValue maximum;
};

struct UIValueGauge *ui_bar_graph_homun_construct(struct UIValueGauge *self);

struct UIINT64BarGraph *uiint64_bar_graph_new(void);
void uiint64_bar_graph_set_values(struct UIINT64BarGraph *self, __int64 current, __int64 maximum);
void uiint64_bar_graph_set_no_warning(struct UIINT64BarGraph *self, BYTE no_warning);

#endif
