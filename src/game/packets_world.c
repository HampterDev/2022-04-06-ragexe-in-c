#include "game/packet_handlers.h"
#include "game/actor_packets.h"
#include "session/state.h"
#include "ui/window_manager_state.h"
#include "game/context.h"
#include "ui/window_access.h"
#include <string.h>
#include "render/renderer.h"
#include "game/packets_world.h"
#include "session/session_party.h"
#include "session/session_windows.h"

static int g_illustShownPosition = -1;

char g_illustImageName[0x100];

#pragma pack(push, 1)
struct PACKET_ZC_UPDATE_MAPINFO {
    unsigned short id;
    short x;
    short y;
    short type;
    char map_name[16];
};
struct PACKET_ZC_RESURRECTION {
    unsigned short id;
    unsigned aid;
    short type;
};
struct PACKET_ZC_SHOW_IMAGE {
    unsigned short id;
    char image[16];
    unsigned char type;
};
struct PACKET_ZC_SHOW_IMAGE2 {
    unsigned short id;
    char image[64];
    unsigned char type;
};
#pragma pack(pop)
STATIC_ASSERT(show_image2_type, offsetof(struct PACKET_ZC_SHOW_IMAGE2, type) == 0x42);

enum {
    AM_TALKBOX_TEXT = 0x07,
    AM_SHOW_TEXT = 0x37,
    AM_RESURRECT = 0x62,
};
enum {
    MM_SHOW_IMAGE = 0x64,
};

void game_mode_zc_update_map_info(struct GameMode *self, const struct PACKET_ZC_UPDATE_MAPINFO *packet)
{
    struct GameMode *mode = game_current_mode();
    struct AttrCell *cell;

    (void)self;
    if (!mode || !mode->world)
        return;
    cell = c3d_attr_get_cell(mode->world->attr, packet->x, packet->y);
    if (cell)
        cell->type = packet->type;
}

void game_mode_zc_talkbox_chat_contents(struct GameMode *self,
                                                 const struct PACKET_ZC_TALKBOX_CHATCONTENTS *packet)
{
    void *talkbox = world_get_skill_by_aid(self->world, packet->aid);

    if (talkbox != 0)
        render_object_send(talkbox, NULL, AM_TALKBOX_TEXT, (long long)(int)packet->text, 0, 0, 0);
}

void game_mode_zc_resurrection(struct GameMode *self, const struct PACKET_ZC_RESURRECTION *packet)
{
    struct GameActor *actor = world_get_game_actor_by_aid(self->world, packet->aid);
    struct PartyMember member;
    unsigned aid;

    if (actor != 0)
        render_object_send(actor, NULL, AM_RESURRECT, 0, 0, 0, 0);
    aid = packet->aid;
    if (aid == ((unsigned)g_session1.ownAid)) {
        render_object_send(self->world->player, NULL, AM_RESURRECT, 0, 0, 0, 0);
        self->npc_dialog_open = 0;
        self->player_dead = 0;
        aid = packet->aid;
    }
    session_get_party_member(&g_session1, &member, aid);
    if (member.found != 0) {
        session_set_party_member_state(&g_session1, packet->aid, 0);
        session_notify_messenger_group_and_plates(NULL);
    }
    string_free(&member.map_name);
    string_free(&member.name);
}

static void show_image(struct GameMode *self, const char *image, int length, unsigned char type)
{
    char name[65];

    strncpy(name, image, length);
    name[length] = 0;
    mode_send_msg(self, MM_SHOW_IMAGE, (int)name, type == 0xff ? -1 : type, 0, 0);
}

void game_mode_zc_show_image2(struct GameMode *self, const struct PACKET_ZC_SHOW_IMAGE2 *packet)
{
    show_image(self, packet->image, 64, packet->type);
}

void game_mode_zc_show_image(struct GameMode *self, const struct PACKET_ZC_SHOW_IMAGE *packet)
{
    show_image(self, packet->image, 16, packet->type);
}

enum {
    WMSG_CUTIN_LAYOUT = 0x22,
    WMSG_CUTIN_IMAGE = 0x4e,
};
enum { WID_ILLUST = 0x4e };
extern char g_illustImageName[];

