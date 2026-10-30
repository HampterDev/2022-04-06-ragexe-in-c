#include "base.h"

int zeny_fee_percent(long long amount)
{
    if (amount >= 10000001)
        return 5;
    return 0;
}
