#include "session/state.h"
#include "std/list.h"
#include "core/object_type.h"
#include "game/context.h"
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <mmsystem.h>
#include <string.h>
#include "diag/perf.h"
#include "ui/window_types.h"

static DWORD floor_ms = 50, floor_named_ms = 100, floor_override;
static int floor_read;

static int soft_next;

void ui_soft_next(int on)
{
    soft_next = on;
}

struct wnd_state { struct UIWindow *w; DWORD lastDraw; };
#define STATE_SLOTS 4096
static struct wnd_state states[STATE_SLOTS];

static struct wnd_state *state_of(struct UIWindow *w)
{
    unsigned h = (((unsigned)(ULONG_PTR)w) >> 4) * 2654435761u >> 20, i;
    struct wnd_state *oldest = NULL;
    for (i = 0; i < 8; i++) {
        struct wnd_state *e = &states[(h + i) & (STATE_SLOTS - 1)];
        if (e->w == w)
            return e;
        if (!e->w) {
            e->w = w;
            e->lastDraw = 0;
            return e;
        }
        if (!oldest || (int)(oldest->lastDraw - e->lastDraw) > 0)
            oldest = e;
    }
    oldest->w = w;
    oldest->lastDraw = 0;
    return oldest;
}

static int cooldown_running(void)
{
    struct SkillCooldownNode *head = g_session1.skillDelays.head;
    return head != NULL;
}

static DWORD floor_for(const struct UIWindow *w)
{
    static const struct { const char *cls; DWORD ms; } named[] = {
        { "UIShortCutWnd", 100 }, { "UIQuikSlotWnd", 100 }, { "UIMinimapZoomWnd", 100 }, { "UINewChatWnd", 100 },
        { "UIChatEditCtrl", 100 }, { "UIBitmapButton", 100 }, { "UIMerchantShopTitle", 250 }, { NULL, 0 } };
    const char *cls = object_type_name(w->ops);
    int i;
    for (i = 0; named[i].cls; i++) {
        if (!strcmp(cls, named[i].cls)) {
            if (i < 2 && cooldown_running())
                return 0;
            return floor_override ? floor_named_ms : named[i].ms;
        }
    }
    return floor_ms;
}

static void read_floor(void)
{
    char buf[32];
    floor_read = 1;
    if (GetEnvironmentVariableA("RAGEXE_UI_FLOOR_MS", buf, sizeof(buf)) && buf[0]) {
        DWORD v = 0;
        const char *p = buf;
        while (*p >= '0' && *p <= '9')
            v = v * 10 + (DWORD)(*p++ - '0');
        floor_ms = v;
        floor_named_ms = v;
        floor_override = 1;
    }
}

void ui_ondraw(struct UIWindow *self)
{
    struct wnd_state *st = state_of(self);
    g_perf.ondraws++;
    if (st)
        st->lastDraw = timeGetTime();
    self->ops->on_draw(self);
    self->dirty = 0;
}

void ui_window_invalidate_base(struct UIWindow *self)
{
    struct UIWindow *parent;
    int soft = soft_next;
    soft_next = 0;
    g_perf.invalidates++;
    if (!floor_read)
        read_floor();
    if (floor_ms && soft && game_current_mode()) {
        struct wnd_state *st = state_of(self);
        if (st && timeGetTime() - st->lastDraw < floor_for(self)) {
            g_perf.invalidatesDeferred++;
            return;
        }
    }
    self->dirty = 1;
    parent = self->parent;
    if (parent && parent->composites == 1)
        parent->ops->invalidate(parent);
}

void ui_window_invalidate_children(struct UIWindow *self)
{
    struct ListNode *head, *node;
    g_perf.invalidates++;
    self->dirty = 1;
    head = self->children.head;
    for (node = head->next; node != head; node = node->next) {
        struct UIWindow *child = *LIST_VALUE(node, struct UIWindow *);

        if (child->visible)
            ui_window_invalidate_children(child);
    }
}