void game_mode_update_cut_in(struct GameMode *self)
{
    struct UIWindow *wnd;
    const char *image;
    int x, y;

    if (self->illust_position == -1) {
        if (g_windowMgr.illust_wnd)
            ui_window_mgr_post_quit1(&g_windowMgr, WID_ILLUST);
        return;
    }
    if (self->illust_file.size == 0)
        return;
    image = string_cstr(&self->illust_file);
    wnd = g_windowMgr.illust_wnd;
    if (!wnd) {
        wnd = ui_window_open(WID_ILLUST);
        g_illustShownPosition = self->illust_position;
    } else {
        if (self->illust_position == g_illustShownPosition && strcmp(g_illustImageName, image) == 0)
            return;
        g_illustShownPosition = self->illust_position;
    }
    _snprintf_s(g_illustImageName, sizeof g_illustImageName, _TRUNCATE, "%s", image);

    x = 0xa0;
    y = 0;
    ui_window_send(wnd, NULL, WMSG_CUTIN_LAYOUT, -1, 0, 0, 0);
    switch (self->illust_position) {
    case 0:
        ui_window_send(wnd, NULL, WMSG_CUTIN_IMAGE, (int)image, 0, 0, 0);
        x = 200;
        y = g_renderer->height - wnd->height;
        break;
    case 1:
        ui_window_send(wnd, NULL, WMSG_CUTIN_IMAGE, (int)image, 0, 0, 0);
        y = g_renderer->height - wnd->height;
        x = g_renderer->centreX;
        break;
    case 2:
        ui_window_send(wnd, NULL, WMSG_CUTIN_IMAGE, (int)image, 0, 0, 0);
        x = g_renderer->width - 200;
        y = g_renderer->height - wnd->height;
        break;
    case 3:
        ui_window_send(wnd, NULL, WMSG_CUTIN_LAYOUT, -2, 0, 0, 0);
        ui_window_send(wnd, NULL, WMSG_CUTIN_IMAGE, (int)image, 0, 0, 0);
        ui_window_send(wnd, NULL, WMSG_CUTIN_LAYOUT, 3, 0, 0, 0);
        x = g_renderer->centreX;
        y = g_renderer->centreY - wnd->height / 2;
        break;
    case 4:
        ui_window_send(wnd, NULL, WMSG_CUTIN_IMAGE, (int)image, 0, 0, 0);
        ui_window_send(wnd, NULL, WMSG_CUTIN_LAYOUT, 4, 0, 0, 0);
        x = g_renderer->centreX;
        y = g_renderer->centreY - wnd->height / 2;
        break;
    }
    ui_window_move_safe(wnd, x - wnd->width / 2, y);
}

#pragma pack(push, 1)
struct PACKET_ZC_MONSTER_INFO {
    unsigned short id;
    unsigned short job;
};
#pragma pack(pop)

enum {
    WID_MONSTER_INFO = 0x4d,
    WMSG_MONSTER_INFO = 0x4f,
};

void game_mode_zc_monster_info(struct GameMode *self, const struct PACKET_ZC_MONSTER_INFO *packet)
{
    struct UIWindow *wnd;

    (void)self;
    switch (packet->job) {
    case 0x505: case 0x506: case 0x507: case 0x725: case 0x726:
        return;
    }
    wnd = ui_window_open(WID_MONSTER_INFO);
    ui_window_send(wnd, NULL, WMSG_MONSTER_INFO, (int)packet, 0, 0, 0);
}

#define WND_STAR_PLACE          0x77
#define STAR_PLACE_WND_SET      0x5b

#pragma pack(push, 1)
struct PACKET_ZC_STARPLACE {
    unsigned short packet_type;
    signed char place;
};
#pragma pack(pop)
SIZE_ASSERT(PACKET_ZC_STARPLACE, 3);

void game_mode_zc_star_place(const unsigned char *pkt)
{
    struct UIWindow *wnd = ui_window_open(WND_STAR_PLACE);

    ui_window_send(wnd, NULL, STAR_PLACE_WND_SET, ((const struct PACKET_ZC_STARPLACE *)pkt)->place, 0, 0, 0);
}
