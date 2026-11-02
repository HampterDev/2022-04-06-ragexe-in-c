#include "contents/private_airplane.h"
#include "contents/pet_auto_feed.h"
#include "contents/check_attendance.h"
#include "web/barter_market.h"
#include "web/barter_expanded.h"
#include "game/world_objects.h"
#include "contents/player_names.h"
#include "contents/map_info_api.h"
#include "contents/party_booking.h"
#include "contents/user_hotkeys.h"
#include "resources/animation.h"
#include "std/list.h"
#include "skills/skill_bundle.h"
#include "skills/skill_info.h"
#include "ui/cooldown.h"
#include "ui/window_messages.h"
#include "platform/code_page.h"
#include "std/string.h"
#include "iteminfo/item_list.h"
#include "iteminfo/item_api.h"
#include "contents/mcstore_backup.h"
#include "contents/client_lua.h"
#include "contents/client_lua_calls.h"
#include "contents/guild_emblem_api.h"
#include "game/own_states.h"
#include "platform/service.h"
#include "contents/quest.h"
#ifndef SESSION_IMPORTS_H
#define SESSION_IMPORTS_H

#include "platform/error_message.h"
#include "game/actor_names.h"
#include "game/mode_misc.h"
#include "skills/skill_action.h"
#include "actor/map_type_table.h"
#include "contents/achievement.h"
#include "render/math3d.h"

#include "actor/render_object.h"
#include "iteminfo/item_table.h"
#include "navi/navi_manager.h"
#include "contents/item_info_mgr.h"
#include "ui/quests/world_view.h"
#include "game/client_state.h"
#include "core/window_state.h"
#include "effects/durations.h"
#include "contents/guild_info.h"
#include "skills/skill_action_registry.h"
#include "ui/skin_manager.h"
#include "session/state.h"
#include "ui/window_manager_state.h"
#include "core/memory.h"
#include "world/mode_manager.h"
#include "ui/window_access.h"
#include "world/actor_lookup.h"
#include "contents/connection_api.h"

#include "fileio/resource_api.h"

#include "std/tree.h"

#include "platform/client_config.h"

#include "core/markup_tags.h"
#include <string.h>
#include "base.h"
#include "render/renderer.h"
#define WIN32_LEAN_AND_MEAN
#include "platform/win_types.h"
#include "std/std.h"
#include "core/client.h"
#include "fileio/res_mgr.h"
#include "core/xml.h"
#include "core/msg_str.h"
#include "core/text_util.h"

struct GameActor;
struct GameMode;
struct RagEffect;
struct Res;
struct SkillActionMgr;
struct ItemInfo;
struct SkillActionAnswer;
struct SkillInfo;
struct List;
struct ListNode;
struct String;
struct Tree;
struct TreeNode;
struct Vec;
struct UIWindow;
#include "render/vec3.h"

#include "effects/effect_mgr.h"

extern int g_bitsPerPixel;
extern int g_displayModeIndex;

extern int g_displayDeviceIndex;

extern int hideui_skip_count;
int read_display_options(void *session, const char *name, int def);
void write_display_options(void *session, void *stream, const char *name, int value);

#endif
