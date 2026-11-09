#include "skill_oracle.h"
#include "game/packet_handlers.h"
#include "game/effect_notify.h"
#include "game/world_objects.h"
#include "game/packet_data.h"
#include "web/name_card_api.h"
#include "ui/window_messages.h"
#include "ui/window_access.h"
#include "ui/shops/merchant_shop.h"
#include "effects/effect_controls.h"
#include "effects/skills/emitter_effect.h"
#include "contents/client_lua_calls.h"
#include "contents/connection_api.h"
#include "session/session_items.h"
#include "session/session_lua.h"
#include "session/session_windows.h"
#include "session/abr.h"
#include "std/string.h"
#include "std/tree.h"
#include "std/integer_map.h"
#include "platform/error_message.h"
#include "game/actor_entry.h"
#include "game/entry_packet_handlers.h"
#include "game/actor_marks.h"
#include "contents/champion_monster_effects.h"
#include "contents/player_names.h"
#include "world/world.h"
#include "session/session_jobs.h"
#include "session/session_talk.h"
#include "session/character_name.h"
#include "platform/clientinfo.h"
#include "game/display_settings.h"
#include "game/character_stats.h"
#include "game/status_values.h"
#include "session/session_state.h"
#include "ui/character/gauge.h"
#include "ui/character/status_wnd.h"
#include "iteminfo/item_api.h"
#include "game/item_info.h"
#include "contents/pet_evolution.h"
#include "session/item_predicates.h"
#include "ui/shops/storage.h"
#include "ui/wndmgr/wndmgr.h"
#include "core/unknown_packet.h"
#include "iteminfo/item_table.h"
#include "replay/replay.h"
#include "game/own_states.h"
#include "game/connection.h"
#include "game/map_entry.h"
#include "game/chat.h"
#include "contents/sound.h"
#include "core/object_type.h"
#include "core/text_util.h"
#include "world/mode_manager.h"
#include "world/mode_actions.h"
#include "ui/quests/say_dialog.h"
#include "ui/character/character.h"
#include "std/vec.h"
#include "ui/items/pet_windows.h"
#include "ui/items/item_list.h"
#include "std/list.h"
#include "skills/skill_info.h"
#include "actor/game_actor.h"
#include "iteminfo/item_name.h"
#include "game/companion_packets.h"
#include "contents/party_booking.h"
#include "game/party_booking_packets.h"
#include "ui/social/party_board.h"
#include "ui/social/seek_party_mb.h"
#include "web/service_managers.h"
#include "contents/manner_filter.h"
#include "session/session_party.h"
#include "game/party_member.h"
#include "game/minimap.h"
#include "ui/chat/chat_room_wnd.h"
#include "game/chat_room.h"
#include "game/actor_packets.h"
#include "world/mode_dispatch.h"
#include "packet_handlers.inc"

#define ORIG_SESSION          0x1133c50u
#define ORIG_SESSION_SIZE     0x83b0u
#define ORIG_EFFECT_ON        0x1138cb4u
#define ORIG_SERVICE_TYPE     0x1226950u
#define ORIG_INSTANT_VARS     0x113b620u
#define ORIG_PLAYER_TARGET    0x4fc
#define ORIG_MODE_NAMES       0x160
#define ORIG_MODE_DIALOG_OPEN 0x24c
#define ORIG_MODE_PLAYER_DEAD 0x250
#define ORIG_NAMES_ERASE      0x8af760u
#define ORIG_NAMES_SET_ALL    0x8b0480u
#define ORIG_ERASE_ABR        0x874db0u
#define ORIG_WORLD_REMOVE     0x65e390u
#define ORIG_JOB_IS_MONSTER   0x74bef0u
#define ORIG_HOMUN_OR_MERC    0x883dd0u
#define ORIG_IS_HOMUNCULUS    0x883d20u
#define ORIG_LAUNCH_DUMMY     0x779bc0u
#define ORIG_PET_HUNGER       0x8774c0u
#define ORIG_PET_INTIMACY     0x877360u
#define ORIG_PET_EMOTION      0x877280u
#define ORIG_PET_TALK         0x8787d0u
#define ORIG_NOTIFY_PLATES    0x86c3b0u
#define ORIG_MESSAGE_BOX      0x62e260u
#define ORIG_POST_QUIT        0x62b6c0u
#define ORIG_MAKE_WINDOW      0x6349a0u
#define ORIG_CLEAR_PURCHASE   0x84fc60u
#define ORIG_ACTOR_SET_JOB    0x75f300u
#define ORIG_SESSION_SET_JOB  0x899b50u
#define ORIG_WEAPON_VIEW      0x87f2b0u
#define ORIG_SET_COMBO_DELAY  0x88b9b0u
#define ORIG_FORCE_ANIM_END   0x75eee0u
#define ORIG_SESSION_ACTOR    0x875cd0u
#define ORIG_SET_ARGB         0x75cf20u
#define ORIG_CONN_INSTANCE    0x90bac0u
#define ORIG_CLEAR_HELD       0x90b640u
#define ORIG_TITLE_STRING     0x87eff0u
#define ORIG_EMITTER_SPEED    0x994520u
#define ORIG_EMITTER_COUNT    0x9944f0u
#define ORIG_STOP_AURA        0x749ca0u
#define ORIG_LEVEL_AURA       0x7497d0u
#define ORIG_REMOVE_TYPE      0x96b3d0u
#define ORIG_REROLL           0x84f5f0u
#define ORIG_REFRESH_BASIC    0x867510u
#define ORIG_IS_PC_JOB_ACTOR  0x74bf70u
#define ORIG_SPRINTF          0x4bcf10u
#define ORIG_SESSION_AID      0x1134c14u
#define ORIG_PET_ACCESSORY    0x1134c3cu
#define ORIG_HOMUN_AI_FLAG    0x1138e84u
#define ORIG_MERC_PRESENT     0x1138f28u
#define ORIG_PVP_FIRST_TIME   0x11343acu
#define ORIG_PVP_RULE         0x1138cc8u
#define ORIG_REPLAY_PLAYING   0x1119d38u
#define ORIG_MODE_SQUARE_MARKS   0x1b4
#define ORIG_MODE_TRIANGLE_MARKS 0x1bc
#define ORIG_WORLD_ACTORS     0x10
#define ORIG_PLAYER_PVP_RANK  0x474
#define ORIG_PLAYER_PVP_TOTAL 0x478
#define ORIG_ACTOR_BALLOON_FONT 0xfc
#define ORIG_ACTOR_MARK_LISTED  0x2e8
#define ORIG_ACTOR_IS_BOSS      0x308
#define ORIG_ACTOR_OBJECT_TYPE  0x318
#define ORIG_PC_SIZE          0x4d8
#define ORIG_OPERATOR_NEW     0xb461beu
#define ORIG_PC_CONSTRUCT     0x82e5e0u
#define ORIG_NEW_ACTOR_FOR_JOB 0x777890u
#define ORIG_LIST_BUY_NODE    0x659710u
#define ORIG_ACTOR_LIST_BUY_NODE 0x7979e0u
#define ORIG_OBJECT_LIST_PUSH 0x70bbb0u
#define ORIG_ACTOR_LIST_PUSH  0x7e3530u
#define ORIG_MARKS_FIND       0x797860u
#define ORIG_MAKE_EMBLEM_WND  0x7737e0u
#define ORIG_IS_SIEGE_MODE    0x884040u
#define ORIG_IS_PLAYER_JOB    0x884010u
#define ORIG_IS_HOMUNCULUS_AID 0x883ec0u
#define ORIG_IS_MERCENARY_AID 0x883ee0u
#define ORIG_PET_ACCESSORY_ACT 0x877120u
#define ORIG_LOAD_ACT_RES     0x75cc50u
#define ORIG_CHAMPION_EFFECTS 0x9238b0u
#define ORIG_PLAYER_NAMES_SET 0x8f1710u
#define ORIG_PLACE_NOWHERE    0x65ed90u
#define ORIG_PLACE_AT_CELL    0x65ec20u
#define ORIG_NAMES_SET_NAME   0x8b0da0u
#define ORIG_SYNC_SERVER_TIME 0x86dc90u
#define ORIG_CHARACTER_NAME   0x875550u
#define ORIG_IS_ADMIN_OPTION  0x8682a0u
#define ORIG_IS_ADMIN_AID     0xb05da0u
#define ORIG_INVALIDATE_WINDOWS 0x867610u
#define ORIG_IS_SUPER_NOVICE  0x884100u
#define ORIG_DRAWN_JOB        0x8539e0u
#define ORIG_BASIC_INFO_HEIGHT 0xaf8c50u
#define ORIG_BAR_SET_VALUES   0x568330u
#define ORIG_REFRESH_ACTOR_EFFECTS 0x772130u
#define ORIG_GET_WINDOW       0x6409d0u
#define ORIG_REFRESH_SKILL_WNDS 0x88af30u
#define ORIG_STATUS_RESTORE   0x57b6f0u
#define ORIG_END_EXCHANGE     0x84f3f0u
#define ORIG_FIND_ABR         0x875070u
#define ORIG_ABR_GET          0x7e6110u
#define ORIG_SET_ADVANCED_STATUS 0x7ae5e0u
#define ORIG_PET_TALK_DUE     0x8903b0u
#define ORIG_WND_STATUS       0xee8974u
#define ORIG_WND_BASIC_INFO   0xee898cu
#define ORIG_WND_GROUP        0xee8a78u
#define ORIG_WND_HEIGHT       0x18
#define ORIG_WND_BASE_BAR     0xd0
#define ORIG_WND_JOB_BAR      0xd4
#define ORIG_WINDOW_VT_SEND   (0x88 / 4)
#define ORIG_MODE_EXCHANGE_OPEN 0x114
#define ORIG_MODE_STATUS_PENDING 0x2e4
#define ORIG_ABR_HP           0x1134374u
#define ORIG_ABR_MAX_HP       0x113437cu
#define ORIG_HP               0x1138ddcu
#define ORIG_MAX_HP           0x1138de0u
#define ORIG_SP               0x1138de4u
#define ORIG_MAX_SP           0x1138de8u
#define ORIG_WALK_SPEED       0x11352e4u
#define ORIG_KARMA            0x11352f4u
#define ORIG_MANNER           0x11352e8u
#define ORIG_STATUS_POINTS    0x1135244u
#define ORIG_BASE_LEVEL       0x1135240u
#define ORIG_JOB_LEVEL        0x1135248u
#define ORIG_SKILL_POINTS     0x113524cu
#define ORIG_WEIGHT           0x11352f0u
#define ORIG_MAX_WEIGHT       0x11352ecu
#define ORIG_WEIGHT_PENALTY   0x113b5ecu
#define ORIG_STATS            0x1135274u
#define ORIG_STAT_COSTS       0x113528cu
#define ORIG_STAT_BONUS       0x113525cu
#define ORIG_TRAIT_BONUS      0x113b638u
#define ORIG_ATK              0x11352a8u
#define ORIG_REFINE_ATK       0x11352bcu
#define ORIG_MATK_MAX         0x11352c0u
#define ORIG_MATK_MIN         0x11352c4u
#define ORIG_DEF              0x11352b4u
#define ORIG_PLUS_DEF         0x11352b8u
#define ORIG_MDEF             0x11352acu
#define ORIG_PLUS_MDEF        0x11352c8u
#define ORIG_HIT              0x11352ccu
#define ORIG_FLEE             0x11352d0u
#define ORIG_PLUS_FLEE        0x11352d8u
#define ORIG_CRITICAL         0x11352d4u
#define ORIG_ASPD             0x11352a4u
#define ORIG_PLUS_ASPD        0x11352b0u
#define ORIG_BASE_EXP         0x1135220u
#define ORIG_NEXT_BASE_EXP    0x1135228u
#define ORIG_NEXT_JOB_EXP     0x1135230u
#define ORIG_JOB_EXP          0x1135238u
#define ORIG_ZENY             0x11352e0u
#define ORIG_PET_FULLNESS     0x1134c48u
#define ORIG_PET_INTIMACY_VAL 0x1134c50u
#define ORIG_PET_JOB          0x1134c40u
#define ORIG_HATCHED_EGG      0x1134c54u
#define ORIG_PET_AUTO_FEED    0x1135200u
#define ORIG_PACKET_BUFFER    0x1121a10u
#define ORIG_ITEM_CONSTRUCT   0x92c970u
#define ORIG_ITEM_SET_ID      0x9305f0u
#define ORIG_ADD_CART_ITEM    0x84e390u
#define ORIG_ADD_STORAGE_ITEM 0x84ef00u
#define ORIG_INVENTORY_UPDATE 0x86cd50u
#define ORIG_ADD_INVENTORY_ITEM 0x84dfd0u
#define ORIG_SET_WORN_ITEM    0x84dd10u
#define ORIG_SET_WORN_COSTUME 0x873c40u
#define ORIG_REMOVE_FILTER_TYPES 0x84faa0u
#define ORIG_REMOVE_OTHER_TYPES 0x84f900u
#define ORIG_CLEAR_WORN_ITEMS 0x84fa60u
#define ORIG_CLEAR_WORN_COSTUMES 0x874c40u
#define ORIG_CLEAR_INVENTORY  0x84fc30u
#define ORIG_CLEAR_CART       0x84fcc0u
#define ORIG_REFRESH_PARTY_UI 0x86c2d0u
#define ORIG_REFRESH_FRIEND_UI 0x86c3e0u
#define ORIG_REFRESH_ITEM_WNDS 0x640e00u
#define ORIG_IS_PET_EGG_ID    0x884020u
#define ORIG_IS_COSTUME_LOCATION 0x883a80u
#define ORIG_GET_INVENTORY_ITEM 0x8532e0u
#define ORIG_PET_AUTO_FEEDING 0x91b940u
#define ORIG_WND_PET_INFO     0xee8b14u
#define ORIG_WND_HOMUN_INFO   0xee8b4cu
#define ORIG_WND_HOMUN_SKILL  0xee8b50u
#define ORIG_PET_INFO_INTIMACY 0xa5fae0u
#define ORIG_PET_SPEAK        0x878480u
#define ORIG_FORMAT_CHAT_LINE 0x878810u
#define ORIG_SELECTED_ITEM    0x57e7c0u
#define ORIG_SET_PK_MODE      0x75f770u
#define ORIG_GET_ITEM_BY_ID   0x853220u
#define ORIG_ITEM_GET_NAME    0x92d5c0u
#define ORIG_ITEM_DISPLAY_NAME 0x92d720u
#define ORIG_HOMUN_REDRAW     0x867570u
#define ORIG_MERC_REDRAW      0x8675e0u
#define ORIG_LIST_CLEAR       0x5fbce0u
#define ORIG_MER_SKILLS       0x1133cbcu
#define ORIG_PET_MODIFIED     0x1134c18u
#define ORIG_PET_NAME         0x1134c1cu
#define ORIG_PET_LEVEL        0x1134c44u
#define ORIG_PET_PREV_FULLNESS 0x1134c58u
#define ORIG_HIDE_CHAT        0x1138ca8u
#define ORIG_HIDE_CHAT_GUILD  0x1138cacu
#define ORIG_HOMUN_AID        0x1138decu
#define ORIG_HOMUN_NAME       0x1138df0u
#define ORIG_HOMUN_STATS      0x1138e10u
#define ORIG_HOMUN_JOB        0x1138e30u
#define ORIG_HOMUN_LEVEL      0x1138e34u
#define ORIG_HOMUN_HP         0x1138e38u
#define ORIG_HOMUN_MAX_HP     0x1138e3cu
#define ORIG_HOMUN_SP         0x1138e40u
#define ORIG_HOMUN_MAX_SP     0x1138e44u
#define ORIG_HOMUN_INTIMACY   0x1138e48u
#define ORIG_HOMUN_ITEM       0x1138e4cu
#define ORIG_HOMUN_EXP        0x1138e50u
#define ORIG_HOMUN_MAX_EXP    0x1138e58u
#define ORIG_HOMUN_FULLNESS   0x1138e60u
#define ORIG_HOMUN_RENAMED    0x1138e68u
#define ORIG_HOMUN_AI_STATE   0x1138e90u
#define ORIG_HOMUN_SKILL_POINTS 0x1135254u
#define ORIG_MERC_AID         0x1138e9cu
#define ORIG_MERC_NAME        0x1138ea0u
#define ORIG_MERC_STATS       0x1138ec0u
#define ORIG_MERC_HP          0x1138ee0u
#define ORIG_MERC_MAX_HP      0x1138ee4u
#define ORIG_MERC_JOB         0x1138ee8u
#define ORIG_MERC_LEVEL       0x1138eecu
#define ORIG_MERC_SP          0x1138ef0u
#define ORIG_MERC_MAX_SP      0x1138ef4u
#define ORIG_MERC_CONTRACT    0x1138ef8u
#define ORIG_ELEMENTAL_AID    0x113943cu
#define ORIG_ELEMENTAL_HP     0x1139440u
#define ORIG_ELEMENTAL_SP     0x1139444u
#define ORIG_ELEMENTAL_MAX_HP 0x1139448u
#define ORIG_ELEMENTAL_MAX_SP 0x113944cu
#define ORIG_ACTOR_VT_SET_SPR_JOB (0x54 / 4)
#define ORIG_WINDOW_VT_INVALIDATE (0x8c / 4)
#define ORIG_CTRL_VT_SET_TEXT (0xc0 / 4)
#define ORIG_WND_NAME_CTRL    0x94
#define ORIG_COLLECTION_PET   0x1e0
#define N_COMPANION_NAMES 5
#define ORIG_WND_CHAT_ROOM    0xee8a28u
#define ORIG_WND_PARTY_SETTING 0xee8abcu
#define ORIG_REQUEST_MGR_PTR  0x121ffc4u
#define ORIG_AGENCY_PTR       0x113be40u
#define ORIG_BOOKING_ADD_ROWS 0x671740u
#define ORIG_BOOKING_REMOVE   0x670b50u
#define ORIG_BOOKING_ON_NEW   0x6714d0u
#define ORIG_BOOKING_SET_JOBS 0x671f40u
#define ORIG_SET_WANTED_JOBS  0x5e57e0u
#define ORIG_STOP_RECRUITING  0x8b4740u
#define ORIG_ADD_JOIN_REQUEST 0xa320c0u
#define ORIG_COMMAND_PLATE_OF 0x644630u
#define ORIG_IS_OWN_UNIT_AID  0x883f00u
#define ORIG_FILTER_CONTAINS  0x66ebe0u
#define ORIG_FILTER_CENSOR    0x66ded0u
#define ORIG_CLEAR_INV_PENDING 0x86dc00u
#define ORIG_CLEAR_CART_PENDING 0x86dc30u
#define ORIG_CLEAR_STORAGE_PENDING 0x86dc60u
#define ORIG_GET_PARTY_MEMBER 0x855930u
#define ORIG_SET_MEMBER_STATE 0x84f550u
#define ORIG_ADD_PARTY_MEMBER 0x84e110u
#define ORIG_OWN_AID          0x113520cu
#define ORIG_PARTY_LEADER     0x1138cd0u
#define ORIG_PARTY_EXP_OPTION 0x1138d10u
#define ORIG_PARTY_PICKUP_RULE 0x1138d14u
#define ORIG_PARTY_DIVISION_RULE 0x1138d18u
#define ORIG_PARTY_NAME       0x1138d34u
#define ORIG_PARTY_INVITE_REFUSED 0x11391b0u
#define ORIG_BOOKING_ALARM_FILTERED 0x11393f9u
#define ORIG_BOOKING_QUEUE_NEW 0x1139420u
#define ORIG_BOOKING_ANSWER_TICK 0x1139428u
#define ORIG_MODE_MAP_NAME    0x68
#define ORIG_MODE_ITEM_MOVE_READY 0x5f0
#define ORIG_POST_SIZE        0x30
#define ORIG_MEMBER_SIZE      0x50
#define ORIG_WND_EXCHANGE     0xee8a30u
#define ORIG_CURRENT_MODE_PTR 0xe8e304u
#define ORIG_ROOM_OWNER_AT    0x79db60u
#define ORIG_IS_YELLOW_AID    0xb05de0u
#define ORIG_REMOVE_FRIEND    0x8515b0u
#define ORIG_NOTIFY_GROUP_WND 0x86c2b0u
#define ORIG_END_EXCHANGE1    0x84f2d0u
#define ORIG_THEIR_ZENY       0x86cf30u
#define ORIG_ADD_THEIR_ITEM   0x86d510u
#define ORIG_REFUSE_TRADES    0x1138cb0u
#define ORIG_MODE_CHAT_PENDING 0xfc
#define ORIG_MODE_EXCHANGE_PENDING 0x100
#define ORIG_MODE_WHISPER_NAME 0x118
#define ORIG_MODE_EXCHANGE_PARTNER 0x148
#define ORIG_MODE_FAMILY_AID  0x290
#define ORIG_MODE_FAMILY_CHAR_ID 0x294
#define ORIG_MODE_COUPLE_NAME 0x298
#define ORIG_MODE_OWN_ROOM    0x3c0
#define ORIG_WND_ROOM         0x94
#define ORIG_ROOM_SIZE        0x40
#define ORIG_ITEM_ARG_SIZE    0xe8
#define N_CHAT_TEXTS 6
#define ORIG_SEND_PET_AUTO_FEED 0x899bd0u
#define ORIG_SET_TEXT_ELLIPSIS 0x5380c0u
#define ORIG_STORAGE_SET_NAME 0x7e3670u
#define ORIG_DISPATCHER       0x7a0570u
#define ORIG_REPLAY_INSTANCE  0x676270u
#define ORIG_REPLAY_NEXT      0x6745b0u
#define ORIG_REPLAY_SET_MODE  0x675fe0u
#define ORIG_LOG_WHISPER      0x786130u
#define ORIG_CLEAR_LISTS      0x84f9f0u
#define ORIG_ITEM_TABLE_GET   0x92d620u
#define ORIG_APPEND_MARKET    0x84e7a0u
#define ORIG_ADD_BOUGHT       0xa7e570u
#define ORIG_RT_DYNAMIC_CAST  0xba592eu
#define ORIG_MODE_MGR_SWITCH  0x662870u
#define ORIG_WAIT_SET_TEXT    0x5abbf0u
#define ORIG_RESET_STORE_SEARCH 0x862e10u
#define ORIG_RESET_FOR_WARP   0x7829b0u
#define ORIG_MODE_INITIALIZE  0x7713e0u
#define ORIG_RUN_FADE_OUT     0x661c60u
#define ORIG_RESET_MAIL_ITEMS 0x874b70u
#define ORIG_CLEAR_MAIL_COUNTS 0x874c70u
#define ORIG_MOVE_TO_SERVER   0x79eab0u
#define ORIG_PLAY_STREAM      0x90e710u
#define ORIG_SET_TEXT_ALIGN   0xaf7670u
#define ORIG_SOUND_MGR_PTR    0x115c1a0u
#define ORIG_SOUND_BGM_ON     0x18
#define ORIG_SOUND_BGM_OVERRIDDEN 0x1c
#define ORIG_WND_SAY_HELPER   0xee89a0u
#define ORIG_WND_REPLAY_CONTROL 0xeebe9cu
#define ORIG_WND_MODAL_RESULT 0xee8c00u
#define ORIG_OWN_STATES_BEGIN 0xeebfbcu
#define ORIG_OWN_STATES_END   0xeebfc0u
#define ORIG_MODE_DIALOG_ID   0x2ec
#define ORIG_MODE_LOOP_SOUNDS 0x1e4
#define ORIG_MAP_NAME         0x1134388u
#define ORIG_PLAYER_POS_X     0x1134b18u
#define ORIG_PLAYER_POS_Y     0x1134b1cu
#define ORIG_AUTO_SAVE_CHAT   0x1138dc8u
#define ORIG_LAST_PACKET_TIME 0x113b618u
#define ORIG_WND_ALIGN_CTRL   0x94
#define ORIG_ACTOR_VT_SET_JOB_MODEL (0x48 / 4)
#define ORIG_LOOP_SOUND_SIZE  0x24
#define ORIG_OWN_STATE_SIZE   0x14
#define N_TEXTS 10
#define N_MAPS 6
#define ORIG_WND_TITLE_CTRL   0x13c
#define OI_TYPE 0x00
#define OI_INDEX 0x04
#define OI_EQUIP_LOCATION 0x08
#define OI_WORN_LOCATION 0x0c
#define OI_COUNT 0x10
#define OI_PRICE 0x14
#define OI_CHARGED_PRICE 0x18
#define OI_SLOT 0x1c
#define OI_ID_TEXT 0x2c
#define OI_NAME 0x44
#define OI_IDENTIFIED 0x5c
#define OI_DAMAGED 0x5d
#define OI_REFINE 0x60
#define OI_BIND 0x64
#define OI_EXPIRE_HOURS 0x66
#define OI_EXPIRE 0x68
#define OI_WEIGHT 0x6c
#define OI_VIEW 0x70
#define OI_FAVORITE 0x74
#define OI_IS_NEW 0x75
#define OI_MOVE_PENDING 0x76
#define OI_BY_INDEX 0x77
#define OI_SALE_KIND 0x78
#define OI_SALE_START 0x7c
#define OI_SALE_END 0x80
#define OI_SALE_REMAINING 0x84
#define OI_GRADE 0x88
#define OI_OPTION_COUNT 0x8c
#define OI_OPTIONS 0x90
#define ORIG_STD_STRING       24
#define ORIG_STD_STRING_SSO   16

#define ORIG_ACTOR_VT_SET_HEAD        (0x58 / 4)
#define ORIG_ACTOR_VT_SET_WEAPON      (0x5c / 4)
#define ORIG_ACTOR_VT_SET_WEAPON_LOOK (0x60 / 4)
#define ORIG_ACTOR_VT_SET_ACCESSORY   (0x64 / 4)
#define ORIG_ACTOR_VT_SET_ACCESSORY2  (0x68 / 4)
#define ORIG_ACTOR_VT_SET_ACCESSORY3  (0x6c / 4)
#define ORIG_ACTOR_VT_SET_SHIELD      (0x70 / 4)
#define ORIG_ACTOR_VT_SET_APPEARANCE  (0x74 / 4)
#define ORIG_ACTOR_VT_SET_IMF_NAME    (0x78 / 4)
#define ORIG_ACTOR_VT_SET_HEAD_PAL    (0x7c / 4)
#define ORIG_ACTOR_VT_SET_BODY_PAL    (0x80 / 4)
#define ORIG_ACTOR_VT_SET_BODY_PALETTE (0x84 / 4)
#define ORIG_ACTOR_VT_SET_HEAD_PALETTE (0x88 / 4)
#define ORIG_ACTOR_VT_SET_HONOR       (0xa0 / 4)
#define ORIG_ACTOR_VT_SET_ROBE        (0xac / 4)
#define ORIG_ACTOR_VT_SET_BODY_STYLE  (0xb0 / 4)
#define ORIG_ACTOR_VT_REFRESH_ACC3    (0xe8 / 4)
#define ORIG_ACTOR_VT_SET_GUILD       (0xc0 / 4)
#define ORIG_ACTOR_VT_GET_GUILD_ID    (0xc4 / 4)

#define AID_UNKNOWN 4242
#define PACKET_MAX 0x1000
#define SWEEP_VALUES 16
#define MAX_REPORTED 40
#define FAKE_CONNECTION 0x5151

enum { FILL_ZERO, FILL_PATTERN, FILL_ONES, FILL_COUNT };
enum { PK_DEFAULT, PK_NAMES, PK_ENTRY, PK_PAR, PK_EXP, PK_LIST, PK_SPLIT_SET, PK_TEXT, PK_MARKET, PK_MAP };

enum { R_SESSION, R_MODE, R_CASTER, R_TARGET, R_PLAYER, R_EFFECT, R_NEW, R_COUNT };
#define M(r) (1u << (r))
#define M_ACTORS (M(R_CASTER) | M(R_TARGET) | M(R_PLAYER) | M(R_NEW))

struct Region {
    const char *name;
    unsigned char *orig, *lift;
    unsigned orig_size, lift_size;
    unsigned char *orig_snap, *lift_snap, *orig_seen, *lift_seen;
};

struct Field {
    const char *name;
    unsigned mask;
    unsigned orig_off, lift_off, size;
    int quiet;
};

static struct Region regions[R_COUNT];
static struct IntInt64Pair lift_instant_var;
static long long orig_instant_var;
static int instant_var_set, lift_instant_var_got;
static unsigned char lift_connection[0x100];
static unsigned char lift_names_holder;
static unsigned char orig_new_actor[0x1000];
static union { struct Pc pc; unsigned char raw[0x1000]; } lift_new_actor;
static int sweep_job, move_mask;
static unsigned orig_actor_list_head[3];
static unsigned fake_nodes[4][4];
static int fake_node_count;
static unsigned fake_tree_node[8];
static struct MemberMark fake_mark;
static int drawn_job, windows_present;
static unsigned char orig_status_wnd[0x400], orig_basic_info_wnd[0x400], orig_group_wnd[0x400], orig_found_wnd[0x400];
static struct UIWindow lift_status_wnd, lift_basic_info_wnd, lift_group_wnd, lift_found_wnd;
static void *orig_window_vt[64];
static long long fake_abr_value[2];
static int fake_abr_used[2];
static struct Int64Pair lift_abr_pair[2];
static struct IntIntPair lift_adv_pair;
static int adv_pending, adv_key, adv_value;
static unsigned char orig_storage_wnd[0x400];
static struct UIWindow lift_storage_wnd;
static unsigned char orig_generic_wnd[0x400], orig_helper_wnd[0x400], orig_replay[0x100], orig_sound_mgr[0x100];
static struct UIWindow lift_generic_wnd, lift_helper_wnd;
static unsigned char orig_pet_info_wnd[0x400], orig_homun_info_wnd[0x400], orig_homun_skill_wnd[0x400], orig_name_ctrl[0x10];
static struct UIWindow lift_pet_info_wnd, lift_homun_info_wnd, lift_homun_skill_wnd;
static void *orig_ctrl_vt[64];
static struct UIWindowOps lift_window_ops;
static struct { short level, hunger, intimacy, accessory; int renamed; char name[32]; } lift_pet_details;
static int current_fill;
static const char *const companion_names[N_COMPANION_NAMES] = { "", "Poring", "badname", "twentythree_characters_", "Lunatic Jr." };
static unsigned char orig_request_mgr[0x100], orig_agency[0x100];
static int lift_request_mgr[0x40], lift_agency[0x40];
struct UIRequestJoinPartyMgr *g_requestJoinPartyMgr;
struct AdventurerAgencyMgr *g_adventurerAgency;
struct WordFilter g_nameFilter;
static short wanted_jobs[PARTY_BOOKING_JOBS];
static struct UIChatRoomWnd lift_chat_room_wnd;
static unsigned orig_room_owners[4];
static struct ChatRoomOwner lift_room_owners[4];
static int current_side;
static const char *const chat_names[N_COMPANION_NAMES] = { "Hero", "Poring", "badname", "", "Lunatic Jr." };
static struct ReplayRecorder lift_replay;
static unsigned char orig_loop_sounds[4][ORIG_LOOP_SOUND_SIZE];
static struct LoopSound lift_loop_sounds[4];
static unsigned char orig_own_states[4][ORIG_OWN_STATE_SIZE];
static struct OwnStateEntry lift_own_states[4];
static size_t lift_own_count;
static const struct Handler *current_handler;
static const unsigned char *pending_pkt;
static int pending_count;
static unsigned char fake_item_entry[0x80];
static struct ItemTableEntry lift_item_entry;
static char long_text_600[601], long_text_max[0x7fe];
struct PetEvolutionMgr *g_petTable;
BYTE g_packetBuffer[0x10000];
void vec_free(struct Vec *v) { (void)v; }

