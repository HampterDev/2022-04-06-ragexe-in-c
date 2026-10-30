#include "std/vec.h"
#include "std/string.h"
#include "actor/render_ops.h"
#include "render/rp_pool.h"
#include "render/texture_globals.h"
#include "render/texture_cache.h"
#include "session/option_pairs.h"
#include "resources/sprite.h"
#include "resources/animation.h"
#include "ui/window_messages.h"
#include "session/state.h"
#include "ui/window_manager_state.h"
#include "core/memory.h"
#include "core/fatal.h"
#include "world/mode_manager.h"
#include "game/context.h"
#include "ui/window_access.h"
#include "world/actor_lookup.h"
#include "contents/connection_api.h"
#ifndef GAMEMODE_CLIENT_H
#define GAMEMODE_CLIENT_H

#include "render/projection.h"

#include "fileio/resource_api.h"

#include "base.h"
#include "render/renderer.h"
#include "ui/window_types.h"

struct RagConnection;

static __inline void send_packet(int packet_id, const void *packet)
{
    int len = rag_connection_get_packet_length(rag_connection_instance(), packet_id);
    rag_connection_send_packet(rag_connection_instance(), len, packet);
}

struct Session;

struct UIWindowMgr;
struct UIWindow;

#define UIWM_CHAT_PRINT 1
#define CHAT_COLOR_RED  0xff

#define FILE_MODE_WRITE 0x1001

enum {
    UIM_CHAT_LINE = 1,
    UIM_REFRESH = 3,
};

struct ModeMgr;

struct Mode;
#include "world/mode_dispatch.h"
#define CMODE_SENDMSG(m, msg, a, b, c, d) mode_send_message((struct Mode *)(m), (msg), (a), (b), (c), (d))
enum {
    CMODE_MSG_WHISPER = 0xb,
};

struct UIWindow;
#define UIWINDOW_SENDMSG(w, msg, a, b, c, d) \
    ui_window_send((struct UIWindow *)(w), NULL, (msg), (a), (b), (c), (d))

struct World;
struct GameActor;

struct Texture;
struct TexCache;

#include "render/texture_quad.h"

enum {
    WID_ILLUST = 0x4e,
};

static __inline void *res_get(const char *name)
{
    return res_mgr_get(g_resMgr(), name);
}

struct ActRes;

#endif
