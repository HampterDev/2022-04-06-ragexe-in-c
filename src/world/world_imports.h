#include "game/cells.h"
#include "render/device.h"
#include "render/keyframes.h"
#include "render/rp_pool.h"
#include "render/texture_globals.h"
#include "render/texture_cache.h"
#include "render/sprite_atlas.h"
#include "resources/animation.h"
#include "ui/window_messages.h"
#include "render/frame_capture.h"
#include "resources/attr_queries.h"
#include "navi/grid_uv.h"
#include "navi/navi_manager.h"
#include "world/hover_cell.h"
#include "actor/weather.h"
#include "platform/client_options.h"
#include "contents/guild_info.h"
#include "resources/model_node_names.h"
#include "session/state.h"
#include "ui/window_manager_state.h"
#include "contents/connection_api.h"
#include "core/fatal.h"
#ifndef WORLD_IMPORTS_H
#define WORLD_IMPORTS_H

#include "ui/window_access.h"
#include "ui/chat/balloon.h"
#include "render/projection.h"

#include "render/math3d.h"

#include "fileio/resource_api.h"

#include "base.h"
#include "render/renderer.h"
#include "render/texture_quad.h"
#include "std/std.h"
#include "fileio/res_mgr.h"
#include "resources/attr.h"

#define WIN32_LEAN_AND_MEAN
#include "platform/win_types.h"

struct String;
struct List;
struct ListNode;
struct Vec;
struct Tree;
struct Texture;
struct RPQuad;
struct RPTwoTex;
struct TexDrawVertex;
struct C3dAttr;
struct AttrCell;
struct FrustumObb;

#include "resources/attr.h"

struct GameMode;

#include "effects/effect_mgr.h"

void actor_index_clear(void);
void actor_index_drop_node(const struct ListNode *node);
extern DWORD actor_list_generation;

#endif