static const char *window_name(const void *w)
{
    if (w == orig_status_wnd || w == &lift_status_wnd) return "status";
    if (w == orig_basic_info_wnd || w == &lift_basic_info_wnd) return "bi";
    if (w == orig_group_wnd || w == &lift_group_wnd) return "group";
    if (w == orig_found_wnd || w == &lift_found_wnd) return "found";
    if (w == orig_storage_wnd || w == &lift_storage_wnd) return "storage";
    if (w == orig_generic_wnd || w == &lift_generic_wnd) return "wnd";
    if (w == orig_helper_wnd || w == &lift_helper_wnd) return "helper";
    if (w == &lift_chat_room_wnd) return "wnd";
    if (w == orig_pet_info_wnd || w == &lift_pet_info_wnd) return "petinfo";
    if (w == orig_homun_info_wnd || w == &lift_homun_info_wnd) return "homuninfo";
    if (w == orig_homun_skill_wnd || w == &lift_homun_skill_wnd) return "homunskill";
    return w ? "?" : "null";
}
static int is_super_novice(int job)
{
    return job == 0x17 || job == 0xfcd || job == 0x1020 || job == 0x104c || (job >= 0x105e && job <= 0x1061) || job == 0x10d3 || job == 0x10da;
}
static const char *orig_std_string(const unsigned char *s);
static int is_pet_egg(int id) { return (unsigned)(id - 9001) <= 0x1f2; }
static int is_costume_location(int flag) { return flag != 0 && flag % 3 == 1; }
static const char *item_fields_text(int index, const char *id, const char *name, int type, int eq, int worn, int count, int price,
                                    int charged, const int *slot, int identified, int damaged, int refine, int bind, int hours,
                                    int expire, int weight, int view, int favorite, int is_new, int move_pending, int by_index,
                                    int sale_kind, int sale_start, int sale_end, int sale_remaining, int grade, int option_count,
                                    const unsigned char *options)
{
    static char text[2][256];
    static int turn;
    char *out = text[turn ^= 1];
    int n, i;
    n = snprintf(out, 256, "i%d id'%s' n'%s' t%d eq%x w%x c%d p%d cp%d s%x,%x,%x,%x idf%d dmg%d r%d b%d eh%d x%x wt%d v%d f%d nw%d mp%d bi%d sk%d ss%d se%d sr%d g%d o%d",
                 index, id, name, type, eq, worn, count, price, charged, slot[0], slot[1], slot[2], slot[3], identified, damaged, refine,
                 bind, hours, expire, weight, view, favorite, is_new, move_pending, by_index, sale_kind, sale_start, sale_end, sale_remaining,
                 grade, option_count);
    for (i = 0; i < ITEM_OPTION_SLOTS; i++) {
        unsigned short oid = *(const unsigned short *)(options + 5 * i);
        short oval = *(const short *)(options + 5 * i + 2);
        unsigned char oparam = options[5 * i + 4];
        if (oid || oval || oparam)
            n += snprintf(out + n, 256 - n, " [%u %d %u]", oid, oval, oparam);
    }
    return out;
}
static const char *item_text(const struct ItemInfo *it)
{
    return item_fields_text(it->index, string_cstr(&it->id_text), string_cstr(&it->name), it->type, it->equip_location, it->worn_location,
                            it->count, it->price, it->charged_price, it->slot, it->identified, it->damaged, it->refine, it->bind_on_equip_type,
                            it->expire_in_hours, it->expire_time, it->weight, it->view, it->favorite, it->is_new, it->move_pending,
                            it->by_index, it->sale_kind, it->sale_start_time, it->sale_end_time, it->sale_remaining, it->grade,
                            it->option_count, (const unsigned char *)it->options);
}
#define OI32(b, off) (*(const int *)((b) + (off)))
#define OI16(b, off) (*(const unsigned short *)((b) + (off)))
#define OI8(b, off) ((b)[off])
static const char *orig_item_text(const unsigned char *b)
{
    return item_fields_text(OI32(b, OI_INDEX), orig_std_string(b + OI_ID_TEXT), orig_std_string(b + OI_NAME), OI32(b, OI_TYPE),
                            OI32(b, OI_EQUIP_LOCATION), OI32(b, OI_WORN_LOCATION), OI32(b, OI_COUNT), OI32(b, OI_PRICE),
                            OI32(b, OI_CHARGED_PRICE), (const int *)(b + OI_SLOT), OI8(b, OI_IDENTIFIED), OI8(b, OI_DAMAGED),
                            OI32(b, OI_REFINE), OI16(b, OI_BIND), OI8(b, OI_EXPIRE_HOURS), OI32(b, OI_EXPIRE), OI16(b, OI_WEIGHT),
                            OI32(b, OI_VIEW), OI8(b, OI_FAVORITE), OI8(b, OI_IS_NEW), OI8(b, OI_MOVE_PENDING), OI8(b, OI_BY_INDEX),
                            OI32(b, OI_SALE_KIND), OI32(b, OI_SALE_START), OI32(b, OI_SALE_END), OI32(b, OI_SALE_REMAINING),
                            *(const short *)(b + OI_GRADE), OI32(b, OI_OPTION_COUNT), b + OI_OPTIONS);
}
static int abr_slot(const void *tree)
{
    if (tree == &g_session1.abrHp || tree == oracle_addr(ORIG_ABR_HP)) return 0;
    if (tree == &g_session1.abrMaxHp || tree == oracle_addr(ORIG_ABR_MAX_HP)) return 1;
    return -1;
}

#define SF(f) offsetof(struct Session, f)
#define AF(f) offsetof(struct GameActor, f)
#define PF(f) offsetof(struct Player, f)
#define GF(f) offsetof(struct GameMode, f)
#define EF(f) offsetof(struct EmitterEffect, f)
#define S(a) ((a) - ORIG_SESSION)

static const struct Field fields[] = {
    { "hairStyle", M(R_SESSION), S(0x1134adcu), SF(hairStyle), 4 },
    { "weapon", M(R_SESSION), S(0x1134ae0u), SF(weapon), 8 },
    { "shield", M(R_SESSION), S(0x1134ae8u), SF(shield), 4 },
    { "clothesColor", M(R_SESSION), S(0x1134aecu), SF(clothesColor), 4 },
    { "hairColor", M(R_SESSION), S(0x1134af0u), SF(hairColor), 4 },
    { "headBottom", M(R_SESSION), S(0x1134af4u), SF(headBottom), 4 },
    { "headTop", M(R_SESSION), S(0x1134af8u), SF(headTop), 4 },
    { "headMid", M(R_SESSION), S(0x1134afcu), SF(headMid), 4 },
    { "robe", M(R_SESSION), S(0x1134b00u), SF(robe), 4 },
    { "body", M(R_SESSION), S(0x1134b08u), SF(body), 4 },
    { "game_over", M(R_SESSION), S(0x1134b68u), SF(game_over), 4 },
    { "hp", M(R_SESSION), S(0x1138ddcu), SF(hp), 4 },
    { "sp", M(R_SESSION), S(0x1138de4u), SF(sp), 4 },
    { "ownTitleId", M(R_SESSION), S(0x1139988u), SF(ownTitleId), 4 },
    { "target_aid", M(R_PLAYER), ORIG_PLAYER_TARGET, PF(target_aid), 4 },
    { "npc_dialog_open", M(R_MODE), ORIG_MODE_DIALOG_OPEN, GF(npc_dialog_open), 4 },
    { "player_dead", M(R_MODE), ORIG_MODE_PLAYER_DEAD, GF(player_dead), 4 },
    { "motion_held", M_ACTORS, 0x9e, AF(motion_held), 1 },
    { "argb_locked", M_ACTORS, 0xa3, AF(argb_locked), 1 },
    { "level", M_ACTORS, 0x214, AF(level), 2 },
    { "velocity_min", M(R_EFFECT), 0xc8, EF(velocity_min), 12 },
    { "velocity_max", M(R_EFFECT), 0xd4, EF(velocity_max), 12 },
    { "acceleration", M(R_EFFECT), 0xe0, EF(acceleration), 12 },
    { "spawn_box", M(R_EFFECT), 0xec, EF(spawn_box), 12 },
    { "size_min", M(R_EFFECT), 0x100, EF(size_min), 4 },
    { "size_max", M(R_EFFECT), 0x104, EF(size_max), 4 },
    { "life_min", M(R_EFFECT), 0x108, EF(life_min), 4 },
    { "life_max", M(R_EFFECT), 0x10c, EF(life_max), 4 },
    { "rate_min", M(R_EFFECT), 0x110, EF(rate_min), 4 },
    { "rate_max", M(R_EFFECT), 0x114, EF(rate_max), 4 },
    { "r", M(R_EFFECT), 0x120, EF(r), 4 },
    { "g", M(R_EFFECT), 0x124, EF(g), 4 },
    { "b", M(R_EFFECT), 0x128, EF(b), 4 },
    { "alpha", M(R_EFFECT), 0x12c, EF(alpha), 4 },
    { "src_blend", M(R_EFFECT), 0x148, EF(src_blend), 4 },
    { "dest_blend", M(R_EFFECT), 0x14c, EF(dest_blend), 4 },
    { "max_frames", M(R_EFFECT), 0x184, EF(max_frames), 4 },
    { "aid", M(R_SESSION), S(ORIG_SESSION_AID), SF(aid), 4 },
    { "petAccessory", M(R_SESSION), S(ORIG_PET_ACCESSORY), SF(petAccessory), 4 },
    { "homun_ai_flag", M(R_SESSION), S(ORIG_HOMUN_AI_FLAG), SF(homun_ai_flags), 4 },
    { "mercPresent", M(R_SESSION), S(ORIG_MERC_PRESENT), SF(mercPresent), 4 },
    { "pvpFirstPlaceTime", M(R_SESSION), S(ORIG_PVP_FIRST_TIME), SF(pvpFirstPlaceTime), 4 },
    { "pvp_rank", M(R_PLAYER), ORIG_PLAYER_PVP_RANK, PF(pvp_rank), 4 },
    { "pvp_total", M(R_PLAYER), ORIG_PLAYER_PVP_TOTAL, PF(pvp_total), 4 },
    { "balloon_font", M_ACTORS, ORIG_ACTOR_BALLOON_FONT, AF(balloon_font), 2 },
    { "mark_listed", M_ACTORS, ORIG_ACTOR_MARK_LISTED, AF(mark_listed), 4 },
    { "is_boss", M_ACTORS, ORIG_ACTOR_IS_BOSS, AF(is_boss), 1 },
    { "object_type", M_ACTORS, ORIG_ACTOR_OBJECT_TYPE, AF(object_type), 1 },
    { "vtable", M(R_NEW), 0, 0, 4, 1 },
    { "maxHp", M(R_SESSION), S(ORIG_MAX_HP), SF(maxHp), 4 },
    { "maxSp", M(R_SESSION), S(ORIG_MAX_SP), SF(maxSp), 4 },
    { "walkSpeed", M(R_SESSION), S(ORIG_WALK_SPEED), SF(walkSpeed), 4 },
    { "karma", M(R_SESSION), S(ORIG_KARMA), SF(karma), 4 },
    { "manner", M(R_SESSION), S(ORIG_MANNER), SF(manner), 4 },
    { "statusPoints", M(R_SESSION), S(ORIG_STATUS_POINTS), SF(statusPoints), 4 },
    { "charBaseLevel", M(R_SESSION), S(ORIG_BASE_LEVEL), SF(charBaseLevel), 4 },
    { "jobLevel", M(R_SESSION), S(ORIG_JOB_LEVEL), SF(jobLevel), 4 },
    { "skillPoints", M(R_SESSION), S(ORIG_SKILL_POINTS), SF(skillPoints), 4 },
    { "weight", M(R_SESSION), S(ORIG_WEIGHT), SF(weight), 4 },
    { "maxWeight", M(R_SESSION), S(ORIG_MAX_WEIGHT), SF(maxWeight), 4 },
    { "stats", M(R_SESSION), S(ORIG_STATS), SF(stats), 24 },
    { "statCosts", M(R_SESSION), S(ORIG_STAT_COSTS), SF(statCosts), 24 },
    { "statBonus", M(R_SESSION), S(ORIG_STAT_BONUS), SF(statBonus), 24 },
    { "traitBonus", M(R_SESSION), S(ORIG_TRAIT_BONUS), SF(traitBonus), 24 },
    { "atk", M(R_SESSION), S(ORIG_ATK), SF(atk), 4 },
    { "refineAtk", M(R_SESSION), S(ORIG_REFINE_ATK), SF(refineAtk), 4 },
    { "matkMax", M(R_SESSION), S(ORIG_MATK_MAX), SF(matkMax), 4 },
    { "matkMin", M(R_SESSION), S(ORIG_MATK_MIN), SF(matkMin), 4 },
    { "def", M(R_SESSION), S(ORIG_DEF), SF(def), 4 },
    { "plusDef", M(R_SESSION), S(ORIG_PLUS_DEF), SF(plusDef), 4 },
    { "mdef", M(R_SESSION), S(ORIG_MDEF), SF(mdef), 4 },
    { "plusMdef", M(R_SESSION), S(ORIG_PLUS_MDEF), SF(plusMdef), 4 },
    { "hit", M(R_SESSION), S(ORIG_HIT), SF(hit), 4 },
    { "flee", M(R_SESSION), S(ORIG_FLEE), SF(flee), 4 },
    { "plusFlee", M(R_SESSION), S(ORIG_PLUS_FLEE), SF(plusFlee), 4 },
    { "critical", M(R_SESSION), S(ORIG_CRITICAL), SF(critical), 4 },
    { "aspd", M(R_SESSION), S(ORIG_ASPD), SF(aspd), 4 },
    { "plusAspd", M(R_SESSION), S(ORIG_PLUS_ASPD), SF(plusAspd), 4 },
    { "baseExp", M(R_SESSION), S(ORIG_BASE_EXP), SF(baseExp), 8 },
    { "nextBaseExp", M(R_SESSION), S(ORIG_NEXT_BASE_EXP), SF(nextBaseExp), 8 },
    { "nextJobExp", M(R_SESSION), S(ORIG_NEXT_JOB_EXP), SF(nextJobExp), 8 },
    { "jobExp", M(R_SESSION), S(ORIG_JOB_EXP), SF(jobExp), 8 },
    { "zeny", M(R_SESSION), S(ORIG_ZENY), SF(zeny), 4 },
    { "exchange_open", M(R_MODE), ORIG_MODE_EXCHANGE_OPEN, GF(exchange_open), 4 },
    { "status_request_pending", M(R_MODE), ORIG_MODE_STATUS_PENDING, GF(status_request_pending), 4 },
    { "hatchedEggIndex", M(R_SESSION), S(ORIG_HATCHED_EGG), SF(hatchedEggIndex), 4 },
    { "npc_dialog_id", M(R_MODE), ORIG_MODE_DIALOG_ID, GF(npc_dialog_id), 4 },
    { "loop_sounds", M(R_MODE), ORIG_MODE_LOOP_SOUNDS, GF(loop_sounds), 12, 1 },
    { "mapName", M(R_SESSION), S(ORIG_MAP_NAME), SF(mapName), 16 },
    { "playerPosX", M(R_SESSION), S(ORIG_PLAYER_POS_X), SF(playerPosX), 4 },
    { "playerPosY", M(R_SESSION), S(ORIG_PLAYER_POS_Y), SF(playerPosY), 4 },
    { "autoSaveChat", M(R_SESSION), S(ORIG_AUTO_SAVE_CHAT), SF(autoSaveChat), 4 },
    { "petModified", M(R_SESSION), S(ORIG_PET_MODIFIED), SF(petModified), 4 },
    { "petName", M(R_SESSION), S(ORIG_PET_NAME), SF(petName), 32 },
    { "petJob", M(R_SESSION), S(ORIG_PET_JOB), SF(petJob), 4 },
    { "petLevel", M(R_SESSION), S(ORIG_PET_LEVEL), SF(petLevel), 4 },
    { "petFullness", M(R_SESSION), S(ORIG_PET_FULLNESS), SF(petFullness), 4 },
    { "petIntimacy", M(R_SESSION), S(ORIG_PET_INTIMACY_VAL), SF(petIntimacy), 4 },
    { "petPrevFullness", M(R_SESSION), S(ORIG_PET_PREV_FULLNESS), SF(petPrevFullness), 4 },
    { "hideChat", M(R_SESSION), S(ORIG_HIDE_CHAT), SF(hideChat), 4 },
    { "hideChatAndGuild", M(R_SESSION), S(ORIG_HIDE_CHAT_GUILD), SF(hideChatAndGuild), 4 },
    { "homunAid", M(R_SESSION), S(ORIG_HOMUN_AID), SF(homunAid), 4 },
    { "homunName", M(R_SESSION), S(ORIG_HOMUN_NAME), SF(homunName), 32 },
    { "homunStats", M(R_SESSION), S(ORIG_HOMUN_STATS), SF(homunStats), 32 },
    { "homunJob", M(R_SESSION), S(ORIG_HOMUN_JOB), SF(homunJob), 4 },
    { "homunLevel", M(R_SESSION), S(ORIG_HOMUN_LEVEL), SF(homunLevel), 4 },
    { "homunHp", M(R_SESSION), S(ORIG_HOMUN_HP), SF(homunHp), 4 },
    { "homunMaxHp", M(R_SESSION), S(ORIG_HOMUN_MAX_HP), SF(homunMaxHp), 4 },
    { "homunSp", M(R_SESSION), S(ORIG_HOMUN_SP), SF(homunSp), 4 },
    { "homunMaxSp", M(R_SESSION), S(ORIG_HOMUN_MAX_SP), SF(homunMaxSp), 4 },
    { "homunIntimacy", M(R_SESSION), S(ORIG_HOMUN_INTIMACY), SF(homunIntimacy), 4 },
    { "homunItem", M(R_SESSION), S(ORIG_HOMUN_ITEM), SF(homunItem), 4 },
    { "homunExp", M(R_SESSION), S(ORIG_HOMUN_EXP), SF(homunExp), 8 },
    { "homunMaxExp", M(R_SESSION), S(ORIG_HOMUN_MAX_EXP), SF(homunMaxExp), 8 },
    { "homunFullness", M(R_SESSION), S(ORIG_HOMUN_FULLNESS), SF(homunFullness), 4 },
    { "homunRenamed", M(R_SESSION), S(ORIG_HOMUN_RENAMED), SF(homunRenamed), 4 },
    { "homun_ai_state", M(R_SESSION), S(ORIG_HOMUN_AI_STATE), SF(homun_ai_state), 12 },
    { "homunSkillPoints", M(R_SESSION), S(ORIG_HOMUN_SKILL_POINTS), SF(homunSkillPoints), 4 },
    { "mercAid", M(R_SESSION), S(ORIG_MERC_AID), SF(mercAid), 4 },
    { "mercName", M(R_SESSION), S(ORIG_MERC_NAME), SF(mercName), 32 },
    { "mercStats", M(R_SESSION), S(ORIG_MERC_STATS), SF(mercStats), 32 },
    { "mercHp", M(R_SESSION), S(ORIG_MERC_HP), SF(mercHp), 4 },
    { "mercMaxHp", M(R_SESSION), S(ORIG_MERC_MAX_HP), SF(mercMaxHp), 4 },
    { "mercJob", M(R_SESSION), S(ORIG_MERC_JOB), SF(mercJob), 4 },
    { "mercLevel", M(R_SESSION), S(ORIG_MERC_LEVEL), SF(mercLevel), 4 },
    { "mercSp", M(R_SESSION), S(ORIG_MERC_SP), SF(mercSp), 4 },
    { "mercMaxSp", M(R_SESSION), S(ORIG_MERC_MAX_SP), SF(mercMaxSp), 4 },
    { "merc_contract_stats", M(R_SESSION), S(ORIG_MERC_CONTRACT), SF(merc_contract_stats), 20 },
    { "elementalAid", M(R_SESSION), S(ORIG_ELEMENTAL_AID), SF(elementalAid), 4 },
    { "elementalHp", M(R_SESSION), S(ORIG_ELEMENTAL_HP), SF(elementalHp), 4 },
    { "elementalSp", M(R_SESSION), S(ORIG_ELEMENTAL_SP), SF(elementalSp), 4 },
    { "elementalMaxHp", M(R_SESSION), S(ORIG_ELEMENTAL_MAX_HP), SF(elementalMaxHp), 4 },
    { "elementalMaxSp", M(R_SESSION), S(ORIG_ELEMENTAL_MAX_SP), SF(elementalMaxSp), 4 },
    { "ownAid", M(R_SESSION), S(ORIG_OWN_AID), SF(ownAid), 4 },
    { "partyLeader", M(R_SESSION), S(ORIG_PARTY_LEADER), SF(partyLeader), 4 },
    { "partyExpOption", M(R_SESSION), S(ORIG_PARTY_EXP_OPTION), SF(partyExpOption), 4 },
    { "partyItemPickupRule", M(R_SESSION), S(ORIG_PARTY_PICKUP_RULE), SF(partyItemPickupRule), 4 },
    { "partyItemDivisionRule", M(R_SESSION), S(ORIG_PARTY_DIVISION_RULE), SF(partyItemDivisionRule), 4 },
    { "partyInviteRefused", M(R_SESSION), S(ORIG_PARTY_INVITE_REFUSED), SF(partyInviteRefused), 1 },
    { "booking_alarm_filtered", M(R_SESSION), S(ORIG_BOOKING_ALARM_FILTERED), SF(partyBooking.alarm_filtered), 1 },
    { "booking_queue_new", M(R_SESSION), S(ORIG_BOOKING_QUEUE_NEW), SF(partyBooking.queue_new), 1 },
    { "booking_answer_tick", M(R_SESSION), S(ORIG_BOOKING_ANSWER_TICK), SF(partyBooking.answer_tick), 4, 1 },
    { "item_move_ready", M(R_MODE), ORIG_MODE_ITEM_MOVE_READY, GF(item_move_ready), 1 },
    { "partyName", M(R_SESSION), S(ORIG_PARTY_NAME), SF(partyName), 12, 1 },
    { "partyName_", M(R_SESSION), S(ORIG_PARTY_NAME + 12), SF(partyName), 12, 1 },
    { "chat_room_pending", M(R_MODE), ORIG_MODE_CHAT_PENDING, GF(chat_room_pending), 4 },
    { "exchange_add_pending", M(R_MODE), ORIG_MODE_EXCHANGE_PENDING, GF(exchange_add_pending), 4 },
    { "exchange_partner", M(R_MODE), ORIG_MODE_EXCHANGE_PARTNER, GF(exchange_partner), 12, 1 },
    { "exchange_partner_", M(R_MODE), ORIG_MODE_EXCHANGE_PARTNER + 12, GF(exchange_partner), 12, 1 },
    { "family_request_aid", M(R_MODE), ORIG_MODE_FAMILY_AID, GF(family_request_aid), 4 },
    { "family_request_char_id", M(R_MODE), ORIG_MODE_FAMILY_CHAR_ID, GF(family_request_char_id), 4 },
    { "couple_name", M(R_MODE), ORIG_MODE_COUPLE_NAME, GF(couple_name), 24 },
    { "own_room_users", M(R_MODE), ORIG_MODE_OWN_ROOM + 0x34, GF(own_chat_room.users), 4 },
    { "lastPacketTime", M(R_SESSION), S(ORIG_LAST_PACKET_TIME), SF(lastPacketTime), 4, 1 },
};

static const struct { unsigned mask; unsigned orig_off, lift_off; const char *name; } strings[] = {
    { M(R_EFFECT), 0x130, EF(texture), "texture" },
};

static const char *who_packets(const void *p)
{
    if (p == (const char *)oracle_addr(ORIG_INSTANT_VARS) - ORIG_ACTOR_COUNTERS) return "V";
    if (p == orig_new_actor || p == &lift_new_actor) return "N";
    return NULL;
}

static const char *hex_bytes(const unsigned char *p, unsigned size)
{
    static char text[2][64];
    static int turn;
    char *out = text[turn ^= 1];
    unsigned i;
    out[0] = 0;
    for (i = size; i-- > 0;)
        snprintf(out + strlen(out), 64 - strlen(out), "%02x", p[i]);
    return out;
}

static const char *orig_std_string(const unsigned char *s)
{
    unsigned cap = *(const unsigned *)(s + 20);
    return cap >= ORIG_STD_STRING_SSO ? (const char *)*(const char *const *)s : (const char *)s;
}

static void snapshot_regions(int side)
{
    int r;
    current_side = side;
    for (r = 0; r < R_COUNT; r++) {
        struct Region *g = &regions[r];
        if (side == 0) {
            memcpy(g->orig_snap, g->orig, g->orig_size);
            memset(g->orig_seen, 0, g->orig_size);
        } else {
            memcpy(g->lift_snap, g->lift, g->lift_size);
            memset(g->lift_seen, 0, g->lift_size);
        }
    }
}

static void report_changes(int side)
{
    unsigned i;
    int r;
    for (i = 0; i < sizeof fields / sizeof *fields; i++) {
        const struct Field *f = &fields[i];
        for (r = 0; r < R_COUNT; r++) {
            struct Region *g = &regions[r];
            unsigned off = side == 0 ? f->orig_off : f->lift_off;
            unsigned char *now = (side == 0 ? g->orig : g->lift) + off;
            unsigned char *was = (side == 0 ? g->orig_snap : g->lift_snap) + off;
            unsigned char *seen = (side == 0 ? g->orig_seen : g->lift_seen) + off;
            if (!(f->mask & M(r)))
                continue;
            memset(seen, 1, f->size);
            if (f->quiet)
                continue;
            if (memcmp(now, was, f->size) != 0)
                event("set %s.%s %s->%s", g->name, f->name, hex_bytes(was, f->size), hex_bytes(now, f->size));
        }
    }
    for (i = 0; i < sizeof strings / sizeof *strings; i++) {
        for (r = 0; r < R_COUNT; r++) {
            struct Region *g = &regions[r];
            unsigned off = side == 0 ? strings[i].orig_off : strings[i].lift_off;
            unsigned size = side == 0 ? ORIG_STD_STRING : sizeof(struct String);
            unsigned char *now = (side == 0 ? g->orig : g->lift) + off;
            unsigned char *was = (side == 0 ? g->orig_snap : g->lift_snap) + off;
            if (!(strings[i].mask & M(r)))
                continue;
            memset((side == 0 ? g->orig_seen : g->lift_seen) + off, 1, size);
            if (memcmp(now, was, size) != 0)
                event("set %s.%s '%s'", g->name, strings[i].name,
                      side == 0 ? orig_std_string(now) : string_cstr((const struct String *)now));
        }
    }
    for (r = 0; r < R_COUNT; r++) {
        struct Region *g = &regions[r];
        unsigned size = side == 0 ? g->orig_size : g->lift_size;
        unsigned char *now = side == 0 ? g->orig : g->lift, *was = side == 0 ? g->orig_snap : g->lift_snap;
        unsigned char *seen = side == 0 ? g->orig_seen : g->lift_seen;
        unsigned off;
        for (off = 0; off + 4 <= size; off += 4) {
            unsigned k, unseen_change = 0;
            for (k = 0; k < 4; k++)
                unseen_change |= !seen[off + k] && now[off + k] != was[off + k];
            if (unseen_change)
                event("set %s+0x%x ? %s->%s", g->name, off, hex_bytes(was + off, 4), hex_bytes(now + off, 4));
        }
    }
}

