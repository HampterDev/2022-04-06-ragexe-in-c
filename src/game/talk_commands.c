#include "game/display_settings.h"
#include "game/gamemode.h"
#include "actor/render_ops.h"
#include "platform/language_api.h"
#include "render/renderer.h"
#include "std/string.h"
#include "contents/client_lua_calls.h"
#include "game/own_states.h"
#include "game/whisper_log.h"
#include "navi/navi_manager.h"
#include "session/map_names.h"
#include "contents/sound.h"
#include "actor/weather.h"
#include "contents/guild_info.h"
#include "session/state.h"
#include "ui/window_manager_state.h"
#include "game/context.h"
#include "contents/connection_api.h"
#include "platform/win_types.h"
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "core/client_path.h"
#include "platform/filesystem.h"
#include "game/talk_type.h"
#include "game/client.h"
#include "game/cells.h"
#include "game/camera.h"
#include "render/render_api.h"
#include "platform/language.h"
#include "actor/game_actor.h"
#include "core/msg_str.h"
#include "core/text_util.h"
#include "game/chat.h"
#include "session/session_items.h"
#include "session/session_names.h"
#include "session/session_party.h"
#include "session/session_state.h"
#include "ui/ui_std.h"
#include "ui/wndmgr/wndmgr.h"
#include "game/mouse_camera.h"
#include "core/app.h"
#include "platform/clock.h"
#include "platform/window.h"

enum {
    MODE_MSG_CHAT = 6,
    MODE_MSG_PARTY_CHAT = 0x42,
    MODE_MSG_GUILD_CHAT = 0x81,
    MODE_MSG_CHAT_FE = 0xfe,
};

void talk_cmd_send_chat(struct GameMode *self, int type)
{
    char line[0x6c];
    int party = 0, guild = 0;
    int target;

    strcpy(line, string_cstr(&g_windowMgr.chat_line));
    if (line[0] == '%') {
        if (strlen(line) <= 1)
            return;
        ui_window_mgr_set_chat_line(&g_windowMgr, line + 1);
        party = 1;
    } else if (line[0] == '$') {
        if (strlen(line) <= 1)
            return;
        ui_window_mgr_set_chat_line(&g_windowMgr, line + 1);
        guild = 1;
    }
    if ((platform_async_key_state(VK_CONTROL) >> 8) & 0xff)
        party = 1;
    if ((platform_async_key_state(VK_MENU) >> 8) & 0xff)
        guild = 1;

    if (type == TALK_CHAT_TO_GUILD) {
        mode_send_msg(self, MODE_MSG_CHAT, 0, 0, 0, 0);
        return;
    }
    if (g_session1.battleChat) {
        mode_send_msg(self, MODE_MSG_CHAT_FE, 0, 0, 0, 0);
        return;
    }
    target = g_session1.chatTarget;
    if (target == 2) {
        if (g_guildInfo.guild_id != 0) {
            mode_send_msg(self, guild ? MODE_MSG_CHAT : MODE_MSG_GUILD_CHAT, 0, 0, 0, 0);
            return;
        }
    } else if (target == 1) {
        if (party || !session_get_party_member_count(&g_session1))
            mode_send_msg(self, MODE_MSG_CHAT, 0, 0, 0, 0);
        else
            mode_send_msg(self, MODE_MSG_PARTY_CHAT, 0, 0, 0, 0);
        return;
    }
    if (party && session_get_party_member_count(&g_session1)) {
        mode_send_msg(self, MODE_MSG_PARTY_CHAT, 0, 0, 0, 0);
        return;
    }
    mode_send_msg(self, guild ? MODE_MSG_GUILD_CHAT : MODE_MSG_CHAT, 0, 0, 0, 0);
}

