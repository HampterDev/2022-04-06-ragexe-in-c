#include "ui/cooldown.h"
#include "iteminfo/item_list.h"
#include "iteminfo/item_api.h"
#include "contents/client_lua_calls.h"
#ifndef SESSION_A_CSESSION_H
#define SESSION_A_CSESSION_H

#include "session/state.h"
#include "base.h"
#include "core/fatal.h"

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#include "std/std.h"
#include "session/session_imports.h"

#define ITEM_LOCATION_AMMO 0x8000

#include "session/party_types.h"

struct PartyMember *party_member_assign(struct PartyMember *m, const struct PartyMember *src);

struct NamedInt { const char *name; int value; };

struct JobName {
    unsigned job;
    const char *name;
};

struct Pulse {
    int item;
    int lo, hi;
    DWORD period;
};

struct MapRect {
    RECT rect;
    int set;
};

struct NamedMapRect {
    struct String name;
    struct MapRect rect;
};

struct GroupMapRect {
    int group;
    struct MapRect rect;
};

void named_map_rect_destroy(void *value);

#define UIMSG_REFRESH 0x17

static __inline void notify_item_window(struct UIWindow *const *slot)
{
    struct UIWindow *wnd = *slot;
    if (wnd)
        ui_window_send(wnd, NULL, UIMSG_REFRESH, 0, 0, 0, 0);
}

#endif