void name_card_holder_erase(struct ActorNameCardHolder *self, unsigned aid) { (void)self; event("names erase %u", aid); }
void name_card_holder_set_all(struct ActorNameCardHolder *self, unsigned aid, struct String name, struct String party,
                              struct String guild, struct String position, struct String title, struct String npc_title, int flag)
{
    (void)self;
    event("names set %u '%s' '%s' '%s' '%s' '%s' '%s' %d", aid, string_cstr(&name), string_cstr(&party), string_cstr(&guild),
          string_cstr(&position), string_cstr(&title), string_cstr(&npc_title), flag);
}
void session_erase_abr(struct Session *self, long long key) { (void)self; event("abr erase %lld", key); }
char world_remove_actor_by_aid(struct World *self, unsigned aid)
{ (void)self; event("remove %u", aid); return aid == AID_CASTER || aid == AID_TARGET; }
unsigned char job_is_monster(int job) { return job >= 1001 && job <= 3999; }
int session_is_homun_or_merc_job(struct Session *self, int id) { (void)self; return id >= 6001 && id <= 6052; }
int session_is_homunculus_job(struct Session *self, int id) { (void)self; return id >= 6001 && id <= 6016; }
unsigned long long game_mode_launch_dummy_effect(struct GameMode *mode, const Vec3 *at, int effect_id, int frames, int value)
{ (void)mode; event("dummy %x %d %d at %.0f %.0f %.0f", effect_id, frames, value, at->x, at->y, at->z); return 0; }
int session_pet_hunger_level(int fullness) { return fullness / 20; }
int session_pet_intimacy_level(int intimacy) { return intimacy / 250; }
int session_get_pet_emotion(struct Session *self, int hungerLevel, int intimacyLevel, int action)
{ (void)self; return hungerLevel * 100 + intimacyLevel * 10 + action; }
int session_make_pet_talk_id(int job, int action, int hunger) { return job * 1000 + action * 10 + hunger; }
void session_notify_messenger_group_and_plates(struct Session *self) { (void)self; event("notify plates"); }
int ui_window_mgr_message_box(struct UIWindowMgr *self, const char *text, int type, int arg3, int arg4, int arg5, const char *title,
                              int width, int height, int arg9)
{ (void)self; (void)title; event("msgbox '%s' %d %d %d %d %d %d %d", text, type, arg3, arg4, arg5, width, height, arg9); return 0; }
struct UIWindow *ui_window_open(int id)
{
    event("open %x", id);
    return id == 0x21 ? &lift_storage_wnd : id == 0x1c ? (struct UIWindow *)&lift_chat_room_wnd : &lift_generic_wnd;
}
unsigned ui_merchant_shop_seller(const struct UIWindow *wnd) { (void)wnd; return 0; }
void session_clear_purchase_lists(struct Session *self) { (void)self; event("clear purchase"); }
void session_set_job(struct Session *self, int job) { (void)self; event("session job %d", job); }
int session_get_weapon_view_id(int id) { return id % 3 == 0 ? -1 : id + 1000; }
void game_actor_set_job(struct GameActor *self, int job) { event("setjob %s %d", who(self), job); }
void *session_get_actor_by_aid(struct Session *self, DWORD aid) { (void)self; return world_get_game_actor_by_aid(NULL, aid); }
void render_object_set_argb(struct RenderObject *self, int a, int r, int g, int b) { event("argb %s %d %d %d %d", who(self), a, r, g, b); }
void *tree_get(struct Tree *tree, const void *key, size_t key_size, size_t size, TreeLessFn less)
{
    int slot = abr_slot(tree);
    (void)key_size; (void)size; (void)less;
    if (slot >= 0) {
        event("abr get %d %lld", slot, *(const long long *)key);
        fake_abr_used[slot] = 1;
        lift_abr_pair[slot].value = 0;
        return &lift_abr_pair[slot];
    }
    if (tree == &lift_mode.chat_room_owners)
        return &lift_room_owners[*(const unsigned *)key % 4];
    if (tree == &g_session1.statusValues) {
        adv_key = *(const int *)key;
        adv_pending = 1;
        lift_adv_pair.value = 0;
        event("advstat get %d", adv_key);
        return &lift_adv_pair;
    }
    event("instvar get %d", *(const int *)key);
    lift_instant_var_got = 1;
    return &lift_instant_var;
}
int tree_less_int(const void *a, const void *b) { return *(const int *)a < *(const int *)b; }
struct RagConnection *rag_connection_instance(void) { return (struct RagConnection *)lift_connection; }
struct Connection *rag_connection_transport(struct RagConnection *connection) { return (struct Connection *)connection; }
void connection_clear_held(struct Connection *connection) { (void)connection; event("clear held"); }
void session_sync_server_time(struct Session *self, DWORD serverTime) { (void)self; event("sync %u", serverTime); }
struct String *session_get_title_string(struct Session *self, struct String *out, int id)
{ (void)self; string_init(out); event("title %d", id); return out; }
void session_set_combo_delay(struct Session *self, int key, int value) { (void)self; event("combo %d %d", key, value); }
void render_object_set_force_anim_end_tick(struct RenderObject *self, int ms) { event("animend %s %d", who(self), ms); }
void session_set_arrow(struct Session *self, int index) { (void)self; event("arrow %d", index); }
void emitter_effect_set_count(struct EmitterEffect *self, int count) { event("emitter count %s %d", who(self), count); }
void emitter_effect_set_particle_speed(struct EmitterEffect *self, float speed) { event("emitter speed %s %.2f", who(self), speed); }
void game_actor_stop_aura_effects(struct GameActor *self) { event("stop aura %s", who(self)); }
void game_actor_show_level_aura(struct GameActor *self) { event("level aura %s", who(self)); }
void session_reroll_values(struct Session *self) { (void)self; event("reroll"); }
void session_refresh_basic_info(struct Session *self) { (void)self; event("refresh basic"); }
void effect_mgr_remove_type(struct EffectMgr *self, int aid, int type) { (void)self; event("remove type %x aid %d", type, aid); }
int client_lua_call(struct ClientLua *lua, const char *function, const char *format, ...) { (void)lua; (void)format; event("lua %s", function); return 0; }
void warning_plane_effect_add_plane(struct Effect *effect, Vec3 pos) { event("plane %s %.0f %.0f %.0f", who(effect), pos.x, pos.y, pos.z); }
void warning_plane_effect_set_end_time(struct Effect *effect, DWORD end_time) { event("plane end %s %u", who(effect), end_time); }
int game_actor_is_pc_job(int job) { return job < 1000; }
void string_assign(struct String *s, const char *text, size_t len)
{
    char *copy = malloc(len + 1);
    memcpy(copy, text, len);
    copy[len] = 0;
    s->data = copy;
    s->size = len;
    s->capacity = len;
}
void string_assign_cstr(struct String *s, const char *text) { string_assign(s, text, strlen(text)); }
void string_copy(struct String *dst, const struct String *src) { string_init(dst); string_assign(dst, string_cstr(src), src->size); }
static const char *room_text(const void *p)
{
    static char text[256];
    if (current_side == 0) {
        const unsigned char *r = p;
        snprintf(text, sizeof text, "room '%s' pw '%s' t%d u%d m%d id%d", orig_std_string(r), orig_std_string(r + 0x18), *(const int *)(r + 0x30),
                 *(const int *)(r + 0x34), *(const int *)(r + 0x38), *(const int *)(r + 0x3c));
    } else {
        const struct ChatRoomInfo *r = p;
        snprintf(text, sizeof text, "room '%s' pw '%s' t%d u%d m%d id%d", string_cstr(&r->title), string_cstr(&r->password), r->type, r->users,
                 r->max_users, r->room_id);
    }
    return text;
}
int ui_window_send(struct UIWindow *window, struct UIWindow *sender, int message, int a, int b, int c, int d)
{
    (void)sender;
    if (message == 0x22 && readable(a))
        event("wndsend %s %x %s", window_name(window), message, room_text((const void *)(int)a));
    else if ((message == 0x38 || message == 0x5a || message == 0x5b) && readable(b))
        event("wndsend %s %x %x '%s' %x %x", window_name(window), message, a, (const char *)(int)b, c, d);
    else if (message == 0x5f && readable(c))
        event("wndsend %s %x %x %x '%s' %x", window_name(window), message, a, b, (const char *)(int)c, d);
    else if ((message == 0x1a || message == 0x1b || message == 0x1f || message == 0x2c || message == 0x2d || message == 0x2e || message == 0x32) && readable(a))
        event("wndsend %s %x '%s' %x %x %x", window_name(window), message, (const char *)(int)a, b, c, d);
    else
        event("wndsend %s %x %x %x %x %x", window_name(window), message, a, b, c, d);
    return 0;
}
static const char *dialog_text(unsigned value)
{
    static const char *const texts[] = { "", "Hello", "yes:no", "one:two:three", "::", "a:", "Welcome, traveler.:Buy:Sell:Cancel", "prontera" };
    if (value < 8)
        return texts[value];
    return value == 8 ? long_text_600 : long_text_max;
}
static const char *record_text(int index, int price, int charged, int type, int identified, const char *id, int location, int view)
{
    static char text[128];
    snprintf(text, sizeof text, "rec i%d p%d cp%d t%d idf%d id'%s' eq%x v%d", index, price, charged, type, identified, id, location, view);
    return text;
}
static const char *lift_record_text(const struct ItemRecordA *r)
{ return record_text(r->index, r->price, r->charged_price, r->type, r->identified, r->id_text, r->equip_location, r->view); }
static const char *orig_record_text(const unsigned char *r)
{
    return record_text(*(const int *)r, *(const int *)(r + 4), *(const int *)(r + 8), *(const int *)(r + 0xc), r[0x10], (const char *)r + 0x11,
                       *(const int *)(r + 0x34), *(const int *)(r + 0x38));
}
static void lift_set_job_model(struct GameActor *actor, int job, int sex) { event("jobmodel %s %d %d", who(actor), job, sex); }
void game_mode_log_whisper(struct GameMode *self, const char *line) { (void)self; event("log '%s'", line); }
void session_clear_lists(struct Session *self) { (void)self; event("clear lists"); }
const struct ItemTableEntry *item_table_get(int item_id) { lift_item_entry.class_num = item_id % 7; return &lift_item_entry; }
void session_append_market_goods(struct Session *self, const struct ItemRecordA *packetItem, int count, WORD weight)
{ (void)self; event("market %s %d %d", lift_record_text(packetItem), count, weight); }
void *object_as_type(void *object, enum ObjectType type) { (void)type; return object; }
struct UIParaResultWnd;
void ui_para_result_wnd_add_bought_record(struct UIParaResultWnd *self, const struct ItemRecordA *record, int count)
{ (void)self; event("bought %s %d", lift_record_text(record), count); }
struct ReplayRecorder *replay_recorder_instance(void) { return &lift_replay; }
void ui_pet_info_update_intimacy(struct UIWindow *wnd) { event("pet intimacy %s", window_name(wnd)); }
void ui_pet_info_set_name(struct UIWindow *wnd, const char *name) { event("pet name %s '%s'", window_name(wnd), name); }
void ui_item_collection_set_pet(struct UIWindow *wnd, const struct PetEggDetails *pet)
{
    (void)wnd;
    lift_pet_details.level = pet->level;
    lift_pet_details.hunger = pet->hunger;
    lift_pet_details.intimacy = pet->intimacy;
    lift_pet_details.accessory = pet->accessory;
    lift_pet_details.renamed = pet->renamed;
    strncpy(lift_pet_details.name, pet->name, sizeof lift_pet_details.name - 1);
}
void session_get_pet_speak(struct Session *self, int petJob, int hunger, int action, char *out, size_t out_size)
{
    (void)self;
    event("speak %d %d %d", petJob, hunger, action);
    if (hunger == 0)
        out[0] = 0;
    else
        snprintf(out, out_size, "pet %d says %d", petJob, action);
}
void session_format_chat_line(struct Session *self, const char *text, char *out, size_t out_size, int aid, int replaceWords)
{
    (void)self;
    event("fmt '%s' %u %d", text, aid, replaceWords);
    if (replaceWords < 0)
        out[0] = 0;
    else
        snprintf(out, out_size, "%u: %s", aid, text);
}
void session_reset_pet(struct Session *self) { self->hatchedEggIndex = -1; self->petJob = -1; }
void session_invalidate_homun_info_wnd(struct Session *self) { (void)self; event("homun redraw"); }
void session_invalidate_mer_info_wnd(struct Session *self) { (void)self; event("merc redraw"); }
void list_clear(struct List *list, ListDestroyFn destroy) { (void)destroy; event("list clear %s", list == &g_session1.merSkills ? "merSkills" : "?"); }
void skill_info_destroy(void *skill) { (void)skill; }
void game_actor_set_pk_mode(struct GameActor *self, int value) { event("pkmode %s %d", who(self), value); }
struct ItemInfo *session_get_inventory_item_by_id(struct Session *self, struct ItemInfo *out, int itemId, BYTE requireIdentified)
{
    (void)self;
    event("inv by id %d %d", itemId, requireIdentified);
    item_info_construct(out);
    out->count = itemId % 5;
    return out;
}
const char *item_info_get_name(const struct ItemInfo *self) { (void)self; return "item"; }
struct String *item_info_get_display_name(const struct ItemInfo *self, struct String *out) { (void)self; string_assign_cstr(out, "item"); return out; }
struct ItemInfo *ui_item_identify_wnd_get_selected_item(struct UIItemIdentifyWnd *self, struct ItemInfo *out, int *found)
{
    event("selected %s", window_name(self));
    item_info_construct(out);
    out->index = 9;
    item_info_set_item_id(out, 9009);
    *found = current_fill == FILL_ONES;
    return out;
}
static void lift_window_invalidate(struct UIWindow *self) { event("invalidate %s", window_name(self)); }
static void lift_set_spr_job(struct GameActor *actor, int job) { event("sprjob %s %d", who(actor), job); }
static const char *post_text(const unsigned char *p)
{
    static char text[128];
    const short *jobs = (const short *)(p + 0x24);
    snprintf(text, sizeof text, "post %d '%.24s' %d %d %d j%d,%d,%d,%d,%d,%d", *(const int *)p, (const char *)p + 4, *(const int *)(p + 0x1c),
             *(const short *)(p + 0x20), *(const short *)(p + 0x22), jobs[0], jobs[1], jobs[2], jobs[3], jobs[4], jobs[5]);
    return text;
}
static const char *member_text(unsigned found, unsigned aid, unsigned char_id, const char *name, const char *map, unsigned role, unsigned state,
                               unsigned job, unsigned level, unsigned dead)
{
    static char text[160];
    snprintf(text, sizeof text, "member %u %u %u '%s' '%s' r%u s%u j%u l%u d%u", found, aid, char_id, name, map, role, state, job, level, dead);
    return text;
}
static void censor_text(char *text)
{
    char *bad = strstr(text, "bad");
    if (bad)
        memcpy(bad, "***", 3);
}
int party_booking_decode_search(struct PartyBookingSearch *out, const unsigned char *packet, size_t size)
{
    if (!packet || size < 5)
        return 0;
    if (*(const unsigned short *)(packet + 2) != size || (size - 5) % ORIG_POST_SIZE)
        return 0;
    out->posts = packet + 5;
    out->count = (size - 5) / ORIG_POST_SIZE;
    out->more = (char)packet[4];
    return 1;
}
void party_booking_add_search(struct PartyBooking *self, const struct PartyBookingSearch *search)
{
    size_t i;
    (void)self;
    event("booking add %u %d", (unsigned)search->count, search->more);
    for (i = 0; i < search->count; i++)
        event("%s", post_text(search->posts + i * ORIG_POST_SIZE));
}
void party_booking_remove(struct PartyBooking *self, int id) { (void)self; event("booking remove %d", id); }
void party_booking_on_new(struct PartyBooking *self, struct PartyBookingPost post) { (void)self; event("booking new %s", post_text((const unsigned char *)&post)); }
void party_booking_set_jobs(struct PartyBooking *self, int id, const short *jobs)
{ (void)self; event("booking jobs %d %d %d %d %d %d %d", id, jobs[0], jobs[1], jobs[2], jobs[3], jobs[4], jobs[5]); }
void ui_window_mgr_command_plate_of(struct UIWindowMgr *self, int aid, int a, int b) { (void)self; event("plate %u %d %d", aid, a, b); }
int session_is_own_unit_aid(struct Session *self, int aid) { (void)self; return aid == (int)AID_OWN; }
void adventurer_agency_stop_recruiting(struct AdventurerAgencyMgr *self, int aid) { (void)self; event("stop recruiting %u", aid); }
void ui_request_join_party_mgr_add_request(struct UIRequestJoinPartyMgr *self, int char_id, int aid, const char *name, int level, int job)
{ (void)self; event("join request %d %u '%s' %d %d", char_id, aid, name, level, job); }
const short *ui_seek_party_mb_jobs(const struct UIWindow *wnd) { (void)wnd; return wanted_jobs; }
void job_list_set_wanted_jobs(const short *jobs) { (void)jobs; event("wanted jobs"); }
int word_filter_contains(const struct WordFilter *filter, const char *text) { (void)filter; return strstr(text, "bad") != NULL; }
void word_filter_censor(const struct WordFilter *filter, char *text) { (void)filter; censor_text(text); event("censor '%s'", text); }
void game_mode_set_actor_mark_flag(struct GameMode *self, int aid, int flag)
{
    struct GameActor *actor = world_get_game_actor_by_aid(self->world, aid);
    if (actor)
        game_actor_set_mark_listed(actor, flag);
}
int session_get_party_member_count(struct Session *self) { (void)self; return 0; }
void session_add_party_member(struct Session *self, struct PartyMember *m)
{
    (void)self;
    event("add %s", member_text(m->found, m->aid, m->char_id, string_cstr(&m->name), string_cstr(&m->map_name), m->role, m->state, m->job, m->level, m->dead));
}
void session_clear_inventory_move_pending(struct Session *self, int index) { (void)self; event("clear inv pending %d", index); }
void session_clear_cart_move_pending(struct Session *self, int index) { (void)self; event("clear cart pending %d", index); }
void session_clear_storage_move_pending(struct Session *self, int index) { (void)self; event("clear storage pending %d", index); }
void string_resize(struct String *s, size_t size, char fill)
{
    char *copy = malloc(size + 1);
    size_t keep = s->size < size ? s->size : size;
    if (keep)
        memcpy(copy, string_cstr(s), keep);
    memset(copy + keep, fill, size - keep);
    copy[size] = 0;
    s->data = copy;
    s->size = size;
    s->capacity = size;
}
int string_equal(const struct String *a, const struct String *b) { return a->size == b->size && memcmp(string_cstr(a), string_cstr(b), a->size) == 0; }
struct PartyMember *session_get_party_member(struct Session *self, struct PartyMember *out, DWORD aid)
{
    (void)self;
    event("member get %u", aid);
    memset(out, 0, sizeof *out);
    string_init(&out->name);
    string_init(&out->map_name);
    out->found = aid == AID_TARGET;
    return out;
}
void session_set_party_member_state(struct Session *self, DWORD aid, DWORD role) { (void)self; event("member state %u %u", aid, role); }
struct ChatRoomInfo *chat_room_info_copy(struct ChatRoomInfo *room, const struct ChatRoomInfo *from)
{
    string_copy(&room->title, &from->title);
    string_copy(&room->password, &from->password);
    room->type = from->type;
    room->users = from->users;
    room->max_users = from->max_users;
    room->room_id = from->room_id;
    return room;
}
BYTE clientinfo_is_yellow_aid(int aid) { return aid == (int)AID_TARGET; }
void vec_resize(struct Vec *v, size_t count, size_t size)
{
    char *p = malloc(count * size + 1);
    memset(p, 0, count * size + 1);
    v->first = p;
    v->last = p + count * size;
    v->end = v->last;
}
void session_end_exchange(struct Session *self) { (void)self; event("end exchange 1"); }
int session_remove_friend(struct Session *self, DWORD value) { (void)self; event("remove friend %u", value); return value & 1; }
void session_notify_messenger_group_wnd(struct Session *self) { (void)self; event("notify group wnd"); }
void session_set_their_trade_zeny(struct Session *self, int money) { (void)self; event("their zeny %d", money); }
void session_add_their_trade_item(struct Session *self, struct ItemInfo item) { (void)self; event("their item %s", item_text(&item)); }
int mode_send_message(struct Mode *mode, int message, int a, int b, int c, int d)
{ (void)mode; event("modemsg %x %x %s %x %x", message, a, who((void *)b), c, d); return 0; }
struct Mode *client_current_mode(void) { return (struct Mode *)&lift_mode; }
BYTE replay_recorder_set_mode(struct ReplayRecorder *self, int mode) { (void)self; event("replay mode %d", mode); return 1; }
void text_drop_map_extension(struct String *map_name)
{
    if (map_name->size >= 4) {
        map_name->size -= 4;
        map_name->data[map_name->size] = 0;
    }
}
struct ModeMgr g_modeMgr;
void mode_mgr_switch(struct ModeMgr *self, int type, const char *name) { (void)self; event("switch %d '%s'", type, name); }
void ui_wait_wnd_set_text(struct UIWindow *wnd, const char *text) { event("wait text %s '%s'", window_name(wnd), text); }
void session_reset_store_search(struct Session *self) { (void)self; event("reset store search"); }
int game_mode_reset_for_warp(struct GameMode *mode) { (void)mode; event("reset warp"); return 1; }
void game_mode_initialize(struct GameMode *self) { (void)self; event("initialize"); }
void mode_run_fade_out(struct Mode *self, int logo) { (void)self; event("fade out %d", logo); }
size_t own_states_count(void) { return lift_own_count; }
const struct OwnStateEntry *own_states_at(size_t position) { return &lift_own_states[position]; }
void own_states_erase_at(size_t position)
{
    memmove(&lift_own_states[position], &lift_own_states[position + 1], (lift_own_count - position - 1) * sizeof *lift_own_states);
    lift_own_count--;
}
void session_reset_mail_items(struct Session *self) { (void)self; event("reset mail"); }
void session_clear_mail_item_counts(struct Session *self) { (void)self; event("clear mail counts"); }
void game_mode_move_to_server(struct GameMode *self, struct String map_name, int x, int y, unsigned long ip, int port, struct String server_map_name)
{ (void)self; event("move '%s' %d %d %x %d '%s'", string_cstr(&map_name), x, y, (unsigned)ip, port, string_cstr(&server_map_name)); }
struct SoundSettings g_sound_settings;
void sound_play_stream(const char *name, int loop_count) { event("stream '%s' %d", name, loop_count); }
void ui_say_dialog_wnd_set_text_align(struct UISayDialogWnd *self, unsigned char align) { (void)self; event("align %d", align); }
void *vec_push(struct Vec *v, const void *elem, size_t size)
{
    size_t count = vec_count(v, size), capacity = vec_capacity(v, size);
    if (count == capacity) {
        size_t grown = capacity ? capacity * 2 : 4;
        char *copy = malloc(grown * size);
        memcpy(copy, v->first, count * size);
        v->first = copy;
        v->last = copy + count * size;
        v->end = copy + grown * size;
    }
    memcpy(v->last, elem, size);
    v->last = (char *)v->last + size;
    return (char *)v->last - size;
}
void vec_erase(struct Vec *v, void *first, void *last)
{
    size_t tail = (size_t)((char *)v->last - (char *)last);
    memmove(first, last, tail);
    v->last = (char *)first + tail;
}
BYTE effect_state_bit6(DWORD state) { return (BYTE)((state >> 6) & 1); }
BYTE is_effect_state_hiding(DWORD state) { return (BYTE)((state >> 1) & 1); }
BYTE is_effect_state_cloaking(DWORD state) { return (BYTE)((state >> 2) & 1); }

int g_replayPlaying;
struct ChampionMonsterMgr *g_championMonster;

static int job_of(const void *actor)
{
    if (actor == orig_caster || actor == &lift_caster) return 46;
    if (actor == orig_target || actor == &lift_target) return 1002;
    if (actor == orig_new_actor || actor == &lift_new_actor) return sweep_job;
    return 0;
}
static int lift_job(const struct GameActor *actor) { return job_of(actor); }
static void *lift_alloc(size_t bytes) { return bytes == sizeof(struct Pc) ? (void *)&lift_new_actor : NULL; }
static const char *tree_name(const void *tree)
{
    if (tree == &lift_mode.square_marks || tree == orig_mode + ORIG_MODE_SQUARE_MARKS) return "sq";
    if (tree == &lift_mode.triangle_marks || tree == orig_mode + ORIG_MODE_TRIANGLE_MARKS) return "tri";
    return "?";
}
static const char *entry_text(long long msg, long long a)
{
    static char text[2][256];
    static int turn;
    char *out = text[turn ^= 1];
    int n;
    if (msg == ACTOR_MSG_ENTRY_OLD || msg == ACTOR_MSG_NEW_ENTRY_OLD || msg == ACTOR_MSG_NEW_ENTRY || msg == ACTOR_MSG_ENTRY) {
        const struct ActorEntryInfo *e = (const struct ActorEntryInfo *)(int)a;
        snprintf(out, 256, "%llx entry a%u j%d s%d at%d,%d,%d sp%d sz%d,%d h%d w%d,%d ac%d st%d bs%d hs%d es%x pk%d sh%d a2%d a3%d hp%d bp%d hd%d g%u ev%d ho%d v%d r%d c%d b%d",
                 msg, e->aid, e->job, e->sex, e->x, e->y, e->dir, e->speed, e->x_size, e->y_size, e->head, e->weapon[0], e->weapon[1],
                 e->accessory, e->state, e->body_state, e->health_state, e->effect_state, e->is_pk_mode, e->shield, e->accessory2,
                 e->accessory3, e->head_palette, e->body_palette, e->head_dir, e->guild_id, e->emblem_version, e->honor, e->virtue,
                 e->robe, e->char_id, e->body);
        return out;
    }
    if (msg == ACTOR_MSG_MOVE_ENTRY_OLD || msg == ACTOR_MSG_NEW_MOVE_ENTRY_OLD || msg == ACTOR_MSG_NEW_MOVE_ENTRY || msg == ACTOR_MSG_MOVE_ENTRY) {
        const struct ActorMoveEntryInfo *e = (const struct ActorMoveEntryInfo *)(int)a;
        n = snprintf(out, 256, "%llx move a%u j%d s%d t%u from%d,%d sub%d,%d to%d,%d sp%d h%d w%d,%d ac%d bs%d hs%d es%x pk%d a2%d a3%d hp%d bp%d hd%d g%u ev%d ho%d v%d",
                     msg, e->aid, e->job, e->sex, e->move_start_time, e->src_x, e->src_y, e->sub_x, e->sub_y, e->dst_x, e->dst_y, e->speed,
                     e->head, e->weapon[0], e->weapon[1], e->accessory, e->body_state, e->health_state, e->effect_state, e->is_pk_mode,
                     e->accessory2, e->accessory3, e->head_palette, e->body_palette, e->head_dir, e->guild_id,
                     e->emblem_version, e->honor, e->virtue);
        if (move_mask & 8) n += snprintf(out + n, 256 - n, " sh%d", e->shield);
        if (move_mask & 1) n += snprintf(out + n, 256 - n, " r%d", e->robe);
        if (move_mask & 2) n += snprintf(out + n, 256 - n, " c%u", e->char_id);
        if (move_mask & 4) n += snprintf(out + n, 256 - n, " b%d", e->body);
        return out;
    }
    return NULL;
}
static const char *msg_text_packets(long long msg, long long a, long long b, long long c, long long d)
{
    static char chat[300];
    (void)c; (void)d;
    if (msg == 7 && readable(a)) {
        snprintf(chat, sizeof chat, "7 '%s' %llx", (const char *)(int)a, b);
        return chat;
    }
    if (msg == 0x20 && readable(a)) {
        snprintf(chat, sizeof chat, "20 %s", room_text((const void *)(int)a));
        return chat;
    }
    return readable(a) ? entry_text(msg, a) : NULL;
}

int game_actor_get_guild_id(struct GameActor *actor) { return actor == &lift_target ? 77 : 0; }
void game_actor_set_guild(struct GameActor *actor, int value0, int value1) { event("guild %s %d %d", who(actor), value0, value1); }
void game_actor_set_mark_listed(struct GameActor *self, int value) { self->mark_listed = value; }
BYTE clientinfo_is_admin_aid(int aid) { return aid == AID_OWN; }
int session_is_siege_mode(struct Session *self) { return self->map_rule_flags[0]; }
int session_is_player_job(struct Session *self, int id) { (void)self; return id < 1000; }
int session_is_guardian(struct Session *self, int job) { (void)self; return job >= 1285 && job <= 1287; }
int session_is_own_aid(struct Session *self, int aid) { return aid != -1 && aid == self->aid; }
int session_is_homunculus_aid(struct Session *self, int aid) { (void)self; return aid == AID_UNKNOWN; }
int session_is_mercenary_aid(struct Session *self, int aid) { (void)self; return aid == AID_UNKNOWN; }
int session_is_admin_option(struct Session *self, int what) { return (what == 7 || what == 2 || what == 3) && self->map_rule_flags[0]; }
const char *session_character_name(const struct Session *session) { (void)session; return "Hero"; }
const char *session_get_pet_accessory_act(struct Session *self, int accessory)
{ static char name[32]; (void)self; snprintf(name, sizeof name, "pet%d.act", accessory); return name; }
void render_object_load_act_res(struct RenderObject *self, const char *name) { event("loadact %s %s", who(self), name); }
void game_mode_make_emblem_wnd(struct GameMode *self, struct GameActor *actor, int guild_id) { (void)self; event("emblem %s %d", who(actor), guild_id); }
struct Pc *pc_construct(struct Pc *self)
{ ((struct GameActor *)self)->ops = (const struct GameObjectOps *)&lift_actor_ops; event("pc construct %s", who(self)); return self; }
struct GameActor *game_mode_new_actor_for_job(struct GameMode *self, int job)
{
    (void)self;
    event("new actor %d", job);
    lift_new_actor.pc.ops = (const struct GameObjectOps *)&lift_actor_ops;
    return (struct GameActor *)&lift_new_actor;
}
void actor_list_push_back(struct List *list, void *actor) { list_push_back(list, &actor, sizeof actor); }
void *tree_find(const struct Tree *tree, const void *key, TreeLessFn less)
{ unsigned aid = *(const unsigned *)key; (void)less; event("marks %s %u", tree_name(tree), aid); return aid == AID_OWN ? &fake_mark : NULL; }
int tree_less_uint(const void *a, const void *b) { return *(const unsigned *)a < *(const unsigned *)b; }
void name_card_holder_set_name(struct ActorNameCardHolder *self, unsigned aid, struct String name)
{ (void)self; event("name %u '%s'", aid, string_cstr(&name)); }
int player_names_set(unsigned char_id, struct TextView name) { event("pname %u '%.*s'", char_id, (int)name.size, name.data); return 0; }
int champion_monster_apply_effects(struct ChampionMonsterMgr *manager, int job, struct GameActor *actor, int actor_id)
{ (void)manager; event("champion %d %s %d", job, who(actor), actor_id); return 0; }
void world_place_id_nowhere(struct World *self, int id, const float *pos) { (void)self; event("place nowhere %d %.0f %.0f", id, pos[0], pos[1]); }
void world_place_id_at_client_cell(struct World *self, int id, const int *cell) { (void)self; event("place cell %d %d %d", id, cell[0], cell[1]); }
void session_invalidate_windows(struct Session *self) { (void)self; event("invalidate"); }
BYTE session_is_any_super_novice_job(struct Session *self, int id) { (void)self; return (BYTE)is_super_novice(id); }
int session_get_drawn_job(struct Session *self) { (void)self; return drawn_job; }
void ui_basic_info_wnd_set_height(struct UIBasicInfoWnd *self, int height) { event("bi height %s %d", window_name(self), height); }
void ui_basic_info_wnd_set_base_exp(struct UIBasicInfoWnd *self, __int64 current, __int64 maximum) { (void)self; event("bar 1 %lld %lld", current, maximum); }
void ui_basic_info_wnd_set_job_exp(struct UIBasicInfoWnd *self, __int64 current, __int64 maximum) { (void)self; event("bar 2 %lld %lld", current, maximum); }
void game_mode_refresh_actor_effects(struct GameMode *self, char player_only) { (void)self; event("refresh effects %d", player_only); }
struct UIWindow *ui_window_find(int id) { event("find %x", id); return windows_present ? &lift_found_wnd : NULL; }
void session_refresh_skill_wnds(struct Session *self) { (void)self; event("refresh skills"); }
void ui_status_wnd_restore_frame(struct UIStatusWnd *self) { event("status restore %s", window_name(self)); }
void session_end_exchange2(struct Session *self) { (void)self; event("end exchange"); }
long long session_find_abr(struct Session *self, long long key) { (void)self; event("find abr %lld", key); return key == AID_UNKNOWN ? -1 : 0; }
BYTE session_pet_talk_due(struct Session *self, __int64 interval) { (void)self; event("talk due %lld", interval); return 1; }
int tree_less_int64(const void *a, const void *b) { return *(const long long *)a < *(const long long *)b; }
void string_append_cstr(struct String *s, const char *text)
{
    size_t len = strlen(text);
    char *copy = malloc(s->size + len + 1);
    if (s->size)
        memcpy(copy, s->data, s->size);
    memcpy(copy + s->size, text, len + 1);
    s->data = copy;
    s->size += len;
    s->capacity = s->size;
}
void string_format(struct String *s, const char *format, ...)
{
    char text[512];
    va_list ap;
    va_start(ap, format);
    vsnprintf(text, sizeof text, format, ap);
    va_end(ap);
    string_assign_cstr(s, text);
}
BYTE session_add_cart_item(struct Session *self, const struct ItemInfo *item) { (void)self; event("cart add %s", item_text(item)); return 1; }
BYTE session_add_storage_item(struct Session *self, const struct ItemInfo *item, BYTE notify) { (void)self; event("storage add %s %d", item_text(item), notify); return 1; }
void session_inventory_update(struct Session *self, const struct ItemInfo *item, BYTE notify) { (void)self; event("inv update %s %d", item_text(item), notify); }
BYTE session_add_inventory_item(struct Session *self, const struct ItemInfo *item, BYTE refresh) { (void)self; event("inv add %s %d", item_text(item), refresh); return 1; }
BYTE session_set_worn_item(struct Session *self, const struct ItemInfo *item) { (void)self; event("worn %s", item_text(item)); return 1; }
BYTE session_set_worn_costume(struct Session *self, const struct ItemInfo *item) { (void)self; event("costume %s", item_text(item)); return 1; }
void session_remove_inventory_filter_types(struct Session *self) { (void)self; event("remove filter types"); }
void session_remove_inventory_other_types(struct Session *self) { (void)self; event("remove other types"); }
void session_clear_worn_items(struct Session *self) { (void)self; event("clear worn"); }
void session_clear_worn_costumes(struct Session *self) { (void)self; event("clear costumes"); }
void session_clear_inventory(struct Session *self) { (void)self; event("clear inventory"); }
void session_clear_cart(struct Session *self) { (void)self; event("clear cart"); }
void session_refresh_party_ui(struct Session *self) { (void)self; event("refresh party"); }
void session_refresh_friend_ui(struct Session *self) { (void)self; event("refresh friend"); }
void ui_window_mgr_refresh_item_windows(struct UIWindowMgr *self) { (void)self; event("refresh item wnds"); }
int session_is_pet_egg_id(struct Session *self, int id) { (void)self; return is_pet_egg(id); }
BYTE session_is_costume_or_shadow_location(struct Session *self, int flag) { (void)self; return (BYTE)is_costume_location(flag); }
struct ItemInfo *session_get_inventory_item(struct Session *self, struct ItemInfo *out, int index)
{
    (void)self;
    event("inv get %d", index);
    item_info_construct(out);
    out->index = index & 1 ? index : 0;
    item_info_set_item_id(out, 9000 + index);
    return out;
}
char pet_evolution_is_auto_feeding(struct PetEvolutionMgr *self, int egg_id) { (void)self; return (char)(egg_id & 1); }
void session_send_pet_auto_feed(struct Session *self, BYTE on) { (void)self; event("autofeed %d", on); }
void ui_storage_set_title(struct UIWindow *wnd, const char *name) { event("storage title %s '%s'", window_name(wnd), name); }
void ui_storage_set_list_received(struct UIWindow *wnd) { event("storage received %s", window_name(wnd)); }

