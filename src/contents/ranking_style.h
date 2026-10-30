#ifndef CONTENTS_RANKING_STYLE_H
#define CONTENTS_RANKING_STYLE_H

enum RankingKind {
    RANKING_BLACKSMITH,
    RANKING_ALCHEMIST,
    RANKING_TAEKWON,
    RANKING_KILLER
};

#define RANK_SEPARATOR "==========================================="

struct RankingStyle {
    const char *title;
    int title_color, line_color;
};

const struct RankingStyle *ranking_style(int kind);

#endif
