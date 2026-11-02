#include "actor/weather.h"
#include "effects/effect_mgr.h"
struct Renderer *g_renderer;
struct EffectMgr *g_effectMgr;
struct Weather g_weather;

struct Effect;
struct EffectMgr;
struct GameFileMgr;
struct Weather;
struct ClientLua;
struct ItemInfo;
struct String;
struct Tree;
struct TreeNode;
struct Vec;
struct UIWindow;
struct Effect *effect_mgr_spawn(struct EffectMgr *self, int aid, int type, Vec3 pos, int target_aid,
                                             int level, unsigned char reuse, int instance_key) { abort(); }
void cloud_respawn(struct Effect *self) { abort(); }
unsigned char game_file_mgr_is_file_exist(struct GameFileMgr *manager, const char *name, unsigned char disk_only) { abort(); }
void weather_launch(struct Weather *self, unsigned char kind) { abort(); }
char *game_mode_short_name(struct String map) { abort(); }
struct ItemInfo *item_info_construct(struct ItemInfo *self) { abort(); }
struct ItemInfo *item_info_copy(struct ItemInfo *self, const struct ItemInfo *src) { abort(); }
int item_info_get_slot_count(const struct ItemInfo *self) { abort(); }
void item_info_set_item_id(struct ItemInfo *self, int itemId) { abort(); }
char *msg_str(int id) { abort(); }
void store_search_item_copy(void *self, const void *src) { abort(); }
void store_search_items_destroy(void *first, void *last) { abort(); }
int client_lua_call(struct ClientLua *lua, const char *function, const char *format, ...) { abort(); }
void client_lua_error_box(struct ClientLua *self, const char *caption, const char *format, ...) { abort(); }
int game_map_dimensions(int *width, int *height) { abort(); }
char *std_copy_memmove(const char *first, const char *last,
                                  char *dest) { abort(); }
struct UIWindow *ui_window_find(int id) { abort(); }
struct UIWindow *ui_window_open(int id) { abort(); }
intptr_t ui_window_send(struct UIWindow *window, struct UIWindow *sender, int message,
                   intptr_t a, intptr_t b, intptr_t c, intptr_t d) { abort(); }