static int lift_get_effect_state(struct RenderObject *self) { (void)self; return 0; }
static void lift_set_head(struct GameActor *a, int v) { event("head %s %d", who(a), v); }
static void lift_set_weapon(struct GameActor *a, int64_t v) { event("weapon %s %lld", who(a), v); }
static void lift_set_weapon_look(struct GameActor *a, int64_t v) { event("weaponlook %s %lld", who(a), v); }
static void lift_set_accessory(struct GameActor *a, int v) { event("acc1 %s %d", who(a), v); }
static void lift_set_accessory2(struct GameActor *a, int v) { event("acc2 %s %d", who(a), v); }
static void lift_set_accessory3(struct GameActor *a, int v) { event("acc3 %s %d", who(a), v); }
static void lift_set_shield(struct GameActor *a, int v) { event("shield %s %d", who(a), v); }
static void lift_set_appearance(struct GameActor *a, int v) { event("appearance %s %d", who(a), v); }
static void lift_set_imf_name(struct GameActor *a) { event("imf %s", who(a)); }
static void lift_set_head_pal_name(struct GameActor *a) { event("headpalname %s", who(a)); }
static void lift_set_body_pal_name(struct GameActor *a) { event("bodypalname %s", who(a)); }
static void lift_set_body_palette(struct GameActor *a, int v) { event("bodypal %s %d", who(a), v); }
static void lift_set_head_palette(struct GameActor *a, int v) { event("headpal %s %d", who(a), v); }
static void lift_set_honor(struct GameActor *a, int v) { event("honor %s %d", who(a), v); }
static void lift_set_robe(struct GameActor *a, int v, unsigned char f) { event("robe %s %d %d", who(a), v, f); }
static void lift_set_body_style(struct GameActor *a, int v, unsigned char f) { event("bodystyle %s %d %d", who(a), v, f); }
static void lift_refresh_accessory3(struct GameActor *a) { event("refreshacc3 %s", who(a)); }

static void __fastcall o_names_erase(void *self, void *edx, unsigned aid) { (void)self; (void)edx; event("names erase %u", aid); }
struct OrigString { unsigned char raw[ORIG_STD_STRING]; };
static void __fastcall o_names_set_all(void *self, void *edx, unsigned aid, struct OrigString name, struct OrigString party,
                                       struct OrigString guild, struct OrigString position, struct OrigString title,
                                       struct OrigString npc_title, int flag)
{
    (void)self; (void)edx;
    event("names set %u '%s' '%s' '%s' '%s' '%s' '%s' %d", aid, orig_std_string(name.raw), orig_std_string(party.raw),
          orig_std_string(guild.raw), orig_std_string(position.raw), orig_std_string(title.raw), orig_std_string(npc_title.raw), flag);
}
static void __fastcall o_erase_abr(void *self, void *edx, unsigned lo, int hi) { (void)self; (void)edx; event("abr erase %lld", ((long long)hi << 32) | lo); }
static int __fastcall o_world_remove(void *self, void *edx, unsigned aid) { (void)self; (void)edx; return world_remove_actor_by_aid(NULL, aid); }
static int __fastcall o_job_is_monster(void *self, void *edx, int job) { (void)self; (void)edx; return job_is_monster(job); }
static int __fastcall o_homun_or_merc(void *self, void *edx, int job) { (void)self; (void)edx; return session_is_homun_or_merc_job(NULL, job); }
static int __fastcall o_is_homunculus(void *self, void *edx, int job) { (void)self; (void)edx; return session_is_homunculus_job(NULL, job); }
static void __fastcall o_launch_dummy(void *self, void *edx, const float *at, int effect, int frames, int value)
{ (void)self; (void)edx; event("dummy %x %d %d at %.0f %.0f %.0f", effect, frames, value, at[0], at[1], at[2]); }
static int __fastcall o_pet_hunger(void *self, void *edx, int fullness) { (void)self; (void)edx; return session_pet_hunger_level(fullness); }
static int __fastcall o_pet_intimacy(void *self, void *edx, int intimacy) { (void)self; (void)edx; return session_pet_intimacy_level(intimacy); }
static int __fastcall o_pet_emotion(void *self, void *edx, int hunger, int intimacy, int action) { (void)self; (void)edx; return session_get_pet_emotion(NULL, hunger, intimacy, action); }
static int __fastcall o_pet_talk(void *self, void *edx, int job, int action, int hunger) { (void)self; (void)edx; return session_make_pet_talk_id(job, action, hunger); }
static void __fastcall o_notify_plates(void *self, void *edx) { (void)self; (void)edx; event("notify plates"); }
static int __fastcall o_message_box(void *self, void *edx, const char *text, int type, int arg3, int arg4, int arg5, const char *title,
                                    int width, int height, int arg9)
{ (void)edx; return ui_window_mgr_message_box((struct UIWindowMgr *)self, text, type, arg3, arg4, arg5, title, width, height, arg9); }
static int __fastcall o_post_quit(void *self, void *edx, int id) { (void)self; (void)edx; event("quit1 %x", id); return 1; }
static void *__fastcall o_make_window(void *self, void *edx, int id) { (void)self; (void)edx; event("open %x", id); return id == 0x21 ? orig_storage_wnd : orig_generic_wnd; }
static void __fastcall o_clear_purchase(void *self, void *edx) { (void)self; (void)edx; event("clear purchase"); }
static void __fastcall o_actor_set_job(void *self, void *edx, int job) { (void)edx; event("setjob %s %d", who(self), job); }
static void __fastcall o_session_set_job(void *self, void *edx, int job) { (void)self; (void)edx; event("session job %d", job); }
static int __fastcall o_weapon_view(void *self, void *edx, int id) { (void)self; (void)edx; return session_get_weapon_view_id(id); }
static void __fastcall o_set_combo_delay(void *self, void *edx, int key, int value) { (void)self; (void)edx; event("combo %d %d", key, value); }
static void __fastcall o_force_anim_end(void *self, void *edx, int ms) { (void)edx; event("animend %s %d", who(self), ms); }
static void *__fastcall o_session_actor(void *self, void *edx, unsigned aid) { (void)self; (void)edx; return o_actor_by_aid(NULL, NULL, aid); }
static void __fastcall o_set_argb(void *self, void *edx, int a, int r, int g, int b) { (void)edx; event("argb %s %d %d %d %d", who(self), a, r, g, b); }
static void *__cdecl o_conn_instance(void) { return (void *)FAKE_CONNECTION; }
static void __fastcall o_clear_held(void *self, void *edx) { (void)self; (void)edx; event("clear held"); }
static unsigned *__fastcall o_title_string(void *self, void *edx, unsigned *out, int id)
{ (void)self; (void)edx; memset(out, 0, ORIG_STD_STRING); out[5] = ORIG_STD_STRING_SSO - 1; event("title %d", id); return out; }
static void __fastcall o_emitter_speed(void *self, void *edx, float speed) { (void)edx; event("emitter speed %s %.2f", who(self), speed); }
static void __fastcall o_emitter_count(void *self, void *edx, int count) { (void)edx; event("emitter count %s %d", who(self), count); }
static void __fastcall o_stop_aura(void *self, void *edx) { (void)edx; event("stop aura %s", who(self)); }
static void __fastcall o_level_aura(void *self, void *edx) { (void)edx; event("level aura %s", who(self)); }
static void __fastcall o_remove_type(void *self, void *edx, int aid, int type) { (void)self; (void)edx; event("remove type %x aid %d", type, aid); }
static void __fastcall o_reroll(void *self, void *edx) { (void)self; (void)edx; event("reroll"); }
static void __fastcall o_refresh_basic(void *self, void *edx) { (void)self; (void)edx; event("refresh basic"); }
static int __stdcall o_is_pc_job_actor(int job) { return game_actor_is_pc_job(job); }
static int __cdecl o_sprintf(char *out, const char *fmt, ...)
{
    va_list ap;
    int n;
    va_start(ap, fmt);
    n = vsprintf(out, fmt, ap);
    va_end(ap);
    return n;
}

static void __fastcall o_set_head(void *a, void *edx, int v) { (void)edx; event("head %s %d", who(a), v); }
static void __fastcall o_set_weapon(void *a, void *edx, unsigned lo, int hi) { (void)edx; event("weapon %s %lld", who(a), ((long long)hi << 32) | lo); }
static void __fastcall o_set_weapon_look(void *a, void *edx, unsigned lo, int hi) { (void)edx; event("weaponlook %s %lld", who(a), ((long long)hi << 32) | lo); }
static void __fastcall o_set_accessory(void *a, void *edx, int v) { (void)edx; event("acc1 %s %d", who(a), v); }
static void __fastcall o_set_accessory2(void *a, void *edx, int v) { (void)edx; event("acc2 %s %d", who(a), v); }
static void __fastcall o_set_accessory3(void *a, void *edx, int v) { (void)edx; event("acc3 %s %d", who(a), v); }
static void __fastcall o_set_shield(void *a, void *edx, int v) { (void)edx; event("shield %s %d", who(a), v); }
static void __fastcall o_set_appearance(void *a, void *edx, int v) { (void)edx; event("appearance %s %d", who(a), v); }
static void __fastcall o_set_imf_name(void *a, void *edx) { (void)edx; event("imf %s", who(a)); }
static void __fastcall o_set_head_pal_name(void *a, void *edx) { (void)edx; event("headpalname %s", who(a)); }
static void __fastcall o_set_body_pal_name(void *a, void *edx) { (void)edx; event("bodypalname %s", who(a)); }
static void __fastcall o_set_body_palette(void *a, void *edx, int v) { (void)edx; event("bodypal %s %d", who(a), v); }
static void __fastcall o_set_head_palette(void *a, void *edx, int v) { (void)edx; event("headpal %s %d", who(a), v); }
static void __fastcall o_set_honor(void *a, void *edx, int v) { (void)edx; event("honor %s %d", who(a), v); }
static void __fastcall o_set_robe(void *a, void *edx, int v, int f) { (void)edx; event("robe %s %d %d", who(a), v, f & 0xff); }
static void __fastcall o_set_body_style(void *a, void *edx, int v, int f) { (void)edx; event("bodystyle %s %d %d", who(a), v, f & 0xff); }
static void __fastcall o_refresh_accessory3(void *a, void *edx) { (void)edx; event("refreshacc3 %s", who(a)); }

static int __fastcall o_job_packets(void *self, void *edx) { (void)edx; return job_of(self); }
static void *__cdecl o_operator_new(unsigned size) { return size == ORIG_PC_SIZE ? (void *)orig_new_actor : malloc(size); }
static void *__fastcall o_pc_construct(void *self, void *edx) { (void)edx; *(void **)self = orig_actor_vt; event("pc construct %s", who(self)); return self; }
static void *__fastcall o_new_actor_for_job(void *self, void *edx, int job)
{ (void)self; (void)edx; event("new actor %d", job); *(void **)orig_new_actor = orig_actor_vt; return orig_new_actor; }
static void *__stdcall o_buy_node(void *next, void *prev, void *value)
{
    unsigned *node = fake_nodes[fake_node_count++ & 3];
    node[0] = (unsigned)(UINT_PTR)next;
    node[1] = (unsigned)(UINT_PTR)prev;
    node[2] = *(unsigned *)value;
    return node;
}
static void __fastcall o_list_push(void *list, void *edx, void *value) { (void)edx; (void)value; ((unsigned *)list)[1]++; }
static unsigned **__fastcall o_marks_find(void *tree, void *edx, unsigned **out, const unsigned *key)
{ (void)edx; event("marks %s %u", tree_name(tree), *key); *out = *key == AID_OWN ? fake_tree_node : *(unsigned **)tree; return out; }
static void __fastcall o_make_emblem_wnd(void *self, void *edx, void *actor, int guild_id, int emblem_version)
{ (void)self; (void)edx; (void)emblem_version; event("emblem %s %d", who(actor), guild_id); }
static int __fastcall o_is_siege_mode(void *self, void *edx) { (void)self; (void)edx; return *(int *)oracle_addr(ORIG_EFFECT_OPTION); }
static int __fastcall o_is_player_job(void *self, void *edx, int id) { (void)self; (void)edx; return id < 1000; }
static int __fastcall o_is_homunculus_aid(void *self, void *edx, int aid) { (void)self; (void)edx; return aid == AID_UNKNOWN; }
static int __fastcall o_is_mercenary_aid(void *self, void *edx, int aid) { (void)self; (void)edx; return aid == AID_UNKNOWN; }
static const char *__fastcall o_pet_accessory_act(void *self, void *edx, int accessory) { (void)self; (void)edx; return session_get_pet_accessory_act(NULL, accessory); }
static int __fastcall o_load_act_res(void *self, void *edx, const char *name) { (void)edx; event("loadact %s %s", who(self), name); return 0; }
static int __fastcall o_champion_effects(void *self, void *edx, int job, void *actor, int aid) { (void)self; (void)edx; event("champion %d %s %d", job, who(actor), aid); return 0; }
static void __fastcall o_player_names_set(void *self, void *edx, unsigned char_id, struct OrigString name)
{ (void)self; (void)edx; event("pname %u '%s'", char_id, orig_std_string(name.raw)); }
static int __fastcall o_place_nowhere(void *self, void *edx, int id, const float *pos) { (void)self; (void)edx; event("place nowhere %d %.0f %.0f", id, pos[0], pos[1]); return 0; }
static int __fastcall o_place_at_cell(void *self, void *edx, int id, const int *cell) { (void)self; (void)edx; event("place cell %d %d %d", id, cell[0], cell[1]); return 0; }
static void __fastcall o_names_set_name(void *self, void *edx, unsigned aid, struct OrigString name)
{ (void)self; (void)edx; event("name %u '%s'", aid, orig_std_string(name.raw)); }
static void __fastcall o_sync_server_time(void *self, void *edx, unsigned t) { (void)self; (void)edx; event("sync %u", t); }
static const char *__fastcall o_character_name(void *self, void *edx) { (void)self; (void)edx; return "Hero"; }
static int __fastcall o_is_admin_option(void *self, void *edx, int what) { (void)self; (void)edx; return (what == 7 || what == 2 || what == 3) && *(int *)oracle_addr(ORIG_EFFECT_OPTION); }
static int __cdecl o_is_admin_aid(int aid) { return aid == AID_OWN; }
static int __fastcall o_get_guild_id(void *self, void *edx) { (void)edx; return self == orig_target ? 77 : 0; }
static void __fastcall o_set_guild(void *self, void *edx, int value0, int value1) { (void)edx; event("guild %s %d %d", who(self), value0, value1); }
static void __cdecl o_invalidate_windows(void) { event("invalidate"); }
static int __fastcall o_is_super_novice(void *self, void *edx, int job) { (void)self; (void)edx; return is_super_novice(job); }
static int __fastcall o_drawn_job(void *self, void *edx) { (void)self; (void)edx; return drawn_job; }
static void __fastcall o_basic_info_height(void *self, void *edx, int height) { (void)edx; event("bi height %s %d", window_name(self), height); }
static void __fastcall o_bar_set_values(void *bar, void *edx, unsigned cur_lo, int cur_hi, unsigned max_lo, int max_hi)
{ (void)edx; event("bar %d %lld %lld", (int)(UINT_PTR)bar, ((long long)cur_hi << 32) | cur_lo, ((long long)max_hi << 32) | max_lo); }
static void __fastcall o_refresh_actor_effects(void *self, void *edx, int player_only) { (void)self; (void)edx; event("refresh effects %d", player_only & 0xff); }
static void *__fastcall o_get_window(void *self, void *edx, int id) { (void)self; (void)edx; event("find %x", id); return windows_present ? orig_found_wnd : NULL; }
static void __cdecl o_refresh_skill_wnds(void) { event("refresh skills"); }
static void __fastcall o_status_restore(void *self, void *edx) { (void)edx; event("status restore %s", window_name(self)); }
static void __fastcall o_end_exchange(void *self, void *edx) { (void)self; (void)edx; event("end exchange"); }
static long long __fastcall o_find_abr(void *self, void *edx, unsigned lo, int hi) { (void)self; (void)edx; return session_find_abr(NULL, ((long long)hi << 32) | lo); }
static long long *__fastcall o_abr_get(void *tree, void *edx, const long long *key)
{
    int slot = abr_slot(tree);
    (void)edx;
    event("abr get %d %lld", slot, *key);
    fake_abr_used[slot] = 1;
    fake_abr_value[slot] = 0;
    return &fake_abr_value[slot];
}
static void __fastcall o_set_advanced_status(void *self, void *edx, int status, int value)
{ (void)self; (void)edx; adv_key = status; adv_value = value; adv_pending = 1; event("advstat get %d", status); }
static int __fastcall o_pet_talk_due(void *self, void *edx, unsigned lo, int hi) { (void)self; (void)edx; return session_pet_talk_due(NULL, ((long long)hi << 32) | lo); }
static int __cdecl o_rand(void) { return rand(); }
static int __fastcall o_window_send(void *self, void *edx, void *sender, int msg, int a, int b, int c, int d)
{ (void)edx; return ui_window_send((struct UIWindow *)self, (struct UIWindow *)sender, msg, a, b, c, d); }
static int __fastcall o_add_cart_item(void *self, void *edx, const unsigned char *item) { (void)self; (void)edx; event("cart add %s", orig_item_text(item)); return 1; }
static int __fastcall o_add_storage_item(void *self, void *edx, const unsigned char *item, int notify) { (void)self; (void)edx; event("storage add %s %d", orig_item_text(item), notify & 0xff); return 1; }
static void __fastcall o_inventory_update(void *self, void *edx, const unsigned char *item, int notify) { (void)self; (void)edx; event("inv update %s %d", orig_item_text(item), notify & 0xff); }
static int __fastcall o_add_inventory_item(void *self, void *edx, const unsigned char *item, int refresh) { (void)self; (void)edx; event("inv add %s %d", orig_item_text(item), refresh & 0xff); return 1; }
static int __fastcall o_set_worn_item(void *self, void *edx, const unsigned char *item) { (void)self; (void)edx; event("worn %s", orig_item_text(item)); return 1; }
static int __fastcall o_set_worn_costume(void *self, void *edx, const unsigned char *item) { (void)self; (void)edx; event("costume %s", orig_item_text(item)); return 1; }
static void __fastcall o_remove_filter_types(void *self, void *edx) { (void)self; (void)edx; event("remove filter types"); }
static void __fastcall o_remove_other_types(void *self, void *edx) { (void)self; (void)edx; event("remove other types"); }
static void __fastcall o_clear_worn_items(void *self, void *edx) { (void)self; (void)edx; event("clear worn"); }
static void __fastcall o_clear_worn_costumes(void *self, void *edx) { (void)self; (void)edx; event("clear costumes"); }
static void __fastcall o_clear_inventory(void *self, void *edx) { (void)self; (void)edx; event("clear inventory"); }
static void __fastcall o_clear_cart(void *self, void *edx) { (void)self; (void)edx; event("clear cart"); }
static void __cdecl o_refresh_party_ui(void) { event("refresh party"); }
static void __cdecl o_refresh_friend_ui(void) { event("refresh friend"); }
static void __fastcall o_refresh_item_wnds(void *self, void *edx) { (void)self; (void)edx; event("refresh item wnds"); }
static int __fastcall o_is_pet_egg_id(void *self, void *edx, int id) { (void)self; (void)edx; return is_pet_egg(id); }
static int __fastcall o_is_costume_location(void *self, void *edx, int flag) { (void)self; (void)edx; return is_costume_location(flag); }
static unsigned char *__fastcall o_get_inventory_item(void *self, void *edx, unsigned char *out, int index)
{
    (void)self; (void)edx;
    event("inv get %d", index);
    ((void *(__fastcall *)(void *, void *))oracle_addr(ORIG_ITEM_CONSTRUCT))(out, NULL);
    *(int *)(out + OI_INDEX) = index & 1 ? index : 0;
    ((void (__fastcall *)(void *, void *, int))oracle_addr(ORIG_ITEM_SET_ID))(out, NULL, 9000 + index);
    return out;
}
static int __fastcall o_pet_auto_feeding(void *self, void *edx, int egg_id) { (void)self; (void)edx; return egg_id & 1; }
static void __fastcall o_send_pet_auto_feed(void *self, void *edx, int on) { (void)self; (void)edx; event("autofeed %d", on & 0xff); }
static void __fastcall o_set_text_ellipsis(void *ctrl, void *edx, const char *text) { (void)ctrl; (void)edx; event("storage title storage '%s'", text); }
static void __fastcall o_storage_set_name(void *wnd, void *edx, struct OrigString name) { (void)edx; event("storage title %s '%s'", window_name(wnd), orig_std_string(name.raw)); }
static void *__cdecl o_replay_instance(void) { return orig_replay; }
static int __fastcall o_replay_next(void *self, void *edx, unsigned char *buffer, unsigned *id_out)
{
    (void)self; (void)edx;
    if (pending_count <= 0)
        return 0;
    pending_count--;
    memcpy(buffer, pending_pkt, PACKET_MAX);
    *id_out = *(const unsigned short *)pending_pkt;
    return 1;
}
static int __fastcall o_replay_set_mode(void *self, void *edx, int mode) { (void)self; (void)edx; event("replay mode %d", mode); return 1; }
static void __fastcall o_pet_info_intimacy(void *self, void *edx) { (void)edx; event("pet intimacy %s", window_name(self)); }
static void __fastcall o_ctrl_set_text(void *self, void *edx, const char *text) { (void)self; (void)edx; event("pet name %s '%s'", window_name(orig_pet_info_wnd), text); }
static void __fastcall o_pet_speak(void *self, void *edx, int job, int hunger, int action, char *out)
{ (void)self; (void)edx; session_get_pet_speak(NULL, job, hunger, action, out, 256); }
static char *__fastcall o_format_chat_line(void *self, void *edx, const char *text, char *out, int aid, int replace)
{ (void)self; (void)edx; session_format_chat_line(NULL, text, out, 256, aid, replace); return out; }
static void __cdecl o_homun_redraw(void) { event("homun redraw"); }
static void __cdecl o_merc_redraw(void) { event("merc redraw"); }
static void __fastcall o_list_clear(void *self, void *edx) { (void)edx; event("list clear %s", self == oracle_addr(ORIG_MER_SKILLS) ? "merSkills" : "?"); }
static void __fastcall o_set_pk_mode(void *self, void *edx, int value) { (void)edx; event("pkmode %s %d", who(self), value); }
static unsigned char *__fastcall o_get_item_by_id(void *self, void *edx, unsigned char *out, int item_id, int identified)
{
    (void)self; (void)edx;
    event("inv by id %d %d", item_id, identified & 0xff);
    ((void *(__fastcall *)(void *, void *))oracle_addr(ORIG_ITEM_CONSTRUCT))(out, NULL);
    *(int *)(out + OI_COUNT) = item_id % 5;
    return out;
}
static char *__fastcall o_item_get_name(void *self, void *edx, char *out) { (void)self; (void)edx; strcpy(out, "item"); return out; }
static unsigned char *__fastcall o_item_display_name(void *self, void *edx, unsigned char *out)
{
    (void)self; (void)edx;
    memset(out, 0, ORIG_STD_STRING);
    strcpy((char *)out, "item");
    *(unsigned *)(out + 0x10) = 4;
    *(unsigned *)(out + 0x14) = ORIG_STD_STRING_SSO - 1;
    return out;
}
static unsigned char *__fastcall o_selected_item(void *self, void *edx, unsigned char *out, int *found)
{
    (void)edx;
    event("selected %s", window_name(self));
    ((void *(__fastcall *)(void *, void *))oracle_addr(ORIG_ITEM_CONSTRUCT))(out, NULL);
    *(int *)(out + OI_INDEX) = 9;
    ((void (__fastcall *)(void *, void *, int))oracle_addr(ORIG_ITEM_SET_ID))(out, NULL, 9009);
    *found = current_fill == FILL_ONES;
    return out;
}
static void __fastcall o_window_invalidate(void *self, void *edx) { (void)edx; event("invalidate %s", window_name(self)); }
static void __fastcall o_set_spr_job(void *self, void *edx, int job) { (void)edx; event("sprjob %s %d", who(self), job); }
struct OrigPost { unsigned char b[ORIG_POST_SIZE]; };
static void __fastcall o_booking_add_rows(void *self, void *edx, const unsigned char *posts, int count, int more)
{
    int i;
    (void)self; (void)edx;
    event("booking add %u %d", (unsigned)count, (char)more);
    for (i = 0; i < count; i++)
        event("%s", post_text(posts + i * ORIG_POST_SIZE));
}
static void __fastcall o_booking_remove(void *self, void *edx, int id) { (void)self; (void)edx; event("booking remove %d", id); }
static void __fastcall o_booking_on_new(void *self, void *edx, struct OrigPost post) { (void)self; (void)edx; event("booking new %s", post_text(post.b)); }
static void __fastcall o_booking_set_jobs(void *self, void *edx, int id, const short *jobs)
{ (void)self; (void)edx; event("booking jobs %d %d %d %d %d %d %d", id, jobs[0], jobs[1], jobs[2], jobs[3], jobs[4], jobs[5]); }
static void __fastcall o_set_wanted_jobs(void *self, void *edx, void *wnd) { (void)self; (void)edx; (void)wnd; event("wanted jobs"); }
static void __fastcall o_stop_recruiting(void *self, void *edx, int aid) { (void)self; (void)edx; event("stop recruiting %u", aid); }
static void __fastcall o_add_join_request(void *self, void *edx, int char_id, int aid, const char *name, int level, int job)
{ (void)self; (void)edx; event("join request %d %u '%s' %d %d", char_id, aid, name, level, job); }
static void __fastcall o_command_plate_of(void *self, void *edx, int aid, int a, int b) { (void)self; (void)edx; event("plate %u %d %d", aid, a, b); }
static int __fastcall o_is_own_unit_aid(void *self, void *edx, int aid) { (void)self; (void)edx; return aid == (int)AID_OWN; }
static int __fastcall o_filter_contains(void *self, void *edx, const char *text) { (void)self; (void)edx; return strstr(text, "bad") != NULL; }
static void __fastcall o_filter_censor(void *self, void *edx, char *text, void *out) { (void)self; (void)edx; (void)out; censor_text(text); event("censor '%s'", text); }
static void __fastcall o_clear_inv_pending(void *self, void *edx, int index) { (void)self; (void)edx; event("clear inv pending %d", index); }
static void __fastcall o_clear_cart_pending(void *self, void *edx, int index) { (void)self; (void)edx; event("clear cart pending %d", index); }
static void __fastcall o_clear_storage_pending(void *self, void *edx, int index) { (void)self; (void)edx; event("clear storage pending %d", index); }
static unsigned char *__fastcall o_get_party_member(void *self, void *edx, unsigned char *out, unsigned aid)
{
    (void)self; (void)edx;
    event("member get %u", aid);
    memset(out, 0, ORIG_MEMBER_SIZE);
    *(unsigned *)out = aid == AID_TARGET;
    return out;
}
static void __fastcall o_set_member_state(void *self, void *edx, unsigned aid, unsigned state) { (void)self; (void)edx; event("member state %u %u", aid, state); }
static void __fastcall o_add_party_member(void *self, void *edx, const unsigned char *m)
{
    (void)self; (void)edx;
    event("add %s", member_text(*(const unsigned *)m, *(const unsigned *)(m + 4), *(const unsigned *)(m + 8), orig_std_string(m + 0xc),
                               orig_std_string(m + 0x24), *(const unsigned *)(m + 0x3c), *(const unsigned *)(m + 0x40), *(const unsigned short *)(m + 0x48),
                               *(const unsigned short *)(m + 0x4a), *(const unsigned *)(m + 0x4c)));
}
static unsigned *__fastcall o_room_owner_at(void *self, void *edx, const unsigned *key) { (void)self; (void)edx; return &orig_room_owners[*key % 4]; }
static int __cdecl o_is_yellow_aid(int aid) { return aid == (int)AID_TARGET; }
static int __fastcall o_remove_friend(void *self, void *edx, unsigned char_id) { (void)self; (void)edx; event("remove friend %u", char_id); return char_id & 1; }
static void __cdecl o_notify_group_wnd(void) { event("notify group wnd"); }
static void __fastcall o_end_exchange1(void *self, void *edx) { (void)self; (void)edx; event("end exchange 1"); }
static void __fastcall o_their_zeny(void *self, void *edx, int money) { (void)self; (void)edx; event("their zeny %d", money); }
struct OrigItemArg { unsigned char b[ORIG_ITEM_ARG_SIZE]; };
static void __fastcall o_add_their_item(void *self, void *edx, struct OrigItemArg item) { (void)self; (void)edx; event("their item %s", orig_item_text(item.b)); }
static void __fastcall o_log_whisper(void *self, void *edx, const char *line) { (void)self; (void)edx; event("log '%s'", line); }
static void __fastcall o_clear_lists(void *self, void *edx) { (void)self; (void)edx; event("clear lists"); }
static void *__cdecl o_item_table_get(int item_id) { *(int *)(fake_item_entry + 0x40) = item_id % 7; return fake_item_entry; }
static void __fastcall o_append_market(void *self, void *edx, const unsigned char *record, int count, int weight)
{ (void)self; (void)edx; event("market %s %d %d", orig_record_text(record), count, weight & 0xffff); }
static void __fastcall o_add_bought(void *self, void *edx, const unsigned char *record, int count) { (void)self; (void)edx; event("bought %s %d", orig_record_text(record), count); }
static void *__cdecl o_rt_dynamic_cast(void *object, int delta, void *from, void *to, int is_reference)
{ (void)delta; (void)from; (void)to; (void)is_reference; return object; }
static void __fastcall o_mode_mgr_switch(void *self, void *edx, int type, const char *name) { (void)self; (void)edx; event("switch %d '%s'", type, name); }
static void __fastcall o_wait_set_text(void *wnd, void *edx, const char *text, int a, int b) { (void)edx; (void)a; (void)b; event("wait text %s '%s'", window_name(wnd), text); }
static void __fastcall o_reset_store_search(void *self, void *edx) { (void)self; (void)edx; event("reset store search"); }
static int __fastcall o_reset_for_warp(void *self, void *edx) { (void)self; (void)edx; event("reset warp"); return 1; }
static void __fastcall o_mode_initialize(void *self, void *edx) { (void)self; (void)edx; event("initialize"); }
static void __fastcall o_run_fade_out(void *self, void *edx, int logo) { (void)self; (void)edx; event("fade out %d", logo); }
static void __fastcall o_reset_mail_items(void *self, void *edx) { (void)self; (void)edx; event("reset mail"); }
static void __fastcall o_clear_mail_counts(void *self, void *edx) { (void)self; (void)edx; event("clear mail counts"); }
static void __fastcall o_move_to_server(void *self, void *edx, struct OrigString map, int x, int y, unsigned ip, int port, struct OrigString extra)
{ (void)self; (void)edx; event("move '%s' %d %d %x %d '%s'", orig_std_string(map.raw), x, y, ip, port, orig_std_string(extra.raw)); }
static void __fastcall o_play_stream(void *self, void *edx, const char *name, int loop_count) { (void)self; (void)edx; event("stream '%s' %d", name, loop_count); }
static void __fastcall o_set_text_align(void *ctrl, void *edx, int align) { (void)ctrl; (void)edx; event("align %d", align & 0xff); }
static void __fastcall o_set_job_model(void *self, void *edx, int job, int sex, int extra) { (void)edx; (void)extra; event("jobmodel %s %d %d", who(self), job, sex); }

