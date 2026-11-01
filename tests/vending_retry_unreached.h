#include <stdlib.h>
#include "replay/replay.h"
#include "session/session_windows.h"
#include "ui/window_access.h"
#include "ui/wndmgr/wndmgr.h"
#include "ui/shops/merchant_shop_make.h"

struct UIWindowMgr g_windowMgr;
int rag_connection_get_packet_length(struct RagConnection *c, int id) { abort(); }
struct ReplayRecorder *replay_recorder_instance(void) { abort(); }
int ui_window_send(struct UIWindow *w, struct UIWindow *s, int m, int a, int b, int c, int d) { abort(); }
void session_refresh_equip_wnd(struct Session *s) { abort(); }
void ui_window_mgr_invalidate_once_per_frame(struct UIWindowMgr *w) { abort(); }
void ui_window_mgr_refresh_item_windows(struct UIWindowMgr *w) { abort(); }
struct ItemInfo *session_get_inventory_item_at(struct Session *s, struct ItemInfo *out, int n) { abort(); }
int session_is_in_switch_sets(struct Session *s, int index) { abort(); }
int ui_merchant_shop_make_wnd_get_chosen_cell(const struct UIMerchantShopMakeWnd *w, int *x, int *y) { abort(); }
