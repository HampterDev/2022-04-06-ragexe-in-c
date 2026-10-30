#include "navi/navi_manager.h"
#include "navi/navi_search.h"
#include "ui/window_mgr.h"
#include "ui/controls/edit.h"
#include <stdlib.h>
int g_replayPlaying;
int ui_window_mgr_get_window_flag(struct UIWindowMgr *manager, int kind, int id)
{ (void)manager; (void)kind; (void)id; abort(); }
int ui_edit_ctrl_get_text_size(struct UIWindow *edit) { (void)edit; abort(); }
const char *ui_edit_ctrl_get_text(struct UIWindow *edit) { (void)edit; abort(); }
struct NaviMap *navi_manager_current_map(struct NaviManager *manager) { (void)manager; abort(); }
void std_sort(void *a, void *b, unsigned size, unsigned char (*less)(const void *, const void *, void *), void *context)
{ (void)a; (void)b; (void)size; (void)less; (void)context; abort(); }
unsigned char navi_result_name_greater(struct SearchResult *a, const struct SearchResult *b) { (void)a; (void)b; abort(); }