void talk_cmd_hi_friends(struct GameMode *self, const struct String *args)
{
    char line[0x80];
    struct String text;
    const char *typed;

    (void)self;
    if (!*string_cstr(&args[0])) {
        session_hi_friends(&g_session1, 0);
        sprintf(line, "[ To Friends ] : (Hi) *^_^*");
    } else {
        string_init(&text);
        typed = string_cstr(&g_windowMgr.chat_line);
        string_assign_cstr(&text, typed);
        session_hi_friends(&g_session1, typed);
        if (g_serviceType == 0)
            sprintf(line, "[ To Friends ] : (Hi) %s", string_cstr(&text));
        else
            sprintf(line, "[ To Friends ] : (Hi) %s%s", language_charset_tag(g_language, 0),
                        string_cstr(&text));
        string_free(&text);
    }
    talk_print(line, 0x1effff);
}

char g_whisperLogPath[0x100];

void talk_cmd_chat_auto_save(struct GameMode *self)
{
    char dir[0x400];
    char message[0x80];
    struct GameMode *mode;
    int n;

    (void)self;
    if (!session_is_admin_option(&g_session1, 7)) {
        talk_print_msg(0x5f, TALK_COLOR_YELLOW);
        return;
    }
    g_session1.autoSaveChat = 1 - g_session1.autoSaveChat;
    if (g_session1.autoSaveChat == 0) {
        sprintf(message, "Chatting Auto Save END");
        game_mode_close_chat_log();
        talk_print(message, TALK_COLOR_YELLOW);
        return;
    }
    mode = game_current_mode();
    if (session_is_admin_option(&g_session1, 7)) {
        mode->whispers_logged = 0;
        client_path(dir, sizeof dir, "Chat");
        n = 0;
        if (platform_path_exists(dir) || platform_directory_create(dir) == 0) {
            client_path(g_whisperLogPath, sizeof g_whisperLogPath, "Chat\\Chat.txt");
            while (platform_path_exists(g_whisperLogPath)) {
                n++;
                client_path(g_whisperLogPath, sizeof g_whisperLogPath, "Chat\\Chat%03d.txt", n);
            }
        }
    }
    sprintf(message, "Chatting Auto Save Start..");
    talk_print(message, TALK_COLOR_YELLOW);
}

void talk_cmd_where(struct GameMode *self)
{
    char line[0x80];
    struct String map;
    int x, y;

    if (!self->world || !self->world->player)
        return;
    game_mode_world_to_cell(self, self->world->player->pos.x, self->world->player->pos.z, &x, &y);
    string_init(&map);
    string_assign_cstr(&map, self->map_name);
    if (map.size > 4)
        string_resize(&map, map.size - 4, 0);
    sprintf(line, "%s(%s) : %d, %d", session_map_display_name(self->map_name),
                string_cstr(&map), x, y);
    ui_window_mgr_send_msg_(&g_windowMgr, 3, 0, 0, 0, 0);
    talk_print(line, TALK_COLOR_NOTICE);
    if (g_serviceType != 10)
        ui_window_mgr_send_msg_(&g_windowMgr, 0x13, (intptr_t)line, TALK_COLOR_NOTICE, 0, 0);
    string_free(&map);
}

void talk_cmd_snow(struct GameMode *self)
{
    int on = 1 - g_session1.effectOn;

    g_session1.effectOn = on;
    if (strcmp("xmas_fild01.rsw", self->map_name) == 0 || strcmp("xmas.rsw", self->map_name) == 0) {
        if (on)
            weather_snow(&g_weather);
        else
            weather_stop_effect(&g_weather);
    }
    ui_window_mgr_send_msg_(&g_windowMgr, 3, 0, 0, 0, 0);
    talk_print_msg(g_session1.effectOn ? 0x17 : 0x18, TALK_COLOR_NOTICE);
    talk_refresh_option_window();
}

#define STATE_ICON_SUHIDE 0x3a5
#define MODE_MSG_CHANGE_DIR 0x8b

