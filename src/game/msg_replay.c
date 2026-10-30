#include "game/gamemode.h"
#include "session/character_name.h"
#include "navi/snapshot.h"
#include "game/actor_snapshot.h"
#include "ui/window_records.h"
#include "game/emblem_snapshot.h"
#include "game/own_states.h"
#include "game/world_snapshot.h"
#include "session/snapshot_party.h"
#include "session/snapshot_items.h"
#include "session/snapshot_values.h"
#include "replay/replay.h"
#include "ui/social/entry_queue_snapshot.h"
#include "ui/social/entry_queue_state.h"
#include "contents/quest_blob.h"
#include "contents/quest.h"
#include "navi/navi_manager.h"
#include "session/state.h"
#include "world/mode_manager.h"
#include "game/context.h"
#include <string.h>
#include <windows.h>
#include <mmsystem.h>
#include "game/mode_msg.h"
#include "game/cells.h"
#include "login/loginmode.h"
#include "game/replay.h"
#include "session/session_talk.h"
#include "ui/ui_std.h"
#include "ui/wndmgr/wndmgr.h"
#include "core/app.h"

#define COLOR_REPLAY 0x62b0ff

static int write_navigation_snapshot(void)
{
    struct ReplayBuffer chunk = {0};
    int written = navi_state_save(&g_navi, &chunk) &&
        replay_recorder_write_chunk(replay_recorder_instance(), 0x17, (DWORD)chunk.size,
                                  chunk.data, REPLAY_CHUNK_APPEND);
    replay_buffer_destroy(&chunk);
    return written;
}

typedef int (*WindowSnapshotWriter)(struct UIWindowMgr *, struct ReplayBuffer *);
static int write_window_snapshot(int slot, WindowSnapshotWriter save)
{
    struct ReplayBuffer chunk = {0};
    int written = save(&g_windowMgr, &chunk) &&
        replay_recorder_write_chunk(replay_recorder_instance(), slot, (DWORD)chunk.size,
                                  chunk.data, REPLAY_CHUNK_APPEND);
    replay_buffer_destroy(&chunk);
    return written;
}

typedef int (*GameSnapshotWriter)(struct GameMode *, struct ReplayBuffer *);
static int write_game_snapshot(struct GameMode *mode, int slot, GameSnapshotWriter save)
{
    struct ReplayBuffer chunk = {0};
    int written = save(mode, &chunk) &&
        replay_recorder_write_chunk(replay_recorder_instance(), slot, (DWORD)chunk.size,
                                  chunk.data, REPLAY_CHUNK_APPEND);
    replay_buffer_destroy(&chunk);
    return written;
}

static int write_quest_snapshot(void)
{
    struct QuestBlob chunk = {0};
    int written = quest_blob_save(g_questTable, &chunk) &&
        replay_recorder_write_chunk(replay_recorder_instance(), 6, (DWORD)chunk.size,
                                  chunk.data, REPLAY_CHUNK_APPEND);
    quest_blob_destroy(&chunk);
    return written;
}

static int write_state_snapshot(void)
{
    struct ReplayBuffer chunk = {0};
    int written = own_states_save(timeGetTime(), &chunk) &&
        replay_recorder_write_chunk(replay_recorder_instance(), 0x11, (DWORD)chunk.size,
                                  chunk.data, REPLAY_CHUNK_APPEND);
    replay_buffer_destroy(&chunk);
    return written;
}

static int write_queue_snapshot(void)
{
    struct ReplayBuffer chunk = {0};
    int written = entry_queue_snapshot_save(g_entryQueueMgr, &chunk) &&
        replay_recorder_write_chunk(replay_recorder_instance(), 0xc, (DWORD)chunk.size,
                                  chunk.data, REPLAY_CHUNK_APPEND);
    replay_buffer_destroy(&chunk);
    return written;
}

typedef int (*SessionSnapshotWriter)(const struct Session *, struct ReplayBuffer *);
static int write_session_snapshot(int slot, SessionSnapshotWriter save)
{
    struct ReplayBuffer chunk = {0};
    int written = save(&g_session1, &chunk) &&
        replay_recorder_write_chunk(replay_recorder_instance(), slot, (DWORD)chunk.size,
                                  chunk.data, REPLAY_CHUNK_APPEND);
    replay_buffer_destroy(&chunk);
    return written;
}

static int write_session_values(int slot, enum SessionValueSnapshot kind)
{
    struct ReplayBuffer chunk = {0};
    int written = session_values_save(&g_session1, kind, &chunk) &&
        replay_recorder_write_chunk(replay_recorder_instance(), slot, (DWORD)chunk.size,
                                  chunk.data, REPLAY_CHUNK_APPEND);
    replay_buffer_destroy(&chunk);
    return written;
}

