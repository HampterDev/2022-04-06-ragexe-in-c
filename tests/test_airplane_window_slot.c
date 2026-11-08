#include "ui/window_manager_state.h"
#include "ui/window_mgr.h"
#include "ui/wndmgr/wndmgr.h"
#include "world/mode_dispatch.h"
#include "world/mode_manager.h"
#include "core/app.h"
#include "core/object_type_tables.h"
#include "render/cdc.h"
#include "render/surface.h"
#include "render/texture.h"
#include "std/list.h"
#include "std/vec.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MODAL_RESULT_FLIGHT 0xb1

struct UIWindowMgr g_windowMgr;
int g_quitRequested;

void *mem_alloc(size_t bytes)
{
    void *p = calloc(1, bytes);
    assert(p);
    return p;
}
void mem_free(void *p) { free(p); }
void actors_forget_window(const struct UIWindow *wnd) { (void)wnd; }
void ui_window_mgr_remove_from_name_waiting_list(struct UIWindowMgr *self, struct UIWindow *wnd)
{ (void)self; (void)wnd; }
void dc_bitmap_destroy(struct DcBitmap *self) { (void)self; abort(); }
void surface_destroy(struct Surface *surface) { (void)surface; abort(); }
void vec_free(struct Vec *v)
{
    free(v->first);
    memset(v, 0, sizeof *v);
}
struct UIWindow *ui_window_get_ancestor(struct UIWindow *self) { return self; }
void ui_window_mgr_focus_first_edit(struct UIWindowMgr *self) { (void)self; }
struct Mode *client_current_mode(void) { return NULL; }

static struct UIWindow *quitting;
void ui_window_mgr_post_quit2(struct UIWindowMgr *self, struct UIWindow *wnd)
{
    assert(self == &g_windowMgr && !quitting);
    quitting = wnd;
}

static int frame_result, frames;
void mode_update(struct Mode *mode)
{
    (void)mode;
    assert(g_windowMgr.private_airplane_wnd && g_windowMgr.modal_wnd == g_windowMgr.private_airplane_wnd);
    frames++;
    g_windowMgr.modal_result = frame_result;
}

static void on_destroy(struct UIWindow *self) { (void)self; }
static int destroyed;
static void airplane_destroy(struct UIWindow *self)
{
    ui_window_destruct(self);
    destroyed++;
}
const struct UIWindowOps ui_window_ops = { .on_destroy = on_destroy };
static const struct UIWindowOps airplane_ops = { .destroy = airplane_destroy, .on_destroy = on_destroy };

void ui_window_destruct(struct UIWindow *self);
#include "airplane_window_slot.inc"

static void fly(int result, const char *what)
{
    static struct UIWindow window;

    memset(&window, 0, sizeof window);
    list_init(&window.draws);
    list_init(&window.children);
    window.ops = &airplane_ops;
    quitting = NULL;
    frames = destroyed = 0;
    frame_result = result;

    open_airship_slot(&window);
    assert(frames == 1 && quitting == &window && !g_windowMgr.modal_wnd);
    ui_window_destroy(quitting);
    assert(destroyed == 1);
    if (g_windowMgr.private_airplane_wnd) {
        printf("%s: private_airplane_wnd still points at the destroyed window\n", what);
        exit(1);
    }
}

int main(void)
{
    fly(AIRPLANE_RESULT_CLOSED, "closed");
    fly(MODAL_RESULT_FLIGHT, "flight");
    puts("Airplane window slot: cleared when the window is destroyed after its modal loop ends by closing (0xb4) or by the flight's map move (0xb1).");
    return 0;
}