void talk_cmd_sit_or_stand(struct GameMode *self)
{
    struct Player *player;
    int stance;
    unsigned now;

    if (own_states_find(STATE_ICON_SUHIDE))
        return;
    player = self->world->player;
    if (player->motion == 2) {
        player->motion = 1;
        stance = 1;
    } else {
        player->motion = 2;
        stance = 2;
    }
    player->roty = (float)ceil((double)player->roty);
    mode_send_msg(self, MODE_MSG_CHANGE_DIR, game_mode_angle_to_direction(player->roty), stance, 0, 0);
    if (player->state != 6)
        return;
    self->sit_times[0] = self->sit_times[1];
    self->sit_times[1] = self->sit_times[2];
    self->sit_times[2] = self->sit_times[3];
    self->sit_times[3] = self->sit_times[4];
    self->sit_times[4] = self->sit_times[5];
    now = platform_ticks_ms();
    self->sit_times[5] = now;
    if (now - self->sit_times[0] - 1501 <= 1498) {
        mode_send_msg(self, 0xa9, 0, 0, 0, 0);
        self->sit_times[0] = 0;
        self->sit_times[1] = 0;
        self->sit_times[2] = 0;
        self->sit_times[3] = 0;
        self->sit_times[4] = 0;
        self->sit_times[5] = 0;
    }
}

void talk_cmd_turn_by45(struct GameMode *self, int counter_clockwise)
{
    struct Player *player = self->world->player;
    int dir = render_object_direction((struct RenderObject *)player, player->roty);
    float facing;

    if (!counter_clockwise) {
        facing = (float)((8 - dir) * 45) + 45.0f;
        player->roty = facing;
        facing -= self->view->longitude;
        player->roty = facing;
        if (facing >= 360.0f) {
            facing -= 360.0f;
            player->roty = facing;
        }
    } else {
        facing = (float)((8 - dir) * 45) - 45.0f;
        player->roty = facing;
        facing -= self->view->longitude;
        player->roty = facing;
        if (facing < 0.0f) {
            facing += 360.0f;
            player->roty = facing;
        }
    }
    mode_send_msg(self, MODE_MSG_CHANGE_DIR, game_mode_angle_to_direction(facing), 0, 0, 0);
}

void talk_cmd_help(struct GameMode *self)
{
    int count = 0;
    int i, r;

    (void)self;
    ui_window_mgr_send_msg_(&g_windowMgr, 3, 0, 0, 0, 0);

    client_lua_call(g_session1.luaState, "GetTableSizeForC", "s>d", "HelpMsgStr", &count);
    for (i = 1; i <= count; i++) {
        const char *line = 0;

        if (client_lua_call(g_session1.luaState, "getHelpMsg", "d>s", i, &line) && strlen(line) != 0)
            talk_print(line, TALK_COLOR_HELP);
    }
    r = rand() % 10;
    if (r < 0)
        r = 0;
    if (r < 4) {
        talk_print_msg(0x399, TALK_COLOR_HELP);
        talk_print_msg(0x39a, TALK_COLOR_HELP);
        return;
    }
    r = rand() % 5;
    if (r < 0)
        r = 0;
    if (r < 2) {
        talk_print_msg(0x2f8, TALK_COLOR_HELP);
        return;
    }
    r = rand() % 3;
    if (r < 0)
        r = 0;
    if (r < 2)
        talk_print_msg(0x315, TALK_COLOR_HELP);
}

void talk_cmd_trace_ai(struct GameMode *self)
{
    char path[0x100], directory[0x100];
    char *data;
    struct File file;

    (void)self;
    g_session1.homun_ai_flags[1] = g_session1.homun_ai_flags[1] == 0;
    ui_window_mgr_send_msg_(&g_windowMgr, 3, 0, 0, 0, 0);
    if (!g_session1.homun_ai_flags[1]) {
        talk_print_msg(0x3bd, TALK_COLOR_NOTICE);
        return;
    }
    platform_current_directory(directory, sizeof directory);
    data = strstr(directory, "\\data");
    if (data)
        memcpy(directory, directory, data - directory);
    platform_path_join(path, sizeof path, directory, "TraceAI.txt");
    file_construct(&file);
    file_open(&file, path, FILE_MODE_WRITE, 1, 0);
    file_write(&file, "", 0);
    file_close(&file);
    talk_print_msg(0x3bc, TALK_COLOR_NOTICE);
    file_destruct(&file);
}

