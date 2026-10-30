#include "game/repute.h"
#include "contents/repute_api.h"

void game_mode_on_repute_info(struct GameMode *self, unsigned char success,
                           unsigned count, const struct ReputeEntry *entries)
{
    (void)self;
    for (; count; count--, entries++)
        repute_mgr_set_points(repute_mgr_instance(), entries->id, entries->points);
    repute_mgr_set_info_result(repute_mgr_instance(), success);
}

void game_mode_on_ui_open_repute(struct GameMode *self, uint64_t group_id,
                            uint64_t info_id)
{
    (void)self;
    repute_mgr_on_changed(repute_mgr_instance(), group_id, info_id);
}
