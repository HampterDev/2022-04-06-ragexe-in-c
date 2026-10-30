#include "game/client.h"
#include "contents/mouse.h"
#include "render/sprite_draw.h"
#include "std/std.h"
#include <stdlib.h>

struct Session g_session1;
struct Mouse g_mouse;
struct Renderer *g_renderer;
int g_replayPlaying;

int game_mode_send_msg(struct Mode *mode, int message, int a, int b, int c, int d) { abort(); }
int ui_window_mgr_message_box(struct UIWindowMgr *self, const char *text, int type,
    int a, int b, int c, const char *title, int width, int height, int arg) { abort(); }
struct ResMgr *g_resMgr(void) { abort(); }
void *res_mgr_get(struct ResMgr *self, const char *name) { abort(); }
char *msg_str(int id) { abort(); }
const char *session_get_item_act_name(const char *name) { abort(); }
struct ItemInfo *session_get_inventory_item(struct Session *self, struct ItemInfo *out, int index) { abort(); }
struct ItemInfo *session_get_equipped_item(struct Session *self, struct ItemInfo *out, int index) { abort(); }
void screen_sprite_frame(struct Renderer *renderer, int x, int y, struct ActRes *act,
    struct SprRes *spr, int action, int motion, float scale, float angle, DWORD colour, int flags) { abort(); }
int ui_window_mgr_is_dragging_member_plate(struct UIWindowMgr *self) { abort(); }
void ui_window_mgr_move_dragged_plate(struct UIWindowMgr *self, int x, int y) { abort(); }