void talk_cmd_volume(struct GameMode *self, const struct String *args, int effects)
{
    char line[0x80];
    int volume = atoi(string_cstr(&args[0]));

    (void)self;
    if (!effects) {
        sound_set_effect_volume(volume);
        sound_set_effect_volume_3d(volume);
    } else {
        sound_set_bgm_volume(volume);
        sound_set_bgm_fade_target(sound_bgm_volume());
    }
    sprintf(line, "%s %d", msg_str(effects ? 0x1a : 0x19), volume);
    talk_print(line, TALK_COLOR_NOTICE);
    talk_refresh_option_window();
}

void talk_cmd_sound(struct GameMode *self, int type)
{
    (void)self;
    if (type == 7) {
        g_sound_settings.bgm_on = g_sound_settings.bgm_on == 0;
        if (g_sound_settings.bgm_on)
            sound_play_stream(0, 0);
        else
            sound_stop_stream();
        ui_window_mgr_send_msg_(&g_windowMgr, 3, 0, 0, 0, 0);
        talk_print_msg(g_sound_settings.bgm_on ? 0x1f : 0x20, TALK_COLOR_NOTICE);
        talk_refresh_option_window();
    } else if (type == 0xb) {
        g_sound_settings.effects_on = g_sound_settings.effects_on == 0;
        if (g_sound_settings.effects_on) {
            talk_print_msg(0x1b, TALK_COLOR_NOTICE);
        } else {
            talk_print_msg(0x1c, TALK_COLOR_NOTICE);
        }
        talk_refresh_option_window();
    }
}

void game_mode_apply_fog(struct GameMode *self, int on)
{
    struct Vec *params;

    if (!on) {
        renderer_enable_fog(g_renderer, 0);
        return;
    }
    params = session_find_fog_parameters(&g_session1, self->map_name, &on);
    if (!on) {
        renderer_enable_fog(g_renderer, 0);
        return;
    }
    g_fogNear = (float)atof(string_cstr(&((struct String *)params->first)[0]));
    g_fogFar = (float)atof(string_cstr(&((struct String *)params->first)[1]));
    g_fogColor = hex_after_prefix_value(string_cstr(&((struct String *)params->first)[2]));
    g_fogDensity = (float)atof(string_cstr(&((struct String *)params->first)[3]));
    renderer_enable_fog(g_renderer, 1);
}

void talk_cmd_fog(struct GameMode *self)
{
    g_session1.fogOn = 1 - g_session1.fogOn;
    game_mode_apply_fog(self, g_session1.fogOn);
    talk_print(g_session1.fogOn ? "Fog On" : "Fog Off", TALK_COLOR_NOTICE);
    talk_refresh_option_window();
}

void game_mode_refresh_actor_effects(struct GameMode *self, char player_only)
{
    struct ListNode *node;

    game_actor_stop_aura_effects((struct GameActor *)self->world->player);
    game_actor_show_level_aura((struct GameActor *)self->world->player);
    if (player_only)
        return;
    for (node = self->world->actors.head->next; node != self->world->actors.head; node = node->next) {
        struct GameActor *actor = *LIST_VALUE(node, struct GameActor *);

        if (actor->aura_on) {
            game_actor_stop_aura_effects(actor);
            game_actor_show_level_aura(actor);
        }
    }
}

void talk_cmd_show_effects(struct GameMode *self, int type)
{
    int msg;

    if (type == 0x68) {
        ui_window_mgr_send_msg_(&g_windowMgr, 3, 0, 0, 0, 0);
        if (g_session1.auraOff) {
            talk_print_msg(0xbb4, TALK_COLOR_NOTICE);
            return;
        }
        g_session1.auraFull = 1 - g_session1.auraFull;
        msg = g_session1.auraFull ? 0x2c7 : 0x2c8;
    } else {
        g_session1.auraOff = g_session1.auraOff == 0;
        ui_window_mgr_send_msg_(&g_windowMgr, 3, 0, 0, 0, 0);
        msg = g_session1.auraOff ? 0xbb3 : 0xbb2;
    }
    talk_print_msg(msg, TALK_COLOR_NOTICE);
    game_mode_refresh_actor_effects(self, 0);
    talk_refresh_option_window();
}

