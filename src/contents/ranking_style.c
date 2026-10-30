#include "contents/ranking_style.h"

const struct RankingStyle *ranking_style(int kind)
{
    static const struct RankingStyle styles[] = {
        { "========== BLACKSMITH RANK ============", 0x9b9b9b, 0xcdcdcd },
        { "=========== ALCHEMIST RANK =============", 0x9bcd9b, 0xcdffcd },
        { "============ TAEKWON RANK =============", 0xcd9b9b, 0xffcdcd },
        { "=========== KILLER RANK ==============", 0x9b9b9b, 0xcdcdcd }
    };
    static const struct RankingStyle unknown = {RANK_SEPARATOR, 0, 0};

    return (unsigned)kind < sizeof styles / sizeof *styles ? &styles[kind] : &unknown;
}