typedef void (*mode_pkt_fn)(struct GameMode *, const void *);
#define LIFT(fn) static void w_##fn(const unsigned char *pkt) { ((mode_pkt_fn)fn)(&lift_mode, pkt); }
LIFT(game_mode_zc_notify_vanish)
LIFT(game_mode_zc_sprite_change)
LIFT(game_mode_zc_sprite_change2)
LIFT(game_mode_zc_fast_move)
LIFT(game_mode_zc_blade_stop)
LIFT(game_mode_zc_notify_actor_init)
LIFT(game_mode_zc_par_change_user)
LIFT(game_mode_on_ack_req_name_all2)
LIFT(game_mode_zc_combo_delay)
LIFT(game_mode_on_notify_effect)
LIFT(game_mode_on_notify_effect2)
LIFT(game_mode_on_notify_effect3)
LIFT(game_mode_on_notify_effect4)
LIFT(game_mode_on_clear_effect)
LIFT(game_mode_on_congratulation)
LIFT(game_mode_on_npc_show_efst_update)
LIFT(game_mode_zc_devotion_list)
LIFT(game_mode_zc_dispel)
LIFT(game_mode_zc_notify_newentry)
LIFT(game_mode_zc_notify_newentry2)
LIFT(game_mode_zc_notify_newentry3)
LIFT(game_mode_zc_notify_newentry4)
LIFT(game_mode_zc_notify_standentry)
LIFT(game_mode_zc_notify_standentry_npc)
LIFT(game_mode_zc_notify_standentry2)
LIFT(game_mode_zc_notify_standentry3)
LIFT(game_mode_zc_notify_standentry4)
LIFT(game_mode_zc_notify_standentry5)
LIFT(game_mode_zc_notify_newentry5)
LIFT(game_mode_zc_notify_moveentry)
LIFT(game_mode_zc_notify_moveentry2)
LIFT(game_mode_zc_notify_moveentry3)
LIFT(game_mode_zc_notify_moveentry6)
LIFT(game_mode_zc_notify_ranking)
LIFT(game_mode_zc_notify_newentry7)
LIFT(game_mode_zc_notify_standentry8)
LIFT(game_mode_zc_notify_moveentry9)
LIFT(game_mode_zc_notify_moveentry10)
LIFT(game_mode_zc_notify_moveentry11)
LIFT(game_mode_zc_notify_newentry10)
LIFT(game_mode_zc_notify_newentry11)
LIFT(game_mode_zc_notify_standentry10)
LIFT(game_mode_zc_notify_standentry11)
LIFT(game_mode_zc_par_change)
LIFT(game_mode_on_long_par_change)
LIFT(game_mode_on_long_long_par_change)
LIFT(game_mode_zc_status)
LIFT(game_mode_zc_status_change_ack)
LIFT(game_mode_zc_couple_status)
LIFT(game_mode_zc_hp_info)
LIFT(game_mode_zc_hp_info_tiny)
LIFT(game_mode_zc_abr_init)
LIFT(game_mode_zc_abr_par_change)
LIFT(game_mode_on_notify_exp)
LIFT(game_mode_on_notify_exp2)
#define LIFT1(fn) static void w_##fn(const unsigned char *pkt) { ((void (*)(const void *))fn)(pkt); }
LIFT1(game_mode_ack_status_gm)
LIFT1(game_mode_cart_equipment_item_list2)
LIFT1(game_mode_cart_equipment_item_list3)
LIFT1(game_mode_cart_item_list_equip_v5)
LIFT1(game_mode_cart_item_list_equip_v6)
LIFT1(game_mode_cart_item_list_normal_v5)
LIFT1(game_mode_equipment_item_list2)
LIFT1(game_mode_equipment_item_list3)
LIFT1(game_mode_equipment_item_list4)
LIFT1(game_mode_equipment_item_list_v5)
LIFT1(game_mode_equipment_item_list_v6)
LIFT1(game_mode_equipment_item_list_v7)
LIFT1(game_mode_inventory_item_list_normal_v5)
LIFT1(game_mode_inventory_tab)
LIFT1(game_mode_store_item_list_equip_v5)
LIFT1(game_mode_store_item_list_equip_v6)
LIFT1(game_mode_store_item_list_normal_v5)
LIFT(game_mode_zc_store_equipment_item_list2)
LIFT(game_mode_zc_store_equipment_item_list3)
LIFT(game_mode_zc_store_item_list_equip_v2)
LIFT(game_mode_zc_store_item_list_equip_v5)
LIFT(game_mode_zc_store_item_list_equip_v6)
LIFT(game_mode_zc_store_item_list_normal_v5)
LIFT(game_mode_zc_split_send_item_list_equip_v8)
LIFT(game_mode_zc_split_send_item_list_set)
LIFT(game_mode_zc_npc_sprite_change)
LIFT(game_mode_zc_npc_chat)
LIFT(game_mode_zc_npc_market_open2)
LIFT(game_mode_zc_npc_market_open)
LIFT(game_mode_zc_npc_market_purchase_result_v2)
LIFT(game_mode_zc_npcack_mapmove)
LIFT(game_mode_zc_npcack_servermove)
LIFT(game_mode_zc_npcack_servermove_domain)
LIFT(game_mode_zc_open_edit_dlg_str)
LIFT(game_mode_zc_say_dialog)
LIFT(game_mode_zc_select_dealtype)
LIFT(game_mode_zc_clear_dialog)
LIFT(game_mode_zc_dynamic_npc_create_result)
LIFT(game_mode_zc_wait_dialog)
LIFT(game_mode_zc_close_dialog)
LIFT(game_mode_zc_menu_list)
LIFT(game_mode_zc_play_npc_bgm)
LIFT(game_mode_zc_sound)
LIFT(game_mode_zc_open_edit_dlg)
static void w_dialog_text_align(const unsigned char *pkt) { game_mode_zc_dialog_text_align(&lift_mode, pkt[2]); }
LIFT(game_mode_zc_property_pet)
LIFT(game_mode_zc_pet_act)
LIFT(game_mode_zc_pet_egg_list)
static void w_pet_evolution_result(const unsigned char *pkt) { game_mode_zc_pet_evolution_result(&lift_mode, *(const unsigned *)(pkt + 2)); }
LIFT(game_mode_zc_change_state_pet)
LIFT(game_mode_zc_feed)
LIFT(game_mode_zc_change_state_homun)
static void w_ho_par_change2(const unsigned char *pkt) { game_mode_zc_ho_par_change2(pkt); }
static void w_ho_par_change(const unsigned char *pkt) { game_mode_zc_ho_par_change(pkt); }
LIFT(game_mode_zc_property_homun)
LIFT(game_mode_zc_property_homun_no_accessory)
LIFT(game_mode_zc_property_homun_no_accessory2)
LIFT(game_mode_zc_property_homun_no_accessory3)
LIFT(game_mode_zc_mer_init)
static void w_mer_par_change(const unsigned char *pkt) { game_mode_zc_mer_par_change(pkt); }
LIFT(game_mode_zc_el_init)
LIFT(game_mode_zc_el_par_change)
static void w_booking_ack_delete(const unsigned char *pkt) { game_mode_zc_party_booking_ack_delete(pkt); }
static void w_booking_ack_register(const unsigned char *pkt) { game_mode_zc_party_booking_ack_register(pkt); }
static void w_booking_ack_search(const unsigned char *pkt) { game_mode_zc_party_booking_ack_search(pkt, *(const unsigned short *)(pkt + 2)); }
static void w_booking_notify_delete(const unsigned char *pkt) { game_mode_zc_party_booking_notify_delete(pkt); }
static void w_booking_notify_insert(const unsigned char *pkt) { game_mode_zc_party_booking_notify_insert(pkt); }
static void w_booking_notify_update(const unsigned char *pkt) { game_mode_zc_party_booking_notify_update(pkt); }
LIFT(game_mode_zc_seek_party)
static void w_party_config(const unsigned char *pkt) { game_mode_zc_party_config(pkt); }
static void w_join_req_ack_from_master(const unsigned char *pkt) { game_mode_zc_party_join_req_ack_from_master(pkt); }
static void w_req_master_to_join(const unsigned char *pkt) { game_mode_zc_party_req_master_to_join(pkt); }
static void w_req_join_group(const unsigned char *pkt) { game_mode_zc_req_join_group(pkt); }
static void w_party_join_ack(const unsigned char *pkt) { game_mode_party_join_ack(pkt); }
static void w_groupinfo_change_v2(const unsigned char *pkt) { game_mode_zc_req_groupinfo_change_v2(pkt); }
static void w_ack_make_group(const unsigned char *pkt) { game_mode_zc_ack_make_group(pkt); }
LIFT(game_mode_zc_notify_hp_to_group_m)
LIFT(game_mode_zc_notify_hp_to_group_m_r2)
LIFT(game_mode_zc_notify_position_to_group_m)
static void w_group_info_change(const unsigned char *pkt) { game_mode_zc_group_info_change(pkt); }
LIFT(game_mode_on_add_member_to_group)
LIFT(game_mode_on_add_member_to_group2)
LIFT(game_mode_on_party_member_state)
LIFT(game_mode_on_list_flag_clear)
LIFT(game_mode_zc_room_newentry)
LIFT(game_mode_zc_refuse_enter_room)
LIFT(game_mode_zc_ack_create_chatroom)
LIFT(game_mode_zc_destroy_room)
LIFT(game_mode_zc_member_newentry)
LIFT(game_mode_zc_member_exit)
LIFT(game_mode_zc_role_change)
LIFT(game_mode_zc_change_chatroom)
LIFT(game_mode_zc_setting_whisper_pc2)
LIFT(game_mode_zc_setting_whisper_pc)
LIFT(game_mode_zc_setting_whisper_state)
LIFT(game_mode_zc_notify_chat)
LIFT(game_mode_zc_whisper_list)
LIFT(game_mode_zc_req_exchange_item2)
LIFT(game_mode_zc_req_exchange_item)
LIFT(game_mode_zc_conclude_exchange_item)
LIFT(game_mode_zc_cancel_exchange_item)
LIFT(game_mode_zc_exec_exchange_item)
LIFT(game_mode_zc_exchange_item_undo)
LIFT(game_mode_zc_req_add_friends)
LIFT(game_mode_zc_delete_friends)
LIFT(game_mode_zc_req_baby)
LIFT(game_mode_zc_req_couple)
LIFT(game_mode_on_add_exchange_item_opt)
LIFT(game_mode_on_add_exchange_item_grade)
LIFT(game_mode_zc_couple_name)
LIFT(game_mode_zc_divorce)
static void w_baby_msg(const unsigned char *pkt) { game_mode_zc_baby_msg(pkt); }
static void w_ack_give_manner_point(const unsigned char *pkt) { game_mode_zc_ack_give_manner_point(pkt); }
static void w_notify_manner_point_given(const unsigned char *pkt) { game_mode_zc_notify_manner_point_given(pkt); }

static const unsigned effect_ids[] = { 0x3fd, 0x5a, 0x5c, 0x5f, 0x238, 0x14b, 0x14c, 0x291, 0x294, 0x298, 0x299 };
static const unsigned actor_aids[] = { AID_CASTER, AID_TARGET, AID_OWN };
static const unsigned state_bits[] = { 0x40, 0x42, 0x44, 0x46 };
static const unsigned ranks[] = { 1, 10, 11, 0xfffffff0u };
static const unsigned entry_types[] = { 0, 1, 5, 6, 7, 8, 9, 10, 12, 13, 14 };
static const int entry_jobs[] = { 0, 19, 20, 45, 46, 999, 1000, 1001, 1285, 2000, 3999, 4000, 6001, 6017, 6053, 20021, 32000, 0x2d2 };
#define N_TYPES ((unsigned)(sizeof entry_types / sizeof *entry_types))
#define N_JOBS ((unsigned)(sizeof entry_jobs / sizeof *entry_jobs))
static const unsigned par_types[] = { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 0xb, 0xc, 13, 14, 15, 16, 17, 18, 0x18, 0x19, 0x1c, 41, 42, 43, 44, 45, 46, 47, 48,
                                      49, 50, 51, 52, 53, 54, 55, 0x3c, 0xdb, 0xe0, 0xe9, 0xea, 0xffff };
static const unsigned exp_types[] = { 0, 1, 2, 3, 20, 21, 22, 23, 0xffff };
static const unsigned ack_types[] = { 0, 13, 14, 15, 16, 17, 18, 19, 41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 51, 52, 53, 54, 55, 56, 100, 0xffff };
static const unsigned couple_types[] = { 0, 12, 13, 14, 15, 16, 17, 18, 19, 0xda, 0xdb, 0xdc, 0xdd, 0xde, 0xdf, 0xe0, 0xe1, 0xffff };
static const unsigned exp_amounts[] = { 0xfffffffbu, 100000, 0xffffffffu };
static const int par_values[12] = { 0, 1, -1, 50, 51, 100, 5000, 9000, 10000, 20000, -61, 123456 };
static const int drawn_jobs[5] = { 0x17, 0x108e, 0xfce, 0xfd2, 1 };
static const char *const list_names[4] = { "", "Kafra", "xxxxxxxxxxxxxxxxxxxxxxx", "Store" };
static const char long_list_name[] = "yyyyyyyyyyyyyyyyyyyyyyyyy";

struct ListLayout { unsigned char rec_size, flags_off, damaged_off, header; };
static const struct ListLayout lay_old34 = { 34, 0, 0xc, 4 };
static const struct ListLayout lay_old38 = { 38, 0, 0xc, 4 };
static const struct ListLayout lay_eq4 = { 37, 0x24, 0, 4 };
static const struct ListLayout lay_v5 = { 41, 0x28, 0, 4 };
static const struct ListLayout lay_v6 = { 67, 0x42, 0, 4 };
static const struct ListLayout lay_v7 = { 68, 0x43, 0, 4 };
static const struct ListLayout lay_n34 = { 34, 0, 0, 4 };
static const struct ListLayout lay_neq4 = { 37, 0x24, 0, 0x1c };
static const struct ListLayout lay_nv5 = { 41, 0x28, 0, 0x1c };
static const struct ListLayout lay_nv6 = { 67, 0x42, 0, 0x1c };
static const struct ListLayout lay_nn34 = { 34, 0, 0, 0x1c };
static const struct ListLayout lay_split8 = { 0x44, 0x43, 0, 5 };

struct EntryLayout { unsigned char type_off, aid_off, job_off, head_off, varlen; };
static const struct EntryLayout lay_new1 = { 0, 2, 0xe, 0x10, 0 };
static const struct EntryLayout lay_new4 = { 0, 2, 0x10, 0x12, 0 };
static const struct EntryLayout lay_stand = { 2, 3, 0xf, 0x11, 0 };
static const struct EntryLayout lay_npc = { 2, 3, 0x15, 0xf, 0 };
static const struct EntryLayout lay_move3 = { 2, 3, 0x11, 0x13, 0 };
static const struct EntryLayout lay_var = { 4, 5, 0x13, 0x15, 1 };
static const struct EntryLayout lay_var10 = { 4, 5, 0x17, 0x19, 1 };

struct Handler {
    const char *name;
    unsigned orig;
    unsigned short id;
    int length;
    void (*lift)(const unsigned char *pkt);
    int kind;
    const unsigned *extra;
    int extra_count;
    const struct EntryLayout *layout;
    int move_mask;
    int no_unknown;
    const struct ListLayout *list;
    int arg_byte;
    int service_type;
    int arg_dword;
};
#define EXTRA(a) a, (int)(sizeof a / sizeof *a)