void talk_cmd_navigate(struct GameMode *self, const struct String *args, int with_flags)
{
    struct NaviPoint pos;
    unsigned char flag1, flag2, flag4;
    int flags = 5;

    (void)self;
    navi_manager_parse_pos(&g_navi, &pos, string_cstr(&args[1]));
    if (pos.x == -1 || pos.y == -1) {
        navi_manager_report_status(&g_navi, -100, 1);
        return;
    }
    if (with_flags) {
        flag4 = 0;
        flag1 = 0;
        flag2 = 0;
        if (!navi_manager_parse_flags(&g_navi, string_cstr(&args[2]), &flag2, &flag1, &flag4)) {
            navi_manager_report_status(&g_navi, -100, 1);
            return;
        }
        flags = (unsigned char)((flag1 ? 1 : 0) + (flag2 ? 2 : 0) + (flag4 ? 4 : 0));
    }
    navi_navigate(&g_navi, string_cstr(&args[0]), 0, flags, 1, pos.x, pos.y, 0x3ea);
}

void talk_cmd_build_info(struct GameMode *self)
{
    char line[0x400];

    (void)self;
    memset(line, 0, sizeof line);
    snprintf(line, sizeof line, "build : %s - %s", "Mar 31 2022", "14:22:14");
    talk_print(line, TALK_COLOR_YELLOW);
    snprintf(line, sizeof line, "ver : %d.%d.%d", 0xe, 2, 3);
    talk_print(line, TALK_COLOR_YELLOW);
    snprintf(line, sizeof line, "s/n : %s", "XXXXXXX-XXXXXXX-XXXXXXX-XXXXXXX-XXXXXXX");
    talk_print(line, TALK_COLOR_YELLOW);
}

#define CZ_CAMERA_INFO 0xa77

#pragma pack(push, 1)
struct PacketCameraInfo {
    unsigned short id;
    unsigned char reset;
    float range;
    float rotation;
    float latitude;
};
#pragma pack(pop)
SIZE_ASSERT(PacketCameraInfo, 15);

static void send_camera_info(const struct PacketCameraInfo *packet)
{
    rag_connection_send_packet(rag_connection_instance(), sizeof *packet, packet);
}

void talk_cmd_send_camera(struct GameMode *self, const struct String *args, int reset)
{
    struct PacketCameraInfo packet;
    struct View *view;

    if (reset) {
        packet.id = CZ_CAMERA_INFO;
        packet.latitude = 0;
        packet.range = 0;
        packet.rotation = 0;
        packet.reset = 1;
        send_camera_info(&packet);
        return;
    }
    view = self->view;
    if (!view)
        return;
    packet.reset = 0;
    packet.id = CZ_CAMERA_INFO;
    packet.range = view->distance;
    packet.rotation = view->longitude;
    packet.latitude = view->latitude;
    if (string_find(&args[0], "range", 0) != STRING_NPOS) {
        packet.range = (float)atof(string_cstr(&args[1]));
    } else if (string_find(&args[0], "rotation", 0) != STRING_NPOS) {
        packet.rotation = (float)atof(string_cstr(&args[1]));
    } else if (string_find(&args[0], "latitude", 0) != STRING_NPOS) {
        packet.latitude = (float)atof(string_cstr(&args[1]));
    } else {
        packet.range = (float)atof(string_cstr(&args[0]));
        packet.rotation = (float)atof(string_cstr(&args[1]));
        packet.latitude = (float)atof(string_cstr(&args[2]));
    }
    send_camera_info(&packet);
}

void talk_cmd_on_off(struct GameMode *self, const struct String *args)
{
    struct String word;

    string_init(&word);
    string_assign_cstr(&word, string_cstr(&args[0]));
    text_to_lower(&word);
    if (string_equal_cstr(&word, "off") || string_equal_cstr(&word, "on"))
        mode_send_msg(self, 0x55, string_equal_cstr(&word, "off") ^ 1, 0, 0, 0);
    string_free(&word);
}

void talk_cmd_zoom_range(struct GameMode *self)
{
    int wide = 1 - g_session1.zoomFarLimit;

    (void)self;
    g_session1.zoomFarLimit = wide;
    talk_print_msg(wide == 1 ? 0xdb7 : 0xdb8, TALK_COLOR_NOTICE);
    g_maxViewDistance = wide == 1 ? 480.0f : 400.0f;
}
