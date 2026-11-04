#include "navi/navigation.h"
#include "navi/grid_uv.h"
#ifndef NAVI_MANAGER_H
#define NAVI_MANAGER_H

#include "navi/navi.h"

struct SearchResult *search_result_construct(struct SearchResult *self, int kind, void *obj);
struct SearchResult *search_result_assign(struct SearchResult *self,
        const struct SearchResult *other);
unsigned char navi_manager_parse_flags(struct NaviManager *self, const char *text,
        unsigned char *useScrolls, unsigned char *useKafra, unsigned char *useAirship);
struct NaviPoint *navi_manager_parse_pos(struct NaviManager *self,
        struct NaviPoint *out, const char *text);
struct NaviUiRecord {
    int style;
    int x, y;
    struct String text;
    unsigned colour;
    unsigned colour2;
};

struct NaviManager *navi_manager_construct(struct NaviManager *self);
void navi_manager_destruct(struct NaviManager *self);
void navi_manager_check_arrival(struct NaviManager *self, int x, int y);
unsigned char navi_manager_set_guide_state(struct NaviManager *self, int state);
struct NaviMap *navi_map_named(struct NaviManager *self, const char *name);
struct NaviMap *navi_map_by_name(struct NaviManager *manager, const char *name, size_t size);

#define NAVI_STATUS_ROUTE_LINK_GUIDED   (-92)
#define NAVI_STATUS_UNKNOWN_MAP         (-93)
#define NAVI_STATUS_NOT_FOUND           (-94)
#define NAVI_STATUS_NO_GAME_MODE        (-95)
#define NAVI_STATUS_NO_ROUTE            (-96)
#define NAVI_STATUS_NO_WAY_IN           (-97)
#define NAVI_STATUS_NO_WAY_OUT          (-98)
#define NAVI_STATUS_NOT_ON_MAP          (-99)
#define NAVI_STATUS_ON_MOB_MAP          2
#define NAVI_STATUS_ON_MAP              3
#define NAVI_STATUS_ROUTE_BY_SCROLL     4
#define NAVI_STATUS_ROUTE               5
#define NAVI_STATUS_ARRIVED_MAP         6
#define NAVI_STATUS_WALK_TO_NPC         7
#define NAVI_STATUS_ARRIVED_MOB_MAP     8
#define NAVI_STATUS_WALK_TO_PLACE       9
#define NAVI_STATUS_AT_DESTINATION      11

#define NAVI_REPORT_BOTH    0
#define NAVI_REPORT_CHAT    1
#define NAVI_REPORT_BOX     2

unsigned char navi_manager_at_destination(struct NaviManager *self, int x, int y);
void navi_manager_reset_guide(struct NaviManager *self);
int navi_manager_start_guide(struct NaviManager *self);
int navi_manager_guide_on_map(struct NaviManager *self, int x, int y);
void navi_manager_stop_guide(struct NaviManager *self);
void navi_manager_reset(struct NaviManager *self, unsigned char all);
void navi_manager_clear_target_and_marker(struct NaviManager *self);
int navi_manager_guide_to(struct NaviManager *self, int x, int y);
int navi_manager_destination_from_guided(struct NaviManager *self);
unsigned char navi_manager_guide_link(struct NaviManager *self, int index);
void navi_manager_arrive(struct NaviManager *self);
unsigned char navi_manager_plan_route(struct NaviManager *self,
        struct NaviMap *here, int x, int y);
void navi_manager_resume(struct NaviManager *self);
unsigned char navi_manager_guide_selected(struct NaviManager *self);

float navi_manager_fit_minimap(struct NaviManager *self, int *ox, int *oy,
        int width, int height, float size);
unsigned char navi_manager_find_path(struct NaviManager *self, int sx, int sy,
        int tx, int ty, unsigned char draw);
unsigned char navi_manager_bind_world(struct NaviManager *self);
struct NaviPoint *navi_manager_minimap_pos(struct NaviManager *self, struct NaviPoint *out,
        struct NaviMap *map, int x, int y, float size);
