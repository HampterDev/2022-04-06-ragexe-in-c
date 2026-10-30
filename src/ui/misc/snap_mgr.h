#ifndef UI_MISC_SNAP_MGR_H
#define UI_MISC_SNAP_MGR_H

#include <windows.h>
#include "base.h"
#include "std/container_types.h"

struct UIWindow;

#define SNAP_LIST_MAX 0x15555554u

struct SnapObject {
    struct UIWindow *wnd;
    struct List sides[4];
    int visited;
};
OFFSET_ASSERT(SnapObject, visited, 0x24);
SIZE_ASSERT(SnapObject, 0x28);

struct SnapMgr {
    struct SnapObject *root;
    struct List objects;
};
SIZE_ASSERT(SnapMgr, 0xc);

struct ListNode *snap_list_buy_node0(struct List *list, struct ListNode *next,
                                                struct ListNode *prev);
struct ListNode *snap_list_buy_node(struct List *list, struct ListNode *next,
                                               struct ListNode *prev, void *const *value);
struct List *snap_list_construct(struct List *list);
void snap_list_destroy(struct List *list);
void snap_list_push_back(struct List *list, void *const *value);

struct SnapObject *snap_object_construct(struct SnapObject *self);
void snap_object_destruct(struct SnapObject *self);
int snap_object_side_of(struct SnapObject *self, struct SnapObject *other);
void snap_object_unlink(struct SnapObject *self, struct SnapObject *other);
void snap_object_sum_heights(struct SnapObject *self, int *sum, int side);
BYTE snap_object_is_connected(struct SnapObject *self, struct SnapObject *target);
BYTE snap_object_is_connected_from(struct SnapObject *self, struct SnapObject *target);
BYTE snap_object_reaches_below(struct SnapObject *self, struct SnapObject *target);
void snap_object_move_chain(struct SnapObject *self, int dx, int dy, int side);
void snap_object_move_chain_checked(struct SnapObject *self, int dx, int dy, int side);

void snap_mgr_destruct(struct SnapMgr *self);
void snap_mgr_register_root(struct SnapMgr *self, struct UIWindow *wnd);
void snap_mgr_register(struct SnapMgr *self, struct UIWindow *wnd);
void snap_mgr_clear_visited(struct SnapMgr *self);
struct SnapObject *snap_mgr_find(struct SnapMgr *self, struct UIWindow *wnd);
BYTE snap_mgr_are_connected(struct SnapMgr *self, struct UIWindow *a,
                                      struct UIWindow *b);
BYTE snap_mgr_is_root_window(struct SnapMgr *self, struct UIWindow *wnd);
BYTE snap_mgr_in_root_group(struct SnapMgr *self, struct UIWindow *wnd);
void snap_mgr_move_chain(struct SnapMgr *self, struct UIWindow *wnd, int dx, int dy,
                                   int side);
void snap_mgr_relink(struct SnapMgr *self, struct SnapObject *obj);
void snap_mgr_clear(struct SnapMgr *self);
void snap_mgr_unregister(struct SnapMgr *self, struct UIWindow *wnd);

extern struct SnapMgr g_snapMgr;
struct SnapMgr *snap_mgr_construct(struct SnapMgr *self);
BYTE snap_mgr_is_registered_to_snap(struct SnapMgr *self, struct UIWindow *wnd);
void snap_mgr_save_group_positions(struct SnapMgr *self, struct UIWindow *wnd);
void snap_mgr_move_group(struct SnapMgr *self, struct UIWindow *root, int dx, int dy);
void snap_mgr_drag_group(struct SnapMgr *self, struct UIWindow *root, int *dx, int *dy);
void snap_mgr_relink_all(struct SnapMgr *self, struct UIWindow *wnd);

struct SnapEdge {
    int pos;
    int from, to;
};

struct SnapInfo {
    struct SnapEdge left, right;
    struct SnapEdge top, bottom;
};

void snap_info_set(struct SnapInfo *self, int x, int y, int w, int h);
void snap_info_flip(struct SnapInfo *self);
void snap_info_get_delta_snap(struct SnapInfo *self, const struct SnapInfo *other,
                                       int *dx, int *dy);

#endif
