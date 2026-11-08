#include "game/packet_handlers.h"
#include "ui/window_access.h"
#include "ui/chat/balloon.h"
#include "iteminfo/item_api.h"
#include "session/character_name.h"
#include "actor/render_object.h"
#include "contents/sound.h"
#include "game/gamemode.h"
#include "game/packets_inventory.h"
#include "core/msg_str.h"
#include "session/session_names.h"
#include "ui/window.h"

#pragma pack(push, 1)
struct PACKET_ZC_STARSKILL {
    unsigned short id;
    char name[24];
    int monster_id;
    unsigned char star;
    unsigned char result;
};
#pragma pack(pop)

enum {
    STARSKILL_FEEL = 0,
    STARSKILL_FEEL_SAVED = 1,
    STARSKILL_HATE = 10,
    STARSKILL_HATE_SAVED = 11,
    STARSKILL_TAEKWON_MISSION = 20,
    STARSKILL_TAMING_MISSION = 21,
    STARSKILL_ITEM = 22,
    STARSKILL_RESET = 30,
    STARSKILL_TARGET_HP = 40,
};

#define EF_STARSKILL 0x21f
#define EF_ELECTRIC 0x1b0
#define NOTICE_COLOR 0xffc8c8

static void launch_effect(struct GameActor *player, int effect_id)
{
    game_actor_launch_effect(player, effect_id, (Vec3){ 0.0f, 0.0f, 0.0f }, 0.0f);
}

static void play_chime(void)
{
    sound_play_wave("effect\\T_피링.wav", 0.0f, -250.0f, 0.0f, 0xfa, 0x28, 1.0f);
}

static void format_star(char *text, int first_msg, unsigned char star, const char *name)
{
    const char *me;

    if (star <= 2) {
        me = session_character_name(&g_session1);
        sprintf(text, msg_str(first_msg + star), me, name);
    }
}

void game_mode_zc_star_skill(struct GameMode *self, struct PACKET_ZC_STARSKILL *packet)
{
    struct GameActor *player = (struct GameActor *)self->world->player;
    char text[256];
    const char *item_name;
    struct ItemInfo item;
    struct UIWindow *wnd;
    struct UIBalloonText *balloon;
    int x;

    self->notice_shown_at = timeGetTime();
    if (self->notice_wnd == 0) {
        wnd = mem_alloc(sizeof(struct UITransBalloonText));
        if (wnd != 0)
            wnd = ui_trans_balloon_text_construct(wnd);
        self->notice_wnd = wnd;
        ui_window_create(wnd, 8, 8);
        ui_window_mgr_add_to_window_list(&g_windowMgr, self->notice_wnd);
    }
    switch (packet->result) {
    case STARSKILL_FEEL:
        format_star(text, 0x345, packet->star, packet->name);
        ui_balloon_text_set_text((struct UIBalloonText *)self->notice_wnd, text, 0x48);
        break;
    case STARSKILL_FEEL_SAVED:
        format_star(text, 0x348, packet->star, packet->name);
        ui_balloon_text_set_text((struct UIBalloonText *)self->notice_wnd, text, 0x48);
        launch_effect(player, EF_STARSKILL);
        play_chime();
        break;
    case STARSKILL_HATE:
    case STARSKILL_HATE_SAVED:
        if (packet->name[0] == 0)
            _snprintf_s(packet->name, sizeof packet->name, _TRUNCATE, "%s",
                        session_get_job_name(&g_session1, packet->monster_id, -1));
        if (packet->result == STARSKILL_HATE) {
            format_star(text, 0x34b, packet->star, packet->name);
            ui_balloon_text_set_text((struct UIBalloonText *)self->notice_wnd, text, 0x48);
        } else {
            format_star(text, 0x34e, packet->star, packet->name);
            ui_balloon_text_set_text((struct UIBalloonText *)self->notice_wnd, text, 0x48);
            launch_effect(player, EF_STARSKILL);
            play_chime();
        }
        break;
    case STARSKILL_TAEKWON_MISSION:
        sprintf(text, msg_str(0x39f), packet->name, packet->star);
        ui_balloon_text_set_text((struct UIBalloonText *)self->notice_wnd, text, 0x48);
        launch_effect(player, EF_STARSKILL);
        break;
    case STARSKILL_TAMING_MISSION:
        sprintf(text, msg_str(0x4a6), packet->name);
        ui_balloon_text_set_text((struct UIBalloonText *)self->notice_wnd, text, 0x48);
        launch_effect(player, EF_STARSKILL);
        break;
    case STARSKILL_ITEM: {
        struct String message = {0};
        item_info_construct(&item);
        item_info_set_item_id(&item, packet->monster_id);
        item_name = item_info_get_name(&item);
        string_format(&message, msg_str(0x4f1), item_name);
        ui_balloon_text_set_text((struct UIBalloonText *)self->notice_wnd, string_cstr(&message), 0x48);
        string_free(&message);
        launch_effect(player, EF_STARSKILL);
        item_info_destruct(&item);
        break;
    }
    case STARSKILL_RESET:
        sprintf(text, msg_str(0x409));
        ui_balloon_text_set_text((struct UIBalloonText *)self->notice_wnd, text, 0x48);
        launch_effect(player, EF_ELECTRIC);
        break;
    case STARSKILL_TARGET_HP:
        sprintf(text, "Target HP : %d", packet->monster_id);
        ui_balloon_text_set_text((struct UIBalloonText *)self->notice_wnd, text, 0x48);
        break;
    }
    ui_balloon_text_set_font_height((struct UIBalloonText *)self->notice_wnd, 0xe);
    x = (g_renderer->width - 640) / 2 - self->notice_wnd->width / 2 + 320;
    ui_window_move_safe(self->notice_wnd, x, 0x33);
    balloon = (struct UIBalloonText *)self->notice_wnd;
    balloon->colour = NOTICE_COLOR;
    balloon->outline = 0;
}