int game_mode_msg_replay_start(struct GameMode *self)
{
    SYSTEMTIME now;
    union { DWORD value; struct { WORD year; BYTE month, day; } f; } date;
    union { DWORD value; BYTE b[4]; } time;
    struct Player *player;
    struct ReplayRecorder *recorder;
    char name[0x40];
    char text[0x32];
    int window_ids[0x100 / sizeof(int)];
    int cell_x, cell_y, list_arg, opened;
    DWORD value;

    replay_recorder_reset_session(replay_recorder_instance());
    GetLocalTime(&now);
    date.f.year = now.wYear;
    date.f.month = (BYTE)now.wMonth;
    date.f.day = (BYTE)now.wDay;
    time.b[0] = 0;
    time.b[1] = (BYTE)now.wHour;
    time.b[2] = (BYTE)now.wMinute;
    time.b[3] = (BYTE)now.wSecond;
    player = self->world->player;
    game_mode_world_to_cell(self, player->pos.x, player->pos.z, &cell_x, &cell_y);
    ui_window_mgr_init_window_flags(&g_windowMgr);
    memset(name, 0, sizeof(name));
    sprintf(name, "%s", session_character_name(&g_session1));
    value = session_get_sex(&g_session1);
    replay_recorder_set_session_info(replay_recorder_instance(), date.value, time.value, name,
                                  (const BYTE *)g_session1.mapName, g_session1.mapType, cell_x, cell_y,
                                  g_session1.playerDir, g_serviceType, g_serverType, value);
    recorder = replay_recorder_instance();
    replay_file_list_set_recorder(replay_file_list_instance(), recorder);
    list_arg = ui_window_mgr_get_window_ids(&g_windowMgr, window_ids);
    opened = replay_file_list_open_selected(replay_file_list_instance(), 0x1f);
    if (opened == 0) {
        replay_recorder_set_mode(replay_recorder_instance(), REPLAY_STATE_IDLE);
        return 0;
    }
    if (opened == -2) {
        replay_recorder_set_mode(replay_recorder_instance(), REPLAY_STATE_IDLE);
        message_box_msg(0x7c5);
        return 0;
    }
    if (!write_navigation_snapshot()) goto failed;
    if (!write_session_values(0x3, SESSION_VALUES_IDENTITY)) goto failed;
    if (!write_session_values(0x4, SESSION_VALUES_CHARACTER)) goto failed;
    if (!write_quest_snapshot()) goto failed;
    if (!write_session_snapshot(0x7, session_party_save)) goto failed;
    if (!write_session_snapshot(0x8, session_items_save)) goto failed;
    if (!write_session_values(0x9, SESSION_VALUES_COMPANIONS)) goto failed;
    if (!write_session_values(0xa, SESSION_VALUES_DUNGEON)) goto failed;
    if (!write_queue_snapshot()) goto failed;
    if (!write_session_values(0xd, SESSION_VALUES_HOTKEYS)) goto failed;
    if (!write_game_snapshot(self, 0x15, game_emblems_save)) goto failed;
    if (!write_state_snapshot()) goto failed;
    if (!write_game_snapshot(self, 0xe, game_actors_save)) goto failed;
    if (!write_game_snapshot(self, 0xf, game_ground_items_save)) goto failed;
    if (!write_game_snapshot(self, 0x10, game_skill_units_save)) goto failed;
    if (!write_game_snapshot(self, 0x16, game_npc_dialog_save)) goto failed;
    if (!write_window_snapshot(0x13, ui_window_mgr_get_option_records)) goto failed;
    if (!write_window_snapshot(0x12, ui_window_mgr_get_window_records)) goto failed;
    if (!write_window_snapshot(0x14, ui_window_mgr_get_empty_records)) goto failed;

    if (!replay_recorder_on_event(recorder, REPLAY_EVENT_START)) goto failed;
    if (!replay_recorder_mark_first_record(recorder)) goto failed;
    replay_recorder_write_stored_packets(replay_recorder_instance());
    if (recorder->writeFailed) goto failed;
    replay_recorder_resume(recorder);
    memset(text, 0, sizeof(text));
    sprintf(text, msg_str(0x7c6));
    chat_print(text, COLOR_REPLAY);
    replay_recorder_set_mode(replay_recorder_instance(), REPLAY_STATE_RECORDING);

    return 0;

failed:
    recorder->writeFailed = 1;
    replay_recorder_close(recorder);
    replay_recorder_set_mode(recorder, REPLAY_STATE_IDLE);
    return 0;
}

int game_mode_msg_replay_stop(void)
{
    char text[0x104];
    char dir[0x104];
    struct String path;

    if (replay_recorder_instance()->state == REPLAY_STATE_RECORDING) {
        replay_recorder_set_mode(replay_recorder_instance(), REPLAY_STATE_IDLE);
        replay_recorder_stamp_total_time(replay_recorder_instance());
        replay_recorder_close(replay_recorder_instance());
    }
    memset(text, 0, sizeof(text));
    memset(dir, 0, sizeof(dir));
    GetCurrentDirectoryA(sizeof(dir), dir);
    string_init(&path);
    string_assign_cstr(&path, dir);
    string_append_cstr(&path, replay_file_list_instance()->path);
    string_append_cstr(&path, msg_str(0x7c7));
    sprintf(text, "%s", string_cstr(&path));
    chat_print(text, COLOR_REPLAY);
    string_free(&path);
    return 0;
}

int game_mode_msg_replay_pause(struct GameMode *self, char pausing)
{
    game_mode_on_replay_pause(self, pausing);
    replay_recorder_set_mode(replay_recorder_instance(), pausing ? REPLAY_STATE_PLAYING : REPLAY_STATE_STOPPED);
    return 0;
}

int game_mode_to_login(void)
{
    mode_mgr_switch(&g_modeMgr, 0, "login.rsw");
    return 0;
}

int game_mode_msg_go_to_login(void)
{
    g_mapServerAidPending = 0;
    g_loginStartMode = 3;
    return game_mode_to_login();
}

int game_mode_msg_replay_to_login(int start_mode, int replay_mode)
{
    g_loginStartMode = start_mode;
    replay_recorder_set_mode(replay_recorder_instance(), replay_mode);
    return game_mode_to_login();
}

int game_mode_msg_replay_speed(int up)
{
    if (up)
        replay_recorder_speed_up(replay_recorder_instance());
    else
        replay_recorder_slow_down(replay_recorder_instance());
    game_mode_apply_replay_speed(game_current_mode());
    return 0;
}
