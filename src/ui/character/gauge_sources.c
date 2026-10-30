#include "ui/character/gauge_sources.h"
#include "session/state.h"
#include "ui/game.h"

static long long read_ap(const void *context)
{
    (void)context;
    return session_get_param(&g_session1, 0xe8);
}

static long long read_max_ap(const void *context)
{
    (void)context;
    return session_get_param(&g_session1, 0xe9);
}

struct GaugeValue gauge_ap_source(void)
{
    struct GaugeValue source = {read_ap, NULL};
    return source;
}

struct GaugeValue gauge_max_ap_source(void)
{
    struct GaugeValue source = {read_max_ap, NULL};
    return source;
}
