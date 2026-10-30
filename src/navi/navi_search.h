#ifndef NAVI_SEARCH_H
#define NAVI_SEARCH_H

#include "navi/navi.h"
#include "std/std.h"

#include <wchar.h>

#define NAVI_QUERY_COMMAND  0
#define NAVI_QUERY_TEXT     1
#define NAVI_QUERY_ALL      2
#define NAVI_QUERY_LEVEL    3
#define NAVI_QUERY_ALIAS    4

unsigned char navi_manager_search(struct NaviManager *self,
        struct String query, int filter, struct Vec *results);
int navi_manager_classify_query(struct NaviManager *self, struct Vec *words);
void navi_map_search_words(struct NaviMap *self, int filter,
        const struct Vec *words, struct Vec *results);
void navi_map_list_all(struct NaviMap *self, int filter, struct Vec *results);
void navi_map_mobs_in_level_range(struct NaviMap *self, int lo, int hi,
        struct Vec *results);
void navi_map_match_map_words(struct NaviMap *self,
        const struct Vec *words, struct Vec *results);
void navi_map_match_mob_words(struct NaviMap *self,
        const struct Vec *words, struct Vec *results);
void navi_map_mobs_with_alias(struct NaviMap *self, struct String alias,
        struct Vec *results);
void navi_map_match_npc_words(struct NaviMap *self,
        const struct Vec *words, struct Vec *results);

unsigned char navi_result_name_greater(struct SearchResult *self, const struct SearchResult *other);

#endif
