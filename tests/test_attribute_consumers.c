#include "game/cells.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "world/pathfinder_state.h"
#include "../src/game/packets_world.c"

static struct GameMode mode;
static struct World world;
static struct C3dAttr grid;
static struct AttrCell cells[4];
static int present = 1;
struct GameMode *game_current_mode(void) { return present ? &mode : NULL; }

struct Session g_session1;
struct UIWindowMgr g_windowMgr;
struct Renderer *g_renderer;
struct PathFinder g_pathFinder;
struct RenewQuestMgr *g_questTable;
void string_free(struct String *s) { abort(); }
struct TreeNode *tree_next(struct TreeNode *node) { (void)node; abort(); }
int crt_sprintf(char *buffer, const char *format, ...) { abort(); }
int game_mode_send_msg(struct Mode *m, int message, int a, int b, int c, int d) { abort(); }
struct UIWindow *ui_window_open(int id) { abort(); }
int ui_window_send(struct UIWindow *w, struct UIWindow *s, int message, int a, int b, int c, int d) { abort(); }
void ui_window_move_safe(struct UIWindow *w, int x, int y) { abort(); }
struct GameActor *world_get_game_actor_by_aid(struct World *w, unsigned aid) { abort(); }
unsigned char ui_window_mgr_post_quit1(struct UIWindowMgr *m, int id) { abort(); }
struct Skill *world_get_skill_by_aid(struct World *w, int aid) { abort(); }
struct PartyMember *session_get_party_member(struct Session *s, struct PartyMember *out, DWORD aid) { abort(); }
void session_set_party_member_state(struct Session *s, DWORD aid, DWORD state) { abort(); }
void session_notify_messenger_group_and_plates(struct Session *s) { abort(); }
char renew_quest_set_active(struct RenewQuestMgr *quests, unsigned id, char active) { abort(); }
void renew_quest_refresh_windows(void) { abort(); }
void path_info_init(struct PathInfo *p) { abort(); }
void path_info_destroy(struct PathInfo *p) { abort(); }
unsigned char path_finder_find_path(struct PathFinder *p, unsigned long time, int sx, int sy,
    int subx, int suby, int tx, int ty, int speed, struct PathInfo *path, int kind) { abort(); }
char is_effect_state_hiding(unsigned state) { abort(); }
int ray_triangle_hit(const struct Ray3 *ray, const Vec3 *v0, const Vec3 *v1, const Vec3 *v2,
                     float *u, float *v, float *t) { abort(); }

int main(void)
{
    struct PACKET_ZC_UPDATE_MAPINFO update = {0};
    float x,z;
    int cx,cy;
    memset(&mode, 0, sizeof mode); memset(&world, 0, sizeof world);
    mode.world = &world; world.attr = &grid;
    grid.width = grid.height = 2; grid.zoom = 5; grid.cells = cells;
    cells[0].type = 0; cells[1].type = 1; cells[2].type = 5; cells[3].type = 3;
    assert(!game_mode_is_cell_blocked(&mode, 0, 0));
    assert(game_mode_is_cell_blocked(&mode, 1, 0));
    assert(game_mode_is_cell_blocked(&mode, 0, 1));
    assert(!game_mode_is_cell_blocked(&mode, 1, 1));
    game_mode_cell_to_world(&mode, 1, 0, &x, &z); assert(x == 2.5f && z == -2.5f);
    game_mode_world_to_cell(&mode, x, z, &cx, &cy); assert(cx == 1 && cy == 0);
    update.x = 1; update.y = 1; update.type = 5;
    game_mode_zc_update_map_info(NULL, &update); assert(cells[3].type == 5);
    update.x = -1; update.type = 8;
    game_mode_zc_update_map_info(NULL, &update); assert(cells[3].type == 5);
    update.x = 2; game_mode_zc_update_map_info(NULL, &update);
    update.x = 0; update.y = 2; game_mode_zc_update_map_info(NULL, &update);
    update.y = 0; grid.cells = NULL;
    game_mode_zc_update_map_info(NULL, &update);
    assert(game_mode_is_cell_blocked(&mode, 0, 0));
    world.attr = NULL; game_mode_zc_update_map_info(NULL, &update);
    mode.world = NULL; game_mode_zc_update_map_info(NULL, &update);
    present = 0; game_mode_zc_update_map_info(NULL, &update);
    puts("attribute consumers: shared cell queries, walkability, coordinate roundtrip and bounded packet updates passed");
    return 0;
}
