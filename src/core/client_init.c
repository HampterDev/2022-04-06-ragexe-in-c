#include <stdlib.h>
#include "base.h"
#include "core/client_init.h"
#include "core/msg_str.h"
#include "core/packet_table.h"
#include "core/res_mgr.h"
#include "contents/guild_info.h"
#include "core/xml.h"
#include "fileio/file_manager.h"
#include "fileio/res_mgr.h"
#include "fileio/resource_errors.h"
#include "game/own_states.h"
#include "game/whisper_log.h"
#include "iteminfo/item_table_internal.h"
#include "iteminfo/item_tables_internal.h"
#include "navi/navi_manager.h"
#include "platform/client_config.h"
#include "platform/clientinfo.h"
#include "platform/error_message.h"
#include "platform/language.h"
#include "render/device.h"
#include "render/fonts.h"
#include "render/texture.h"
#include "render/texture_cache.h"
#include "render/texture_globals.h"
#include "resources/granny_res.h"
#include "session/session_state.h"
#include "session/state.h"
#include "skills/skill_action.h"
#include "std/std.h"
#include "actor/weather.h"
#include "ui/character/character.h"
#include "ui/misc/snap_mgr.h"
#include "ui/quests/tag_list_box.h"
#include "ui/quests/world_view.h"
#include "ui/skin_manager.h"
#include "ui/social/guild_member_sort.h"
#include "ui/wndmgr/wndmgr.h"
#include "ui/window_manager_lifecycle.h"
#include "ui/window_manager_state.h"
#include "ui/worldmap/kafra_services.h"
#include "world/pathfinder_state.h"
#include "world/pick_node.h"
#include "game/connection.h"

static void release_packet_table(void) { packet_table_destruct(&g_packetLengthMap); }
static void release_white_texture(void) { surface_reset(&g_whiteTexture.base); }
static void release_file_archives(void) { game_file_manager_clear(&g_fileMgr); }
static void release_world_view(void) { world_view_clear(&g_worldView); }
static void release_snap_mgr(void) { snap_mgr_destruct(&g_snapMgr); }
static void release_window_mgr(void) { ui_window_mgr_destroy(&g_windowMgr); }
static void release_actor_pick_node(void) { actor_pick_node_destruct(&g_actorPickNode); }
static void release_guild_info(void) { guild_info_destruct(&g_guildInfo); }
static void release_navi(void) { navi_manager_destruct(&g_navi); }
static void release_weather(void) { weather_destruct(&g_weather); }
static void release_path_finder(void) { path_finder_destruct(&g_pathFinder); }
static void release_keep_alive_times(void) { list_free(&g_keepAliveTimes, NULL); }
static void release_whisper_log(void) { file_destruct(&g_whisperLog); }
static void release_session(void) { session_destruct(&g_session1); }
static void release_granny_bone_res(void) { granny_animation_cache_clear(&g_grannyBoneRes); }
static void release_tag_prototypes(void) { tag_prototypes_free(&g_tagPrototypes); }
static void release_client_info_document(void)
{
    xml_document_release(&g_clientInfoDocument);
    xml_element_destruct(&g_clientInfoDocument.root);
}
static void release_admin_aids(void) { vec_free(&g_adminAids); }
static void release_yellow_aids(void) { vec_free(&g_yellowAids); }
static void release_loading_screens(void) { string_vec_free(&g_loadingScreens); }

/* Lower layers call up into higher ones only through these hooks and handlers. This is
   the one place that installs them; each is documented where it is declared. */
static void client_install_hooks(void)
{
    g_errorMsgPrepareDisplay = device_flip_to_gdi;
}

void client_init(void)
{
    client_install_hooks();
    msg_strings_init();
    packet_table_construct(&g_packetLengthMap);
    atexit(release_packet_table);
    gdiplus_init();
    texture_cache_init();
    g_whiteTexture = (struct Texture){
        .base = {.ops = &texture_surface_ops, .width = 1, .height = 1},
        .reqWidth = 1, .reqHeight = 1,
    };
    atexit(release_white_texture);
    atexit(release_file_archives);
    resource_errors_init();
    atexit(resource_errors_destroy);
    world_view_initialize(&g_worldView);
    atexit(release_world_view);
    skin_name_init();
    basic_skin_name_init();
    snap_mgr_construct(&g_snapMgr);
    atexit(release_snap_mgr);
    atexit(kafra_destinations_clear);
    atexit(kafra_positions_clear);
    window_mgr_construct(&g_windowMgr);
    atexit(release_window_mgr);
    atexit(own_states_destroy);
    actor_pick_node_construct(&g_actorPickNode);
    atexit(release_actor_pick_node);
    guild_info_construct(&g_guildInfo);
    atexit(release_guild_info);
    navi_manager_construct(&g_navi);
    atexit(release_navi);
    weather_construct(&g_weather);
    atexit(release_weather);
    path_finder_construct(&g_pathFinder);
    atexit(release_path_finder);
    list_init(&g_keepAliveTimes);
    atexit(release_keep_alive_times);
    file_construct(&g_whisperLog);
    atexit(release_whisper_log);
    session_construct(&g_session1);
    atexit(release_session);
    item_table_init();
    card_affix_table_init();
    metal_process_table_init();
    table_reader_init();
    item_move_info_table_init();
    resource_cache_init();
    atexit(release_granny_bone_res);
    skill_actions_register();
    tag_prototypes_construct(&g_tagPrototypes);
    atexit(release_tag_prototypes);
    xml_element_construct(&g_clientInfoDocument.root, "XMLDocument");
    g_clientInfoDocument.copy = NULL;
    atexit(release_client_info_document);
    atexit(release_admin_aids);
    atexit(release_yellow_aids);
    atexit(release_loading_screens);
}