static const unsigned pet_act_values[] = { 22, 32, 123, 1234, 4567, 7777, 100000 };
static const unsigned pet_state_values[] = { 17, 20, 27 };
static const unsigned ho_par_vars[] = { 1, 5, 7, 0, 6, 8 };
static const unsigned mer_par_vars[] = { 5, 6, 7, 8, 41, 49, 53, 165, 189, 190, 0, 9, 0xb9, 0xba, 1000 };
static const unsigned el_par_vars[] = { 5, 6, 7, 8, 0, 1, 3, 9 };
static const unsigned join_ack_values[] = { 18, 100 };
static const struct Handler handlers[] = {
    { "notify_vanish", 0x7cc580u, 0x80, 7, w_game_mode_zc_notify_vanish, PK_DEFAULT, NULL, 0 },
    { "sprite_change", 0x7d6650u, 0xc3, 8, w_game_mode_zc_sprite_change, PK_DEFAULT, NULL, 0 },
    { "sprite_change2", 0x7f5320u, 0x1d7, 15, w_game_mode_zc_sprite_change2, PK_DEFAULT, NULL, 0 },
    { "fast_move", 0x7fae70u, 0x8d2, 10, w_game_mode_zc_fast_move, PK_DEFAULT, NULL, 0 },
    { "blade_stop", 0x7f9bb0u, 0x1d1, 14, w_game_mode_zc_blade_stop, PK_DEFAULT, EXTRA(actor_aids) },
    { "notify_actor_init", 0x7f4d40u, 0xb1b, 2, w_game_mode_zc_notify_actor_init, PK_DEFAULT, NULL, 0 },
    { "par_change_user", 0x7ce200u, 0x1ab, 12, w_game_mode_zc_par_change_user, PK_DEFAULT, NULL, 0 },
    { "ack_req_name_all2", 0x7f4470u, 0xa30, 106, w_game_mode_on_ack_req_name_all2, PK_NAMES, NULL, 0 },
    { "combo_delay", 0x7ba170u, 0x1d2, 10, w_game_mode_zc_combo_delay, PK_DEFAULT, NULL, 0 },
    { "notify_effect", 0x7ff750u, 0x19b, 10, w_game_mode_on_notify_effect, PK_DEFAULT, NULL, 0 },
    { "notify_effect2", 0x7ff460u, 0x1f3, 10, w_game_mode_on_notify_effect2, PK_DEFAULT, EXTRA(effect_ids) },
    { "notify_effect3", 0x7ff730u, 0x284, 14, w_game_mode_on_notify_effect3, PK_DEFAULT, EXTRA(effect_ids) },
    { "notify_effect4", 0x7f4ed0u, 0xb69, 18, w_game_mode_on_notify_effect4, PK_DEFAULT, EXTRA(effect_ids) },
    { "clear_effect", 0x7fa8b0u, 0xb0d, 10, w_game_mode_on_clear_effect, PK_DEFAULT, NULL, 0 },
    { "congratulation", 0x7fac60u, 0x1ea, 6, w_game_mode_on_congratulation, PK_DEFAULT, NULL, 0 },
    { "npc_show_efst_update", 0x803b30u, 0x28a, 18, w_game_mode_on_npc_show_efst_update, PK_DEFAULT, EXTRA(state_bits) },
    { "devotion_list", 0x7ba9d0u, 0x1cf, 28, w_game_mode_zc_devotion_list, PK_DEFAULT, EXTRA(actor_aids) },
    { "dispel", 0x7bac20u, 0x1b9, 6, w_game_mode_zc_dispel, PK_DEFAULT, NULL, 0 },
    { "newentry", 0x7c9ce0u, 0x79, 0x35, w_game_mode_zc_notify_newentry, PK_ENTRY, NULL, 0, &lay_new1, 0 },
    { "newentry2", 0x7c8c50u, 0x1d9, 0x39, w_game_mode_zc_notify_newentry2, PK_ENTRY, NULL, 0, &lay_new1, 0 },
    { "newentry3", 0x7c91d0u, 0x22b, 0x3f, w_game_mode_zc_notify_newentry3, PK_ENTRY, NULL, 0, &lay_new4, 0 },
    { "newentry4", 0x7c9750u, 0x2ed, 0x3f, w_game_mode_zc_notify_newentry4, PK_ENTRY, NULL, 0, &lay_new4, 0 },
    { "standentry", 0x7cbbf0u, 0x78, 0x37, w_game_mode_zc_notify_standentry, PK_ENTRY, NULL, 0, &lay_stand, 0 },
    { "standentry_npc", 0x7cc180u, 0x7c, 0x2c, w_game_mode_zc_notify_standentry_npc, PK_ENTRY, NULL, 0, &lay_npc, 0 },
    { "standentry2", 0x7cacc0u, 0x1d8, 0x3a, w_game_mode_zc_notify_standentry2, PK_ENTRY, NULL, 0, &lay_new1, 0 },
    { "standentry3", 0x7cb1d0u, 0x22a, 0x40, w_game_mode_zc_notify_standentry3, PK_ENTRY, NULL, 0, &lay_new4, 0 },
    { "standentry4", 0x7cb6e0u, 0x2ee, 0x40, w_game_mode_zc_notify_standentry4, PK_ENTRY, NULL, 0, &lay_new4, 0 },
    { "standentry5", 0x7ca7b0u, 0x7f9, 0x45, w_game_mode_zc_notify_standentry5, PK_ENTRY, NULL, 0, &lay_var, 0 },
    { "newentry5", 0x7c87b0u, 0x7f8, 0x44, w_game_mode_zc_notify_newentry5, PK_ENTRY, NULL, 0, &lay_var, 0 },
    { "moveentry", 0x7c8280u, 0x7b, 0x3c, w_game_mode_zc_notify_moveentry, PK_ENTRY, NULL, 0, &lay_new1, 12 },
    { "moveentry2", 0x7c7150u, 0x1da, 0x40, w_game_mode_zc_notify_moveentry2, PK_ENTRY, NULL, 0, &lay_new1, 4 },
    { "moveentry3", 0x7c76d0u, 0x22c, 0x45, w_game_mode_zc_notify_moveentry3, PK_ENTRY, NULL, 0, &lay_move3, 4 },
    { "moveentry6", 0x7c7c70u, 0x2ec, 0x4b, w_game_mode_zc_notify_moveentry6, PK_ENTRY, NULL, 0, &lay_var, 5 },
    { "ranking", 0x7ca510u, 0x19a, 14, w_game_mode_zc_notify_ranking, PK_DEFAULT, EXTRA(ranks), NULL, 0, 1 },
    { "newentry7", 0x7f1ba0u, 0x90f, 0x4d, w_game_mode_zc_notify_newentry7, PK_ENTRY, NULL, 0, &lay_var, 0 },
    { "standentry8", 0x7f2a90u, 0x915, 0x4e, w_game_mode_zc_notify_standentry8, PK_ENTRY, NULL, 0, &lay_var, 0 },
    { "moveentry9", 0x7f0d40u, 0x914, 0x54, w_game_mode_zc_notify_moveentry9, PK_ENTRY, NULL, 0, &lay_var, 5 },
    { "moveentry10", 0x7f0a90u, 0x9db, 0x58, w_game_mode_zc_notify_moveentry10, PK_ENTRY, NULL, 0, &lay_var10, 7 },
    { "moveentry11", 0x7f0be0u, 0x9fd, 0x5a, w_game_mode_zc_notify_moveentry11, PK_ENTRY, NULL, 0, &lay_var10, 7 },
    { "newentry10", 0x7f1900u, 0x9dc, 0x51, w_game_mode_zc_notify_newentry10, PK_ENTRY, NULL, 0, &lay_var10, 0 },
    { "newentry11", 0x7f1a50u, 0x9fe, 0x53, w_game_mode_zc_notify_newentry11, PK_ENTRY, NULL, 0, &lay_var10, 0 },
    { "standentry10", 0x7f27e0u, 0x9dd, 0x52, w_game_mode_zc_notify_standentry10, PK_ENTRY, NULL, 0, &lay_var10, 0 },
    { "standentry11", 0x7f2930u, 0x9ff, 0x54, w_game_mode_zc_notify_standentry11, PK_ENTRY, NULL, 0, &lay_var10, 0 },
    { "par_change", 0x7cd5c0u, 0xb0, 8, w_game_mode_zc_par_change, PK_PAR, EXTRA(par_types) },
    { "long_par_change", 0x7c4530u, 0xb1, 8, w_game_mode_on_long_par_change, PK_PAR, EXTRA(exp_types) },
    { "long_long_par_change", 0x7fce30u, 0xacb, 12, w_game_mode_on_long_long_par_change, PK_PAR, EXTRA(exp_types) },
    { "status", 0x7d7110u, 0xbd, 44, w_game_mode_zc_status, PK_DEFAULT, NULL, 0 },
    { "status_change_ack", 0x7d7560u, 0xbc, 6, w_game_mode_zc_status_change_ack, PK_PAR, EXTRA(ack_types) },
    { "ack_status_gm", 0x7b33a0u, 0x214, 44, w_game_mode_ack_status_gm, PK_DEFAULT, NULL, 0 },
    { "couple_status", 0x7dd890u, 0x141, 14, w_game_mode_zc_couple_status, PK_PAR, EXTRA(couple_types) },
    { "hp_info", 0x7f09f0u, 0x977, 14, w_game_mode_zc_hp_info, PK_DEFAULT, NULL, 0 },
    { "hp_info_tiny", 0x7f0a40u, 0xa36, 7, w_game_mode_zc_hp_info_tiny, PK_DEFAULT, NULL, 0 },
    { "abr_init", 0x7f4290u, 0xb6b, 14, w_game_mode_zc_abr_init, PK_DEFAULT, NULL, 0 },
    { "abr_par_change", 0x7f4330u, 0xb6c, 12, w_game_mode_zc_abr_par_change, PK_DEFAULT, NULL, 0 },
    { "notify_exp", 0x7c6ba0u, 0x7f6, 14, w_game_mode_on_notify_exp, PK_EXP, EXTRA(exp_amounts) },
    { "notify_exp2", 0x7ff9b0u, 0xacc, 18, w_game_mode_on_notify_exp2, PK_EXP, EXTRA(exp_amounts) },
    { "cart_equip2", 0x7b84a0u, 0x297, 4, w_game_mode_cart_equipment_item_list2, PK_LIST, NULL, 0, NULL, 0, 0, &lay_old34 },
    { "cart_equip3", 0x7b8600u, 0x2d2, 4, w_game_mode_cart_equipment_item_list3, PK_LIST, NULL, 0, NULL, 0, 0, &lay_old38 },
    { "cart_equip_v5", 0x7b8a30u, 0x994, 4, w_game_mode_cart_item_list_equip_v5, PK_LIST, NULL, 0, NULL, 0, 0, &lay_v5 },
    { "cart_equip_v6", 0x7b8ce0u, 0xa0f, 4, w_game_mode_cart_item_list_equip_v6, PK_LIST, NULL, 0, NULL, 0, 0, &lay_v6 },
    { "cart_normal_v5", 0x7b9100u, 0x993, 4, w_game_mode_cart_item_list_normal_v5, PK_LIST, NULL, 0, NULL, 0, 0, &lay_n34 },
    { "equip2", 0x7bb600u, 0x295, 4, w_game_mode_equipment_item_list2, PK_LIST, NULL, 0, NULL, 0, 0, &lay_old34 },
    { "equip3", 0x7bb7e0u, 0x2d0, 4, w_game_mode_equipment_item_list3, PK_LIST, NULL, 0, NULL, 0, 0, &lay_old38 },
    { "equip4", 0x7bf850u, 0x901, 4, w_game_mode_equipment_item_list4, PK_LIST, NULL, 0, NULL, 0, 0, &lay_eq4 },
    { "equip_v5", 0x7bfa60u, 0x992, 4, w_game_mode_equipment_item_list_v5, PK_LIST, NULL, 0, NULL, 0, 0, &lay_v5 },
    { "equip_v6", 0x7bfe60u, 0xa0d, 4, w_game_mode_equipment_item_list_v6, PK_LIST, NULL, 0, NULL, 0, 0, &lay_v6 },
    { "equip_v7", 0x7c0290u, 0xb3c, 4, w_game_mode_equipment_item_list_v7, PK_LIST, NULL, 0, NULL, 0, 0, &lay_v7 },
    { "inventory_normal_v5", 0x7c0830u, 0x991, 4, w_game_mode_inventory_item_list_normal_v5, PK_LIST, NULL, 0, NULL, 0, 0, &lay_n34 },
    { "inventory_tab", 0x7c0990u, 0x908, 6, w_game_mode_inventory_tab, PK_DEFAULT, NULL, 0 },
    { "guild_equip_v5", 0x7be970u, 0x9d3, 0x1c, w_game_mode_store_item_list_equip_v5, PK_LIST, NULL, 0, NULL, 0, 0, &lay_nv5 },
    { "guild_equip_v6", 0x7beca0u, 0xa11, 0x1c, w_game_mode_store_item_list_equip_v6, PK_LIST, NULL, 0, NULL, 0, 0, &lay_nv6 },
    { "guild_normal_v5", 0x7bf030u, 0x9d2, 0x1c, w_game_mode_store_item_list_normal_v5, PK_LIST, NULL, 0, NULL, 0, 0, &lay_nn34 },
    { "store_equip2", 0x7d7850u, 0x296, 4, w_game_mode_zc_store_equipment_item_list2, PK_LIST, NULL, 0, NULL, 0, 0, &lay_old34 },
    { "store_equip3", 0x7d79c0u, 0x2d1, 4, w_game_mode_zc_store_equipment_item_list3, PK_LIST, NULL, 0, NULL, 0, 0, &lay_old38 },
    { "store_equip_v2", 0x7d7e20u, 0x976, 0x1c, w_game_mode_zc_store_item_list_equip_v2, PK_LIST, NULL, 0, NULL, 0, 0, &lay_neq4 },
    { "store_equip_v5", 0x7d8040u, 0x996, 0x1c, w_game_mode_zc_store_item_list_equip_v5, PK_LIST, NULL, 0, NULL, 0, 0, &lay_nv5 },
    { "store_equip_v6", 0x7d8370u, 0xa10, 0x1c, w_game_mode_zc_store_item_list_equip_v6, PK_LIST, NULL, 0, NULL, 0, 0, &lay_nv6 },
    { "store_normal_v5", 0x7d8a40u, 0x995, 0x1c, w_game_mode_zc_store_item_list_normal_v5, PK_LIST, NULL, 0, NULL, 0, 0, &lay_nn34 },
    { "split_equip_v8", 0x7d54c0u, 0xb39, 5, w_game_mode_zc_split_send_item_list_equip_v8, PK_LIST, NULL, 0, NULL, 0, 0, &lay_split8 },
    { "split_set", 0x7d6100u, 0xb08, 5, w_game_mode_zc_split_send_item_list_set, PK_SPLIT_SET, NULL, 0 },
    { "npc_sprite_change", 0x7cc9b0u, 0x1b0, 11, w_game_mode_zc_npc_sprite_change, PK_DEFAULT, NULL, 0 },
    { "npc_chat", 0x7cc9e0u, 0x2c1, 12, w_game_mode_zc_npc_chat, PK_TEXT, NULL, 200 },
    { "npc_market_open2", 0x7cca90u, 0xb7a, 4, w_game_mode_zc_npc_market_open2, PK_MARKET, NULL, 19 },
    { "npc_market_open", 0, 0x9d5, 4, w_game_mode_zc_npc_market_open, PK_MARKET, NULL, 15 },
    { "market_purchase_v2", 0x7cce00u, 0xb4e, 6, w_game_mode_zc_npc_market_purchase_result_v2, PK_MARKET, NULL, 10 },
    { "npcack_mapmove", 0x7cd0b0u, 0x91, 0x16, w_game_mode_zc_npcack_mapmove, PK_MAP, NULL, 0 },
    { "npcack_servermove", 0, 0x92, 0x1c, w_game_mode_zc_npcack_servermove, PK_MAP, NULL, 0 },
    { "servermove_domain", 0x7a00b0u, 0xac7, 0x9c, w_game_mode_zc_npcack_servermove_domain, PK_MAP, NULL, 0 },
    { "open_edit_dlg_str", 0x7cd540u, 0x1d4, 6, w_game_mode_zc_open_edit_dlg_str, PK_DEFAULT, NULL, 0 },
    { "say_dialog", 0, 0xb4, 8, w_game_mode_zc_say_dialog, PK_TEXT, NULL, 0x7fd, NULL, 0, 0, NULL, 0, 1 },
    { "select_dealtype", 0, 0xc4, 6, w_game_mode_zc_select_dealtype, PK_DEFAULT, NULL, 0 },
    { "clear_dialog", 0x7fa880u, 0x8d6, 2, w_game_mode_zc_clear_dialog, PK_DEFAULT, NULL, 0 },
    { "dynamic_npc_result", 0x7f08d0u, 0xa17, 6, w_game_mode_zc_dynamic_npc_create_result, PK_DEFAULT, NULL, 0 },
    { "wait_dialog", 0, 0xb5, 6, w_game_mode_zc_wait_dialog, PK_DEFAULT, NULL, 0 },
    { "close_dialog", 0, 0xb6, 6, w_game_mode_zc_close_dialog, PK_DEFAULT, NULL, 0 },
    { "menu_list", 0, 0xb7, 8, w_game_mode_zc_menu_list, PK_TEXT, NULL, 0x7fd },
    { "play_npc_bgm", 0x7cf9f0u, 0x7fe, 2, w_game_mode_zc_play_npc_bgm, PK_TEXT, NULL, 100 },
    { "sound", 0x7d4820u, 0x1d3, 0x23, w_game_mode_zc_sound, PK_DEFAULT, NULL, 0 },
    { "open_edit_dlg", 0, 0x142, 6, w_game_mode_zc_open_edit_dlg, PK_DEFAULT, NULL, 0 },
    { "dialog_text_align", 0x7fad40u, 0xba1, 3, w_dialog_text_align, PK_DEFAULT, NULL, 0, NULL, 0, 0, NULL, 1 },
    { "property_pet", 0x7d02c0u, 0x1a2, 0x25, w_game_mode_zc_property_pet, PK_DEFAULT, NULL, 0 },
    { "pet_act", 0x7cf780u, 0x1aa, 10, w_game_mode_zc_pet_act, PK_DEFAULT, EXTRA(pet_act_values) },
    { "pet_egg_list", 0x7cf660u, 0x1a6, 4, w_game_mode_zc_pet_egg_list, PK_DEFAULT, NULL, 0 },
    { "pet_evolution_result", 0x7cf930u, 0x9fc, 6, w_pet_evolution_result, PK_DEFAULT, NULL, 0, NULL, 0, 0, NULL, 0, 0, 1 },
    { "change_state_pet", 0x7b9d90u, 0x1a4, 11, w_game_mode_zc_change_state_pet, PK_DEFAULT, EXTRA(pet_state_values) },
    { "feed_pet", 0x7bd8f0u, 0x1a3, 7, w_game_mode_zc_feed, PK_DEFAULT, NULL, 0 },
    { "feed_homun", 0x7bd8f0u, 0x22f, 7, w_game_mode_zc_feed, PK_DEFAULT, NULL, 0 },
    { "change_state_homun", 0x7b9bb0u, 0x230, 12, w_game_mode_zc_change_state_homun, PK_DEFAULT, NULL, 0 },
    { "ho_par_change", 0x7bf420u, 0x7db, 8, w_ho_par_change, PK_PAR, EXTRA(ho_par_vars) },
    { "ho_par_change2", 0x7bf3c0u, 0xba5, 12, w_ho_par_change2, PK_PAR, EXTRA(ho_par_vars) },
    { "property_homun", 0x7cfbc0u, 0x22e, 0x49, w_game_mode_zc_property_homun, PK_DEFAULT, NULL, 0, NULL, 0, 0, NULL, 0, 1 },
    { "property_homun_v1", 0x7d00f0u, 0xb2f, 0x49, w_game_mode_zc_property_homun_no_accessory, PK_DEFAULT, NULL, 0, NULL, 0, 0, NULL, 0, 1 },
    { "property_homun_v2", 0x7cfd90u, 0xb76, 0x4d, w_game_mode_zc_property_homun_no_accessory2, PK_DEFAULT, NULL, 0 },
    { "property_homun_v3", 0x7cff40u, 0xba4, 0x55, w_game_mode_zc_property_homun_no_accessory3, PK_DEFAULT, NULL, 0 },
    { "mer_init", 0x7c5010u, 0x29b, 0x50, w_game_mode_zc_mer_init, PK_DEFAULT, NULL, 0 },
    { "mer_par_change", 0x7c5130u, 0x2a2, 8, w_mer_par_change, PK_PAR, EXTRA(mer_par_vars) },
    { "el_init", 0x7bade0u, 0x81d, 0x16, w_game_mode_zc_el_init, PK_DEFAULT, NULL, 0 },
    { "el_par_change", 0x7bae50u, 0x81e, 8, w_game_mode_zc_el_par_change, PK_PAR, EXTRA(el_par_vars) },
    { "booking_ack_delete", 0x7ce240u, 0x807, 4, w_booking_ack_delete, PK_DEFAULT, NULL, 0 },
    { "booking_ack_register", 0x7ce2e0u, 0x803, 4, w_booking_ack_register, PK_DEFAULT, NULL, 0 },
    { "booking_ack_search", 0x7ce3a0u, 0x805, 5, w_booking_ack_search, PK_DEFAULT, NULL, 0 },
    { "booking_notify_delete", 0x7ce480u, 0x80b, 6, w_booking_notify_delete, PK_DEFAULT, NULL, 0 },
    { "booking_notify_insert", 0x7ce4c0u, 0x809, 50, w_booking_notify_insert, PK_DEFAULT, NULL, 0 },
    { "booking_notify_update", 0x7ce510u, 0x80a, 18, w_booking_notify_update, PK_DEFAULT, NULL, 0 },
    { "seek_party", 0, 0x7dd, 34, w_game_mode_zc_seek_party, PK_DEFAULT, NULL, 0 },
    { "party_config", 0x7ce550u, 0x2c9, 3, w_party_config, PK_DEFAULT, NULL, 0 },
    { "join_req_ack_from_master", 0x7ce590u, 0xafa, 58, w_join_req_ack_from_master, PK_DEFAULT, NULL, 0 },
    { "req_master_to_join", 0, 0xae7, 38, w_req_master_to_join, PK_DEFAULT, NULL, 0 },
    { "req_join_group", 0, 0xfe, 30, w_req_join_group, PK_DEFAULT, NULL, 0 },
    { "party_join_ack", 0x7b2790u, 0xfd, 30, w_party_join_ack, PK_DEFAULT, EXTRA(join_ack_values) },
    { "party_join_req_ack", 0x7b2790u, 0x2c5, 30, w_party_join_ack, PK_DEFAULT, EXTRA(join_ack_values) },
    { "groupinfo_change_v2", 0x7d0f70u, 0x7d8, 8, w_groupinfo_change_v2, PK_DEFAULT, NULL, 0 },
    { "ack_make_group", 0, 0xfa, 3, w_ack_make_group, PK_DEFAULT, NULL, 0 },
    { "notify_hp_to_group", 0, 0x106, 10, w_game_mode_zc_notify_hp_to_group_m, PK_DEFAULT, NULL, 0 },
    { "notify_hp_to_group_r2", 0, 0x80e, 14, w_game_mode_zc_notify_hp_to_group_m_r2, PK_DEFAULT, NULL, 0 },
    { "notify_position_to_group", 0, 0x107, 10, w_game_mode_zc_notify_position_to_group_m, PK_DEFAULT, NULL, 0 },
    { "group_info_change", 0, 0x101, 6, w_group_info_change, PK_DEFAULT, NULL, 0 },
    { "group_info_change_fn", 0x7ddc90u, 0x101, 6, w_group_info_change, PK_DEFAULT, NULL, 0 },
    { "add_member_to_group3", 0x8156e0u, 0xa43, 0x55, w_game_mode_on_add_member_to_group, PK_DEFAULT, NULL, 0 },
    { "add_member_to_group4", 0x7f4510u, 0xae4, 0x59, w_game_mode_on_add_member_to_group2, PK_DEFAULT, NULL, 0 },
    { "party_member_state", 0x815bc0u, 0xab2, 7, w_game_mode_on_party_member_state, PK_DEFAULT, NULL, 0 },
    { "list_flag_clear", 0x815b50u, 0xaa7, 4, w_game_mode_on_list_flag_clear, PK_DEFAULT, NULL, 0 },
    { "room_newentry", 0, 0xd7, 0x11, w_game_mode_zc_room_newentry, PK_DEFAULT, NULL, 0 },
    { "refuse_enter_room", 0x7d0620u, 0xda, 3, w_game_mode_zc_refuse_enter_room, PK_DEFAULT, NULL, 0 },
    { "ack_create_chatroom", 0, 0xd6, 3, w_game_mode_zc_ack_create_chatroom, PK_DEFAULT, NULL, 0 },
    { "destroy_room", 0, 0xd8, 6, w_game_mode_zc_destroy_room, PK_DEFAULT, NULL, 0 },
    { "member_newentry", 0, 0xdc, 28, w_game_mode_zc_member_newentry, PK_DEFAULT, NULL, 0 },
    { "member_exit", 0, 0xdd, 29, w_game_mode_zc_member_exit, PK_DEFAULT, NULL, 0 },
    { "role_change", 0, 0xe1, 30, w_game_mode_zc_role_change, PK_DEFAULT, NULL, 0 },
    { "change_chatroom", 0, 0xdf, 0x11, w_game_mode_zc_change_chatroom, PK_DEFAULT, NULL, 0 },
    { "setting_whisper_pc2", 0x7d2600u, 0xae1, 28, w_game_mode_zc_setting_whisper_pc2, PK_DEFAULT, NULL, 0 },
    { "setting_whisper_pc", 0x7d27f0u, 0xd1, 4, w_game_mode_zc_setting_whisper_pc, PK_DEFAULT, NULL, 0 },
    { "setting_whisper_state", 0, 0xd2, 4, w_game_mode_zc_setting_whisper_state, PK_DEFAULT, NULL, 0 },
    { "notify_chat", 0x7c6710u, 0x8d, 8, w_game_mode_zc_notify_chat, PK_DEFAULT, NULL, 0 },
    { "notify_chat_gm", 0x7c6710u, 0x8d, 8, w_game_mode_zc_notify_chat, PK_DEFAULT, NULL, 0, NULL, 0, 0, NULL, 0, 1 },
    { "whisper_list", 0, 0xd4, 4, w_game_mode_zc_whisper_list, PK_DEFAULT, NULL, 0 },
    { "req_exchange_item2", 0, 0x1f4, 32, w_game_mode_zc_req_exchange_item2, PK_DEFAULT, NULL, 0 },
    { "req_exchange_item", 0, 0xe5, 26, w_game_mode_zc_req_exchange_item, PK_DEFAULT, NULL, 0 },
    { "conclude_exchange_item", 0, 0xec, 3, w_game_mode_zc_conclude_exchange_item, PK_DEFAULT, NULL, 0 },
    { "cancel_exchange_item", 0, 0xee, 2, w_game_mode_zc_cancel_exchange_item, PK_DEFAULT, NULL, 0 },
    { "exec_exchange_item", 0, 0xf0, 3, w_game_mode_zc_exec_exchange_item, PK_DEFAULT, NULL, 0 },
    { "exchange_item_undo", 0, 0xf1, 2, w_game_mode_zc_exchange_item_undo, PK_DEFAULT, NULL, 0 },
    { "req_add_friends", 0x7d11e0u, 0x207, 34, w_game_mode_zc_req_add_friends, PK_DEFAULT, NULL, 0 },
    { "delete_friends", 0x7d0dd0u, 0x20a, 10, w_game_mode_zc_delete_friends, PK_DEFAULT, NULL, 0 },
    { "req_baby", 0, 0x1f6, 34, w_game_mode_zc_req_baby, PK_DEFAULT, NULL, 0 },
    { "req_couple", 0, 0x1e2, 34, w_game_mode_zc_req_couple, PK_DEFAULT, NULL, 0 },
    { "add_exchange_item_opt", 0x7f8cd0u, 0xa96, 0x3d, w_game_mode_on_add_exchange_item_opt, PK_DEFAULT, NULL, 0 },
    { "add_exchange_item_grade", 0x7f8e90u, 0xb42, 0x3e, w_game_mode_on_add_exchange_item_grade, PK_DEFAULT, NULL, 0 },
    { "couple_name", 0x7face0u, 0x1e6, 26, w_game_mode_zc_couple_name, PK_DEFAULT, NULL, 0 },
    { "divorce", 0x7bad60u, 0x205, 26, w_game_mode_zc_divorce, PK_DEFAULT, NULL, 0 },
    { "baby_msg", 0, 0x216, 6, w_baby_msg, PK_DEFAULT, NULL, 0 },
    { "ack_give_manner_point", 0, 0x14a, 6, w_ack_give_manner_point, PK_DEFAULT, NULL, 0 },
    { "notify_manner_point_given", 0, 0x14b, 27, w_notify_manner_point_given, PK_DEFAULT, NULL, 0 },
};

static unsigned handler_values(const struct Handler *h)
{
    if (h->kind == PK_ENTRY)
        return (h->layout->type_off ? N_TYPES : 1) * N_JOBS * 2;
    if (h->kind == PK_PAR)
        return (unsigned)h->extra_count;
    if (h->kind == PK_LIST || h->kind == PK_SPLIT_SET)
        return 8;
    if (h->kind == PK_TEXT)
        return N_TEXTS;
    if (h->kind == PK_MARKET)
        return h->id == 0xb4e ? 28 : 4;
    if (h->kind == PK_MAP)
        return N_MAPS;
    return SWEEP_VALUES + h->extra_count;
}

static void build_packet(unsigned char *pkt, const struct Handler *h, unsigned aid, unsigned value, int fill)
{
    int i;
    current_fill = fill;
    memset(pkt, 0, PACKET_MAX);
    for (i = 2; i < h->length; i++)
        pkt[i] = fill == FILL_ZERO ? 0 : fill == FILL_ONES ? 0xff : (unsigned char)(i * 7 + 3);
    *(unsigned short *)pkt = h->id;
    drawn_job = drawn_jobs[(value + fill) % 5];
    if (h->kind == PK_LIST || h->kind == PK_SPLIT_SET) {
        unsigned ai = aid == AID_CASTER ? 0 : aid == AID_TARGET ? 1 : aid == AID_OWN ? 2 : 3;
        const char *name = h->kind == PK_SPLIT_SET && (value & 4) ? long_list_name : list_names[ai];
        short total;
        memset(pkt + 2, 0, PACKET_MAX - 2);
        if (h->kind == PK_SPLIT_SET) {
            total = (short)(5 + strlen(name));
            pkt[4] = (unsigned char)(value & 3);
            memcpy(pkt + 5, name, strlen(name));
        } else {
            const struct ListLayout *l = h->list;
            unsigned records = value & 3, egg = value >> 2, r;
            total = (short)(l->header + records * l->rec_size);
            if (l->header == 0x1c)
                strncpy((char *)pkt + 4, name, 24);
            if (l->header == 5)
                pkt[4] = (unsigned char)ai;
            for (r = 0; r < records; r++) {
                unsigned char *rec = pkt + l->header + r * l->rec_size;
                short index = (short)(r + 1);
                int item_id = egg ? 9005 + (int)r : 1201 + 100 * fill + (int)r;
                for (i = 0; i < l->rec_size; i++)
                    rec[i] = fill == FILL_ZERO ? 0 : fill == FILL_ONES ? 0xff : (unsigned char)(i * 7 + 3 + r * 13);
                memcpy(rec, &index, 2);
                memcpy(rec + 2, &item_id, 4);
                if (egg && l->damaged_off)
                    rec[l->damaged_off] = 1;
                if (egg && l->flags_off)
                    rec[l->flags_off] |= 2;
            }
        }
        memcpy(pkt + 2, &total, 2);
        memcpy(g_packetBuffer, pkt, PACKET_MAX);
        memcpy(oracle_addr(ORIG_PACKET_BUFFER), pkt, PACKET_MAX);
        return;
    }
    if (h->kind == PK_TEXT) {
        const char *text = dialog_text(value);
        size_t len = strlen(text);
        unsigned short total;
        int color = fill * 0x111111;
        if (len > (size_t)h->extra_count)
            len = (size_t)h->extra_count;
        memcpy(pkt + h->length, text, len);
        pkt[h->length + len] = 0;
        total = (unsigned short)(h->length + len + (h->id == 0x2c1 ? 1 : 0));
        if (h->length >= 8) {
            memcpy(pkt + 2, &total, 2);
            memcpy(pkt + 4, &aid, 4);
        }
        if (h->length >= 12)
            memcpy(pkt + 8, &color, 4);
        return;
    }
    if (h->kind == PK_MARKET) {
        static const short results[] = { -1, 0, 1, 2, 3, 9, 4 };
        unsigned records = value & 3, r;
        int rec_size = h->extra_count;
        unsigned short total = (unsigned short)(h->length + records * rec_size);
        memcpy(pkt + 2, &total, 2);
        if (h->length == 6)
            memcpy(pkt + 4, &results[(value >> 2) % 7], 2);
        for (r = 0; r < records; r++) {
            unsigned char *rec = pkt + h->length + r * rec_size;
            int item_id = 501 + 100 * fill + (int)r;
            for (i = 0; i < rec_size; i++)
                rec[i] = fill == FILL_ZERO ? 0 : fill == FILL_ONES ? 0xff : (unsigned char)(i * 7 + 3 + r * 13);
            memcpy(rec, &item_id, 4);
        }
        return;
    }
    if (h->kind == PK_MAP) {
        static const char *const maps[N_MAPS] = { "prontera.gat", "prontera", "geffen.gat", "prt_fild", "sixteen_chars_xy", "payon.rsw" };
        const char *map = maps[value % N_MAPS];
        short x = (short)(10 + 7 * fill), y = (short)(20 - 3 * fill);
        unsigned ip = 0x0100007fu + (unsigned)fill;
        short port = (short)(6900 + fill);
        memset(pkt + 2, 0, 16);
        memcpy(pkt + 2, map, strlen(map));
        memcpy(pkt + 0x12, &x, 2);
        memcpy(pkt + 0x14, &y, 2);
        if (h->length >= 0x1c) {
            memcpy(pkt + 0x16, &ip, 4);
            memcpy(pkt + 0x1a, &port, 2);
        }
        if (h->length == 0x9c) {
            memset(pkt + 0x1c, 0, 0x80);
            strcpy((char *)pkt + 0x1c, fill == FILL_ZERO ? "" : "example.domain.test");
        }
        return;
    }
    if (h->kind == PK_PAR) {
        unsigned ai = aid == AID_CASTER ? 0 : aid == AID_TARGET ? 1 : aid == AID_OWN ? 2 : 3;
        int pv = par_values[ai * FILL_COUNT + fill];
        unsigned short type = (unsigned short)value;
        memcpy(pkt + 2, &type, 2);
        *(int *)oracle_addr(ORIG_SESSION_AID) = g_session1.aid = (int)aid;
        if (h->id == 0xbc) {
            pkt[4] = (unsigned char)(ai & 1);
            pkt[5] = (unsigned char)pv;
        } else if (h->id == 0x141) {
            int bonus = pv ^ 0x55;
            memcpy(pkt + 2, &value, 4);
            memcpy(pkt + 6, &pv, 4);
            memcpy(pkt + 10, &bonus, 4);
        } else if (h->length == 12) {
            long long wide = pv;
            memcpy(pkt + 4, &wide, 8);
        } else {
            memcpy(pkt + 4, &pv, 4);
        }
        return;
    }
    if (h->kind == PK_EXP) {
        unsigned short var_id = (unsigned short)fill, exp_type = (unsigned short)(value & 1);
        int var_off = h->id == 0xacc ? 14 : 10;
        memcpy(pkt + 2, &aid, 4);
        if (h->id == 0xacc) {
            long long wide = (int)value;
            memcpy(pkt + 6, &wide, 8);
        } else {
            memcpy(pkt + 6, &value, 4);
        }
        memcpy(pkt + var_off, &var_id, 2);
        memcpy(pkt + var_off + 2, &exp_type, 2);
        return;
    }
    if (h->kind == PK_ENTRY) {
        const struct EntryLayout *l = h->layout;
        unsigned types = l->type_off ? N_TYPES : 1;
        unsigned char type = (unsigned char)entry_types[value % types];
        unsigned pet = value / (types * N_JOBS);
        short job = (short)entry_jobs[(value / types) % N_JOBS];
        short head = 0x2b;
        int name_len = fill == FILL_ZERO ? 0 : fill == FILL_PATTERN ? 4 : 15;
        sweep_job = job;
        if (l->type_off)
            pkt[l->type_off] = type;
        memcpy(pkt + l->aid_off, &aid, 4);
        memcpy(pkt + l->job_off, &job, 2);
        if (pet)
            memcpy(pkt + l->head_off, &head, 2);
        if (l->varlen) {
            short total = (short)(h->length + name_len);
            memcpy(pkt + 2, &total, 2);
            memcpy(pkt + h->length, "Entry name here", name_len);
        }
        return;
    }
    if (h->id == 0x1a2 || h->id == 0x22e || h->id == 0xb2f || h->id == 0xb76 || h->id == 0xba4 || h->id == 0x29b) {
        static const short fullness[] = { 1, 5, 10, 7, 0, 100 };
        const char *name = companion_names[value % N_COMPANION_NAMES];
        short level = (short)(h->id == 0x1a2 && (value & 1) ? 10000 + (int)value : 10 + (int)value);
        short full = fullness[value % 6], intimacy = (short)(100 * value), accessory = (short)(3 * value), job = (short)(1002 + fill);
        if (h->id == 0x29b) {
            memcpy(pkt + 2, &aid, 4);
            memcpy(pkt + 0x16, name, strlen(name) + 1);
            return;
        }
        memcpy(pkt + 2, name, strlen(name) + 1);
        pkt[0x1a] = (unsigned char)fill;
        memcpy(pkt + 0x1b, &level, 2);
        memcpy(pkt + 0x1d, &full, 2);
        memcpy(pkt + 0x1f, &intimacy, 2);
        if (h->id == 0x1a2) {
            memcpy(pkt + 0x21, &accessory, 2);
            memcpy(pkt + 0x23, &job, 2);
        }
        return;
    }
    if (h->id == 0x807 || h->id == 0x803 || h->id == 0x101 || h->id == 0xaa7 || h->id == 0x2c9 || h->id == 0xfa) {
        unsigned short v = (unsigned short)(h->id == 0x803 ? value % 4 : h->id == 0x2c9 ? value & 1 : value);
        if (h->length == 3)
            pkt[2] = (unsigned char)v;
        else if (h->length == 4)
            memcpy(pkt + 2, &v, 2);
        else
            memcpy(pkt + 2, &value, 4);
        return;
    }
    if (h->id == 0x805 || h->id == 0x809 || h->id == 0x80a || h->id == 0x80b) {
        unsigned count = h->id == 0x805 ? value & 3 : 1, r;
        unsigned short total = (unsigned short)(5 + count * ORIG_POST_SIZE);
        unsigned char *post = pkt + (h->id == 0x805 ? 5 : 2);
        int id = 100 + (int)value;
        if (h->id == 0x805) {
            memcpy(pkt + 2, &total, 2);
            pkt[4] = (unsigned char)(fill & 1);
        }
        if (h->id == 0x80b || h->id == 0x80a) {
            memcpy(pkt + 2, &id, 4);
            if (h->id == 0x80a)
                for (r = 0; r < PARTY_BOOKING_JOBS; r++)
                    *(short *)(pkt + 6 + 2 * r) = (short)((value + r) % 3);
            return;
        }
        for (r = 0; r < count; r++, post += ORIG_POST_SIZE) {
            const char *leader = companion_names[(value + r) % N_COMPANION_NAMES];
            int posted = 1000 * fill, pid = id + (int)r, j;
            short level = (short)value, map = (short)fill;
            memset(post, 0, ORIG_POST_SIZE);
            memcpy(post, &pid, 4);
            memcpy(post + 4, leader, strlen(leader));
            memcpy(post + 0x1c, &posted, 4);
            memcpy(post + 0x20, &level, 2);
            memcpy(post + 0x22, &map, 2);
            for (j = 0; j < PARTY_BOOKING_JOBS; j++)
                *(short *)(post + 0x24 + 2 * j) = (short)((value + r + j) % 3);
        }
        return;
    }
    if (h->id == 0x7dd) {
        const char *name = companion_names[value % N_COMPANION_NAMES];
        int job = 10 * (int)value, v = fill;
        memcpy(pkt + 2, name, strlen(name) + 1);
        memcpy(pkt + 0x1a, &job, 4);
        memcpy(pkt + 0x1e, &v, 4);
        return;
    }
    if (h->id == 0xafa || h->id == 0xfd || h->id == 0x2c5 || h->id == 0xfe || h->id == 0xae7) {
        const char *name = companion_names[value % N_COMPANION_NAMES];
        const char *other = companion_names[(value + 1) % N_COMPANION_NAMES];
        if (h->id == 0xfe) {
            memcpy(pkt + 2, &value, 4);
            memcpy(pkt + 6, name, strlen(name) + 1);
            return;
        }
        if (h->id == 0xae7) {
            int request = 500 + (int)value;
            unsigned short level = (unsigned short)(10 + fill), job = (unsigned short)value;
            memcpy(pkt + 2, &aid, 4);
            memcpy(pkt + 6, &request, 4);
            memcpy(pkt + 0xa, name, strlen(name) + 1);
            memcpy(pkt + 0x22, &level, 2);
            memcpy(pkt + 0x24, &job, 2);
            return;
        }
        memcpy(pkt + 2, name, strlen(name) + 1);
        if (h->id == 0xafa) {
            memcpy(pkt + 0x1a, other, strlen(other) + 1);
            memcpy(pkt + 0x32, &aid, 4);
            memcpy(pkt + 0x36, &value, 4);
        } else {
            memcpy(pkt + 0x1a, &value, 4);
        }
        return;
    }
    if (h->id == 0x7d8) {
        int exp = (int)(value % 4);
        memcpy(pkt + 2, &exp, 4);
        pkt[6] = (unsigned char)fill;
        pkt[7] = (unsigned char)(value & 1);
        return;
    }
    if (h->id == 0x106 || h->id == 0x80e || h->id == 0x107) {
        int hp = (int)value, max = 100 * fill;
        memcpy(pkt + 2, &aid, 4);
        if (h->id == 0x80e) {
            memcpy(pkt + 6, &hp, 4);
            memcpy(pkt + 0xa, &max, 4);
        } else {
            *(short *)(pkt + 6) = (short)hp;
            *(short *)(pkt + 8) = (short)(h->id == 0x107 ? fill : max);
        }
        return;
    }
    if (h->id == 0xa43 || h->id == 0xae4) {
        static const char *const maps[] = { "prontera.gat", "geffen.gat", "prt_fild" };
        unsigned off = h->id == 0xae4 ? 4 : 0;
        int gid = 777, role = (int)((value >> 1) & 1);
        unsigned short job = 4, level = 50;
        const char *party = companion_names[(value / 4) % N_COMPANION_NAMES];
        const char *name = companion_names[value % N_COMPANION_NAMES];
        memcpy(pkt + 2, &aid, 4);
        if (off)
            memcpy(pkt + 6, &gid, 4);
        memcpy(pkt + 6 + off, &role, 4);
        memcpy(pkt + 0xa + off, &job, 2);
        memcpy(pkt + 0xc + off, &level, 2);
        pkt[0x12 + off] = (unsigned char)(value & 3);
        memset(pkt + 0x13 + off, 0, 64);
        memcpy(pkt + 0x13 + off, party, strlen(party));
        memcpy(pkt + 0x2b + off, name, strlen(name));
        memcpy(pkt + 0x43 + off, maps[fill], strlen(maps[fill]));
        pkt[0x53 + off] = (unsigned char)(fill & 1);
        pkt[0x54 + off] = (unsigned char)(fill >> 1);
        return;
    }
    if (h->id == 0xd7 || h->id == 0xdf) {
        const char *title = dialog_text(value % N_TEXTS);
        size_t len = strlen(title);
        short total, max_users = 20, users = (short)value;
        int room_id = (int)(value % 4);
        if (len > 100)
            len = 100;
        total = (short)(0x11 + len);
        memcpy(pkt + 2, &total, 2);
        memcpy(pkt + 4, &aid, 4);
        memcpy(pkt + 8, &room_id, 4);
        memcpy(pkt + 0xc, &max_users, 2);
        memcpy(pkt + 0xe, &users, 2);
        pkt[0x10] = (unsigned char)fill;
        memcpy(pkt + 0x11, title, len);
        return;
    }
    if (h->id == 0xda || h->id == 0xd6 || h->id == 0xec || h->id == 0xf0) {
        pkt[2] = (unsigned char)value;
        return;
    }
    if (h->id == 0xd8 || h->id == 0x216 || h->id == 0x14a) {
        memcpy(pkt + 2, &value, 4);
        return;
    }
    if (h->id == 0xdc || h->id == 0xdd || h->id == 0xe1 || h->id == 0xae1 || h->id == 0x14b || h->id == 0x205 || h->id == 0x1e6) {
        const char *name = chat_names[value % N_COMPANION_NAMES];
        unsigned off = h->id == 0xe1 ? 6 : h->id == 0x14b ? 3 : h->id == 0xdc || h->id == 0xdd ? 4 : 2;
        short users = (short)value;
        int role = (int)value;
        memset(pkt + off, 0, 24);
        memcpy(pkt + off, name, strlen(name));
        if (h->id == 0xdc || h->id == 0xdd)
            memcpy(pkt + 2, &users, 2);
        if (h->id == 0xdd)
            pkt[0x1c] = (unsigned char)(value % 3);
        if (h->id == 0xe1)
            memcpy(pkt + 2, &role, 4);
        if (h->id == 0xae1) {
            pkt[0x1a] = (unsigned char)(value & 1);
            pkt[0x1b] = (unsigned char)(value >> 1);
        }
        if (h->id == 0x14b)
            pkt[2] = (unsigned char)((value >> 3) & 1);
        return;
    }
    if (h->id == 0xd1 || h->id == 0xd2) {
        pkt[2] = (unsigned char)(value & 1);
        pkt[3] = (unsigned char)(value >> 1);
        return;
    }
    if (h->id == 0xd4) {
        unsigned count = value & 3, r;
        short total = (short)(4 + 24 * count);
        memcpy(pkt + 2, &total, 2);
        for (r = 0; r < count; r++) {
            const char *name = chat_names[(value + r) % N_COMPANION_NAMES];
            memset(pkt + 4 + 24 * r, 0, 24);
            memcpy(pkt + 4 + 24 * r, name, strlen(name));
        }
        return;
    }
    if (h->id == 0x8d) {
        static const char *const texts[N_CHAT_TEXTS] = { "", "hello", "bad words here", "Hello %s %d", long_text_600, "x" };
        const char *text = texts[value % N_CHAT_TEXTS];
        size_t len = strlen(text);
        unsigned short total = (unsigned short)(8 + len);
        memcpy(pkt + 2, &total, 2);
        memcpy(pkt + 4, &aid, 4);
        memcpy(pkt + 8, text, len);
        return;
    }
    if (h->id == 0xe5 || h->id == 0x1f4) {
        const char *name = chat_names[value % N_COMPANION_NAMES];
        int char_id = 100 + (int)value;
        short level = (short)(10 * fill);
        memset(pkt + 2, 0, 24);
        memcpy(pkt + 2, name, strlen(name));
        if (h->id == 0x1f4) {
            memcpy(pkt + 0x1a, &char_id, 4);
            memcpy(pkt + 0x1e, &level, 2);
        }
        return;
    }
    if (h->id == 0x207 || h->id == 0x20a || h->id == 0x1e2 || h->id == 0x1f6) {
        const char *name = chat_names[value % N_COMPANION_NAMES];
        unsigned char_id = h->id == 0x20a ? value : 100 + value;
        memcpy(pkt + 2, &aid, 4);
        memcpy(pkt + 6, &char_id, 4);
        if (h->id != 0x20a) {
            memset(pkt + 0xa, 0, 24);
            memcpy(pkt + 0xa, name, strlen(name));
        }
        return;
    }
    if (h->id == 0xa96 || h->id == 0xb42) {
        int item_id = value == 0 ? 0 : 501 + (int)value, amount = (value & 1) ? -1 - (int)value : (int)value * 7;
        int location = 2 << fill, i;
        unsigned short look = (unsigned short)(value * 3);
        unsigned cards = h->id == 0xa96 ? 0xe : 0xd, options = h->id == 0xa96 ? 0x1e : 0x1d;
        memcpy(pkt + 2, &item_id, 4);
        pkt[6] = (unsigned char)(fill + 2);
        memcpy(pkt + 7, &amount, 4);
        pkt[0xb] = (unsigned char)(value & 1);
        pkt[0xc] = (unsigned char)(fill == FILL_ONES);
        for (i = 0; i < 4; i++) {
            int card = 4001 + i * fill;
            memcpy(pkt + cards + 4 * i, &card, 4);
        }
        for (i = 0; i < 5; i++) {
            unsigned short id = (unsigned)i < value % 6 ? (unsigned short)(100 + i) : 0;
            short v = (short)(7 * i);
            memcpy(pkt + options + 5 * i, &id, 2);
            memcpy(pkt + options + 5 * i + 2, &v, 2);
            pkt[options + 5 * i + 4] = (unsigned char)fill;
        }
        if (h->id == 0xa96) {
            pkt[0xd] = (unsigned char)fill;
            memcpy(pkt + 0x37, &location, 4);
            memcpy(pkt + 0x3b, &look, 2);
        } else {
            memcpy(pkt + 0x36, &location, 4);
            memcpy(pkt + 0x3a, &look, 2);
            pkt[0x3c] = (unsigned char)fill;
            pkt[0x3d] = (unsigned char)(value & 3);
        }
        return;
    }
    if (h->id == 0x1a6) {
        unsigned count = value & 3, r;
        short total = (short)(4 + 2 * count);
        memcpy(pkt + 2, &total, 2);
        for (r = 0; r < count; r++) {
            short index = (short)(value + r);
            memcpy(pkt + 4 + 2 * r, &index, 2);
        }
        return;
    }
    if (h->id == 0x9fc) {
        memcpy(pkt + 2, &value, 4);
        return;
    }
    if (h->id == 0x1a4) {
        int type = (int)(value % 7), v = type == 6 ? (int)(value / 7) : (int)(value / 7) * 60;
        pkt[2] = (unsigned char)type;
        memcpy(pkt + 3, &aid, 4);
        memcpy(pkt + 7, &v, 4);
        return;
    }
    if (h->id == 0x1a3 || h->id == 0x22f) {
        int food_id = 501 + 100 * fill + (int)(value / 2);
        pkt[2] = (unsigned char)(value & 1);
        memcpy(pkt + 3, &food_id, 4);
        return;
    }
    if (h->id == 0x230) {
        int v = (int)(value / 5) * 10;
        pkt[2] = (unsigned char)fill;
        pkt[3] = (unsigned char)(value % 5);
        memcpy(pkt + 4, &aid, 4);
        memcpy(pkt + 8, &v, 4);
        return;
    }
    if (h->length >= 6)
        memcpy(pkt + 2, &aid, 4);
    if (h->arg_byte)
        pkt[2] = (unsigned char)value;
    if (h->id == 0xa17)
        memcpy(pkt + 2, &value, 4);
    if (h->id == 0x1d3) {
        static const char *const names[] = { "snd_a", "snd_b", "snd_c" };
        unsigned interval = 500 + 1000 * (unsigned)fill;
        memset(pkt + 2, 0, 24);
        strcpy((char *)pkt + 2, names[(value / 3) % 3]);
        pkt[0x1a] = (unsigned char)(value % 3);
        memcpy(pkt + 0x1b, &interval, 4);
        memcpy(pkt + 0x1f, &aid, 4);
        return;
    }
    if (h->kind == PK_NAMES) {
        const char *names[5] = { "ann", "party1", "guild22", "pos", "x" };
        for (i = 0; i < 4; i++) {
            memset(pkt + 6 + i * 24, 0, 24);
            if (fill != FILL_ZERO)
                strcpy((char *)pkt + 6 + i * 24, names[i]);
        }
        memcpy(pkt + 102, &value, 4);
        return;
    }
    if (h->length >= 10)
        memcpy(pkt + 6, &value, 4);
    else if (h->length >= 8)
        memcpy(pkt + 6, &value, 2);
    else if (h->length >= 7)
        pkt[6] = (unsigned char)value;
}