struct NaviPoint *navi_manager_minimap_pos_by_name(struct NaviManager *self,
        struct NaviPoint *out, const char *name, int x, int y, float size);
void navi_manager_draw_path(struct NaviManager *self, void *view);
void navi_manager_load_grid_images(struct NaviManager *self);

unsigned char navi_manager_fill_from_passes(struct NaviManager *self);
unsigned char navi_manager_fill_target_passes(struct NaviManager *self, int x, int y);

int navi_manager_guided_kind(struct NaviManager *self);
unsigned char navi_manager_player_cell(struct NaviManager *self, int *x, int *y);
int navi_manager_route_step(struct NaviManager *self);
struct NaviLink *navi_manager_route_link_at(struct NaviManager *self, int index);
struct SearchResult *navi_manager_guided(struct NaviManager *self, struct SearchResult *out);
struct String *navi_manager_guided_map_name(struct NaviManager *self, struct String *out);
struct SearchResult *navi_manager_destination(struct NaviManager *self, struct SearchResult *out);
struct NaviMap *navi_manager_target_map2(struct NaviManager *self);
struct NaviPoint *navi_manager_target_pos2(struct NaviManager *self, struct NaviPoint *out);
int navi_manager_result_count(struct NaviManager *self);
int navi_manager_route_link_count(struct NaviManager *self);

struct String *navi_manager_step_text_dots(struct NaviManager *self, struct String *out);
struct String *navi_manager_target_chat_text(struct NaviManager *self, struct String *out);
struct String *navi_manager_step_text(struct NaviManager *self, struct String *out);
struct String *navi_manager_step_map_name(struct NaviManager *self, struct String *out);
struct String *navi_manager_target_label(struct NaviManager *self, struct String *out);
void navi_manager_report_status(struct NaviManager *self, int status, int how);
unsigned char navi_manager_run_search(struct NaviManager *self);

struct NaviUiRecord *navi_ui_record_construct(struct NaviUiRecord *self, int style,
        int x, int y, const char *text, unsigned colour, unsigned colour2);
void navi_ui_record_destroy(struct NaviUiRecord *self);
int navi_manager_add_text_lines(struct NaviManager *self, int style, int x, int y,
        const char *text);
unsigned char navi_manager_update_window(struct NaviManager *self);

void navi_manager_request_airship(struct NaviManager *self, struct String mapName,
        int itemId);

void navi_manager_clear_info_lines(struct NaviManager *self);
void navi_manager_clear_groups(struct NaviManager *self);
int navi_manager_group_of(struct NaviManager *self, int index);
struct SearchResult *navi_manager_result_at(struct NaviManager *self,
        struct SearchResult *out, int index);
struct String *navi_manager_result_map_name(struct NaviManager *self,
        struct String *out, int index, int *sprite);
struct NaviMap *navi_manager_result_map(const struct SearchResult *r);
void navi_manager_build_groups(struct NaviManager *self);

struct NaviPoint *navi_manager_target_pos(struct NaviManager *self, struct NaviPoint *out);
struct NaviMap *navi_manager_target_map(struct NaviManager *self);
void navi_manager_clear_target(struct NaviManager *self);
struct NaviManager *navi_manager_base_construct(struct NaviManager *self);
void navi_manager_base_destruct(struct NaviManager *self);

void search_results_push_copy(struct Vec *v, const struct SearchResult *r);
void search_results_clear(struct Vec *v);
void search_results_free(struct Vec *v);
void navi_results_sort(struct SearchResult *first, struct SearchResult *last);
void navi_ui_records_free(struct Vec *v);
void navi_groups_free(struct Vec *v);
struct C3dAttr;

#define NAVI_UI_DIR "유저인터페이스\\"
extern int g_naviMarkerFrame;
extern unsigned g_naviMarkerTime;

#endif
