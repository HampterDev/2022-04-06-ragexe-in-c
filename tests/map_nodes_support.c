#include "contents/client_lua_calls.h"
#include "std/std.h"
#include "session/state.h"
#include "core/fatal.h"
#include "game/cells.h"
#include "game/context.h"
#include "ui/ui_std.h"
#include <stdlib.h>
#include <assert.h>

struct Session g_session1;
struct Session;
struct SkillActionMgr;
struct CooldownTimer;
struct ItemInfo;
struct List;
struct ListNode;
struct String;
struct Tree;
struct TreeNode;
struct Vec;
struct UIWindow;

const char * session_get_job_name(struct Session * arg0, int arg1, int arg2) { abort(); }
struct SkillActionMgr * skill_action_mgr_instance(void) { abort(); }
unsigned char skill_action_mgr_is_good_for_actor(struct SkillActionMgr * arg0, int arg1) { abort(); }
struct ItemInfo * item_info_copy(struct ItemInfo * arg0, const struct ItemInfo * arg1) { abort(); }
int client_lua_call(struct ClientLua *arg0, const char *arg1, const char * arg2, ...) { abort(); }
void store_search_item_copy(void * arg0, const void * arg1) { abort(); }
void store_search_items_destroy(void * arg0, void * arg1) { abort(); }

char * std_copy_memmove(const char * arg1, const char * arg2, char * arg3) { abort(); }
struct UIWindow *map_nodes_test_window;
unsigned map_nodes_window_queries;
struct UIWindow *ui_window_find(int id)
{
    assert(id == 0x25);
    ++map_nodes_window_queries;
    return map_nodes_test_window;
}

struct GameMode *game_current_mode(void) { abort(); }
void game_mode_cell_to_world(struct GameMode *self, int x, int y, float *world_x, float *world_z) { abort(); }
void game_mode_world_to_cell(struct GameMode *self, float world_x, float world_z, int *x, int *y) { abort(); }