static void seed_loop_sounds(void)
{
    static const char *const names[] = { "snd_a", "snd_b" };
    int i;
    memset(orig_loop_sounds, 0, sizeof orig_loop_sounds);
    memset(lift_loop_sounds, 0, sizeof lift_loop_sounds);
    lift_mode.loop_sounds.first = lift_mode.loop_sounds.last = lift_loop_sounds;
    lift_mode.loop_sounds.end = lift_loop_sounds + 4;
    for (i = 0; i < 2; i++) {
        unsigned char *s = orig_loop_sounds[i];
        struct LoopSound sound;
        strcpy((char *)s, names[i]);
        *(unsigned *)(s + 0x10) = (unsigned)strlen(names[i]);
        *(unsigned *)(s + 0x14) = ORIG_STD_STRING_SSO - 1;
        *(unsigned *)(s + 0x18) = 1000 + (unsigned)i;
        *(unsigned *)(s + 0x1c) = 500 * ((unsigned)i + 1);
        *(unsigned *)(s + 0x20) = 77;
        string_init(&sound.name);
        string_assign_cstr(&sound.name, names[i]);
        sound.npc_aid = 1000 + (unsigned)i;
        sound.interval = 500 * ((unsigned)i + 1);
        sound.next_tick = 77;
        vec_push(&lift_mode.loop_sounds, &sound, sizeof sound);
    }
    *(void **)(orig_mode + ORIG_MODE_LOOP_SOUNDS) = orig_loop_sounds[0];
    *(void **)(orig_mode + ORIG_MODE_LOOP_SOUNDS + 4) = orig_loop_sounds[2];
    *(void **)(orig_mode + ORIG_MODE_LOOP_SOUNDS + 8) = (unsigned char *)orig_loop_sounds + sizeof orig_loop_sounds;
}

static void seed_own_states(void)
{
    static const int indexes[3] = { 5, 0xc8, 7 };
    int i;
    memset(orig_own_states, 0, sizeof orig_own_states);
    memset(lift_own_states, 0, sizeof lift_own_states);
    for (i = 0; i < 3; i++) {
        *(int *)orig_own_states[i] = indexes[i];
        *(int *)(orig_own_states[i] + 4) = 1000 + i;
        lift_own_states[i].index = indexes[i];
        lift_own_states[i].end_tick = 1000 + (unsigned)i;
    }
    lift_own_count = 3;
    *(void **)oracle_addr(ORIG_OWN_STATES_BEGIN) = orig_own_states[0];
    *(void **)oracle_addr(ORIG_OWN_STATES_END) = orig_own_states[3];
}

static void seed_packets(int option)
{
    unsigned i;
    memset(oracle_addr(ORIG_SESSION), 0, ORIG_SESSION_SIZE);
    seed_all(option);
    *(int *)oracle_addr(ORIG_EFFECT_ON) = 1;
    g_session1.effectOn = 1;
    g_serviceType = current_handler && current_handler->service_type && option ? 10 : 0;
    *(int *)oracle_addr(ORIG_SERVICE_TYPE) = g_serviceType;
    *(unsigned *)(orig_player + ORIG_PLAYER_TARGET) = AID_TARGET;
    for (i = 0; i < sizeof strings / sizeof *strings; i++)
        *(unsigned *)(regions[R_EFFECT].orig + strings[i].orig_off + 20) = ORIG_STD_STRING_SSO - 1;
    ((struct Player *)lift_player)->target_aid = AID_TARGET;
    instant_var_set = 0;
    lift_instant_var_got = 0;
    lift_instant_var.value = 0x55;
    *(int *)oracle_addr(ORIG_SESSION_AID) = AID_OWN;
    g_session1.aid = AID_OWN;
    *(int *)oracle_addr(ORIG_PVP_RULE) = option;
    g_session1.map_rule_flags[3] = option;
    *(int *)oracle_addr(ORIG_REPLAY_PLAYING) = 0;
    memset(orig_new_actor, 0, sizeof orig_new_actor);
    memset(&lift_new_actor, 0, sizeof lift_new_actor);
    orig_actor_list_head[0] = orig_actor_list_head[1] = (unsigned)(UINT_PTR)orig_actor_list_head;
    orig_actor_list_head[2] = 0;
    *(void **)(orig_world + ORIG_WORLD_ACTORS) = orig_actor_list_head;
    *(unsigned *)(orig_world + ORIG_WORLD_ACTORS + 4) = 0;
    fake_node_count = 0;
    windows_present = option;
    *(void **)oracle_addr(ORIG_WND_STATUS) = option ? orig_status_wnd : NULL;
    *(void **)oracle_addr(ORIG_WND_GROUP) = option ? orig_group_wnd : NULL;
    *(void **)oracle_addr(ORIG_WND_BASIC_INFO) = orig_basic_info_wnd;
    g_windowMgr.status_wnd = option ? &lift_status_wnd : NULL;
    g_windowMgr.messenger_group_wnd = option ? &lift_group_wnd : NULL;
    g_windowMgr.basic_info_wnd = &lift_basic_info_wnd;
    lift_basic_info_wnd.height = 77;
    *(int *)(orig_basic_info_wnd + ORIG_WND_HEIGHT) = 77;
    *(int *)(orig_basic_info_wnd + ORIG_WND_BASE_BAR) = 1;
    *(int *)(orig_basic_info_wnd + ORIG_WND_JOB_BAR) = 2;
    *(void **)orig_status_wnd = *(void **)orig_basic_info_wnd = *(void **)orig_group_wnd = *(void **)orig_found_wnd = orig_window_vt;
    *(int *)(orig_mode + ORIG_MODE_EXCHANGE_OPEN) = option;
    lift_mode.exchange_open = option;
    *(int *)(orig_mode + ORIG_MODE_STATUS_PENDING) = 1;
    lift_mode.status_request_pending = 1;
    fake_abr_used[0] = fake_abr_used[1] = 0;
    adv_pending = 0;
#define SEED(orig, field, v) (*(int *)oracle_addr(orig) = (v), g_session1.field = (v))
#define SEED64(orig, field, v) (*(long long *)oracle_addr(orig) = (v), g_session1.field = (v))
    SEED(ORIG_HP, hp, 100);
    SEED(ORIG_MAX_HP, maxHp, 1000);
    SEED(ORIG_SP, sp, 50);
    SEED(ORIG_MAX_SP, maxSp, 200);
    SEED(ORIG_WEIGHT, weight, option ? 950 : 600);
    SEED(ORIG_MAX_WEIGHT, maxWeight, 1000);
    SEED(ORIG_WEIGHT_PENALTY, weightPenaltyPercent, 50);
    SEED(ORIG_ZENY, zeny, 7);
    SEED(ORIG_PET_FULLNESS, petFullness, option ? 60 : 20);
    SEED(ORIG_PET_INTIMACY_VAL, petIntimacy, 500);
    SEED(ORIG_PET_JOB, petJob, 1002);
    SEED(ORIG_BASE_LEVEL, charBaseLevel, 5);
    SEED(ORIG_JOB_LEVEL, jobLevel, 3);
    SEED64(ORIG_BASE_EXP, baseExp, 1000);
    SEED64(ORIG_NEXT_BASE_EXP, nextBaseExp, 5000);
    SEED64(ORIG_JOB_EXP, jobExp, 10);
    SEED64(ORIG_NEXT_JOB_EXP, nextJobExp, 300);
    for (i = 0; i < 6; i++)
        *(int *)oracle_addr(ORIG_STATS + 4 * i) = g_session1.stats[i] = (int)i + 1;
    SEED(ORIG_HATCHED_EGG, hatchedEggIndex, option ? 3 : 4);
    *(unsigned char *)oracle_addr(ORIG_PET_AUTO_FEED) = 1;
    g_session1.petAutoFeed = 1;
    *(void **)orig_storage_wnd = orig_window_vt;
    *(int *)(orig_storage_wnd + ORIG_WND_TITLE_CTRL) = 1;
    *(void **)orig_generic_wnd = *(void **)orig_helper_wnd = orig_window_vt;
    *(void **)(orig_found_wnd + ORIG_WND_ALIGN_CTRL) = orig_found_wnd;
    *(void **)oracle_addr(ORIG_WND_SAY_HELPER) = option ? orig_helper_wnd : NULL;
    g_windowMgr.say_dialog_wnd = option ? &lift_helper_wnd : NULL;
    *(void **)oracle_addr(ORIG_WND_REPLAY_CONTROL) = orig_generic_wnd;
    g_windowMgr.replay_reccontrol_wnd = &lift_generic_wnd;
    *(int *)oracle_addr(ORIG_WND_MODAL_RESULT) = 0;
    g_windowMgr.modal_result = 0;
    memset(orig_replay, 0, sizeof orig_replay);
    memset(&lift_replay, 0, sizeof lift_replay);
    *(int *)(orig_replay + 0xc) = lift_replay.state = current_handler && current_handler->orig ? (option ? 2 : 7) : 0;
    pending_count = 0;
    memset(orig_sound_mgr, 0, sizeof orig_sound_mgr);
    *(void **)oracle_addr(ORIG_SOUND_MGR_PTR) = orig_sound_mgr;
    *(int *)(orig_sound_mgr + ORIG_SOUND_BGM_ON) = g_sound_settings.bgm_on = option;
    g_sound_settings.bgm_overridden = 0;
    seed_loop_sounds();
    seed_own_states();
    strcpy((char *)oracle_addr(ORIG_MAP_NAME), "prontera");
    memset(g_session1.mapName, 0, sizeof g_session1.mapName);
    strcpy(g_session1.mapName, "prontera");
    SEED(ORIG_PLAYER_POS_X, playerPosX, 3);
    SEED(ORIG_PLAYER_POS_Y, playerPosY, 4);
    SEED(ORIG_AUTO_SAVE_CHAT, autoSaveChat, option);
    SEED(ORIG_HIDE_CHAT, hideChat, option);
    SEED(ORIG_HIDE_CHAT_GUILD, hideChatAndGuild, 0);
    SEED(ORIG_HOMUN_AID, homunAid, option ? (int)AID_TARGET : (int)AID_UNKNOWN);
    SEED(ORIG_ELEMENTAL_AID, elementalAid, option ? (int)AID_TARGET : -1);
    SEED(ORIG_MERC_PRESENT, mercPresent, option);
    SEED(ORIG_HOMUN_AI_STATE + 4, homun_ai_state[1], option ? 5 : 0);
    SEED(ORIG_PET_PREV_FULLNESS, petPrevFullness, 40);
    memset(orig_pet_info_wnd, 0, sizeof orig_pet_info_wnd);
    memset(orig_homun_info_wnd, 0, sizeof orig_homun_info_wnd);
    memset(orig_homun_skill_wnd, 0, sizeof orig_homun_skill_wnd);
    *(void **)orig_pet_info_wnd = *(void **)orig_homun_info_wnd = *(void **)orig_homun_skill_wnd = orig_window_vt;
    *(void **)(orig_pet_info_wnd + ORIG_WND_NAME_CTRL) = orig_name_ctrl;
    *(void **)orig_name_ctrl = orig_ctrl_vt;
    *(void **)oracle_addr(ORIG_WND_PET_INFO) = option ? orig_pet_info_wnd : NULL;
    *(void **)oracle_addr(ORIG_WND_HOMUN_INFO) = option ? orig_homun_info_wnd : NULL;
    *(void **)oracle_addr(ORIG_WND_HOMUN_SKILL) = option ? orig_homun_skill_wnd : NULL;
    g_windowMgr.pet_info_wnd = option ? &lift_pet_info_wnd : NULL;
    g_windowMgr.homun_info_wnd = option ? &lift_homun_info_wnd : NULL;
    g_windowMgr.homun_skill_list_wnd = option ? &lift_homun_skill_wnd : NULL;
    SEED(ORIG_OWN_AID, ownAid, (int)AID_OWN);
    SEED(ORIG_PARTY_LEADER, partyLeader, option);
    *(void **)oracle_addr(ORIG_WND_CHAT_ROOM) = option != (current_handler && current_handler->id == 0x8d && current_handler->service_type) ? orig_generic_wnd : NULL;
    *(void **)oracle_addr(ORIG_WND_PARTY_SETTING) = option ? orig_generic_wnd : NULL;
    g_windowMgr.party_setting_wnd = option ? &lift_generic_wnd : NULL;
    *(void **)oracle_addr(ORIG_REQUEST_MGR_PTR) = orig_request_mgr;
    *(void **)oracle_addr(ORIG_AGENCY_PTR) = orig_agency;
    g_requestJoinPartyMgr = (struct UIRequestJoinPartyMgr *)lift_request_mgr;
    g_adventurerAgency = (struct AdventurerAgencyMgr *)lift_agency;
    memset(oracle_addr(ORIG_PARTY_NAME), 0, ORIG_STD_STRING);
    *(unsigned *)((unsigned char *)oracle_addr(ORIG_PARTY_NAME) + 0x14) = ORIG_STD_STRING_SSO - 1;
    string_init(&g_session1.partyName);
    strcpy((char *)orig_mode + ORIG_MODE_MAP_NAME, "prontera.rsw");
    memset(lift_mode.map_name, 0, sizeof lift_mode.map_name);
    strcpy(lift_mode.map_name, "prontera.rsw");
    SEED(ORIG_REFUSE_TRADES, refuseTrades, option);
    *(void **)oracle_addr(ORIG_WND_EXCHANGE) = option ? orig_generic_wnd : NULL;
    g_windowMgr.exchange_wnd = option ? &lift_generic_wnd : NULL;
    *(void **)oracle_addr(ORIG_CURRENT_MODE_PTR) = orig_mode;
    g_windowMgr.chat_room_wnd = option != (current_handler && current_handler->id == 0x8d && current_handler->service_type) ? (struct UIWindow *)&lift_chat_room_wnd : NULL;
    memset(orig_generic_wnd + ORIG_WND_ROOM, 0, ORIG_ROOM_SIZE);
    strcpy((char *)orig_generic_wnd + ORIG_WND_ROOM, "room");
    *(unsigned *)(orig_generic_wnd + ORIG_WND_ROOM + 0x10) = 4;
    *(unsigned *)(orig_generic_wnd + ORIG_WND_ROOM + 0x14) = ORIG_STD_STRING_SSO - 1;
    *(unsigned *)(orig_generic_wnd + ORIG_WND_ROOM + 0x18 + 0x14) = ORIG_STD_STRING_SSO - 1;
    *(int *)(orig_generic_wnd + ORIG_WND_ROOM + 0x30) = 1;
    *(int *)(orig_generic_wnd + ORIG_WND_ROOM + 0x34) = 3;
    *(int *)(orig_generic_wnd + ORIG_WND_ROOM + 0x38) = 20;
    *(int *)(orig_generic_wnd + ORIG_WND_ROOM + 0x3c) = 5;
    memset(&lift_chat_room_wnd, 0, sizeof lift_chat_room_wnd);
    lift_chat_room_wnd.frame.base.ops = &lift_window_ops;
    string_init(&lift_chat_room_wnd.room.title);
    string_init(&lift_chat_room_wnd.room.password);
    string_assign_cstr(&lift_chat_room_wnd.room.title, "room");
    lift_chat_room_wnd.room.type = 1;
    lift_chat_room_wnd.room.users = 3;
    lift_chat_room_wnd.room.max_users = 20;
    lift_chat_room_wnd.room.room_id = 5;
    memset(orig_mode + ORIG_MODE_OWN_ROOM, 0, ORIG_ROOM_SIZE);
    strcpy((char *)orig_mode + ORIG_MODE_OWN_ROOM, "own");
    *(unsigned *)(orig_mode + ORIG_MODE_OWN_ROOM + 0x10) = 3;
    *(unsigned *)(orig_mode + ORIG_MODE_OWN_ROOM + 0x14) = ORIG_STD_STRING_SSO - 1;
    *(unsigned *)(orig_mode + ORIG_MODE_OWN_ROOM + 0x18 + 0x14) = ORIG_STD_STRING_SSO - 1;
    *(int *)(orig_mode + ORIG_MODE_OWN_ROOM + 0x30) = 2;
    *(int *)(orig_mode + ORIG_MODE_OWN_ROOM + 0x38) = 10;
    *(int *)(orig_mode + ORIG_MODE_OWN_ROOM + 0x3c) = 9;
    string_init(&lift_mode.own_chat_room.title);
    string_init(&lift_mode.own_chat_room.password);
    string_assign_cstr(&lift_mode.own_chat_room.title, "own");
    lift_mode.own_chat_room.type = 2;
    lift_mode.own_chat_room.users = 0;
    lift_mode.own_chat_room.max_users = 10;
    lift_mode.own_chat_room.room_id = 9;
    memset(orig_mode + ORIG_MODE_WHISPER_NAME, 0, ORIG_STD_STRING);
    strcpy((char *)orig_mode + ORIG_MODE_WHISPER_NAME, "Whisp");
    *(unsigned *)(orig_mode + ORIG_MODE_WHISPER_NAME + 0x10) = 5;
    *(unsigned *)(orig_mode + ORIG_MODE_WHISPER_NAME + 0x14) = ORIG_STD_STRING_SSO - 1;
    string_init(&lift_mode.whisper_setting_name);
    string_assign_cstr(&lift_mode.whisper_setting_name, "Whisp");
    memset(orig_mode + ORIG_MODE_EXCHANGE_PARTNER, 0, ORIG_STD_STRING);
    *(unsigned *)(orig_mode + ORIG_MODE_EXCHANGE_PARTNER + 0x14) = ORIG_STD_STRING_SSO - 1;
    string_init(&lift_mode.exchange_partner);
    memset(orig_mode + ORIG_MODE_COUPLE_NAME, 0, 24);
    memset(lift_mode.couple_name, 0, sizeof lift_mode.couple_name);
    for (i = 0; i < 4; i++) {
        static const unsigned owners[4] = { AID_CASTER, AID_TARGET, AID_UNKNOWN, 0 };
        orig_room_owners[i] = lift_room_owners[i].aid = owners[i];
        lift_room_owners[i].room_id = i;
    }
    *(int *)(orig_mode + ORIG_MODE_CHAT_PENDING) = lift_mode.chat_room_pending = 1;
    *(int *)(orig_mode + ORIG_MODE_EXCHANGE_PENDING) = lift_mode.exchange_add_pending = 1;
    memset(orig_generic_wnd + ORIG_COLLECTION_PET, 0, 0x40);
    memset(&lift_pet_details, 0, sizeof lift_pet_details);
#undef SEED
#undef SEED64
}

static void __fastcall o_set_counter_packets(void *counters, void *edx, int key, unsigned lo, int hi)
{
    (void)edx;
    if (counters == oracle_addr(ORIG_INSTANT_VARS)) {
        event("instvar get %d", key);
        orig_instant_var = ((long long)hi << 32) | lo;
        instant_var_set = 1;
        return;
    }
    event("counter %s %d %lld", who((char *)counters - ORIG_ACTOR_COUNTERS), key, ((long long)hi << 32) | lo);
}

static void after_call(int side)
{
    if (side == 0 && instant_var_set)
        event("instvar 5 = %lld", orig_instant_var);
    if (side == 1 && lift_instant_var_got)
        event("instvar 5 = %lld", lift_instant_var.value);
    {
        unsigned objects = side == 0 ? *(unsigned *)(orig_world + ORIG_WORLD_OBJECTS + 4) : lift_world.gameObjects.size;
        unsigned actors = side == 0 ? *(unsigned *)(orig_world + ORIG_WORLD_ACTORS + 4) : lift_world.actors.size;
        if (objects || actors)
            event("lists %u %u", objects, actors);
    }
    {
        int slot;
        for (slot = 0; slot < 2; slot++)
            if (fake_abr_used[slot]) {
                event("abr %d = %lld", slot, side == 0 ? fake_abr_value[slot] : lift_abr_pair[slot].value);
                fake_abr_used[slot] = 0;
            }
        if (adv_pending) {
            event("advstat %d = %d", adv_key, side == 0 ? adv_value : lift_adv_pair.value);
            adv_pending = 0;
        }
    }
    if (current_handler->id == 0x1d3) {
        char text[256];
        int n = 0;
        unsigned count, i;
        if (side == 0) {
            const unsigned char *begin = *(unsigned char **)(orig_mode + ORIG_MODE_LOOP_SOUNDS);
            const unsigned char *end = *(unsigned char **)(orig_mode + ORIG_MODE_LOOP_SOUNDS + 4);
            count = (unsigned)(end - begin) / ORIG_LOOP_SOUND_SIZE;
            for (i = 0; i < count; i++) {
                const unsigned char *s = begin + i * ORIG_LOOP_SOUND_SIZE;
                n += snprintf(text + n, sizeof text - n, " '%s' %u %u %u", orig_std_string(s), *(const unsigned *)(s + 0x18),
                              *(const unsigned *)(s + 0x1c), *(const unsigned *)(s + 0x20));
            }
        } else {
            const struct LoopSound *s = lift_mode.loop_sounds.first;
            count = (unsigned)vec_count(&lift_mode.loop_sounds, sizeof *s);
            for (i = 0; i < count; i++)
                n += snprintf(text + n, sizeof text - n, " '%s' %u %u %u", string_cstr(&s[i].name), s[i].npc_aid, s[i].interval, s[i].next_tick);
        }
        event("loops %u%s", count, text);
    }
    if (current_handler->id == 0x91) {
        char text[64];
        int n = 0;
        unsigned count, i;
        if (side == 0) {
            const unsigned char *begin = *(unsigned char **)oracle_addr(ORIG_OWN_STATES_BEGIN);
            const unsigned char *end = *(unsigned char **)oracle_addr(ORIG_OWN_STATES_END);
            count = (unsigned)(end - begin) / ORIG_OWN_STATE_SIZE;
            for (i = 0; i < count; i++)
                n += snprintf(text + n, sizeof text - n, " %d", *(const int *)(begin + i * ORIG_OWN_STATE_SIZE));
        } else {
            count = (unsigned)lift_own_count;
            for (i = 0; i < count; i++)
                n += snprintf(text + n, sizeof text - n, " %d", lift_own_states[i].index);
        }
        event("own states %u%s", count, text);
        event("modal %x", side == 0 ? *(int *)oracle_addr(ORIG_WND_MODAL_RESULT) : g_windowMgr.modal_result);
    }
    if (current_handler->id == 0x1a2) {
        if (side == 0) {
            const unsigned char *w = orig_generic_wnd + ORIG_COLLECTION_PET;
            event("collection pet %d %d %d %d %d '%.24s'", *(const short *)w, *(const short *)(w + 2), *(const short *)(w + 4),
                  *(const short *)(w + 0x1e), *(const int *)(w + 0x20), (const char *)w + 6);
        } else {
            event("collection pet %d %d %d %d %d '%.24s'", lift_pet_details.level, lift_pet_details.hunger, lift_pet_details.intimacy,
                  lift_pet_details.accessory, lift_pet_details.renamed, lift_pet_details.name);
        }
    }
    if (current_handler->id == 0xfe || current_handler->id == 0xa43 || current_handler->id == 0xae4)
        event("party name '%s'", side == 0 ? orig_std_string(oracle_addr(ORIG_PARTY_NAME)) : string_cstr(&g_session1.partyName));
    if (current_handler->id == 0xe5 || current_handler->id == 0x1f4)
        event("partner '%s'", side == 0 ? orig_std_string(orig_mode + ORIG_MODE_EXCHANGE_PARTNER) : string_cstr(&lift_mode.exchange_partner));
    if (current_handler->id == 0x7fe)
        event("bgm %d", side == 0 ? *(int *)(orig_sound_mgr + ORIG_SOUND_BGM_OVERRIDDEN) : g_sound_settings.bgm_overridden);
    report_changes(side);
}

typedef void (__fastcall *handler_fn)(void *self, void *edx, void *pkt);

static int call_guarded(const struct Handler *h, unsigned char *pkt)
{
    __try {
        if (h->orig == 0) {
            pending_pkt = pkt;
            pending_count = 1;
            *(int *)oracle_addr(ORIG_REPLAY_PLAYING) = 1;
            ((void (__fastcall *)(void *, void *))oracle_addr(ORIG_DISPATCHER))(orig_mode, NULL);
            *(int *)oracle_addr(ORIG_REPLAY_PLAYING) = 0;
        } else if (h->arg_dword) {
            ((handler_fn)oracle_addr(h->orig))(orig_mode, NULL, (void *)(UINT_PTR)*(const unsigned *)(pkt + 2));
        } else if (h->arg_byte) {
            ((handler_fn)oracle_addr(h->orig))(orig_mode, NULL, (void *)(UINT_PTR)pkt[2]);
        } else {
            ((handler_fn)oracle_addr(h->orig))(orig_mode, NULL, pkt);
        }
        return 1;
    } __except (note_fault(GetExceptionInformation())) {
        return 0;
    }
}

static int call_original(const struct Handler *h, unsigned char *pkt)
{
    jmp_buf env;
    int ok;
    trap_jump = &env;
    if (setjmp(env) != 0) {
        trap_jump = NULL;
        return 1;
    }
    ok = call_guarded(h, pkt);
    trap_jump = NULL;
    return ok;
}

static void setup_regions(void)
{
    static const struct { const char *name; int r; } names[R_COUNT] = {
        { "S", R_SESSION }, { "M", R_MODE }, { "C", R_CASTER }, { "T", R_TARGET }, { "P", R_PLAYER }, { "E", R_EFFECT }, { "N", R_NEW },
    };
    int r;
    for (r = 0; r < R_COUNT; r++)
        regions[names[r].r].name = names[r].name;
    regions[R_SESSION].orig = (unsigned char *)oracle_addr(ORIG_SESSION);
    regions[R_SESSION].orig_size = ORIG_SESSION_SIZE;
    regions[R_SESSION].lift = (unsigned char *)&g_session1;
    regions[R_SESSION].lift_size = sizeof g_session1;
    regions[R_MODE].orig = orig_mode;
    regions[R_MODE].orig_size = sizeof orig_mode;
    regions[R_MODE].lift = (unsigned char *)&lift_mode;
    regions[R_MODE].lift_size = sizeof lift_mode;
    regions[R_CASTER].orig = orig_caster;
    regions[R_CASTER].orig_size = sizeof orig_caster;
    regions[R_CASTER].lift = (unsigned char *)&lift_caster;
    regions[R_CASTER].lift_size = sizeof lift_caster;
    regions[R_TARGET].orig = orig_target;
    regions[R_TARGET].orig_size = sizeof orig_target;
    regions[R_TARGET].lift = (unsigned char *)&lift_target;
    regions[R_TARGET].lift_size = sizeof lift_target;
    regions[R_PLAYER].orig = orig_player;
    regions[R_PLAYER].orig_size = sizeof orig_player;
    regions[R_PLAYER].lift = lift_player;
    regions[R_PLAYER].lift_size = sizeof(struct Player);
    regions[R_EFFECT].orig = orig_effect;
    regions[R_EFFECT].orig_size = sizeof orig_effect;
    regions[R_EFFECT].lift = (unsigned char *)&lift_effect_storage;
    regions[R_EFFECT].lift_size = sizeof lift_effect_storage;
    regions[R_NEW].orig = orig_new_actor;
    regions[R_NEW].orig_size = sizeof orig_new_actor;
    regions[R_NEW].lift = (unsigned char *)&lift_new_actor;
    regions[R_NEW].lift_size = sizeof lift_new_actor;
    for (r = 0; r < R_COUNT; r++) {
        regions[r].orig_snap = calloc(1, regions[r].orig_size);
        regions[r].orig_seen = calloc(1, regions[r].orig_size);
        regions[r].lift_snap = calloc(1, regions[r].lift_size);
        regions[r].lift_seen = calloc(1, regions[r].lift_size);
    }
}

static void packet_oracle_setup(int verbose)
{
    int i;
    skill_oracle_setup(verbose);
    who_extra = who_packets;
    msg_str_named = 1;
    setup_regions();
    oracle_patch_jump(ORIG_NAMES_ERASE, (void *)o_names_erase);
    oracle_patch_jump(ORIG_NAMES_SET_ALL, (void *)o_names_set_all);
    oracle_patch_jump(ORIG_ERASE_ABR, (void *)o_erase_abr);
    oracle_patch_jump(ORIG_WORLD_REMOVE, (void *)o_world_remove);
    oracle_patch_jump(ORIG_JOB_IS_MONSTER, (void *)o_job_is_monster);
    oracle_patch_jump(ORIG_HOMUN_OR_MERC, (void *)o_homun_or_merc);
    oracle_patch_jump(ORIG_IS_HOMUNCULUS, (void *)o_is_homunculus);
    oracle_patch_jump(ORIG_LAUNCH_DUMMY, (void *)o_launch_dummy);
    oracle_patch_jump(ORIG_PET_HUNGER, (void *)o_pet_hunger);
    oracle_patch_jump(ORIG_PET_INTIMACY, (void *)o_pet_intimacy);
    oracle_patch_jump(ORIG_PET_EMOTION, (void *)o_pet_emotion);
    oracle_patch_jump(ORIG_PET_TALK, (void *)o_pet_talk);
    oracle_patch_jump(ORIG_NOTIFY_PLATES, (void *)o_notify_plates);
    oracle_patch_jump(ORIG_MESSAGE_BOX, (void *)o_message_box);
    oracle_patch_jump(ORIG_POST_QUIT, (void *)o_post_quit);
    oracle_patch_jump(ORIG_MAKE_WINDOW, (void *)o_make_window);
    oracle_patch_jump(ORIG_CLEAR_PURCHASE, (void *)o_clear_purchase);
    oracle_patch_jump(ORIG_ACTOR_SET_JOB, (void *)o_actor_set_job);
    oracle_patch_jump(ORIG_SESSION_SET_JOB, (void *)o_session_set_job);
    oracle_patch_jump(ORIG_WEAPON_VIEW, (void *)o_weapon_view);
    oracle_patch_jump(ORIG_SET_COMBO_DELAY, (void *)o_set_combo_delay);
    oracle_patch_jump(ORIG_FORCE_ANIM_END, (void *)o_force_anim_end);
    oracle_patch_jump(ORIG_SESSION_ACTOR, (void *)o_session_actor);
    oracle_patch_jump(ORIG_SET_ARGB, (void *)o_set_argb);
    oracle_patch_jump(ORIG_CONN_INSTANCE, (void *)o_conn_instance);
    oracle_patch_jump(ORIG_CLEAR_HELD, (void *)o_clear_held);
    oracle_patch_jump(ORIG_TITLE_STRING, (void *)o_title_string);
    oracle_patch_jump(ORIG_EMITTER_SPEED, (void *)o_emitter_speed);
    oracle_patch_jump(ORIG_EMITTER_COUNT, (void *)o_emitter_count);
    oracle_patch_jump(ORIG_STOP_AURA, (void *)o_stop_aura);
    oracle_patch_jump(ORIG_LEVEL_AURA, (void *)o_level_aura);
    oracle_patch_jump(ORIG_REMOVE_TYPE, (void *)o_remove_type);
    oracle_patch_jump(ORIG_REROLL, (void *)o_reroll);
    oracle_patch_jump(ORIG_REFRESH_BASIC, (void *)o_refresh_basic);
    oracle_patch_jump(ORIG_IS_PC_JOB_ACTOR, (void *)o_is_pc_job_actor);
    oracle_patch_jump(ORIG_SPRINTF, (void *)o_sprintf);
    oracle_patch_jump(ORIG_SET_COUNTER, (void *)o_set_counter_packets);
    oracle_patch_jump(ORIG_GET_JOB, (void *)o_job_packets);
    oracle_patch_jump(ORIG_OPERATOR_NEW, (void *)o_operator_new);
    oracle_patch_jump(ORIG_PC_CONSTRUCT, (void *)o_pc_construct);
    oracle_patch_jump(ORIG_NEW_ACTOR_FOR_JOB, (void *)o_new_actor_for_job);
    oracle_patch_jump(ORIG_LIST_BUY_NODE, (void *)o_buy_node);
    oracle_patch_jump(ORIG_ACTOR_LIST_BUY_NODE, (void *)o_buy_node);
    oracle_patch_jump(ORIG_OBJECT_LIST_PUSH, (void *)o_list_push);
    oracle_patch_jump(ORIG_ACTOR_LIST_PUSH, (void *)o_list_push);
    oracle_patch_jump(ORIG_MARKS_FIND, (void *)o_marks_find);
    oracle_patch_jump(ORIG_MAKE_EMBLEM_WND, (void *)o_make_emblem_wnd);
    oracle_patch_jump(ORIG_IS_SIEGE_MODE, (void *)o_is_siege_mode);
    oracle_patch_jump(ORIG_IS_PLAYER_JOB, (void *)o_is_player_job);
    oracle_patch_jump(ORIG_IS_HOMUNCULUS_AID, (void *)o_is_homunculus_aid);
    oracle_patch_jump(ORIG_IS_MERCENARY_AID, (void *)o_is_mercenary_aid);
    oracle_patch_jump(ORIG_PET_ACCESSORY_ACT, (void *)o_pet_accessory_act);
    oracle_patch_jump(ORIG_LOAD_ACT_RES, (void *)o_load_act_res);
    oracle_patch_jump(ORIG_CHAMPION_EFFECTS, (void *)o_champion_effects);
    oracle_patch_jump(ORIG_PLAYER_NAMES_SET, (void *)o_player_names_set);
    oracle_patch_jump(ORIG_PLACE_NOWHERE, (void *)o_place_nowhere);
    oracle_patch_jump(ORIG_PLACE_AT_CELL, (void *)o_place_at_cell);
    oracle_patch_jump(ORIG_NAMES_SET_NAME, (void *)o_names_set_name);
    oracle_patch_jump(ORIG_SYNC_SERVER_TIME, (void *)o_sync_server_time);
    oracle_patch_jump(ORIG_CHARACTER_NAME, (void *)o_character_name);
    oracle_patch_jump(ORIG_IS_ADMIN_OPTION, (void *)o_is_admin_option);
    oracle_patch_jump(ORIG_IS_ADMIN_AID, (void *)o_is_admin_aid);
    oracle_patch_jump(ORIG_INVALIDATE_WINDOWS, (void *)o_invalidate_windows);
    oracle_patch_jump(ORIG_IS_SUPER_NOVICE, (void *)o_is_super_novice);
    oracle_patch_jump(ORIG_DRAWN_JOB, (void *)o_drawn_job);
    oracle_patch_jump(ORIG_BASIC_INFO_HEIGHT, (void *)o_basic_info_height);
    oracle_patch_jump(ORIG_BAR_SET_VALUES, (void *)o_bar_set_values);
    oracle_patch_jump(ORIG_REFRESH_ACTOR_EFFECTS, (void *)o_refresh_actor_effects);
    oracle_patch_jump(ORIG_GET_WINDOW, (void *)o_get_window);
    oracle_patch_jump(ORIG_REFRESH_SKILL_WNDS, (void *)o_refresh_skill_wnds);
    oracle_patch_jump(ORIG_STATUS_RESTORE, (void *)o_status_restore);
    oracle_patch_jump(ORIG_END_EXCHANGE, (void *)o_end_exchange);
    oracle_patch_jump(ORIG_FIND_ABR, (void *)o_find_abr);
    oracle_patch_jump(ORIG_ABR_GET, (void *)o_abr_get);
    oracle_patch_jump(ORIG_SET_ADVANCED_STATUS, (void *)o_set_advanced_status);
    oracle_patch_jump(ORIG_PET_TALK_DUE, (void *)o_pet_talk_due);
    oracle_patch_jump(ORIG_ADD_CART_ITEM, (void *)o_add_cart_item);
    oracle_patch_jump(ORIG_ADD_STORAGE_ITEM, (void *)o_add_storage_item);
    oracle_patch_jump(ORIG_INVENTORY_UPDATE, (void *)o_inventory_update);
    oracle_patch_jump(ORIG_ADD_INVENTORY_ITEM, (void *)o_add_inventory_item);
    oracle_patch_jump(ORIG_SET_WORN_ITEM, (void *)o_set_worn_item);
    oracle_patch_jump(ORIG_SET_WORN_COSTUME, (void *)o_set_worn_costume);
    oracle_patch_jump(ORIG_REMOVE_FILTER_TYPES, (void *)o_remove_filter_types);
    oracle_patch_jump(ORIG_REMOVE_OTHER_TYPES, (void *)o_remove_other_types);
    oracle_patch_jump(ORIG_CLEAR_WORN_ITEMS, (void *)o_clear_worn_items);
    oracle_patch_jump(ORIG_CLEAR_WORN_COSTUMES, (void *)o_clear_worn_costumes);
    oracle_patch_jump(ORIG_CLEAR_INVENTORY, (void *)o_clear_inventory);
    oracle_patch_jump(ORIG_CLEAR_CART, (void *)o_clear_cart);
    oracle_patch_jump(ORIG_REFRESH_PARTY_UI, (void *)o_refresh_party_ui);
    oracle_patch_jump(ORIG_REFRESH_FRIEND_UI, (void *)o_refresh_friend_ui);
    oracle_patch_jump(ORIG_REFRESH_ITEM_WNDS, (void *)o_refresh_item_wnds);
    oracle_patch_jump(ORIG_IS_PET_EGG_ID, (void *)o_is_pet_egg_id);
    oracle_patch_jump(ORIG_IS_COSTUME_LOCATION, (void *)o_is_costume_location);
    oracle_patch_jump(ORIG_GET_INVENTORY_ITEM, (void *)o_get_inventory_item);
    oracle_patch_jump(ORIG_PET_AUTO_FEEDING, (void *)o_pet_auto_feeding);
    oracle_patch_jump(ORIG_SEND_PET_AUTO_FEED, (void *)o_send_pet_auto_feed);
    oracle_patch_jump(ORIG_SET_TEXT_ELLIPSIS, (void *)o_set_text_ellipsis);
    oracle_patch_jump(ORIG_STORAGE_SET_NAME, (void *)o_storage_set_name);
    oracle_patch_jump(ORIG_REPLAY_INSTANCE, (void *)o_replay_instance);
    oracle_patch_jump(ORIG_REPLAY_NEXT, (void *)o_replay_next);
    oracle_patch_jump(ORIG_REPLAY_SET_MODE, (void *)o_replay_set_mode);
    oracle_patch_jump(ORIG_LOG_WHISPER, (void *)o_log_whisper);
    oracle_patch_jump(ORIG_CLEAR_LISTS, (void *)o_clear_lists);
    oracle_patch_jump(ORIG_ITEM_TABLE_GET, (void *)o_item_table_get);
    oracle_patch_jump(ORIG_APPEND_MARKET, (void *)o_append_market);
    oracle_patch_jump(ORIG_ADD_BOUGHT, (void *)o_add_bought);
    oracle_patch_jump(ORIG_RT_DYNAMIC_CAST, (void *)o_rt_dynamic_cast);
    oracle_patch_jump(ORIG_MODE_MGR_SWITCH, (void *)o_mode_mgr_switch);
    oracle_patch_jump(ORIG_WAIT_SET_TEXT, (void *)o_wait_set_text);
    oracle_patch_jump(ORIG_RESET_STORE_SEARCH, (void *)o_reset_store_search);
    oracle_patch_jump(ORIG_RESET_FOR_WARP, (void *)o_reset_for_warp);
    oracle_patch_jump(ORIG_MODE_INITIALIZE, (void *)o_mode_initialize);
    oracle_patch_jump(ORIG_RUN_FADE_OUT, (void *)o_run_fade_out);
    oracle_patch_jump(ORIG_RESET_MAIL_ITEMS, (void *)o_reset_mail_items);
    oracle_patch_jump(ORIG_CLEAR_MAIL_COUNTS, (void *)o_clear_mail_counts);
    oracle_patch_jump(ORIG_MOVE_TO_SERVER, (void *)o_move_to_server);
    oracle_patch_jump(ORIG_PLAY_STREAM, (void *)o_play_stream);
    oracle_patch_jump(ORIG_SET_TEXT_ALIGN, (void *)o_set_text_align);
    oracle_patch_jump(ORIG_PET_INFO_INTIMACY, (void *)o_pet_info_intimacy);
    oracle_patch_jump(ORIG_PET_SPEAK, (void *)o_pet_speak);
    oracle_patch_jump(ORIG_FORMAT_CHAT_LINE, (void *)o_format_chat_line);
    oracle_patch_jump(ORIG_SELECTED_ITEM, (void *)o_selected_item);
    oracle_patch_jump(ORIG_SET_PK_MODE, (void *)o_set_pk_mode);
    oracle_patch_jump(ORIG_GET_ITEM_BY_ID, (void *)o_get_item_by_id);
    oracle_patch_jump(ORIG_ITEM_GET_NAME, (void *)o_item_get_name);
    oracle_patch_jump(ORIG_ITEM_DISPLAY_NAME, (void *)o_item_display_name);
    oracle_patch_jump(ORIG_HOMUN_REDRAW, (void *)o_homun_redraw);
    oracle_patch_jump(ORIG_MERC_REDRAW, (void *)o_merc_redraw);
    oracle_patch_jump(ORIG_LIST_CLEAR, (void *)o_list_clear);
    oracle_patch_jump(ORIG_BOOKING_ADD_ROWS, (void *)o_booking_add_rows);
    oracle_patch_jump(ORIG_BOOKING_REMOVE, (void *)o_booking_remove);
    oracle_patch_jump(ORIG_BOOKING_ON_NEW, (void *)o_booking_on_new);
    oracle_patch_jump(ORIG_BOOKING_SET_JOBS, (void *)o_booking_set_jobs);
    oracle_patch_jump(ORIG_SET_WANTED_JOBS, (void *)o_set_wanted_jobs);
    oracle_patch_jump(ORIG_STOP_RECRUITING, (void *)o_stop_recruiting);
    oracle_patch_jump(ORIG_ADD_JOIN_REQUEST, (void *)o_add_join_request);
    oracle_patch_jump(ORIG_COMMAND_PLATE_OF, (void *)o_command_plate_of);
    oracle_patch_jump(ORIG_IS_OWN_UNIT_AID, (void *)o_is_own_unit_aid);
    oracle_patch_jump(ORIG_FILTER_CONTAINS, (void *)o_filter_contains);
    oracle_patch_jump(ORIG_FILTER_CENSOR, (void *)o_filter_censor);
    oracle_patch_jump(ORIG_CLEAR_INV_PENDING, (void *)o_clear_inv_pending);
    oracle_patch_jump(ORIG_CLEAR_CART_PENDING, (void *)o_clear_cart_pending);
    oracle_patch_jump(ORIG_CLEAR_STORAGE_PENDING, (void *)o_clear_storage_pending);
    oracle_patch_jump(ORIG_GET_PARTY_MEMBER, (void *)o_get_party_member);
    oracle_patch_jump(ORIG_SET_MEMBER_STATE, (void *)o_set_member_state);
    oracle_patch_jump(ORIG_ADD_PARTY_MEMBER, (void *)o_add_party_member);
    oracle_patch_jump(ORIG_ROOM_OWNER_AT, (void *)o_room_owner_at);
    oracle_patch_jump(ORIG_IS_YELLOW_AID, (void *)o_is_yellow_aid);
    oracle_patch_jump(ORIG_REMOVE_FRIEND, (void *)o_remove_friend);
    oracle_patch_jump(ORIG_NOTIFY_GROUP_WND, (void *)o_notify_group_wnd);
    oracle_patch_jump(ORIG_END_EXCHANGE1, (void *)o_end_exchange1);
    oracle_patch_jump(ORIG_THEIR_ZENY, (void *)o_their_zeny);
    oracle_patch_jump(ORIG_ADD_THEIR_ITEM, (void *)o_add_their_item);
    for (i = 0; i < (int)sizeof long_text_600 - 1; i++)
        long_text_600[i] = "abcdefghij"[i % 10];
    for (i = 0; i < (int)sizeof long_text_max - 1; i++)
        long_text_max[i] = "klmnopqrst"[i % 10];
    for (i = 0; i < 64; i++)
        orig_window_vt[i] = traps[i];
    orig_window_vt[ORIG_WINDOW_VT_SEND] = (void *)o_window_send;
    orig_window_vt[ORIG_WINDOW_VT_INVALIDATE] = (void *)o_window_invalidate;
    for (i = 0; i < 64; i++)
        orig_ctrl_vt[i] = traps[i];
    orig_ctrl_vt[ORIG_CTRL_VT_SET_TEXT] = (void *)o_ctrl_set_text;
    lift_window_ops.invalidate = lift_window_invalidate;
    lift_status_wnd.ops = lift_basic_info_wnd.ops = lift_group_wnd.ops = lift_found_wnd.ops = lift_storage_wnd.ops = lift_generic_wnd.ops =
        lift_helper_wnd.ops = lift_pet_info_wnd.ops = lift_homun_info_wnd.ops = lift_homun_skill_wnd.ops = &lift_window_ops;
    if (!oracle_bind_import("api-ms-win-crt-utility-l1-1-0.dll", "rand", (void *)o_rand))
        fprintf(stderr, "the original's rand import was not found\n");

    orig_actor_vt[ORIG_ACTOR_VT_SET_HEAD] = (void *)o_set_head;
    orig_actor_vt[ORIG_ACTOR_VT_SET_WEAPON] = (void *)o_set_weapon;
    orig_actor_vt[ORIG_ACTOR_VT_SET_WEAPON_LOOK] = (void *)o_set_weapon_look;
    orig_actor_vt[ORIG_ACTOR_VT_SET_ACCESSORY] = (void *)o_set_accessory;
    orig_actor_vt[ORIG_ACTOR_VT_SET_ACCESSORY2] = (void *)o_set_accessory2;
    orig_actor_vt[ORIG_ACTOR_VT_SET_ACCESSORY3] = (void *)o_set_accessory3;
    orig_actor_vt[ORIG_ACTOR_VT_SET_SHIELD] = (void *)o_set_shield;
    orig_actor_vt[ORIG_ACTOR_VT_SET_APPEARANCE] = (void *)o_set_appearance;
    orig_actor_vt[ORIG_ACTOR_VT_SET_IMF_NAME] = (void *)o_set_imf_name;
    orig_actor_vt[ORIG_ACTOR_VT_SET_HEAD_PAL] = (void *)o_set_head_pal_name;
    orig_actor_vt[ORIG_ACTOR_VT_SET_BODY_PAL] = (void *)o_set_body_pal_name;
    orig_actor_vt[ORIG_ACTOR_VT_SET_BODY_PALETTE] = (void *)o_set_body_palette;
    orig_actor_vt[ORIG_ACTOR_VT_SET_HEAD_PALETTE] = (void *)o_set_head_palette;
    orig_actor_vt[ORIG_ACTOR_VT_SET_HONOR] = (void *)o_set_honor;
    orig_actor_vt[ORIG_ACTOR_VT_SET_ROBE] = (void *)o_set_robe;
    orig_actor_vt[ORIG_ACTOR_VT_SET_BODY_STYLE] = (void *)o_set_body_style;
    orig_actor_vt[ORIG_ACTOR_VT_REFRESH_ACC3] = (void *)o_refresh_accessory3;
    orig_actor_vt[ORIG_ACTOR_VT_SET_GUILD] = (void *)o_set_guild;
    orig_actor_vt[ORIG_ACTOR_VT_GET_GUILD_ID] = (void *)o_get_guild_id;
    orig_actor_vt[ORIG_ACTOR_VT_SET_JOB_MODEL] = (void *)o_set_job_model;
    orig_actor_vt[ORIG_ACTOR_VT_SET_SPR_JOB] = (void *)o_set_spr_job;

    lift_actor_ops.base.get_effect_state = lift_get_effect_state;
    lift_actor_ops.set_head = lift_set_head;
    lift_actor_ops.set_job_model = lift_set_job_model;
    lift_actor_ops.set_spr_job = lift_set_spr_job;
    lift_actor_ops.set_weapon = lift_set_weapon;
    lift_actor_ops.set_weapon_look = lift_set_weapon_look;
    lift_actor_ops.set_accessory = lift_set_accessory;
    lift_actor_ops.set_accessory2 = lift_set_accessory2;
    lift_actor_ops.set_accessory3 = lift_set_accessory3;
    lift_actor_ops.set_shield = lift_set_shield;
    lift_actor_ops.set_appearance_value = lift_set_appearance;
    lift_actor_ops.set_imf_name = lift_set_imf_name;
    lift_actor_ops.set_head_pal_name = lift_set_head_pal_name;
    lift_actor_ops.set_body_pal_name = lift_set_body_pal_name;
    lift_actor_ops.set_body_palette = lift_set_body_palette;
    lift_actor_ops.set_head_palette = lift_set_head_palette;
    lift_actor_ops.set_honor = lift_set_honor;
    lift_actor_ops.set_robe = lift_set_robe;
    lift_actor_ops.set_body_style = lift_set_body_style;
    lift_actor_ops.refresh_accessory3 = lift_refresh_accessory3;
    msg_text_extra = msg_text_packets;
    job_hook = lift_job;
    mem_alloc_hook = lift_alloc;
}

int main(int argc, char **argv)
{
    const char *exe = argc > 1 ? argv[1] : getenv("ORACLE_EXE");
    const char *only = getenv("ORACLE_ONLY");
    const int verbose = getenv("ORACLE_VERBOSE") != NULL;
    const int trace = getenv("ORACLE_TRACE") != NULL;
    const unsigned max_reported = getenv("ORACLE_REPORTS") ? (unsigned)atoi(getenv("ORACLE_REPORTS")) : MAX_REPORTED;
    static const unsigned aids[] = { AID_CASTER, AID_TARGET, AID_OWN, AID_UNKNOWN };
    unsigned char pkt[PACKET_MAX];
    unsigned hi, ai, vi, fill, option;
    unsigned compared = 0, mismatches = 0, faults = 0, reported = 0, recorded = 0;

    if (!exe) {
        fprintf(stderr, "usage: test_packet_handlers <original exe>\n");
        return 2;
    }
    if (!oracle_load(exe))
        return 2;
    packet_oracle_setup(verbose);

    for (hi = 0; hi < sizeof handlers / sizeof *handlers; hi++) {
        const struct Handler *h = &handlers[hi];
        unsigned values = handler_values(h), handler_compared = 0, handler_events = 0, handler_bad = 0;
        current_handler = h;
        if (only && strcmp(only, h->name) != 0)
            continue;
        for (option = 0; option < 2; option++)
        for (ai = 0; ai < sizeof aids / sizeof *aids; ai++)
        for (vi = 0; vi < values; vi++)
        for (fill = 0; fill < FILL_COUNT; fill++) {
            if (h->no_unknown && aids[ai] == AID_UNKNOWN)
                continue;
            unsigned value = h->kind == PK_ENTRY || h->kind == PK_TEXT || h->kind == PK_MARKET || h->kind == PK_MAP ? vi : h->kind == PK_PAR ? h->extra[vi] : vi < SWEEP_VALUES ? vi : h->extra[vi - SWEEP_VALUES];
            char orig_text[EVENTS * EVENT_LEN], lift_text[EVENTS * EVENT_LEN];

            compared++;
            handler_compared++;
            move_mask = h->kind == PK_ENTRY ? h->move_mask : 7;
            if (verbose) { fprintf(stderr, "%s option %u aid %u value %u fill %u\n", h->name, option, aids[ai], value, fill); fflush(stderr); }
            seed_packets(option);
            build_packet(pkt, h, aids[ai], value, fill);
            event_count = 0;
            overflow = 0;
            srand(1);
            snapshot_regions(0);
            if (!call_original(h, pkt)) {
                faults++;
                handler_bad++;
                if (reported++ < max_reported)
                    printf("  %s option %u aid %u value %u fill %u: the original faulted (code %08x at 0x%x)\n", h->name, option, aids[ai],
                           value, fill, fault_code, fault_va);
                continue;
            }
            after_call(0);
            recorded += event_count;
            handler_events += event_count;
            joined(orig_text, sizeof orig_text);
            if (trace) printf("%s aid %u value %u fill %u original: %s\n", h->name, aids[ai], value, fill, orig_text);

            seed_packets(option);
            build_packet(pkt, h, aids[ai], value, fill);
            event_count = 0;
            overflow = 0;
            srand(1);
            snapshot_regions(1);
            h->lift(pkt);
            after_call(1);
            joined(lift_text, sizeof lift_text);
            if (trace) printf("%s aid %u value %u fill %u lift:     %s\n", h->name, aids[ai], value, fill, lift_text);

            if (strcmp(orig_text, lift_text) != 0) {
                mismatches++;
                handler_bad++;
                if (reported++ < max_reported)
                    printf("  %s option %u aid %u value %u fill %u:\n    original: %s\n    lift:     %s\n", h->name, option, aids[ai], value, fill,
                           orig_text, lift_text);
            }
        }
        printf("  %-22s %5u calls %6u events %s\n", h->name, handler_compared, handler_events, handler_bad ? "MISMATCH" : "ok");
    }
    printf("Packet handlers: %u calls compared against the original (%u events recorded), %u mismatches, %u faults%s\n", compared, recorded,
           mismatches, faults, reported > max_reported ? " (not all shown)" : "");
    return mismatches || faults ? 1 : 0;
}
